#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>

// Account status is stored as a simple string in the file, but we use these
// named constants everywhere in code to avoid typos.
namespace status {
    const std::string ACTIVE = "ACTIVE";
    const std::string LOCKED = "LOCKED";  // too many wrong PINs
    const std::string CLOSED = "CLOSED";  // frozen / deactivated by admin
}

// ---------------------------------------------------------------------------
// Account  --  All the information the bank stores about one customer account.
// ---------------------------------------------------------------------------
class Account {
public:
    // ----- customer details -----
    std::string accountNumber;   // unique id, e.g. 1001
    std::string name;
    std::string cnic;            // national id (unique)
    std::string phone;

    // ----- account details -----
    std::string type;            // SAVINGS or CURRENT
    double      balance;
    std::string pin;             // 4-digit PIN
    std::string accStatus;       // ACTIVE / LOCKED / CLOSED

    // ----- security / limits -----
    int         pinAttempts;     // wrong-PIN counter (locks at 3)
    double      dailyWithdrawn;  // total withdrawn today
    std::string lastWithdrawDate;// date the daily counter refers to

    Account();

    // Reset the daily withdrawal counter if the stored date is not today.
    void refreshDailyLimit(const std::string& today);

    // Serialize / deserialize for the accounts.txt file.
    std::string toFileLine() const;
    static bool fromFileLine(const std::string& line, Account& out);
};

#endif // ACCOUNT_H
