#include "account.h"
#include "utils.h"

#include <sstream>
#include <vector>

using namespace std;

Account::Account()
    : type("SAVINGS"), balance(0.0), accStatus(status::ACTIVE),
      pinAttempts(0), dailyWithdrawn(0.0), lastWithdrawDate("") {}

void Account::refreshDailyLimit(const string& today) {
    // A new calendar day resets how much the customer has withdrawn.
    if (lastWithdrawDate != today) {
        dailyWithdrawn = 0.0;
        lastWithdrawDate = today;
    }
}

// Note: the PIN is deliberately NOT written here. Secret credentials live in a
// separate file (pins.txt) so they never sit next to the public account data.
string Account::toFileLine() const {
    stringstream ss;
    ss << accountNumber << '|' << name << '|' << cnic << '|' << phone << '|'
       << type << '|' << balance << '|' << accStatus << '|'
       << pinAttempts << '|' << dailyWithdrawn << '|' << lastWithdrawDate;
    return ss.str();
}

bool Account::fromFileLine(const string& line, Account& out) {
    vector<string> p = utils::split(line, '|');
    if (p.size() != 10) return false;

    out.accountNumber    = p[0];
    out.name             = p[1];
    out.cnic             = p[2];
    out.phone            = p[3];
    out.type             = p[4];
    out.accStatus        = p[6];
    out.lastWithdrawDate = p[9];
    try {
        out.balance        = stod(p[5]);
        out.pinAttempts    = stoi(p[7]);
        out.dailyWithdrawn = stod(p[8]);
    } catch (...) {
        return false;
    }
    out.pin = ""; // filled in later from the separate credentials file
    return true;
}
