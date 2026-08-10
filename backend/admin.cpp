#include "admin.h"
#include "utils.h"

#include <iostream>
#include <iomanip>

using namespace std;

Admin::Admin(Bank& bank) : bank_(bank) {}

bool Admin::authenticate() {
    // The admin password is entered as normal text for the demo.
    cout << "  Enter admin password: ";
    string pass;
    getline(cin, pass);
    return utils::trim(pass) == ADMIN_PASSWORD;
}

void Admin::run() {
    utils::clearScreen();
    cout << "\n========== BANK ADMINISTRATION ==========\n";
    if (!authenticate()) {
        cout << "  Wrong password. Access denied.\n";
        bank_.audit("Failed admin login attempt");
        utils::pause();
        return;
    }
    bank_.audit("Admin logged in");

    bool running = true;
    while (running) {
        cout << "\n---------- ADMIN MENU ----------\n";
        cout << "  1. Add Account\n";
        cout << "  2. View All Accounts\n";
        cout << "  3. Search Account\n";
        cout << "  4. Edit Account (name / phone / type)\n";
        cout << "  5. Freeze / Unlock Account\n";
        cout << "  6. Delete Account\n";
        cout << "  7. Manage ATM Cash\n";
        cout << "  8. Reports & Savings Profit\n";
        cout << "  9. Search Transactions\n";
        cout << " 10. View Loans\n";
        cout << " 11. Logout\n";
        int choice = utils::readInt("  Choose an option: ");

        switch (choice) {
            case 1:  addAccount();        break;
            case 2:  viewAllAccounts();   break;
            case 3:  searchAccount();     break;
            case 4:  editAccount();       break;
            case 5:  freezeOrUnlock();    break;
            case 6:  deleteAccount();     break;
            case 7:  manageCash();        break;
            case 8:  reports();           break;
            case 9:  searchTransactions();break;
            case 10: viewLoans();         break;
            case 11:
                running = false;
                cout << "  Admin logged out.\n";
                break;
            default:
                cout << "  Please choose 1-11.\n";
        }
        bank_.save(); // persist after every admin action
    }
    utils::pause();
}

void Admin::addAccount() {
    cout << "\n  --- New Account ---\n";
    string name = utils::readLine("  Customer name: ");
    if (name.empty()) { cout << "  Name cannot be empty.\n"; return; }

    string cnic = utils::readLine("  CNIC / National ID: ");
    if (cnic.empty()) { cout << "  CNIC cannot be empty.\n"; return; }
    if (bank_.cnicExists(cnic)) {
        cout << "  An account with this CNIC already exists (must be unique).\n";
        return;
    }

    string phone = utils::readLine("  Phone: ");

    string type;
    while (true) {
        type = utils::readLine("  Account type (S = Savings, C = Current): ");
        if (type == "S" || type == "s") { type = "SAVINGS"; break; }
        if (type == "C" || type == "c") { type = "CURRENT"; break; }
        cout << "  Please enter S or C.\n";
    }

    string pin;
    while (true) {
        pin = utils::readHiddenPin("  Set 4-digit PIN: ");
        if (utils::isValidPin(pin)) break;
        cout << "  PIN must be exactly 4 digits.\n";
    }

    double opening = 0.0;
    while (true) {
        string raw = utils::readLine("  Opening deposit (0 or more): Rs. ");
        try {
            opening = stod(raw);
            if (opening < 0) { cout << "  Cannot be negative.\n"; continue; }
            break;
        } catch (...) {
            cout << "  Please enter a number.\n";
        }
    }

    Account& acc = bank_.createAccount(name, cnic, phone, type, pin, opening);
    cout << "\n  Account created! Account number: " << acc.accountNumber << "\n";
}

void Admin::viewAllAccounts() {
    const vector<Account>& all = bank_.accounts();
    cout << "\n  --- All Accounts (" << all.size() << ") ---\n";
    if (all.empty()) { cout << "  No accounts yet.\n"; return; }

    cout << left
         << "  " << setw(8)  << "Acc#"
         << setw(20) << "Name"
         << setw(10) << "Type"
         << setw(14) << "Balance"
         << setw(10) << "Status" << "\n";
    cout << "  ------------------------------------------------------------\n";
    cout << fixed << setprecision(2);
    for (const Account& a : all) {
        cout << "  " << setw(8) << a.accountNumber
             << setw(20) << a.name
             << setw(10) << a.type
             << setw(14) << a.balance
             << setw(10) << a.accStatus << "\n";
    }
}

void Admin::searchAccount() {
    string accNo = utils::readLine("  Enter account number to search: ");
    Account* a = bank_.findByNumber(accNo);
    if (!a) { cout << "  Not found.\n"; return; }

    cout << fixed << setprecision(2);
    cout << "\n  Account : " << a->accountNumber << "\n"
         << "  Name    : " << a->name  << "\n"
         << "  CNIC    : " << a->cnic  << "\n"
         << "  Phone   : " << a->phone << "\n"
         << "  Type    : " << a->type  << "\n"
         << "  Balance : Rs. " << a->balance << "\n"
         << "  Status  : " << a->accStatus << "\n"
         << "  Wrong PIN attempts: " << a->pinAttempts << "\n";
}

void Admin::editAccount() {
    string accNo = utils::readLine("  Account number to edit: ");
    Account* a = bank_.findByNumber(accNo);
    if (!a) { cout << "  Not found.\n"; return; }

    string name = utils::readLine("  New name (blank = keep '" + a->name + "'): ");
    if (!name.empty()) a->name = name;

    string phone = utils::readLine("  New phone (blank = keep '" + a->phone + "'): ");
    if (!phone.empty()) a->phone = phone;

    string type = utils::readLine("  New type S/C (blank = keep): ");
    if (type == "S" || type == "s") a->type = "SAVINGS";
    else if (type == "C" || type == "c") a->type = "CURRENT";

    bank_.audit("Edited account " + accNo);
    cout << "  Account updated.\n";
}

void Admin::freezeOrUnlock() {
    string accNo = utils::readLine("  Account number: ");
    Account* a = bank_.findByNumber(accNo);
    if (!a) { cout << "  Not found.\n"; return; }

    cout << "  Current status: " << a->accStatus << "\n";
    cout << "  1. Unlock / Activate\n  2. Freeze (Close)\n";
    int choice = utils::readInt("  Choose: ");
    if (choice == 1) {
        bank_.unlockAccount(accNo);
        cout << "  Account is now ACTIVE.\n";
    } else if (choice == 2) {
        bank_.freezeAccount(accNo);
        cout << "  Account is now CLOSED.\n";
    } else {
        cout << "  Cancelled.\n";
    }
}

void Admin::deleteAccount() {
    string accNo = utils::readLine("  Account number to DELETE: ");
    Account* a = bank_.findByNumber(accNo);
    if (!a) { cout << "  Not found.\n"; return; }

    string confirm = utils::readLine("  Type YES to permanently delete " + accNo + ": ");
    if (confirm == "YES") {
        bank_.deleteAccount(accNo);
        cout << "  Account deleted.\n";
    } else {
        cout << "  Deletion cancelled.\n";
    }
}

void Admin::manageCash() {
    CashInventory& c = bank_.cash();
    cout << "\n  --- ATM Cash Inventory ---\n";
    cout << "  5000 x " << c.notes5000 << "\n";
    cout << "  1000 x " << c.notes1000 << "\n";
    cout << "   500 x " << c.notes500  << "\n";
    cout << "   100 x " << c.notes100  << "\n";
    cout << "  Total cash: Rs. " << c.total() << "\n\n";

    cout << "  Refill notes (enter how many to ADD):\n";
    c.notes5000 += utils::readInt("  Add 5000 notes: ");
    c.notes1000 += utils::readInt("  Add 1000 notes: ");
    c.notes500  += utils::readInt("  Add 500 notes: ");
    c.notes100  += utils::readInt("  Add 100 notes: ");
    bank_.audit("Admin refilled ATM cash. New total Rs. " + to_string(c.total()));
    cout << "  Updated total cash: Rs. " << c.total() << "\n";
}

void Admin::reports() {
    const vector<Account>& all = bank_.accounts();
    double totalDeposits = 0.0;
    int active = 0, locked = 0, closed = 0;
    for (const Account& a : all) {
        totalDeposits += a.balance;
        if (a.accStatus == status::ACTIVE) active++;
        else if (a.accStatus == status::LOCKED) locked++;
        else closed++;
    }

    cout << fixed << setprecision(2);
    cout << "\n  --- Bank Report ---\n";
    cout << "  Total accounts : " << all.size() << "\n";
    cout << "  Active         : " << active << "\n";
    cout << "  Locked         : " << locked << "\n";
    cout << "  Closed         : " << closed << "\n";
    cout << "  Total deposits : Rs. " << totalDeposits << "\n";
    cout << "  ATM cash       : Rs. " << bank_.cash().total() << "\n";

    string run = utils::readLine("\n  Run monthly savings profit now? (y/n): ");
    if (run == "y" || run == "Y") {
        bank_.applyMonthlyProfit();
        cout << "  Monthly profit applied to all savings accounts.\n";
    }
}

void Admin::searchTransactions() {
    string accNo = utils::readLine("  Account number (blank = all): ");
    string type  = utils::readLine("  Filter by type (DEPOSIT/WITHDRAW/TRANSFER-IN/TRANSFER-OUT, blank = any): ");

    const vector<Transaction>& all = bank_.transactions();
    cout << fixed << setprecision(2);
    cout << "\n  --- Matching Transactions ---\n";
    int count = 0;
    for (const Transaction& t : all) {
        if (!accNo.empty() && t.accountNumber != accNo) continue;
        if (!type.empty()  && t.type != type) continue;
        cout << "  " << t.id << "  acc " << setw(6) << t.accountNumber
             << "  " << setw(13) << left << t.type
             << "  Rs. " << setw(10) << t.amount
             << "  " << t.dateTime << "\n";
        count++;
    }
    if (count == 0) cout << "  No matching transactions.\n";
    else cout << "  (" << count << " found)\n";
}

void Admin::viewLoans() {
    const vector<Loan>& all = bank_.loans();
    cout << "\n  --- All Loans (" << all.size() << ") ---\n";
    if (all.empty()) { cout << "  No loans issued yet.\n"; return; }

    cout << left
         << "  " << setw(8)  << "Loan#"
         << setw(8)  << "Acc#"
         << setw(12) << "Principal"
         << setw(12) << "Payable"
         << setw(12) << "Paid"
         << setw(12) << "Remaining"
         << setw(8)  << "Status" << "\n";
    cout << "  ----------------------------------------------------------------------\n";
    cout << fixed << setprecision(2);
    double totalOut = 0.0;
    for (const Loan& ln : all) {
        cout << "  " << setw(8) << ln.loanId
             << setw(8)  << ln.accountNumber
             << setw(12) << ln.principal
             << setw(12) << ln.totalPayable
             << setw(12) << ln.paid
             << setw(12) << ln.remaining()
             << setw(8)  << ln.status << "\n";
        totalOut += ln.remaining();
    }
    cout << "  Total outstanding (unpaid): Rs. " << totalOut << "\n";
}
