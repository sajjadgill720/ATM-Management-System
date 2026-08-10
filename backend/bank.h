#ifndef BANK_H
#define BANK_H

#include "account.h"
#include "transaction.h"
#include "loan.h"

#include <string>
#include <vector>
#include <iosfwd>

// ---------------------------------------------------------------------------
// Business-rule constants -- kept in one spot so they are easy to find/explain.
// ---------------------------------------------------------------------------
namespace rules {
    const double  DAILY_WITHDRAW_LIMIT = 50000.0; // max cash per day
    const double  MIN_BALANCE          = 0.0;     // account cannot go negative
    const int     MAX_PIN_ATTEMPTS     = 3;       // wrong tries before lock
    const double  OTP_THRESHOLD        = 25000.0; // transfers above this need OTP
    const double  SAVINGS_PROFIT_RATE  = 0.05;    // 5% yearly profit for SAVINGS
    const double  LOAN_INTEREST_RATE   = 0.10;    // 10% flat interest on a loan
    const double  MAX_LOAN             = 500000.0;// biggest loan we will grant
}

// ATM note denominations we can dispense, largest first.
// (Cash inventory is a bonus feature: the machine can run out of notes.)
struct CashInventory {
    int notes5000 = 0;
    int notes1000 = 0;
    int notes500  = 0;
    int notes100  = 0;

    long long total() const;
};

// ---------------------------------------------------------------------------
// Bank  --  Owns all accounts + transactions and every file-handling routine.
// Both the Admin module and the ATM module talk to the data through this class.
// ---------------------------------------------------------------------------
class Bank {
public:
    Bank();

    // ---- persistence ----
    void load();   // read every data file into memory at startup
    void save();   // write accounts, cash and JSON export back to disk

    // ---- account lookup ----
    Account* findByNumber(const std::string& accNo);   // nullptr if missing
    bool     cnicExists(const std::string& cnic) const; // uniqueness check

    // ---- admin operations ----
    Account& createAccount(const std::string& name, const std::string& cnic,
                           const std::string& phone, const std::string& type,
                           const std::string& pin, double openingBalance);
    bool     freezeAccount(const std::string& accNo);
    bool     unlockAccount(const std::string& accNo);   // clears LOCKED + attempts
    bool     deleteAccount(const std::string& accNo);
    void     applyMonthlyProfit();                       // savings profit (bonus)

    const std::vector<Account>&     accounts() const { return accounts_; }
    std::vector<Account>&           accounts()       { return accounts_; }
    const std::vector<Transaction>& transactions() const { return transactions_; }
    const std::vector<Loan>&        loans() const    { return loans_; }
    CashInventory&                  cash()           { return cash_; }

    // ---- loan management (bonus) ----
    // The account's current unpaid loan, or nullptr if it has none.
    Loan* findActiveLoan(const std::string& accNo);
    // Grant a new loan: credits the principal to the balance. false + message on error.
    bool  issueLoan(const std::string& accNo, double principal, std::string& message);
    // Repay part/all of a loan from the balance. false + message on error.
    bool  repayLoan(const std::string& accNo, double amount, std::string& message);

    // ---- core banking operations (shared by the console ATM and the server) ----
    // Each returns true on success; on failure it puts a reason in `message`.
    bool verifyPin(const std::string& accNo, const std::string& pin, std::string& message);
    bool deposit(const std::string& accNo, double amount, std::string& message);
    bool withdraw(const std::string& accNo, double amount, std::string& message);
    bool transfer(const std::string& fromNo, const std::string& toNo, double amount, std::string& message);
    bool changePin(const std::string& accNo, const std::string& oldPin,
                   const std::string& newPin, std::string& message);

    // ---- transaction helpers (used by ATM module) ----
    // Records a transaction, appends to file, and updates the in-memory list.
    void recordTransaction(const std::string& accNo, const std::string& type,
                           double amount, double balanceAfter);

    // Returns the transactions for one account, newest first.
    std::vector<Transaction> historyFor(const std::string& accNo) const;

    // ---- audit log (bonus) ----
    // Appends a line such as "[time] ADMIN froze account 1001" to audit.log.
    void audit(const std::string& message);

    // Full state as a JSON string (same shape as bank_data.json; no PINs).
    // Used by the HTTP server to answer GET /api/state.
    std::string stateJson() const;

    // ---- ATM cash dispensing (bonus) ----
    // Tries to break "amount" into available notes. On success it deducts the
    // notes from inventory and fills "breakdown" with a human-readable string.
    bool dispense(double amount, std::string& breakdown);

private:
    std::vector<Account>     accounts_;
    std::vector<Transaction> transactions_;
    std::vector<Loan>        loans_;
    CashInventory            cash_;

    int nextAccountSeq_ = 1001; // auto-incrementing account number
    int nextTxnSeq_     = 1;    // auto-incrementing transaction id
    int nextLoanSeq_    = 1;    // auto-incrementing loan id

    // File names (kept next to the executable).
    const std::string ACCOUNTS_FILE = "accounts.txt";
    const std::string PINS_FILE      = "pins.txt";       // hashed PINs, kept apart
    const std::string TXN_FILE       = "transactions.txt";
    const std::string LOANS_FILE     = "loans.txt";
    const std::string CASH_FILE      = "cash.txt";
    const std::string AUDIT_FILE     = "audit.log";
    const std::string JSON_FILE      = "bank_data.json"; // for the React UI

    void loadAccounts();
    void loadPins();       // reads pins.txt and attaches hashed PINs to accounts
    void loadTransactions();
    void loadLoans();
    void loadCash();
    void saveAccounts();
    void savePins();       // writes hashed PINs to their own file
    void saveLoans();
    void saveCash();
    void exportJson();               // writes bank_data.json for the frontend
    void writeJson(std::ostream& out) const; // shared JSON builder
    std::string makeTxnId();    // e.g. TXN0007
    std::string makeLoanId();   // e.g. LN0001
};

#endif // BANK_H
