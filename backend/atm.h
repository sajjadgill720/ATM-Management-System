#ifndef ATM_H
#define ATM_H

#include "bank.h"
#include <string>

// ---------------------------------------------------------------------------
// ATM  --  The customer-facing module. It borrows the shared Bank object so
// every change (balance, transaction, cash) is saved through the same data.
// ---------------------------------------------------------------------------
class ATM {
public:
    explicit ATM(Bank& bank);

    // Full customer session: login, then show the ATM menu until logout.
    void run();

private:
    Bank& bank_;

    // Returns the logged-in account, or nullptr if login failed / cancelled.
    Account* login();

    // Menu actions (each operates on the currently logged-in account).
    void checkBalance(Account& acc);
    void deposit(Account& acc);
    void withdraw(Account& acc);
    void transfer(Account& acc);
    void miniStatement(Account& acc);
    void changePin(Account& acc);
    void applyLoan(Account& acc);   // bonus: request a loan
    void repayLoan(Account& acc);   // bonus: repay an active loan

    // Bonus: write a receipt file the customer could "print".
    void writeReceipt(const Account& acc, const std::string& type,
                      double amount, const std::string& extra);
};

#endif // ATM_H
