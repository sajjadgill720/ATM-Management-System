# MYBANK — Complete Technical Documentation

A line-by-line, function-by-function explanation of the whole project: the
**C++ console banking system** (the graded core) and the **React web dashboard**
that reads the C++ program's live data.

> Read this top-to-bottom to prepare for a viva. The C++ section explains **every
> function**; the React section explains every module and action.

---

## Table of contents

1. [What the program is](#1-what-the-program-is)
2. [How to build & run](#2-how-to-build--run)
3. [Architecture & data flow](#3-architecture--data-flow)
4. [The data model](#4-the-data-model)
5. [File formats on disk](#5-file-formats-on-disk)
6. [Business rules](#6-business-rules)
7. [C++ code — every file, every function](#7-c-code--every-file-every-function)
    - [7.1 utils.h / utils.cpp](#71-utilsh--utilscpp)
    - [7.2 transaction.h / transaction.cpp](#72-transactionh--transactioncpp)
    - [7.3 account.h / account.cpp](#73-accounth--accountcpp)
    - [7.4 bank.h / bank.cpp](#74-bankh--bankcpp)
    - [7.5 atm.h / atm.cpp](#75-atmh--atmcpp)
    - [7.6 admin.h / admin.cpp](#76-adminh--admincpp)
    - [7.7 main.cpp](#77-maincpp)
8. [React frontend — every file, every function](#8-react-frontend--every-file-every-function)
9. [How the web app connects to the C++ backend](#9-how-the-web-app-connects-to-the-c-backend)
10. [C++ concepts checklist (where each is used)](#10-c-concepts-checklist-where-each-is-used)
11. [Likely viva questions & answers](#11-likely-viva-questions--answers)

---

## 1. What the program is

MYBANK is a menu-driven banking application with two roles that share the same
data and the same rules:

- **Bank Administration** (staff): open, view, search, edit, freeze, unlock and
  delete accounts; manage the ATM's cash; run reports; search transactions.
- **ATM Customer**: log in with an account number and PIN; check balance;
  deposit; withdraw; transfer; view a mini-statement; change PIN.

Everything is stored in plain-text files so data survives between runs. The C++
program also exports `bank_data.json`, which the React dashboard reads.

---

## 2. How to build & run

There are **two** programs, so do **not** use `*.cpp` (both `main.cpp` and
`server.cpp` define `main()`). Compile the console app by listing its files:

```bash
cd backend
g++ -std=c++11 main.cpp atm.cpp admin.cpp bank.cpp account.cpp transaction.cpp loan.cpp utils.cpp -o bank
./bank                            # Windows: bank.exe   (or: make run)
```

Demo logins: **Admin** password `admin123`; **ATM** account `1001` PIN `1234`,
account `1002` PIN `2000`.

The optional HTTP server (for the React web app) is built separately — see
[section 9](#9-how-the-web-app-connects-to-the-c-backend):
`g++ -std=c++11 server.cpp bank.cpp account.cpp transaction.cpp loan.cpp utils.cpp -o server -lws2_32 -pthread -static`.

---

## 3. Architecture & data flow

```
                                 ┌──────────────┐
                                 │   main.cpp   │  main menu: Admin / ATM / Exit
                                 └──────┬───────┘
                        ┌───────────────┴───────────────┐
                  ┌─────▼─────┐                    ┌─────▼─────┐
                  │  Admin    │  staff console     │   ATM     │  customer console
                  │ admin.cpp │                    │  atm.cpp  │
                  └─────┬─────┘                    └─────┬─────┘
                        └───────────────┬───────────────┘
                                  ┌─────▼─────┐
                                  │   Bank    │  the single owner of all data:
                                  │  bank.cpp │  accounts, transactions, cash,
                                  └─────┬─────┘  file load/save, JSON export
                    ┌─────────────┬─────┴──────┬───────────────┐
              ┌─────▼───┐   ┌─────▼────┐  ┌────▼─────┐   ┌──────▼──────┐
              │ Account │   │Transaction│  │  utils   │   │ data files  │
              └─────────┘   └──────────┘  └──────────┘   └─────────────┘
```

**Key design idea:** there is exactly **one** `Bank` object in `main()`. Both the
`Admin` and `ATM` objects hold a **reference** to that same `Bank`, so any change
one makes is instantly visible to the other and to the files.

---

## 4. The data model

### `Account` — one customer account
| Field | Type | Meaning |
|-------|------|---------|
| `accountNumber` | `string` | Unique id, e.g. `1001` |
| `name` | `string` | Customer name |
| `cnic` | `string` | National ID (must be unique) |
| `phone` | `string` | Phone number |
| `type` | `string` | `SAVINGS` or `CURRENT` |
| `balance` | `double` | Current money in the account |
| `pin` | `string` | 4-digit PIN (loaded from the **separate** `pins.txt`) |
| `accStatus` | `string` | `ACTIVE` / `LOCKED` / `CLOSED` |
| `pinAttempts` | `int` | Wrong-PIN counter (locks at 3) |
| `dailyWithdrawn` | `double` | Total withdrawn today |
| `lastWithdrawDate` | `string` | The date the daily counter refers to |

### `Transaction` — one financial event
`id`, `accountNumber`, `type` (DEPOSIT / WITHDRAW / TRANSFER-IN / TRANSFER-OUT /
PROFIT), `amount`, `dateTime`, `balanceAfter`.

### `CashInventory` — notes physically inside the ATM
Counts of `notes5000`, `notes1000`, `notes500`, `notes100`.

---

## 5. File formats on disk

All files sit next to the program. Fields are separated by `|`.

| File | One line looks like | Notes |
|------|---------------------|-------|
| `accounts.txt` | `1001\|Ali Khan\|3520...\|0300...\|SAVINGS\|30000\|ACTIVE\|0\|0\|2026-08-08` | 10 fields, **no PIN** |
| `pins.txt` | `1001\|bf3cdf83` | **hashed** PINs, kept separate from account data |
| `transactions.txt` | `TXN0001\|1001\|DEPOSIT\|30000\|2026-08-08 02:00:00\|30000` | 6 fields, append-only |
| `loans.txt` | `LN0001\|1001\|10000\|11000\|11000\|PAID\|2026-08-09` | 7 fields: id, acc, principal, payable, paid, status, date |
| `cash.txt` | `20 50 50 100` | note counts: 5000, 1000, 500, 100 |
| `audit.log` | `[2026-08-08 02:00:00] Created account 1001 for Ali Khan` | security trail |
| `receipt_1001.txt` | printed receipt blocks | one per account |
| `bank_data.json` | JSON export | read by the React app; **PINs never included** |

---

## 6. Business rules

Defined once in the `rules` namespace (`bank.h`) so they are easy to find:

| Constant | Value | Rule |
|----------|-------|------|
| `DAILY_WITHDRAW_LIMIT` | 50000 | Max cash a customer can withdraw per day |
| `MIN_BALANCE` | 0 | Account cannot go negative |
| `MAX_PIN_ATTEMPTS` | 3 | Wrong PINs before the account auto-locks |
| `OTP_THRESHOLD` | 25000 | Transfers above this need a one-time password |
| `SAVINGS_PROFIT_RATE` | 0.05 | 5% yearly profit on savings accounts |
| `LOAN_INTEREST_RATE` | 0.10 | 10% flat interest on a loan |
| `MAX_LOAN` | 500000 | Largest loan the bank will grant |

Extra rules enforced in code: withdrawals must be a multiple of 100 and the ATM
must physically hold the right notes; CNIC and account numbers must be unique;
money input must be positive with at most 2 decimals.

---

## 7. C++ code — every file, every function

The program is split into modules. Each has a **header** (`.h`, the *interface* —
what functions exist) and a **source** (`.cpp`, the *implementation* — how they
work). Headers keep the `std::` prefix; source files add `using namespace std;`.

---

### 7.1 utils.h / utils.cpp

General-purpose helpers used by every other module, grouped in a `namespace
utils` so calls read as `utils::trim(...)`. Putting validation in one place means
every module validates input the same way.

#### `string trim(const string& s)`
Removes spaces, tabs and newlines from **both ends** of a string.
- `find_first_not_of(" \t\r\n")` finds the first "real" character; if there is
  none the string was all blank, so it returns `""`.
- `find_last_not_of(...)` finds the last real character.
- `substr(start, length)` returns just the middle part.
- **Used for:** cleaning up every line read from a file or the keyboard.

#### `vector<string> split(const string& s, char delim)`
Splits a string into pieces on a delimiter, **keeping empty fields** (including a
trailing empty one). This is the fix for a real bug: the standard
`getline(ss, token, '|')` trick silently drops a trailing empty field, so a line
ending in `|` (an account whose `lastWithdrawDate` is empty) would parse with the
wrong number of fields and be rejected. Here we walk the string manually:
- `find(delim, start)` locates the next `|`.
- Everything from `start` up to it is one field (`substr`).
- When no more `|` is found, the remaining text — even if empty — is the final
  field, so `"a|b|"` correctly yields three fields `["a","b",""]`.
- **Returns:** a `vector<string>` of all fields.

#### `string readLine(const string& prompt)`
Prints a prompt, reads one whole line, trims it, and returns it.
- If `getline` **fails** (end-of-input: Ctrl+Z / Ctrl+D, or a closed pipe) the
  program prints a message and calls `exit(0)` instead of looping forever on an
  empty stream. This prevents an infinite loop when input runs out.
- Every keyboard read in the program ultimately goes through this one function.

#### `int readInt(const string& prompt)`
Reads an **integer**, re-asking until the user types a valid whole number.
- Reads a line, wraps it in a `stringstream`.
- `ss >> value` tries to read an int; `!(ss >> leftover)` checks nothing extra
  follows (so `"12abc"` is rejected). Only if both hold do we return the number.
- Otherwise it prints "Invalid number" and loops. **Menu choices use this.**

#### `bool readMoney(const string& prompt, double& out)`
Reads and validates a **money amount**.
- Rejects non-numbers (`ss >> value` fails) and anything with trailing junk.
- Rejects zero or negative amounts (money must be positive).
- Rounds to 2 decimal places: `(long long)(value*100 + 0.5) / 100.0` — multiply
  by 100, add 0.5 to round, truncate to an integer number of paisa, divide back.
- **Returns** `true` and stores the value in `out` (an *output parameter* passed
  by reference); returns `false` if the input was invalid.

#### `string readHiddenPin(const string& prompt)`
Reads a PIN **without showing it on screen** (a bonus "hidden PIN entry" feature).
- On Windows it uses `_getch()` from `<conio.h>`, which reads one key at a time
  without echoing it. For each digit it stores the character and prints `*`;
  Backspace removes the last digit and erases the on-screen `*` with `"\b \b"`;
  Enter (`'\r'`) ends input.
- **Robustness:** if input is redirected (a pipe/file, e.g. during automated
  testing), `_isatty(_fileno(stdin))` is false and `_getch()` would block, so it
  falls back to a normal `getline`.
- On non-Windows systems it falls back to plain `getline`.

#### `bool isValidPin(const string& pin)`
Returns true only if the string is **exactly 4 digits**. Checks the length is 4,
then loops over each character ensuring it is between `'0'` and `'9'`.

#### `string hashPin(const string& pin)`
Turns a PIN into a one-way **hash** so the raw PIN is never stored on disk (the
"PIN/data protection" bonus). It prepends a fixed salt, then runs the classic
**djb2** hash (`h = h*33 + c` for each character), and returns the result as a
hex string (e.g. `"bf3cdf83"`). The same PIN always hashes to the same value, so
we verify by hashing the entered PIN and comparing it to the stored hash — but
the stored hash cannot be reversed back into the PIN.

#### `string currentDateTime()`
Returns the current date **and** time as `"YYYY-MM-DD HH:MM:SS"`.
- `time(nullptr)` gets the current time; `localtime` converts it to a broken-down
  `tm` struct; `strftime` formats it into a text buffer using the format string.

#### `string currentDate()`
Same as above but only the date (`"YYYY-MM-DD"`). Used to compare against
`lastWithdrawDate` so the daily withdrawal counter resets on a new day.

#### `string generateOtp()`
Builds a random **6-digit OTP** string. Loops six times, each time appending a
random digit `'0' + rand() % 10`. (`rand` is seeded once in `main()` with
`srand`, so OTPs differ each run.)

#### `void pause()`
Prints "Press Enter to continue..." and waits for one line — the classic
menu-program pause so the user can read output before the screen clears.

#### `void clearScreen()`
Clears the console: runs `system("cls")` on Windows, `system("clear")` elsewhere.

---

### 7.2 transaction.h / transaction.cpp

Represents one financial event and knows how to convert itself to/from a text
line.

#### `Transaction()` (default constructor)
Creates an empty transaction with `amount` and `balanceAfter` set to `0.0`
(strings default to empty). Needed so we can declare a `Transaction` variable and
fill it in later (e.g. in `fromFileLine`).

#### `Transaction(id, acc, type, amount, dateTime, balanceAfter)` (constructor)
Builds a fully-populated transaction. Uses a **member initializer list**
(`: id(move(id)), ...`) to set every field directly. `move(...)` avoids copying
the strings (a small efficiency detail).

#### `string toFileLine() const`
Serializes the transaction into one `|`-separated line for `transactions.txt`,
using a `stringstream` to glue the fields together in order.

#### `static bool fromFileLine(const string& line, Transaction& out)`
The inverse: parses a saved line back into a `Transaction`.
- `utils::split(line, '|')` breaks it into fields; if there are not exactly 6, the
  line is corrupt and it returns `false`.
- Copies the string fields directly; converts the numeric fields with `stod`
  (string-to-double) inside a `try/catch` so malformed numbers return `false`
  instead of crashing.
- `static` means you call it as `Transaction::fromFileLine(...)` without needing
  an existing object; the result is written into the `out` reference.

---

### 7.2b loan.h / loan.cpp

Represents one loan the bank has lent to an account (the loan-management bonus).
`loan.h` also defines a `namespace loanStatus` with `ACTIVE` and `PAID`.

Fields: `loanId` (e.g. `LN0001`), `accountNumber`, `principal` (borrowed),
`totalPayable` (principal + interest), `paid` (repaid so far), `status`,
`dateIssued`.

#### `Loan()` (constructor)
Defaults the numbers to `0` and the status to `ACTIVE`.

#### `double remaining() const`
Returns `totalPayable - paid` (clamped at 0) — how much is still owed.

#### `string toFileLine() const`
Serializes the loan into a 7-field `|`-separated line for `loans.txt`.

#### `static bool fromFileLine(const string& line, Loan& out)`
Parses a saved line back into a `Loan`; splits on `|`, requires 7 fields, and
converts the three numeric fields with `stod` inside a `try/catch`.

---

### 7.3 account.h / account.cpp

`account.h` also defines a `namespace status` with the three status strings
(`ACTIVE`, `LOCKED`, `CLOSED`) so the code never mistypes them.

#### `Account()` (constructor)
Sets sensible defaults for a new account: type `SAVINGS`, balance `0`, status
`ACTIVE`, zero PIN attempts, zero withdrawn today, empty last-withdraw date.

#### `void refreshDailyLimit(const string& today)`
Resets the **daily withdrawal counter** when a new day begins.
- If `lastWithdrawDate` is not today's date, it zeroes `dailyWithdrawn` and stores
  today's date. Called at the start of every withdrawal so yesterday's total
  never counts against today's Rs. 50,000 limit.

#### `string toFileLine() const`
Serializes the account to a 10-field `|`-separated line. **The PIN is
deliberately NOT written here** — secret credentials live in `pins.txt` instead,
so `accounts.txt` holds only public data.

#### `static bool fromFileLine(const string& line, Account& out)`
Parses one saved account line back into an `Account`.
- Splits on `|`; requires exactly **10** fields (matching `toFileLine`).
- Copies the string fields; converts `balance`/`dailyWithdrawn` with `stod` and
  `pinAttempts` with `stoi` inside a `try/catch`.
- Sets `out.pin = ""`; the PIN is filled in afterwards by `Bank::loadPins()`.

---

### 7.4 bank.h / bank.cpp

The heart of the program. `Bank` **owns** all accounts and transactions and is the
only place that touches files. `bank.h` also defines the `rules` namespace
(business constants) and the `CashInventory` struct.

#### `long long CashInventory::total() const`
Returns the total rupees in the ATM: `notes5000*5000 + notes1000*1000 +
notes500*500 + notes100*100`. Uses `long long` (and casts) so large totals never
overflow a 32-bit `int`.

#### `Bank()` (constructor)
Empty body — the members (vectors, cash, sequence counters) already have default
values, so nothing extra is needed.

#### Persistence — loading

**`void load()`** — called once at startup. Calls, in order, `loadAccounts()`,
`loadPins()`, `loadTransactions()`, `loadCash()`. Order matters: accounts must
exist before PINs can be attached to them.

**`void loadPins()`** — reads `pins.txt` (`accountNumber|pin` per line). For each
line it finds the matching account with `findByNumber` and sets its `pin`. This
is what re-unites the separated credentials with the public account data in
memory. If the file doesn't exist yet (first run), it simply returns.

**`void loadAccounts()`** — reads `accounts.txt` line by line. Each non-empty line
is parsed with `Account::fromFileLine`; valid accounts are added to the
`accounts_` vector. While reading it tracks the highest numeric account number and
sets `nextAccountSeq_` to one more than that, so newly created accounts always get
a unique number.

**`void loadTransactions()`** — same pattern for `transactions.txt`, and it tracks
the highest `TXN####` number so new transaction ids stay unique
(`t.id.substr(3)` strips the `"TXN"` prefix before converting to a number).

**`void loadCash()`** — reads the four note counts from `cash.txt`. It checks both
`is_open()` **and** that the four reads succeed; if either fails (missing or
corrupt file) it loads a sensible starting inventory (20×5000, 50×1000, 50×500,
100×100 = Rs. 185,000). *Why both checks:* on some compilers a missing file is
not reported until you actually try to read it.

#### Persistence — saving

**`void save()`** — writes everything back: `saveAccounts()`, `savePins()`,
`saveCash()`, then `exportJson()`. Called after every operation so data is never
lost.

**`void saveAccounts()`** — opens `accounts.txt` in truncate mode (overwrite) and
writes every account's `toFileLine()`.

**`void savePins()`** — writes each account's `accountNumber|pin` to `pins.txt`
(the separate credentials file).

**`void saveCash()`** — writes the four note counts to `cash.txt` separated by
spaces.

#### Lookup

**`Account* findByNumber(const string& accNo)`** — linear search over the accounts
vector; returns a **pointer** to the matching account, or `nullptr` if none. A
pointer lets callers modify the real account in place (e.g. change its balance).

**`bool cnicExists(const string& cnic) const`** — returns true if any account
already has that CNIC. Enforces the "unique records" rule when opening accounts.
Marked `const` because it only reads data.

#### Admin operations

**`Account& createAccount(name, cnic, phone, type, pin, openingBalance)`** — makes
a new account:
- Assigns the next unique number (`to_string(nextAccountSeq_++)`), fills in the
  fields (storing the PIN as a **hash**, never raw), marks it `ACTIVE`, and pushes
  it onto the vector.
- If there is an opening balance, records an opening `DEPOSIT` transaction.
- Writes an audit-log entry and returns a **reference** to the stored account.

**`bool freezeAccount(const string& accNo)`** — sets an account's status to
`CLOSED` and audits it. Returns false if the account isn't found.

**`bool unlockAccount(const string& accNo)`** — sets status back to `ACTIVE`
**and** resets `pinAttempts` to 0 (used to release a locked account).

**`bool deleteAccount(const string& accNo)`** — finds the account by index and
`erase`s it from the vector; audits the deletion.

**`void applyMonthlyProfit()`** — the savings-profit bonus. For every `ACTIVE`
`SAVINGS` account with a positive balance it computes one month of profit
(`balance * 0.05 / 12`), rounds to 2 dp, adds it to the balance, and records a
`PROFIT` transaction.

#### Transactions

**`string makeTxnId()`** — formats the next id as `TXN` + a 4-digit zero-padded
number (`setw(4)` + `setfill('0')`), e.g. `TXN0007`, and increments the counter.

**`void recordTransaction(accNo, type, amount, balanceAfter)`** — the single place
a transaction is created. Builds a `Transaction` (stamped with the current time),
adds it to the in-memory list, and **appends** it to `transactions.txt` (append
mode, so history only ever grows).

**`vector<Transaction> historyFor(const string& accNo) const`** — collects all
transactions for one account and `reverse`s them so the newest is first (used by
the ATM mini-statement).

#### Audit log

**`void audit(const string& message)`** — appends `[timestamp] message` to
`audit.log`. Called from account creation, freezing, deleting, admin login, cash
refills, PIN changes, etc.

#### Loan management (bonus)

**`string makeLoanId()`** — formats the next loan id as `LN` + a 4-digit
zero-padded number (e.g. `LN0001`).

**`Loan* findActiveLoan(const string& accNo)`** — returns a pointer to the
account's current `ACTIVE` loan, or `nullptr` if it has none (one active loan per
account).

**`bool issueLoan(const string& accNo, double principal, string& message)`** —
grants a loan: checks the account is active, the amount is positive and within
`MAX_LOAN`, and that there is no existing active loan. It computes
`totalPayable = principal * (1 + LOAN_INTEREST_RATE)` (10% flat), stores the loan,
**credits the principal to the balance**, records a `LOAN` transaction and audits
it. Returns `false` with a reason in `message` on any rule violation.

**`bool repayLoan(const string& accNo, double amount, string& message)`** —
repays from the balance: clamps the amount to what is still owed (no overpaying),
rejects if the balance is too low, then subtracts from the balance, adds to
`paid`, and records a `LOAN-REPAY` transaction. When `remaining()` reaches 0 it
marks the loan `PAID`.

*(`loadLoans()` / `saveLoans()` read and write `loans.txt`, just like the account
and transaction persistence.)*

#### ATM cash dispensing

**`bool dispense(double amount, string& breakdown)`** — decides whether the ATM
can hand out exactly `amount` and, if so, which notes to use:
- Rejects amounts that aren't a whole multiple of 100.
- Rejects amounts larger than the total cash on hand.
- Uses a **greedy** algorithm: take as many 5000s as possible (but no more than
  are in the drawer), then 1000s, 500s, 100s. `min(needed, available)` at each
  step. If after all four denominations there is still a remainder (`amt != 0`),
  the exact amount can't be formed from the available notes, so it returns false.
- On success it deducts the used notes from inventory and writes a human-readable
  `breakdown` like `"4 x 1000  1 x 500  2 x 100"` into the output parameter.

#### JSON export

**`static string jsonEscape(const string& s)`** — file-local helper that escapes
`"` and `\` so text fields produce valid JSON.

**`void exportJson()`** — hand-writes `bank_data.json` (no external library, to
keep it "basic C++"). It prints an `accounts` array, a `transactions` array and a
`cash` object. **PINs are never written**, so the export is safe to share with the
web app. This is the file the React dashboard reads.

*(The tiny one-line accessors — `accounts()`, `transactions()`, `cash()` — just
return references to the private members so other modules can read/iterate them.)*

---

### 7.5 atm.h / atm.cpp

The **customer-facing** module. It holds a `Bank&` (reference), so every change
goes through the shared data and gets saved.

#### `ATM(Bank& bank)` (constructor)
Stores the reference to the shared `Bank` in `bank_`.

#### `Account* login()`
Handles authentication with the attempt limit and auto-lock:
- Reads an account number (typing `b` goes back). Looks it up; if missing/closed/
  locked it explains and returns `nullptr`.
- Loops up to `MAX_PIN_ATTEMPTS`, reading the PIN **hidden**. On a match it resets
  `pinAttempts` and returns a pointer to the account.
- On each wrong PIN it increments `pinAttempts` and shows how many tries remain.
- If the tries run out it sets the account to `LOCKED`, audits it, saves, and
  returns `nullptr`.

#### `void run()`
The customer session. Clears the screen, calls `login()`, and if successful loops
the ATM menu (1–7) calling the matching method, saving after each action, until
the user logs out.

#### `void checkBalance(Account& acc)`
Prints the balance formatted to 2 decimals (`fixed` + `setprecision(2)`).

#### `void deposit(Account& acc)`
Reads a validated amount, adds it to the balance, records a `DEPOSIT`, and — since
a cash deposit physically adds notes — increases the ATM's 1000- and 100-rupee
note counts accordingly. Writes a receipt.

#### `void withdraw(Account& acc)`
Enforces every withdrawal rule in order:
1. `refreshDailyLimit(today)` resets the daily counter if it's a new day.
2. Rejects if the amount exceeds the balance (no overdraft).
3. Rejects if it would exceed the Rs. 50,000 daily limit.
4. Calls `bank_.dispense(...)`; if the ATM can't form the amount it stops.
On success it subtracts from the balance, adds to `dailyWithdrawn`, records a
`WITHDRAW`, shows the note breakdown, and writes a receipt.

#### `void transfer(Account& acc)`
Moves money to another account:
- Validates the destination exists, isn't the same account, and is `ACTIVE`.
- Validates the amount against the balance.
- **OTP for large transfers:** if the amount exceeds `OTP_THRESHOLD` it generates
  a 6-digit OTP (shown on screen for the demo; a real bank would SMS it) and
  cancels the transfer unless the user re-types it correctly.
- On success it debits the sender, credits the receiver, and records **two**
  transactions (`TRANSFER-OUT` and `TRANSFER-IN`).

#### `void miniStatement(Account& acc)`
Asks `Bank` for this account's history (newest first) and prints up to the last 5,
neatly column-aligned with `setw`.

#### `void changePin(Account& acc)`
Reads the current PIN (hidden) and verifies it by comparing **hashes**
(`hashPin(entered) == acc.pin`); reads and confirms a new 4-digit PIN; stores its
**hash** and audits the change. (Login works the same way — it never compares raw
PINs, only their hashes.)

#### `void applyLoan(Account& acc)`
The loan-application bonus. If the account already has an active loan it shows the
remaining amount and stops. Otherwise it shows the interest rate and maximum,
reads a validated amount, and calls `Bank::issueLoan` (which credits the money to
the balance).

#### `void repayLoan(Account& acc)`
Shows the active loan's total / paid / remaining, reads a repayment amount, and
calls `Bank::repayLoan`. Reports the new remaining balance, or that the loan is
fully paid.

#### `void writeReceipt(const Account& acc, const string& type, double amount, const string& extra)`
The receipt-file bonus. Appends a formatted receipt block (bank name, date,
account, type, amount, optional detail, resulting balance) to
`receipt_<account>.txt`.

---

### 7.6 admin.h / admin.cpp

The **staff-facing** module. Also holds a `Bank&`. Protected by a password
constant `ADMIN_PASSWORD = "admin123"`.

#### `Admin(Bank& bank)` (constructor)
Stores the shared `Bank` reference.

#### `bool authenticate()`
Reads a password line and returns whether it equals `ADMIN_PASSWORD`.

#### `void run()`
Clears the screen, authenticates (audits a failed attempt and returns on wrong
password), then loops the 10-option admin menu, dispatching to the method for each
choice and saving after each action.

#### `void addAccount()`
Interactive account creation with validation: non-empty name and CNIC; **CNIC
uniqueness** via `cnicExists`; type must be S or C; PIN must pass `isValidPin`;
opening deposit must be a non-negative number. Then calls `bank_.createAccount`
and shows the new account number.

#### `void viewAllAccounts()`
Prints every account in an aligned table (account #, name, type, balance, status)
using `setw` and `left`.

#### `void searchAccount()`
Looks up one account by number and prints all its details (including PIN-attempt
count).

#### `void editAccount()`
Lets staff change an account's name, phone and/or type; a blank entry keeps the
existing value. Audits the edit.

#### `void freezeOrUnlock()`
Shows the current status and offers to Unlock/Activate (`unlockAccount`) or Freeze
/Close (`freezeAccount`).

#### `void deleteAccount()`
Requires typing `YES` to confirm, then calls `bank_.deleteAccount`.

#### `void manageCash()`
Shows the ATM's current note inventory and total, then lets staff **add** notes of
each denomination (a refill). Audits the new total.

#### `void reports()`
Summary report: total accounts, counts by status, total deposits across all
accounts, and total ATM cash. Optionally runs `applyMonthlyProfit()` on the spot.

#### `void searchTransactions()`
The transaction search/filter bonus. Filters the full transaction list by account
number and/or type (either can be left blank to mean "any") and prints the
matches with a count.

#### `void viewLoans()`
Lists every loan in an aligned table (loan #, account, principal, payable, paid,
remaining, status) and prints the total outstanding (unpaid) amount.

---

### 7.7 main.cpp

#### `int main()`
The program entry point and top-level menu:
- `srand(time(nullptr))` seeds the random generator once (for OTPs).
- Creates the **single** `Bank`, calls `load()` to read all data, then `save()`
  so `bank_data.json` exists for the web app immediately.
- Creates one `Admin` and one `ATM`, both sharing that `Bank`.
- Loops the main menu (1 = Admin, 2 = ATM, 3 = Exit), saving on exit.

---

## 8. React frontend — every file, every function

A single-page app (React + Vite) that presents the same bank with a modern UI and
can pull live data from the C++ program. It is intentionally simple — no external
UI library — so the logic is easy to follow.

### `src/seed.js`
- **`SEED`** — the starting data (two accounts, their transactions, ATM cash) with
  a **separate `credentials` map** `{ '1001':'1234', ... }` so PINs are never part
  of the displayed account objects (mirroring the backend's `pins.txt`).
- **`RULES`** — the same business constants as the C++ `rules` namespace
  (daily limit, max PIN attempts, OTP threshold, admin password).

### `src/store.js` — the data layer (a custom React hook)
Plain helper functions:
- **`loadState()` / `saveState(state)`** — read/write the whole state to
  `localStorage` (the browser's stand-in for the C++ text files).
- **`today()` / `now()`** — current date / date-time strings.
- **`money2(n)`** — round to 2 decimals. **`txnId(seq)`** — format `TXN0007`.
- **`dispense(cash, amount)`** — the same greedy note-breakdown as the C++
  `dispense`, returning either the notes used + new inventory, or an error.
- **`cashTotal(cash)`** — total rupees in the ATM.

**`useBank()`** returns the live state plus every action; each action returns
`{ ok, message }` so the UI can show a toast. Actions mirror the C++ methods:
- **`login(accNo, pin)`** — checks the PIN against the **credentials** map (never
  account data), with the 3-attempt auto-lock.
- **`deposit` / `withdraw` / `transfer` / `changePin`** — same rules as the C++
  ATM (overdraft, daily limit, note dispensing, OTP threshold, PIN change).
- **`createAccount` / `setStatus` / `removeAccount` / `refillCash` /
  `applyProfit`** — the admin operations.
- **`resetDemo()`** — restore the seeded data.
- **`syncFromBackend()`** — **fetches `/bank_data.json`** (the C++ export) and
  loads its accounts, transactions and cash into state, so the dashboard shows
  real C++ data. PINs stay in the local credentials map since the export omits
  them. (See section 9.)

### `src/ui.jsx` — reusable presentational pieces
- **`Brand()`** — the logo + name in the top bar.
- **`Field` / `Select`** — labelled text input / dropdown.
- **`Stat`** — a labelled statistic tile. **`Money`** — formats a number as
  `Rs. 1,234.00`.
- **`TxnTable`** — renders a transaction table (deposits/credits green, debits
  red).
- **`Toast`** — the auto-dismissing notification at the bottom of the screen.

### `src/App.jsx` — the shell
- **`App()`** — holds the shared `useBank()` store, the current screen
  (home / atm / admin), the toast, and the **Sync C++ data** button in the top bar
  (calls `syncFromBackend`).
- **`Home()`** — the landing page: hero, the two portals (ATM / Admin), a live
  stat band computed from real data, a features grid, a "how it works" section and
  a footer.

### `src/AtmView.jsx` — the customer screens
`AtmView` shows the login form (`Login`) then a tabbed dashboard. `AmountForm` is
a small reusable amount input reused by `Deposit` and `Withdraw`. `Transfer`
handles the OTP step for large amounts. `Security` changes the PIN. `toPair` turns
a store result into `(ok, message)` for the toast.

### `src/AdminView.jsx` — the staff screens
`AdminView` gates on the admin password (`AdminLogin`) then shows a `Dashboard`
(live stat tiles) and tabs: `Accounts` (freeze/unlock/delete), `CreateAccount`,
`CashManager` (view + refill notes), `Transactions` (search/filter) and `Reports`
(summary + profit run + reset).

### `src/main.jsx`
The React bootstrap — mounts `<App/>` into the page.

### `vite.config.js`
Standard Vite + React config **plus** `backendDataPlugin()`, a small dev-server
middleware that serves the C++ program's `../backend/bank_data.json` at the URL
`/bank_data.json`. This is what lets the browser read the C++ file live.

---

## 9. How the web app connects to the C++ backend

```
   C++ console app                        React dashboard
   ───────────────                        ───────────────
   every save()  ──writes──▶  backend/bank_data.json
                                      │
                     Vite serves it at /bank_data.json
                                      │
                    "Sync C++ data" button ──fetch──▶ updates the UI
```

1. The C++ program writes `bank_data.json` on every `save()` (it always did).
2. `vite.config.js`'s middleware serves that exact file to the browser.
3. Clicking **Sync C++ data** calls `syncFromBackend()`, which fetches the file
   and loads the real accounts, balances, ATM cash and transactions.

**Demo:** run the C++ app, make a withdrawal, then click Sync in the web app — the
balance, ATM cash and transaction count all update to match. The web app can also
run standalone (using its own `localStorage` copy) when the backend isn't running.

---

## 10. C++ concepts checklist (where each is used)

| Concept | Where |
|---------|-------|
| Variables & data types (`int`, `double`, `string`, `bool`, `long long`) | everywhere; `Account`, `CashInventory` |
| Conditions (`if`/`else`, `switch`) | every menu and business rule |
| Loops (`while`, `for`, range-`for`) | menus, file reading, note dispensing |
| Functions | every method; the `utils` namespace |
| Vectors | `vector<Account>`, `vector<Transaction>` in `Bank` |
| Structures & classes | `struct CashInventory`; classes `Account`, `Transaction`, `Bank`, `ATM`, `Admin` |
| Strings & parsing | account fields, `trim`, `split`, `toFileLine`/`fromFileLine` |
| File handling (`ifstream`/`ofstream`) | `Bank` load/save, receipts, audit log |
| Input validation | `readInt`, `readMoney`, `isValidPin`, uniqueness checks |
| References & pointers | `Bank&` shared by ATM/Admin; `Account*` from `findByNumber` |
| `static` members / functions | `fromFileLine`, `jsonEscape` |
| Exception handling (`try/catch`) | numeric parsing in `fromFileLine` |
| Namespaces | `utils`, `status`, `rules` |
| Basic OOP (encapsulation) | private data + public methods in `Bank` |

---

## 11. Likely viva questions & answers

**Q: Why is there only one `Bank` object?**
So both the Admin and ATM modules act on the same data. They each store a
reference (`Bank&`) to it, meaning a change made in one module is immediately seen
by the other and by the files.

**Q: Why are PINs in a separate file?**
To keep secret credentials apart from public account data. `accounts.txt` holds
no PINs; `pins.txt` maps account numbers to PINs, and the JSON export never
includes them.

**Q: How does the ATM decide which notes to give?**
`Bank::dispense` uses a greedy algorithm: as many of the largest note as possible
(limited by what's in the drawer), then the next size down, and so on. If the exact
amount can't be formed from available notes, it refuses.

**Q: What stops an account being withdrawn past zero, or past the daily limit?**
`ATM::withdraw` checks `amount > balance` (no overdraft) and
`dailyWithdrawn + amount > DAILY_WITHDRAW_LIMIT` before dispensing;
`refreshDailyLimit` resets the daily total when a new day starts.

**Q: How does auto-lock work?**
`ATM::login` counts wrong PINs in `pinAttempts`; after `MAX_PIN_ATTEMPTS` (3) it
sets the account status to `LOCKED` and saves. An admin clears it with
`unlockAccount`, which also resets the counter.

**Q: How is data made permanent?**
Every operation ends with `Bank::save()`, which rewrites `accounts.txt`,
`pins.txt`, `cash.txt` and re-exports `bank_data.json`, and transactions are
appended to `transactions.txt` as they happen. On startup `load()` reads it all
back.

**Q: What was a real bug you fixed?**
Two: (1) accounts that had never withdrawn ended their file line with `|`, and the
standard `getline` split dropped that trailing empty field, so the account failed
to parse and was lost on the next save — fixed with a custom `split` that keeps
empty fields. (2) On this compiler a missing `cash.txt` wasn't detected until a
read was attempted, so the ATM started empty — fixed by checking `is_open()` and
the read result together.
