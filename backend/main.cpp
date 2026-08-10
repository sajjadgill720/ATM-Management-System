// ===========================================================================
//  MYBANK  --  Console Banking System (Bank Administration + ATM Customer)
//
//  Introductory C++ project. Demonstrates: variables & data types, conditions,
//  loops, functions, vectors, classes/structs, strings, file handling, input
//  validation, menu-driven design and basic OOP.
//
//  Build:   g++ -std=c++11 *.cpp -o bank
//  Run:     ./bank      (Windows: bank.exe)
//
//  Data files created next to the program:
//    accounts.txt        - one line per account (public data, no PINs)
//    pins.txt            - secret PINs, kept in their own file
//    transactions.txt    - full transaction history
//    cash.txt            - ATM note inventory
//    audit.log           - security / admin audit trail
//    receipt_<acc>.txt   - printed receipts per account
//    bank_data.json      - live export the React frontend reads
// ===========================================================================

#include "bank.h"
#include "admin.h"
#include "atm.h"
#include "utils.h"

#include <iostream>
#include <ctime>
#include <cstdlib>

using namespace std;

int main() {
    srand(static_cast<unsigned>(time(nullptr))); // seed OTP generator

    Bank bank;
    bank.load();   // read all data files into memory
    bank.save();   // make sure bank_data.json exists for the frontend

    Admin admin(bank);
    ATM   atm(bank);

    bool running = true;
    while (running) {
        utils::clearScreen();
        cout << "==========================================\n";
        cout << "            MYBANK  -  MAIN MENU          \n";
        cout << "==========================================\n";
        cout << "  1. Bank Administration (staff)\n";
        cout << "  2. ATM  (customer)\n";
        cout << "  3. Exit\n";
        cout << "------------------------------------------\n";
        int choice = utils::readInt("  Choose an option: ");

        switch (choice) {
            case 1: admin.run(); break;
            case 2: atm.run();   break;
            case 3:
                running = false;
                bank.save();
                cout << "  Data saved. Goodbye!\n";
                break;
            default:
                cout << "  Please choose 1, 2 or 3.\n";
                utils::pause();
        }
    }
    return 0;
}
