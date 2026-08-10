# ATM Management System — MYBANK

A two-part project for an Introductory C++ course:

| Part | Folder | Tech | Role |
|------|--------|------|------|
| **Graded core** | [`backend/`](backend) | Plain C++ (classes + file handling) | Console banking app — the deliverable the viva grades |
| **Bonus GUI** | [`frontend/`](frontend) | React + Vite | Futuristic web dashboard for the same bank |

Both parts model the **same** bank and apply the **same** business rules, and
they are **truly connected two-way**: the C++ program runs as an HTTP server
(`server.cpp`), and the React app calls it for **every read and write**. So an
account created on the web *or* the console works everywhere, and a web
transaction is saved straight to the C++ files.

```
   React web app  ⇄  Vite proxy (/api)  ⇄  C++ HTTP server  ⇄  data files
   (browser UI)                            (server.cpp + Bank)   (accounts.txt, ...)
```

The same `Bank` / `Account` / `Transaction` / `Loan` classes power both the
console app and the server — this is genuinely "backend in C++".

### Presentation flow (connected demo)

1. `cd backend && make server && ./server` — starts the C++ API on port 8080.
2. `cd frontend && npm run dev` — starts the web app (proxies `/api` to the server).
3. Open `http://localhost:5173/` and use it. Create an account in the staff
   console, then log into it at the ATM; make a withdrawal, then check
   `backend/accounts.txt` — the change is there. The web and the C++ files are
   one shared state.

---

## 1. C++ Console App (the graded project)

```bash
cd backend
g++ -std=c++11 *.cpp -o bank      # or: make
./bank                            # Windows: bank.exe
```

Menu-driven: **Bank Administration** (staff) and **ATM Customer** modules over
shared, file-persisted data. Full details, the viva concept-checklist and the
business-rule table are in [`backend/README.md`](backend/README.md).

**Demo logins:** Admin `admin123` · ATM `1001`/`1234`, `1002`/`2000`.

## 2. React Web App (bonus) — connected two-way to the C++ backend

The web app talks to a **C++ HTTP server** for every read and write, so the web
and the C++ files share one live state. You run **two** things:

**a) Start the C++ server** (the backend the web app calls). Build it with this
one command — the `-lws2_32 -pthread -static` flags **must come after** the
source files (they are what make it link and run on Windows/MinGW):
```bash
cd backend
g++ -std=c++11 server.cpp bank.cpp account.cpp transaction.cpp loan.cpp utils.cpp -o server -lws2_32 -pthread -static
./server           # listens on http://localhost:8080  (Windows: server.exe)
```
(If you have `make`: `make server` does the same. If you get
`undefined reference to __imp_WSAStartup`, you left off `-lws2_32` or put it
before the sources; if the built `.exe` won't start, keep `-static`.)

**b) Start the web app** (in a second terminal):
```bash
cd frontend
npm install        # first time only
npm run dev        # serves at http://localhost:5173, proxies /api to the server
```

Customers and banking staff use **separate endpoints**:

| Endpoint | For | Shows |
|----------|-----|-------|
| `http://localhost:5173/` | Customers | Landing page + "Sign in to ATM" |
| `http://localhost:5173/atm` | Customers | ATM login + self-service (balance, deposit, withdraw, transfer, loans, PIN) |
| `http://localhost:5173/staff` | Banking staff | Admin login + console (accounts, cash, loans, reports, ledger) |

**Two-way, genuinely connected:** an account created on the web *or* the console
works everywhere; a web withdrawal is saved to the C++ files; logins are checked
against the real **hashed** PINs on the server. If the server isn't running the
web app shows a banner telling you to start it. A warm "atelier" theme (espresso
+ cream, Chillax display type, photographic imagery, inspired by mynext9to5.com).

> **Note:** run the interactive **console app** (`./bank`) *or* the **server**
> (`./server`) — not both at once, since they share the same data files. Typical
> demo: use the console to explain the C++, then start the server for the web UI.

---

## Required features → where they live

**Bank Administration module** — add / view / search / edit / freeze / unlock /
delete accounts, manage ATM cash, reports, transaction search.
→ `backend/admin.cpp`, `frontend/src/AdminView.jsx`

**ATM Customer module** — login, balance, deposit, withdraw, transfer,
mini-statement, change PIN.
→ `backend/atm.cpp`, `frontend/src/AtmView.jsx`

## Business rules (identical in both apps)

| Rule | Requirement |
|------|-------------|
| Unique records | auto-increment account numbers; CNIC must be unique |
| PIN | exactly 4 digits, entered hidden |
| Money input | positive numbers only, max 2 decimals |
| Withdrawal | no overdraw; daily limit Rs. 50,000; multiples of 100; ATM must hold the notes |
| Account status | ACTIVE / LOCKED (3 wrong PINs) / CLOSED (admin) |
| Transfer | both accounts active; > Rs. 25,000 needs an OTP |
| Persistence | every action saved immediately (files / localStorage) |

## Bonus features implemented (all 11)

Hidden PIN entry · automatic account locking · ATM cash inventory with note
breakdown · receipt file generation · savings profit calculation · transaction
search & filter · OTP for large transfers · audit log · **loan management**
(apply / repay / interest, `loans.txt`) · **PIN/data protection** (PINs stored
hashed in a separate `pins.txt`) · a full GUI (React).

## Suggested team distribution (viva)

Division of work is allowed, but every member must be able to explain and modify
any part of the program.

| Member | Primary responsibility | Evidence |
|--------|------------------------|----------|
| 1 | Account & Transaction classes + file handling (`account`, `transaction`, `bank` load/save) | data persists correctly across runs |
| 2 | ATM Customer module (`atm.cpp`) + validation (`utils.cpp`) | live withdraw/transfer/OTP demo |
| 3 | Bank Administration module (`admin.cpp`) + reports/cash | create/freeze/unlock + reports demo |
| 4 (optional) | React bonus GUI (`frontend/`) | working futuristic dashboard |
