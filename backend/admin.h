#ifndef ADMIN_H
#define ADMIN_H

#include "bank.h"
#include <string>

// ---------------------------------------------------------------------------
// Admin  --  The bank-staff module. Protected by a password, it can create,
// view, search, edit, freeze/unlock and delete accounts, manage ATM cash and
// run reports. It shares the same Bank data as the ATM module.
// ---------------------------------------------------------------------------
class Admin {
public:
    explicit Admin(Bank& bank);

    // Ask for the admin password, then show the admin menu if correct.
    void run();

private:
    Bank& bank_;
    const std::string ADMIN_PASSWORD = "admin123"; // demo password

    bool authenticate();

    void addAccount();
    void viewAllAccounts();
    void searchAccount();
    void editAccount();
    void freezeOrUnlock();
    void deleteAccount();
    void manageCash();
    void reports();          // totals + savings profit run
    void searchTransactions(); // bonus: filter transaction history
    void viewLoans();          // bonus: list all loans
};

#endif // ADMIN_H
