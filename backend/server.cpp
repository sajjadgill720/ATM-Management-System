// ===========================================================================
//  MYBANK HTTP SERVER
//
//  Turns the C++ banking backend into a small REST API the React app calls for
//  every read and write, so the web app and the files share one live state.
//  Reuses the exact same Bank / Account / Transaction / Loan classes as the
//  console program — this is genuinely "backend in C++".
//
//  Build:  see the Makefile  (make server)   — links -lws2_32 on Windows.
//  Run:    ./server        -> listens on http://localhost:8080
// ===========================================================================
#include "bank.h"
#include "utils.h"
#include "httplib.h"

#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <mutex>
#include <cctype>
#include <ctime>
#include <cstdlib>

using namespace std;

static const string ADMIN_PASSWORD = "admin123";

// --- tiny JSON helpers for the flat request bodies the frontend sends ---
static string jsonEsc(const string& s) {
    string out;
    for (char c : s) { if (c == '"' || c == '\\') out.push_back('\\'); out.push_back(c); }
    return out;
}
static string jsonStr(const string& body, const string& key) {
    string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == string::npos) return "";
    size_t colon = body.find(':', k + pat.size());
    if (colon == string::npos) return "";
    size_t q1 = body.find('"', colon + 1);
    if (q1 == string::npos) return "";
    size_t q2 = body.find('"', q1 + 1);
    if (q2 == string::npos) return "";
    return body.substr(q1 + 1, q2 - q1 - 1);
}
static double jsonNum(const string& body, const string& key) {
    string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == string::npos) return 0;
    size_t colon = body.find(':', k + pat.size());
    if (colon == string::npos) return 0;
    size_t i = colon + 1;
    while (i < body.size() && (body[i] == ' ' || body[i] == '\t' || body[i] == '"')) i++;
    size_t start = i;
    while (i < body.size() && (isdigit((unsigned char)body[i]) || body[i] == '.' || body[i] == '-' || body[i] == '+')) i++;
    try { return stod(body.substr(start, i - start)); } catch (...) { return 0; }
}

// Build a { "ok": bool, "message": "..." } response, with optional extra fields.
static string result(bool ok, const string& message, const string& extra = "") {
    ostringstream ss;
    ss << "{\"ok\":" << (ok ? "true" : "false")
       << ",\"message\":\"" << jsonEsc(message) << "\"";
    if (!extra.empty()) ss << "," << extra;
    ss << "}";
    return ss.str();
}

// Serialize one account (public fields only — never the PIN).
static string accountJson(const Account& a) {
    ostringstream ss;
    ss << "\"account\":{"
       << "\"accountNumber\":\"" << jsonEsc(a.accountNumber) << "\","
       << "\"name\":\"" << jsonEsc(a.name) << "\","
       << "\"type\":\"" << jsonEsc(a.type) << "\","
       << "\"balance\":" << a.balance << ","
       << "\"status\":\"" << jsonEsc(a.accStatus) << "\"}";
    return ss.str();
}

int main() {
    srand(static_cast<unsigned>(time(nullptr))); // for OTP generation

    Bank bank;
    bank.load();
    bank.save();

    httplib::Server svr;
    mutex mtx; // one lock so concurrent requests never corrupt the data

    // Pending OTP transfers, keyed by the source account number.
    struct Pending { string to; double amount; string otp; };
    map<string, Pending> pending;

    // Allow the browser to call us directly (in addition to the Vite proxy).
    svr.set_post_routing_handler([](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    });
    svr.Options(R"(/.*)", [](const httplib::Request&, httplib::Response& res) { res.status = 200; });

    // ---- reads ----
    svr.Get("/api/state", [&](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        res.set_content(bank.stateJson(), "application/json");
    });

    // ---- customer: login ----
    svr.Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        string pin = jsonStr(req.body, "pin");
        string msg;
        bool ok = bank.verifyPin(acc, pin, msg);
        bank.save();
        if (ok) {
            Account* a = bank.findByNumber(acc);
            res.set_content(result(true, "Welcome, " + a->name + "!", accountJson(*a)), "application/json");
        } else {
            res.set_content(result(false, msg), "application/json");
        }
    });

    // ---- customer: money operations ----
    svr.Post("/api/deposit", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        double amt = jsonNum(req.body, "amount");
        string msg; bool ok = bank.deposit(acc, amt, msg);
        bank.save();
        res.set_content(result(ok, msg), "application/json");
    });

    svr.Post("/api/withdraw", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        double amt = jsonNum(req.body, "amount");
        string msg; bool ok = bank.withdraw(acc, amt, msg);
        bank.save();
        res.set_content(result(ok, msg), "application/json");
    });

    // Transfer with server-side OTP for large amounts.
    svr.Post("/api/transfer", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string from = jsonStr(req.body, "from");
        string to   = jsonStr(req.body, "to");
        double amt  = jsonNum(req.body, "amount");
        string otp  = jsonStr(req.body, "otp");

        // Large transfer, first call (no OTP yet): issue a challenge.
        if (otp.empty() && amt > rules::OTP_THRESHOLD) {
            // Basic validation before issuing an OTP.
            Account* f = bank.findByNumber(from);
            Account* t = bank.findByNumber(to);
            if (!f || !t) { res.set_content(result(false, "Account not found."), "application/json"); return; }
            if (amt > f->balance) { res.set_content(result(false, "Insufficient balance."), "application/json"); return; }
            string code = utils::generateOtp();
            pending[from] = { to, amt, code };
            // Deliver the OTP OUT OF BAND so it never reaches the browser:
            // write it to otp.txt and print it on the server console. (A real
            // bank would SMS it.) The customer reads it from there.
            {
                ofstream f("otp.txt", ios::trunc);
                f << "MYBANK One-Time Password\n"
                  << "Transfer from account " << from << " to " << to
                  << " of Rs. " << amt << "\n"
                  << "OTP: " << code << "\n"
                  << "Time: " << utils::currentDateTime() << "\n";
            }
            cout << "[OTP] Transfer " << from << " -> " << to << " Rs. " << amt
                 << "  code=" << code << "  (also written to otp.txt)" << endl;
            res.set_content(result(false, "An OTP has been sent. Check otp.txt (or the server console) and enter it.",
                                   "\"needOtp\":true"), "application/json");
            return;
        }

        // Large transfer, second call: verify the OTP against the pending record.
        if (amt > rules::OTP_THRESHOLD) {
            auto it = pending.find(from);
            if (it == pending.end() || it->second.otp != otp ||
                it->second.to != to || it->second.amount != amt) {
                res.set_content(result(false, "OTP did not match. Transfer cancelled."), "application/json");
                return;
            }
            pending.erase(it);
        }

        string msg; bool ok = bank.transfer(from, to, amt, msg);
        bank.save();
        res.set_content(result(ok, msg), "application/json");
    });

    svr.Post("/api/changepin", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        string oldP = jsonStr(req.body, "oldPin");
        string newP = jsonStr(req.body, "newPin");
        string msg; bool ok = bank.changePin(acc, oldP, newP, msg);
        bank.save();
        res.set_content(result(ok, msg), "application/json");
    });

    // ---- loans ----
    svr.Post("/api/loan/apply", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        double amt = jsonNum(req.body, "amount");
        string msg; bool ok = bank.issueLoan(acc, amt, msg);
        bank.save();
        res.set_content(result(ok, msg), "application/json");
    });
    svr.Post("/api/loan/repay", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        double amt = jsonNum(req.body, "amount");
        string msg; bool ok = bank.repayLoan(acc, amt, msg);
        bank.save();
        res.set_content(result(ok, msg), "application/json");
    });

    // ---- staff / admin ----
    svr.Post("/api/admin/login", [&](const httplib::Request& req, httplib::Response& res) {
        string pass = jsonStr(req.body, "password");
        bool ok = (pass == ADMIN_PASSWORD);
        res.set_content(result(ok, ok ? "Access granted." : "Wrong password."), "application/json");
    });

    svr.Post("/api/admin/create", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string name = jsonStr(req.body, "name");
        string cnic = jsonStr(req.body, "cnic");
        string phone = jsonStr(req.body, "phone");
        string type = jsonStr(req.body, "type");
        string pin = jsonStr(req.body, "pin");
        double opening = jsonNum(req.body, "opening");
        if (name.empty() || cnic.empty()) { res.set_content(result(false, "Name and CNIC are required."), "application/json"); return; }
        if (bank.cnicExists(cnic)) { res.set_content(result(false, "An account with this CNIC already exists."), "application/json"); return; }
        if (!utils::isValidPin(pin)) { res.set_content(result(false, "PIN must be exactly 4 digits."), "application/json"); return; }
        if (opening < 0) { res.set_content(result(false, "Opening deposit cannot be negative."), "application/json"); return; }
        Account& a = bank.createAccount(name, cnic, phone, (type == "CURRENT" ? "CURRENT" : "SAVINGS"), pin, opening);
        bank.save();
        res.set_content(result(true, "Account created. Number: " + a.accountNumber,
                               "\"accountNumber\":\"" + a.accountNumber + "\""), "application/json");
    });

    svr.Post("/api/admin/status", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        string st = jsonStr(req.body, "status");
        bool ok = (st == "ACTIVE") ? bank.unlockAccount(acc) : bank.freezeAccount(acc);
        bank.save();
        res.set_content(result(ok, ok ? ("Account " + acc + " is now " + st + ".") : "Account not found."), "application/json");
    });

    svr.Post("/api/admin/delete", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        string acc = jsonStr(req.body, "accountNumber");
        bool ok = bank.deleteAccount(acc);
        bank.save();
        res.set_content(result(ok, ok ? ("Account " + acc + " deleted.") : "Account not found."), "application/json");
    });

    svr.Post("/api/admin/cash", [&](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        bank.cash().notes5000 += (int)jsonNum(req.body, "notes5000");
        bank.cash().notes1000 += (int)jsonNum(req.body, "notes1000");
        bank.cash().notes500  += (int)jsonNum(req.body, "notes500");
        bank.cash().notes100  += (int)jsonNum(req.body, "notes100");
        bank.audit("Admin refilled ATM cash via web. New total Rs. " + to_string(bank.cash().total()));
        bank.save();
        res.set_content(result(true, "ATM cash refilled."), "application/json");
    });

    svr.Post("/api/admin/profit", [&](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lk(mtx);
        bank.applyMonthlyProfit();
        bank.save();
        res.set_content(result(true, "Monthly savings profit applied."), "application/json");
    });

    cout << "MYBANK server listening on http://localhost:8080" << endl;
    cout << "Press Ctrl+C to stop." << endl;
    if (!svr.listen("127.0.0.1", 8080)) {
        cerr << "ERROR: could not start the server (is port 8080 already in use?)" << endl;
        return 1;
    }
    return 0;
}
