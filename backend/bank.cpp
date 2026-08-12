#include "bank.h"
#include "utils.h"
#include "luhn.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

long long CashInventory::total() const {
    return static_cast<long long>(notes5000) * 5000 +
           static_cast<long long>(notes1000) * 1000 +
           static_cast<long long>(notes500)  * 500  +
           static_cast<long long>(notes100)  * 100;
}

Bank::Bank() {}


void Bank::load() {
    loadAccounts();
    loadPins();          // attach hashed PINs from their separate file
    loadTransactions();
    loadLoans();
    loadCash();
}

void Bank::loadLoans() {
    ifstream in(LOANS_FILE);
    if (!in) return;
    string line;
    int maxSeq = 0;
    while (getline(in, line)) {
        line = utils::trim(line);
        if (line.empty()) continue;
        Loan ln;
        if (Loan::fromFileLine(line, ln)) {
            loans_.push_back(ln);
            // Loan ids look like "LN0007" -> pull out the number after "LN".
            if (ln.loanId.size() > 2) {
                try {
                    int n = stoi(ln.loanId.substr(2));
                    if (n > maxSeq) maxSeq = n;
                } catch (...) {}
            }
        }
    }
    nextLoanSeq_ = maxSeq + 1;
}

// PINs live in their own file, one "accountNumber|pin" per line, so the secret
// credentials are never stored beside the public account details.
void Bank::loadPins() {
    ifstream in(PINS_FILE);
    if (!in) return; // no PINs saved yet (first run)
    string line;
    while (getline(in, line)) {
        line = utils::trim(line);
        if (line.empty()) continue;
        vector<string> p = utils::split(line, '|');
        if (p.size() != 2) continue;
        Account* a = findByNumber(p[0]);
        if (a) a->pin = p[1];
    }
}

void Bank::loadAccounts() {
    ifstream in(ACCOUNTS_FILE);
    if (!in) return; // first run: no file yet, that's fine
    string line;
    int maxSeq = 1000;
    while (getline(in, line)) {
        line = utils::trim(line);
        if (line.empty()) continue;
        Account acc;
        if (Account::fromFileLine(line, acc)) {
            accounts_.push_back(acc);
            // Track the highest numeric account number so new ones stay unique.
            try {
                int n = stoi(acc.accountNumber);
                int baseVal = (n >= 10000) ? (n / 10) : n;
                if (baseVal > maxSeq) maxSeq = baseVal;
            } catch (...) {}
        }
    }
    nextAccountSeq_ = maxSeq + 1;
}

void Bank::loadTransactions() {
    ifstream in(TXN_FILE);
    if (!in) return;
    string line;
    int maxSeq = 0;
    while (getline(in, line)) {
        line = utils::trim(line);
        if (line.empty()) continue;
        Transaction t;
        if (Transaction::fromFileLine(line, t)) {
            transactions_.push_back(t);
            // Transaction ids look like "TXN0007" -> pull out the number.
            if (t.id.size() > 3) {
                try {
                    int n = stoi(t.id.substr(3));
                    if (n > maxSeq) maxSeq = n;
                } catch (...) {}
            }
        }
    }
    nextTxnSeq_ = maxSeq + 1;
}

void Bank::loadCash() {
    ifstream in(CASH_FILE);
    // Note: on some compilers a missing file is not detected until we actually
    // try to read, so we check is_open() AND whether the read succeeds, and
    // fall back to a sensible starting inventory in either case.
    bool ok = in.is_open() &&
              (in >> cash_.notes5000 >> cash_.notes1000
                  >> cash_.notes500  >> cash_.notes100);
    if (!ok) {
        // Fresh ATM already holds some cash so withdrawals work out of the box.
        cash_.notes5000 = 20;   // Rs 100,000
        cash_.notes1000 = 50;   // Rs  50,000
        cash_.notes500  = 50;   // Rs  25,000
        cash_.notes100  = 100;  // Rs  10,000  -> Rs 185,000 total
    }
}


void Bank::save() {
    saveAccounts();
    savePins();
    saveLoans();
    saveCash();
    exportJson();
}

void Bank::saveLoans() {
    ofstream out(LOANS_FILE, ios::trunc);
    for (const Loan& ln : loans_) {
        out << ln.toFileLine() << "\n";
    }
}

void Bank::saveAccounts() {
    ofstream out(ACCOUNTS_FILE, ios::trunc);
    for (const Account& a : accounts_) {
        out << a.toFileLine() << "\n";
    }
}

void Bank::savePins() {
    ofstream out(PINS_FILE, ios::trunc);
    for (const Account& a : accounts_) {
        out << a.accountNumber << '|' << a.pin << "\n";
    }
}

void Bank::saveCash() {
    ofstream out(CASH_FILE, ios::trunc);
    out << cash_.notes5000 << " " << cash_.notes1000 << " "
        << cash_.notes500  << " " << cash_.notes100  << "\n";
}


Account* Bank::findByNumber(const string& accNo) {
    if (!luhn::isValid(accNo)) return nullptr;
    for (Account& a : accounts_) {
        if (a.accountNumber == accNo) return &a;
    }
    return nullptr;
}

bool Bank::cnicExists(const string& cnic) const {
    for (const Account& a : accounts_) {
        if (a.cnic == cnic) return true;
    }
    return false;
}


Account& Bank::createAccount(const string& name, const string& cnic,
                             const string& phone, const string& type,
                             const string& pin, double openingBalance) {
    Account acc;
    acc.accountNumber = luhn::generateNext(nextAccountSeq_++);
    acc.name          = name;
    acc.cnic          = cnic;
    acc.phone         = phone;
    acc.type          = type;
    acc.pin           = utils::hashPin(pin); // store the hash, never the raw PIN
    acc.balance       = openingBalance;
    acc.accStatus     = status::ACTIVE;
    accounts_.push_back(acc);

    if (openingBalance > 0) {
        recordTransaction(acc.accountNumber, "DEPOSIT", openingBalance, openingBalance);
    }
    audit("Created account " + acc.accountNumber + " for " + name);
    return accounts_.back();
}

bool Bank::freezeAccount(const string& accNo) {
    Account* a = findByNumber(accNo);
    if (!a) return false;
    a->accStatus = status::CLOSED;
    audit("Froze (closed) account " + accNo);
    return true;
}

bool Bank::unlockAccount(const string& accNo) {
    Account* a = findByNumber(accNo);
    if (!a) return false;
    a->accStatus   = status::ACTIVE;
    a->pinAttempts = 0;
    audit("Unlocked / reactivated account " + accNo);
    return true;
}

bool Bank::deleteAccount(const string& accNo) {
    for (size_t i = 0; i < accounts_.size(); ++i) {
        if (accounts_[i].accountNumber == accNo) {
            accounts_.erase(accounts_.begin() + i);
            audit("Deleted account " + accNo);
            return true;
        }
    }
    return false;
}

void Bank::applyMonthlyProfit() {
    // Bonus feature: pay simple monthly profit on SAVINGS accounts.
    // Monthly profit = balance * yearlyRate / 12.
    for (Account& a : accounts_) {
        if (a.type == "SAVINGS" && a.accStatus == status::ACTIVE && a.balance > 0) {
            double profit = a.balance * rules::SAVINGS_PROFIT_RATE / 12.0;
            profit = static_cast<long long>(profit * 100 + 0.5) / 100.0; // 2 dp
            if (profit > 0) {
                a.balance += profit;
                recordTransaction(a.accountNumber, "PROFIT", profit, a.balance);
            }
        }
    }
    audit("Applied monthly savings profit to all savings accounts");
}


bool Bank::verifyPin(const string& accNo, const string& pin, string& message) {
    Account* a = findByNumber(accNo);
    if (!a) { message = "No account found with that number."; return false; }
    if (a->accStatus == status::CLOSED) { message = "This account is closed."; return false; }
    if (a->accStatus == status::LOCKED) { message = "This account is locked. Ask an admin to unlock it."; return false; }

    if (utils::hashPin(pin) == a->pin) { // compare hashes, never raw PINs
        a->pinAttempts = 0;
        message = "ok";
        return true;
    }
    a->pinAttempts++;
    if (a->pinAttempts >= rules::MAX_PIN_ATTEMPTS) {
        a->accStatus = status::LOCKED;
        audit("Auto-locked account " + accNo + " (3 wrong PINs)");
        message = "Account locked after 3 wrong PIN attempts.";
    } else {
        message = "Wrong PIN. Attempts left: " + to_string(rules::MAX_PIN_ATTEMPTS - a->pinAttempts);
    }
    return false;
}

bool Bank::deposit(const string& accNo, double amount, string& message) {
    Account* a = findByNumber(accNo);
    if (!a) { message = "Account not found."; return false; }
    if (a->accStatus != status::ACTIVE) { message = "Account is not active."; return false; }
    if (amount <= 0) { message = "Amount must be greater than zero."; return false; }

    a->balance += amount;
    // A cash deposit also physically adds notes to the ATM.
    long long amt = static_cast<long long>(amount);
    cash_.notes1000 += static_cast<int>(amt / 1000);
    cash_.notes100  += static_cast<int>((amt % 1000) / 100);
    recordTransaction(accNo, "DEPOSIT", amount, a->balance);
    message = "Deposited.";
    return true;
}

bool Bank::withdraw(const string& accNo, double amount, string& message) {
    Account* a = findByNumber(accNo);
    if (!a) { message = "Account not found."; return false; }
    if (a->accStatus != status::ACTIVE) { message = "Account is not active."; return false; }
    a->refreshDailyLimit(utils::currentDate());
    if (amount <= 0) { message = "Amount must be greater than zero."; return false; }
    if (amount > a->balance) { message = "Insufficient balance."; return false; }
    if (a->dailyWithdrawn + amount > rules::DAILY_WITHDRAW_LIMIT) {
        message = "Exceeds the daily withdrawal limit.";
        return false;
    }
    string breakdown;
    if (!dispense(amount, breakdown)) { message = breakdown; return false; }

    a->balance        -= amount;
    a->dailyWithdrawn += amount;
    recordTransaction(accNo, "WITHDRAW", amount, a->balance);
    message = "Please collect your cash: " + breakdown;
    return true;
}

bool Bank::transfer(const string& fromNo, const string& toNo, double amount, string& message) {
    Account* from = findByNumber(fromNo);
    if (!from) { message = "Source account not found."; return false; }
    Account* to = findByNumber(toNo);
    if (!to) { message = "Destination account not found."; return false; }
    if (fromNo == toNo) { message = "Cannot transfer to the same account."; return false; }
    if (to->accStatus != status::ACTIVE) { message = "Destination account is not active."; return false; }
    if (amount <= 0) { message = "Amount must be greater than zero."; return false; }
    if (amount > from->balance) { message = "Insufficient balance."; return false; }

    from->balance -= amount;
    to->balance   += amount;
    recordTransaction(fromNo, "TRANSFER-OUT", amount, from->balance);
    recordTransaction(toNo,   "TRANSFER-IN",  amount, to->balance);
    message = "Transfer complete.";
    return true;
}

bool Bank::changePin(const string& accNo, const string& oldPin,
                     const string& newPin, string& message) {
    Account* a = findByNumber(accNo);
    if (!a) { message = "Account not found."; return false; }
    if (utils::hashPin(oldPin) != a->pin) { message = "Current PIN is wrong."; return false; }
    if (!utils::isValidPin(newPin)) { message = "PIN must be exactly 4 digits."; return false; }
    a->pin = utils::hashPin(newPin);
    audit("Account " + accNo + " changed its PIN");
    message = "PIN updated.";
    return true;
}


string Bank::makeTxnId() {
    stringstream ss;
    ss << "TXN" << setw(4) << setfill('0') << nextTxnSeq_++;
    return ss.str();
}

void Bank::recordTransaction(const string& accNo, const string& type,
                             double amount, double balanceAfter) {
    Transaction t(makeTxnId(), accNo, type, amount,
                  utils::currentDateTime(), balanceAfter);
    transactions_.push_back(t);
    // Append (not rewrite) so the history file only ever grows.
    ofstream out(TXN_FILE, ios::app);
    out << t.toFileLine() << "\n";
}

vector<Transaction> Bank::historyFor(const string& accNo) const {
    vector<Transaction> result;
    for (const Transaction& t : transactions_) {
        if (t.accountNumber == accNo) result.push_back(t);
    }
    reverse(result.begin(), result.end()); // newest first
    return result;
}


string Bank::makeLoanId() {
    stringstream ss;
    ss << "LN" << setw(4) << setfill('0') << nextLoanSeq_++;
    return ss.str();
}

Loan* Bank::findActiveLoan(const string& accNo) {
    for (Loan& ln : loans_) {
        if (ln.accountNumber == accNo && ln.status == loanStatus::ACTIVE) return &ln;
    }
    return nullptr;
}

bool Bank::issueLoan(const string& accNo, double principal, string& message) {
    Account* a = findByNumber(accNo);
    if (!a) { message = "Account not found."; return false; }
    if (a->accStatus != status::ACTIVE) { message = "Account is not active."; return false; }
    if (principal <= 0) { message = "Loan amount must be greater than zero."; return false; }
    if (principal > rules::MAX_LOAN) { message = "Loan exceeds the maximum allowed."; return false; }
    if (findActiveLoan(accNo)) { message = "This account already has an active loan."; return false; }

    // Build the loan: total payable = principal + flat interest.
    Loan ln;
    ln.loanId        = makeLoanId();
    ln.accountNumber = accNo;
    ln.principal     = principal;
    ln.totalPayable  = static_cast<long long>(principal * (1 + rules::LOAN_INTEREST_RATE) * 100 + 0.5) / 100.0;
    ln.paid          = 0.0;
    ln.status        = loanStatus::ACTIVE;
    ln.dateIssued    = utils::currentDate();
    loans_.push_back(ln);

    // Disburse the money into the account balance.
    a->balance += principal;
    recordTransaction(accNo, "LOAN", principal, a->balance);
    audit("Issued loan " + ln.loanId + " of " + to_string((long long)principal) + " to " + accNo);

    message = "Loan approved. Total to repay: " + to_string((long long)ln.totalPayable);
    return true;
}

bool Bank::repayLoan(const string& accNo, double amount, string& message) {
    Loan* ln = findActiveLoan(accNo);
    if (!ln) { message = "No active loan on this account."; return false; }
    Account* a = findByNumber(accNo);
    if (!a) { message = "Account not found."; return false; }
    if (amount <= 0) { message = "Repayment must be greater than zero."; return false; }

    // Never pay more than what is still owed.
    double due = ln->remaining();
    if (amount > due) amount = due;
    if (amount > a->balance) { message = "Insufficient balance for this repayment."; return false; }

    a->balance -= amount;
    ln->paid   += amount;
    recordTransaction(accNo, "LOAN-REPAY", amount, a->balance);

    if (ln->remaining() <= 0.0) {
        ln->status = loanStatus::PAID;
        audit("Loan " + ln->loanId + " fully repaid by " + accNo);
        message = "Repaid " + to_string((long long)amount) + ". Loan fully paid off!";
    } else {
        message = "Repaid " + to_string((long long)amount) + ". Remaining: " + to_string((long long)ln->remaining());
    }
    return true;
}


void Bank::audit(const string& message) {
    ofstream out(AUDIT_FILE, ios::app);
    out << "[" << utils::currentDateTime() << "] " << message << "\n";
}


bool Bank::dispense(double amount, string& breakdown) {
    // Withdrawals must be whole rupees and a multiple of 100 (smallest note).
    long long amt = static_cast<long long>(amount);
    if (amt != amount || amt % 100 != 0) {
        breakdown = "Amount must be a multiple of 100.";
        return false;
    }
    if (amt > cash_.total()) {
        breakdown = "ATM does not have enough cash right now.";
        return false;
    }

    // Greedy note breakdown, largest denomination first.
    int use5000 = min(static_cast<int>(amt / 5000), cash_.notes5000);
    amt -= static_cast<long long>(use5000) * 5000;
    int use1000 = min(static_cast<int>(amt / 1000), cash_.notes1000);
    amt -= static_cast<long long>(use1000) * 1000;
    int use500 = min(static_cast<int>(amt / 500), cash_.notes500);
    amt -= static_cast<long long>(use500) * 500;
    int use100 = min(static_cast<int>(amt / 100), cash_.notes100);
    amt -= static_cast<long long>(use100) * 100;

    if (amt != 0) {
        // Available notes could not form the exact amount.
        breakdown = "ATM cannot dispense this exact amount with available notes.";
        return false;
    }

    cash_.notes5000 -= use5000;
    cash_.notes1000 -= use1000;
    cash_.notes500  -= use500;
    cash_.notes100  -= use100;

    stringstream ss;
    if (use5000) ss << use5000 << " x 5000  ";
    if (use1000) ss << use1000 << " x 1000  ";
    if (use500)  ss << use500  << " x 500  ";
    if (use100)  ss << use100  << " x 100";
    breakdown = ss.str();
    return true;
}


static string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

void Bank::exportJson() {
    ofstream out(JSON_FILE, ios::trunc);
    writeJson(out);
}

string Bank::stateJson() const {
    ostringstream ss;
    writeJson(ss);
    return ss.str();
}

void Bank::writeJson(ostream& out) const {
    out << "{\n";

    // --- accounts ---
    out << "  \"accounts\": [\n";
    for (size_t i = 0; i < accounts_.size(); ++i) {
        const Account& a = accounts_[i];
        out << "    {"
            << "\"accountNumber\": \"" << jsonEscape(a.accountNumber) << "\", "
            << "\"name\": \"" << jsonEscape(a.name) << "\", "
            << "\"cnic\": \"" << jsonEscape(a.cnic) << "\", "
            << "\"phone\": \"" << jsonEscape(a.phone) << "\", "
            << "\"type\": \"" << jsonEscape(a.type) << "\", "
            << "\"balance\": " << fixed << setprecision(2) << a.balance << ", "
            << "\"status\": \"" << jsonEscape(a.accStatus) << "\"}";
        out << (i + 1 < accounts_.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    // --- transactions ---
    out << "  \"transactions\": [\n";
    for (size_t i = 0; i < transactions_.size(); ++i) {
        const Transaction& t = transactions_[i];
        out << "    {"
            << "\"id\": \"" << jsonEscape(t.id) << "\", "
            << "\"accountNumber\": \"" << jsonEscape(t.accountNumber) << "\", "
            << "\"type\": \"" << jsonEscape(t.type) << "\", "
            << "\"amount\": " << fixed << setprecision(2) << t.amount << ", "
            << "\"dateTime\": \"" << jsonEscape(t.dateTime) << "\", "
            << "\"balanceAfter\": " << fixed << setprecision(2) << t.balanceAfter << "}";
        out << (i + 1 < transactions_.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    // --- loans ---
    out << "  \"loans\": [\n";
    for (size_t i = 0; i < loans_.size(); ++i) {
        const Loan& ln = loans_[i];
        out << "    {"
            << "\"loanId\": \"" << jsonEscape(ln.loanId) << "\", "
            << "\"accountNumber\": \"" << jsonEscape(ln.accountNumber) << "\", "
            << "\"principal\": " << fixed << setprecision(2) << ln.principal << ", "
            << "\"totalPayable\": " << fixed << setprecision(2) << ln.totalPayable << ", "
            << "\"paid\": " << fixed << setprecision(2) << ln.paid << ", "
            << "\"remaining\": " << fixed << setprecision(2) << ln.remaining() << ", "
            << "\"status\": \"" << jsonEscape(ln.status) << "\", "
            << "\"dateIssued\": \"" << jsonEscape(ln.dateIssued) << "\"}";
        out << (i + 1 < loans_.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    // --- ATM cash ---
    out << "  \"cash\": {"
        << "\"notes5000\": " << cash_.notes5000 << ", "
        << "\"notes1000\": " << cash_.notes1000 << ", "
        << "\"notes500\": "  << cash_.notes500  << ", "
        << "\"notes100\": "  << cash_.notes100  << ", "
        << "\"total\": " << cash_.total() << "}\n";

    out << "}\n";
}
