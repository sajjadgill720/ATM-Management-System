# Frontend Architectures — Askari Bank Digital Portal

This project contains two fully-featured, separate frontends that communicate with the C++ REST API backend server (`server.cpp`) on `http://127.0.0.1:8080`.

Both frontends represent identical banking business logic, and any operations performed on either client immediately synchronize to the central C++ database files (`accounts.txt`, `transactions.txt`, etc.).

---

## 1. C++ Askari Bank Desktop Client (`cpp_frontend/`)

The C++ desktop app is a lightweight, high-performance native Windows GUI client styled as a **clean corporate portal** suitable for commercial and retail banking clients. It replaces command shell layouts with standard rounded components and a professional light-theme design.

### Branding Color Specifications (Light Corporate Theme)
- **Primary Askari Blue**: `RGB(9, 78, 161)` (Hex `#094EA1`) — Used for primary panels, inputs outline focus, and navigation sidebar columns.
- **Accent Askari Gold/Orange**: `RGB(247, 148, 29)` (Hex `#F7941D`) — Used for active navigation tabs and highlights.
- **Danger Crimson Red**: `RGB(189, 36, 38)` (Hex `#BD2426`) — Used for secure logouts and close actions.
- **Container Panel White**: Solid `RGB(255, 255, 255)` (Hex `#FFFFFF`) — Flat clean white cards with rounded corners.
- **Slate Body Background**: Light blue-grey `RGB(240, 244, 248)` (Hex `#F0F4F8`).
- **Soft Border Outline**: Light grey `RGB(218, 224, 233)` (Hex `#DAE0E9`) for grid grids and borders.

### Technical Implementation

- **Modern Retail Typography**: Uses the Windows **Segoe UI** font system to create a clean, crisp, and high-fidelity interface instead of developer monospaced fonts.
- **Rounded Card Panel Geometry**: Drawn using standard Windows GDI `RoundRect` drawing APIs, removing developer "cyber cockpit" styling.
- **Responsive Layout Engine**: Uses resizable window styles (`WS_OVERLAPPEDWINDOW`) and calculates layout bounds relative to `g_winWidth` and `g_winHeight`.
- **Dynamic Table Grid Sizing**: The accounts directory and transaction statement table rows-per-page capacities scale automatically depending on the window's vertical size.
- **Double-Buffered Draw**: Avoids visual canvas flickering by rendering all components to an offscreen device context memory bitmap before performing a single `BitBlt` copy on `WM_PAINT`.
- **Non-Blocking Multithreading**: Performs background network tasks on separate worker threads (`std::thread`), updating the primary window procedure asynchronously via thread-safe Windows message posting (`PostMessage`).

### Core File Structure
- [main.cpp](file:///c:/Users/Sajjad%20Ur%20Rehman/Desktop/ATM_MANAGEMENT_SYSTEM/cpp_frontend/main.cpp): Desktop portal containing state machines, rendering callbacks, and REST workers.
- [Makefile](file:///c:/Users/Sajjad%20Ur%20Rehman/Desktop/ATM_MANAGEMENT_SYSTEM/cpp_frontend/Makefile): Native build script.

---

## 2. React Web Application (`frontend/`)

The React frontend is a modern web application built on React + Vite, styled with a sleek coffee-shop inspired dark theme ("espresso + cream") that feels warm, organic, and premium.

### Technical Implementation

- **State Management**: Uses React Hooks (`useCallback`, `useState`) and custom stores (`store.js`) to cache the active bank state.
- **REST Synchronization**: Performs asynchronous `fetch` calls for all mutations. After every successful operation (such as a withdrawal or PIN change), the client triggers a background refresh of `/api/state` to keep the UI in sync.
- **API Proxying**: Vite configuration (`vite.config.js`) proxies all `/api/*` traffic directly to the C++ backend server running at `127.0.0.1:8080`.
- **OTP Gateway**: Implements a two-phase transfer sequence. Large transactions (> Rs. 25,000) prompt the server to issue a secure OTP challenge. The UI shifts stages, waiting for the user to retrieve the token and verify it.

### Core File Structure
- [App.jsx](file:///c:/Users/Sajjad%20Ur%20Rehman/Desktop/ATM_MANAGEMENT_SYSTEM/frontend/src/App.jsx): Portal route dispatcher (ATM login vs Admin console).
- [AtmView.jsx](file:///c:/Users/Sajjad%20Ur%20Rehman/Desktop/ATM_MANAGEMENT_SYSTEM/frontend/src/AtmView.jsx): ATM user console, including balance boards, deposit panels, transfer modules, and mini-statements.
- [AdminView.jsx](file:///c:/Users/Sajjad%20Ur%20Rehman/Desktop/ATM_MANAGEMENT_SYSTEM/frontend/src/AdminView.jsx): Teller console with cash vault status, customer registration, and audit reports.
- [store.js](file:///c:/Users/Sajjad%20Ur%20Rehman/Desktop/ATM_MANAGEMENT_SYSTEM/frontend/src/store.js): Client-side API fetch dispatcher and account lookup hooks.

### Execution
Run the web application:
```bash
cd frontend
npm install   # (First run only)
npm run dev
```

---

## Technical Comparison Matrix

| Feature | C++ Native Desktop Client | React + Vite Web App |
|---------|---------------------------|----------------------|
| **Primary Focus** | Lightweight native execution | Responsive browser views |
| **Aesthetic Theme**| Clean Askari Retail Light Theme | Espresso & Cream dark theme |
| **Window Frame** | Fully Resizable (`WS_OVERLAPPEDWINDOW`) | Web Browser Viewport |
| **Typography** | Segoe UI (Standard modern system font) | Outfits / Inter (Web fonts) |
| **Dynamic Scaling**| Responsive coordinate recalculations | Flexbox/Grid CSS layouts |
| **GUI Framework** | Native Win32 GDI Canvas | React Component Trees |
| **Binary Size** | ~500 KB (Highly compressed standalone `.exe`) | ~10 MB (HTML/JS + `node_modules` browser runtime) |
| **Memory Footprint**| ~8 MB RAM | ~120 MB RAM (within web browser sandbox) |
| **Dependency Count**| **Zero** (Only standard system library DLLs) | > 200 `npm` package dependencies |
| **Networking Mode**| Background threads (`std::thread` + `httplib`) | Browser `fetch` API |
