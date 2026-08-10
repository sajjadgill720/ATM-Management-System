# MYBANK — Console Banking System (C++)

A menu-driven banking application combining a **Bank Administration** module and an
**ATM Customer** module. Both share the same account data and save everything to
files, so changes are permanent between runs.

This is the graded C++ fundamentals project. A separate React app (`../frontend`)
reads the exported `bank_data.json` to show a futuristic dashboard.

## Build & Run

With `g++` (MinGW on Windows, or GCC/Clang on Linux/Mac):

There are **two programs** here, so don't use `*.cpp` (that would compile both
`main.cpp` and `server.cpp`, which each define `main()`).

**Console app** (the graded, interactive program):
```bash
g++ -std=c++11 main.cpp atm.cpp admin.cpp bank.cpp account.cpp transaction.cpp loan.cpp utils.cpp -o bank
./bank            # Windows: bank.exe
```

**HTTP server** (for the React web app) — the link flags must come *after* the
sources; `-static` makes the .exe run without extra DLLs on Windows:
```bash
g++ -std=c++11 server.cpp bank.cpp account.cpp transaction.cpp loan.cpp utils.cpp -o server -lws2_32 -pthread -static
./server          # listens on http://localhost:8080
```

If you have `make`: `make` builds both, `make run` runs the console app,
`make serve` runs the server, `make reset` wipes saved data.

**Default logins for the demo**
- Admin password: `admin123`
- Sample accounts (created on first run of the demo): `1001` PIN `1234`, `1002` PIN `2000`

## Concepts demonstrated (viva checklist)

| Requirement            | Where in the code |
|------------------------|-------------------|
| Variables & data types | throughout (`double balance`, `int pinAttempts`, `std::string`) |
| Conditions             | every menu + business rule (`atm.cpp`, `admin.cpp`) |
| Loops                  | menu loops, note dispensing, file reading |
| Functions              | every method; helpers in `utils.cpp` |
| Vectors                | `std::vector<Account>`, `std::vector<Transaction>` in `bank.cpp` |
| Structures & classes   | `struct CashInventory`, classes `Account/Transaction/Bank/ATM/Admin` |
| Strings                | account fields, parsing, `utils::split` |
| File handling          | `bank.cpp` load/save, `atm.cpp` receipts, audit log |
| Input validation       | `utils::readInt/readMoney/isValidPin` |
| Menu-driven design     | `main.cpp`, `admin.cpp`, `atm.cpp` |
| Basic OOP              | classes with private data + public methods |

## Modules & files

```
main.cpp          Main menu: choose Admin, ATM, or Exit.
account.{h,cpp}   Account class: customer + account details, file (de)serialize.
transaction.{h,cpp} Transaction record: id, type, amount, time, balance-after.
bank.{h,cpp}      Central data store: all accounts/transactions, file handling,
                  ATM cash inventory, audit log, JSON export, business constants.
atm.{h,cpp}       ATM Customer module: login, balance, deposit, withdraw,
                  transfer, mini-statement, change PIN, receipts.
admin.{h,cpp}     Bank Administration module: add/view/search/edit/freeze/
                  unlock/delete accounts, manage ATM cash, reports, txn search.
utils.{h,cpp}     Shared helpers: validation, hidden PIN (hashed), date/time, OTP.
loan.{h,cpp}      Loan class: apply/repay with interest (bonus).
server.cpp        HTTP API server (uses the same Bank class) for the React app.
httplib.h         Vendored header-only HTTP library (cpp-httplib, MIT).
```

Two programs are built from this code:
- `bank`   — the interactive console app (main.cpp + atm + admin + core).
- `server` — the HTTP API the web app calls (server.cpp + core). Run one at a
  time; they share the same data files. Build both with `make`.

## Business rules

| Rule            | Implementation |
|-----------------|----------------|
| Unique records  | account numbers auto-increment; CNIC checked for duplicates |
| PIN             | exactly 4 digits, entered hidden (shown as `*`); stored **hashed** in a separate `pins.txt`, never as raw text beside account data |
| Loan            | one active loan per account; 10% flat interest; principal credited to balance, repaid until cleared |
| Money input     | must be a positive number, max 2 decimals |
| Withdrawal      | can't overdraw; daily limit Rs. 50,000; multiple of 100; ATM must hold the notes |
| Account status  | ACTIVE / LOCKED (3 wrong PINs) / CLOSED (admin) |
| Transfer        | both accounts must be active; transfers > Rs. 25,000 need an OTP |
| Persistence     | every action saved to text files immediately |

## Bonus features included

Hidden PIN entry · automatic account locking · ATM cash inventory with note
breakdown · receipt file generation (`receipt_<acc>.txt`) · savings profit
calculation (admin report) · transaction search/filter (admin) · OTP for large
transfers · audit log (`audit.log`) · **loan management** (apply/repay with
interest, `loans.txt`) · **PIN protection** (hashed PINs in `pins.txt`) ·
JSON export for the web UI. (All 11 bonus features.)

## Data files (created next to the program)

`accounts.txt` (public account data, **no PINs**), `pins.txt` (**hashed** PINs
kept in their own file), `transactions.txt`, `loans.txt`, `cash.txt`,
`audit.log`, `receipt_<acc>.txt`, and `bank_data.json` (consumed by the React
frontend; PINs are never exported).
