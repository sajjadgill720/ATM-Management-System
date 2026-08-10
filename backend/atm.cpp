#include "atm.h"
#include "utils.h"

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

ATM::ATM(Bank& bank) : bank_(bank) {}

// ---------------------------------------------------------------------------
//  Login: account number + hidden PIN, with attempt limit and auto-locking.
// ---------------------------------------------------------------------------
Account* ATM::login() {
    string accNo = utils::readLine("  Enter account number (or 'b' to go back): ");
    if (accNo == "b" || accNo == "B") return nullptr;

    Account* acc = bank_.findByNumber(accNo);
    if (!acc) {
        cout << "  No account found with that number.\n";
        return nullptr;
    }
    if (acc->accStatus == status::CLOSED) {
        cout << "  This account is closed. Please contact the bank.\n";
        return nullptr;
    }
    if (acc->accStatus == status::LOCKED) {
        cout << "  This account is locked after too many wrong PINs.\n";
        cout << "  Ask an administrator to unlock it.\n";
        return nullptr;
    }

    // Give the customer up to MAX_PIN_ATTEMPTS tries in this session, but also
    // respect the counter already stored on the account.
    while (acc->pinAttempts < rules::MAX_PIN_ATTEMPTS) {
        string pin = utils::readHiddenPin("  Enter 4-digit PIN: ");
        // Compare the hash of what was typed against the stored hash.
        if (utils::hashPin(pin) == acc->pin) {
            acc->pinAttempts = 0; // reset on success
            cout << "\n  Welcome, " << acc->name << "!\n";
            return acc;
        }
        acc->pinAttempts++;
        int left = rules::MAX_PIN_ATTEMPTS - acc->pinAttempts;
        if (left > 0) {
            cout << "  Wrong PIN. Attempts left: " << left << "\n";
        }
    }

    // Ran out of attempts -> lock the account and persist immediately.
    acc->accStatus = status::LOCKED;
    bank_.audit("Auto-locked account " + acc->accountNumber + " (3 wrong PINs)");
    bank_.save();
    cout << "  Account locked due to 3 wrong PIN attempts.\n";
    return nullptr;
}

// ---------------------------------------------------------------------------
//  Customer session menu
// ---------------------------------------------------------------------------
void ATM::run() {
    utils::clearScreen();
    cout << "\n========== ATM CUSTOMER LOGIN ==========\n";
    Account* acc = login();
    if (!acc) { utils::pause(); return; }

    bool loggedIn = true;
    while (loggedIn) {
        cout << "\n---------- ATM MENU ----------\n";
        cout << "  1. Check Balance\n";
        cout << "  2. Deposit\n";
        cout << "  3. Withdraw\n";
        cout << "  4. Transfer\n";
        cout << "  5. Mini Statement\n";
        cout << "  6. Change PIN\n";
        cout << "  7. Apply for Loan\n";
        cout << "  8. Repay Loan\n";
        cout << "  9. Logout\n";
        int choice = utils::readInt("  Choose an option: ");

        switch (choice) {
            case 1: checkBalance(*acc); break;
            case 2: deposit(*acc);      break;
            case 3: withdraw(*acc);     break;
            case 4: transfer(*acc);     break;
            case 5: miniStatement(*acc);break;
            case 6: changePin(*acc);    break;
            case 7: applyLoan(*acc);    break;
            case 8: repayLoan(*acc);    break;
            case 9:
                loggedIn = false;
                cout << "  Logged out. Thank you!\n";
                break;
            default:
                cout << "  Please choose 1-9.\n";
        }
        bank_.save(); // persist after every action (safety)
    }
    utils::pause();
}

void ATM::checkBalance(Account& acc) {
    cout << fixed << setprecision(2);
    cout << "\n  Available balance: Rs. " << acc.balance << "\n";
}

void ATM::deposit(Account& acc) {
    double amount;
    if (!utils::readMoney("  Enter deposit amount: Rs. ", amount)) return;

    acc.balance += amount;
    bank_.recordTransaction(acc.accountNumber, "DEPOSIT", amount, acc.balance);
    // A cash deposit also adds notes to the ATM. We add it as 1000-rupee notes
    // where possible, remainder as 100s, so inventory stays consistent.
    long long amt = static_cast<long long>(amount);
    bank_.cash().notes1000 += static_cast<int>(amt / 1000);
    bank_.cash().notes100  += static_cast<int>((amt % 1000) / 100);

    cout << fixed << setprecision(2);
    cout << "  Deposited Rs. " << amount << ". New balance: Rs. " << acc.balance << "\n";
    writeReceipt(acc, "DEPOSIT", amount, "");
}

void ATM::withdraw(Account& acc) {
    acc.refreshDailyLimit(utils::currentDate());

    double amount;
    if (!utils::readMoney("  Enter withdrawal amount: Rs. ", amount)) return;

    // Rule: cannot overdraw.
    if (amount > acc.balance) {
        cout << "  Insufficient balance.\n";
        return;
    }
    // Rule: daily withdrawal limit.
    if (acc.dailyWithdrawn + amount > rules::DAILY_WITHDRAW_LIMIT) {
        cout << fixed << setprecision(2);
        cout << "  Daily limit is Rs. " << rules::DAILY_WITHDRAW_LIMIT
             << ". Already withdrawn today: Rs. " << acc.dailyWithdrawn << "\n";
        return;
    }
    // Rule + bonus: the ATM must physically have the notes.
    string breakdown;
    if (!bank_.dispense(amount, breakdown)) {
        cout << "  " << breakdown << "\n";
        return;
    }

    acc.balance        -= amount;
    acc.dailyWithdrawn += amount;
    bank_.recordTransaction(acc.accountNumber, "WITHDRAW", amount, acc.balance);

    cout << fixed << setprecision(2);
    cout << "  Please collect your cash: " << breakdown << "\n";
    cout << "  New balance: Rs. " << acc.balance << "\n";
    writeReceipt(acc, "WITHDRAW", amount, "Notes: " + breakdown);
}

void ATM::transfer(Account& acc) {
    string targetNo = utils::readLine("  Transfer to account number: ");
    Account* target = bank_.findByNumber(targetNo);
    if (!target) {
        cout << "  Destination account not found.\n";
        return;
    }
    if (target->accountNumber == acc.accountNumber) {
        cout << "  Cannot transfer to the same account.\n";
        return;
    }
    if (target->accStatus != status::ACTIVE) {
        cout << "  Destination account is not active.\n";
        return;
    }

    double amount;
    if (!utils::readMoney("  Enter transfer amount: Rs. ", amount)) return;
    if (amount > acc.balance) {
        cout << "  Insufficient balance.\n";
        return;
    }

    // Bonus: large transfers require a One Time Password.
    if (amount > rules::OTP_THRESHOLD) {
        string otp = utils::generateOtp();
        // Delivered out of band (never shown on the ATM screen): written to
        // otp.txt, the way a real bank would SMS it to the customer's phone.
        ofstream f("otp.txt", ios::trunc);
        f << "MYBANK One-Time Password\n"
          << "Transfer from account " << acc.accountNumber << " to " << targetNo
          << " of Rs. " << amount << "\n"
          << "OTP: " << otp << "\n"
          << "Time: " << utils::currentDateTime() << "\n";
        f.close();
        cout << "  This is a large transfer. A one-time password has been sent.\n";
        cout << "  Check the file 'otp.txt' to read it.\n";
        string entered = utils::readLine("  Enter the OTP to confirm: ");
        if (entered != otp) {
            cout << "  OTP did not match. Transfer cancelled.\n";
            return;
        }
    }

    acc.balance     -= amount;
    target->balance += amount;
    bank_.recordTransaction(acc.accountNumber, "TRANSFER-OUT", amount, acc.balance);
    bank_.recordTransaction(target->accountNumber, "TRANSFER-IN", amount, target->balance);

    cout << fixed << setprecision(2);
    cout << "  Transferred Rs. " << amount << " to account " << targetNo << ".\n";
    cout << "  New balance: Rs. " << acc.balance << "\n";
    writeReceipt(acc, "TRANSFER", amount, "To account: " + targetNo);
}

void ATM::miniStatement(Account& acc) {
    vector<Transaction> history = bank_.historyFor(acc.accountNumber);
    cout << "\n  ---- Mini Statement (last 5) ----\n";
    if (history.empty()) {
        cout << "  No transactions yet.\n";
        return;
    }
    cout << fixed << setprecision(2);
    int shown = 0;
    for (const Transaction& t : history) {
        cout << "  " << t.dateTime << "  " << setw(12) << left << t.type
             << "  Rs. " << setw(10) << t.amount
             << "  bal " << t.balanceAfter << "\n";
        if (++shown == 5) break;
    }
}

void ATM::changePin(Account& acc) {
    string oldPin = utils::readHiddenPin("  Current PIN: ");
    if (utils::hashPin(oldPin) != acc.pin) {   // compare hashes
        cout << "  Current PIN is wrong.\n";
        return;
    }
    string newPin = utils::readHiddenPin("  New 4-digit PIN: ");
    if (!utils::isValidPin(newPin)) {
        cout << "  PIN must be exactly 4 digits.\n";
        return;
    }
    string confirm = utils::readHiddenPin("  Confirm new PIN: ");
    if (newPin != confirm) {
        cout << "  PINs do not match.\n";
        return;
    }
    acc.pin = utils::hashPin(newPin);          // store the hash, not the PIN
    bank_.audit("Account " + acc.accountNumber + " changed its PIN");
    cout << "  PIN updated successfully.\n";
}

void ATM::applyLoan(Account& acc) {
    // Show any existing loan first.
    Loan* existing = bank_.findActiveLoan(acc.accountNumber);
    if (existing) {
        cout << fixed << setprecision(2);
        cout << "\n  You already have an active loan (" << existing->loanId << ").\n";
        cout << "  Remaining to repay: Rs. " << existing->remaining() << "\n";
        return;
    }

    cout << fixed << setprecision(2);
    cout << "\n  --- Apply for a Loan ---\n";
    cout << "  Interest: " << (rules::LOAN_INTEREST_RATE * 100) << "% flat.  Max loan: Rs. "
         << rules::MAX_LOAN << "\n";

    double amount;
    if (!utils::readMoney("  Loan amount: Rs. ", amount)) return;

    string message;
    if (bank_.issueLoan(acc.accountNumber, amount, message)) {
        cout << "  " << message << "\n";
        cout << "  Amount credited. New balance: Rs. " << acc.balance << "\n";
    } else {
        cout << "  " << message << "\n";
    }
}

void ATM::repayLoan(Account& acc) {
    Loan* ln = bank_.findActiveLoan(acc.accountNumber);
    if (!ln) {
        cout << "\n  You have no active loan to repay.\n";
        return;
    }
    cout << fixed << setprecision(2);
    cout << "\n  --- Repay Loan " << ln->loanId << " ---\n";
    cout << "  Total payable: Rs. " << ln->totalPayable << "\n";
    cout << "  Paid so far  : Rs. " << ln->paid << "\n";
    cout << "  Remaining    : Rs. " << ln->remaining() << "\n";

    double amount;
    if (!utils::readMoney("  Repay amount: Rs. ", amount)) return;

    string message;
    if (bank_.repayLoan(acc.accountNumber, amount, message)) {
        cout << "  " << message << "\n";
        cout << "  New balance: Rs. " << acc.balance << "\n";
    } else {
        cout << "  " << message << "\n";
    }
}

// ---------------------------------------------------------------------------
//  Bonus: receipt file. Each receipt is appended to receipt_<account>.txt.
// ---------------------------------------------------------------------------
void ATM::writeReceipt(const Account& acc, const string& type,
                       double amount, const string& extra) {
    string file = "receipt_" + acc.accountNumber + ".txt";
    ofstream out(file, ios::app);
    out << "----------------------------------------\n";
    out << "  MYBANK ATM RECEIPT\n";
    out << "  Date   : " << utils::currentDateTime() << "\n";
    out << "  Account: " << acc.accountNumber << " (" << acc.name << ")\n";
    out << "  Type   : " << type << "\n";
    out << fixed << setprecision(2);
    out << "  Amount : Rs. " << amount << "\n";
    if (!extra.empty()) out << "  Detail : " << extra << "\n";
    out << "  Balance: Rs. " << acc.balance << "\n";
    out << "----------------------------------------\n\n";
}
