

#include "../backend/httplib.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <thread>
#include <future>
#include <mutex>
#include <map>
#include <chrono>
#include <cmath>
#include <ctime>

using namespace std;

// --- Enums & Screen States ---
enum Screen {
    SCR_CONNECTING,
    SCR_GATEWAY,
    SCR_ATM_LOGIN,
    SCR_ATM_DASHBOARD,
    SCR_ADMIN_LOGIN,
    SCR_ADMIN_DASHBOARD
};

enum InputId {
    IN_ATM_ACC_NO,
    IN_ATM_PIN,
    IN_ATM_DEPOSIT_AMT,
    IN_ATM_WITHDRAW_AMT,
    IN_ATM_TRANSFER_TO,
    IN_ATM_TRANSFER_AMT,
    IN_ATM_TRANSFER_OTP,
    IN_ATM_LOAN_APPLY_AMT,
    IN_ATM_LOAN_REPAY_AMT,
    IN_ATM_OLD_PIN,
    IN_ATM_NEW_PIN,
    IN_ATM_CONFIRM_PIN,
    
    IN_ADMIN_PASS,
    IN_ADMIN_CREATE_NAME,
    IN_ADMIN_CREATE_CNIC,
    IN_ADMIN_CREATE_PHONE,
    IN_ADMIN_CREATE_PIN,
    IN_ADMIN_CREATE_OPENING,
    
    IN_ADMIN_REFILL_5000,
    IN_ADMIN_REFILL_1000,
    IN_ADMIN_REFILL_500,
    IN_ADMIN_REFILL_100
};

enum ButtonId {
    BTN_NAV_GATEWAY = 100,
    BTN_LAUNCH_ATM,
    BTN_LAUNCH_ADMIN,
    
    BTN_ATM_LOGIN_SUBMIT,
    BTN_ATM_LOGOUT,
    
    BTN_ATM_TAB_OVERVIEW,
    BTN_ATM_TAB_DEPOSIT,
    BTN_ATM_TAB_WITHDRAW,
    BTN_ATM_TAB_TRANSFER,
    BTN_ATM_TAB_LOANS,
    BTN_ATM_TAB_SECURITY,
    
    BTN_ATM_DEPOSIT_SUBMIT,
    BTN_ATM_WITHDRAW_SUBMIT,
    BTN_ATM_TRANSFER_SUBMIT,
    BTN_ATM_TRANSFER_CANCEL_OTP,
    BTN_ATM_TRANSFER_CONFIRM_OTP,
    BTN_ATM_LOAN_APPLY,
    BTN_ATM_LOAN_REPAY,
    BTN_ATM_CHANGE_PIN,
    
    BTN_ADMIN_LOGIN_SUBMIT,
    BTN_ADMIN_LOGOUT,
    BTN_ADMIN_TAB_ACCOUNTS,
    BTN_ADMIN_TAB_CREATE,
    BTN_ADMIN_TAB_REFILL,
    BTN_ADMIN_TAB_REPORTS,
    BTN_ADMIN_TAB_LEDGER,
    
    BTN_ADMIN_CREATE_TOGGLE_TYPE,
    BTN_ADMIN_CREATE_SUBMIT,
    BTN_ADMIN_REFILL_SUBMIT,
    BTN_ADMIN_APPLY_PROFIT,
    
    BTN_ADMIN_FREEZE,
    BTN_ADMIN_UNLOCK,
    BTN_ADMIN_DELETE,
    
    BTN_ADMIN_PREV_PAGE,
    BTN_ADMIN_NEXT_PAGE
};

// --- Data Models ---
struct Account {
    string accountNumber;
    string name;
    string cnic;
    string phone;
    string type;
    double balance;
    string status;
};

struct Transaction {
    string id;
    string accountNumber;
    string type;
    double amount;
    string dateTime;
    double balanceAfter;
};

struct Loan {
    string loanId;
    string accountNumber;
    double principal;
    double totalPayable;
    double paid;
    double remaining;
    string status;
    string dateIssued;
};

struct CashInventory {
    int notes5000 = 0;
    int notes1000 = 0;
    int notes500 = 0;
    int notes100 = 0;
    double total = 0;
};

struct InputField {
    InputId id;
    string label;
    string placeholder;
    string value;
    int x, y, w, h;
    bool isFocused;
    bool isNumeric;
    bool isMasked;
    size_t maxLen;
};

struct CustomButton {
    ButtonId id;
    string label;
    int x, y, w, h;
    bool isHovered;
    bool isCyberPink;
};

// --- Askari Bank Corporate Light Colors ---
const COLORREF COLOR_BG = RGB(240, 244, 248);          // Light blue-grey body background
const COLORREF COLOR_PANEL_BG = RGB(255, 255, 255);    // Clean white panel card background
const COLORREF COLOR_BORDER = RGB(218, 224, 233);      // Soft slate container border
const COLORREF COLOR_GRID_LINE = RGB(232, 236, 243);   // Thin table grid border line
const COLORREF COLOR_ROW_ALT = RGB(248, 250, 253);     // Table grid zebra alternating row
const COLORREF COLOR_GRID = COLOR_GRID_LINE;

const COLORREF COLOR_CYAN_NEON = RGB(9, 78, 161);     // Askari Brand Blue (primary blue)
const COLORREF COLOR_CYAN_DARK = RGB(6, 60, 130);     // Hover state blue
const COLORREF COLOR_CYAN_TRANSP = RGB(235, 243, 255); // Very soft blue fill for greeting box

const COLORREF COLOR_PINK_NEON = RGB(247, 148, 29);    // Askari Brand Gold/Orange (accents)
const COLORREF COLOR_PINK_DARK = RGB(220, 125, 15);    // Hover state gold
const COLORREF COLOR_PINK_TRANSP = RGB(255, 245, 232);  // Very soft gold fill for accents

const COLORREF COLOR_GREEN_NEON = RGB(40, 167, 69);    // Professional green (success / credit)
const COLORREF COLOR_RED_NEON = RGB(189, 36, 38);      // Askari Brand Red (Danger / secure logouts)

const COLORREF COLOR_TEXT_DARK = RGB(33, 37, 41);       // Primary text color (dark charcoal)
const COLORREF COLOR_TEXT_MUTED = RGB(108, 117, 125);   // Subtitles / placeholders (grey)
const COLORREF COLOR_TEXT_WHITE = RGB(255, 255, 255);   // White text

// --- Global UI State Variables ---
HWND g_hwnd = NULL;
Screen g_currentScreen = SCR_CONNECTING;
string g_currentAtmAccount = "";
string g_currentAtmTab = "overview";
string g_currentAdminTab = "accounts";

// Responsive width & height
int g_winWidth = 1024;
int g_winHeight = 720;

bool g_isConnected = false;
bool g_isCheckingConnection = false;
float g_animationPhase = 0.0f;

// OTP transfer state
bool g_transferNeedOtp = false;
string g_transferTarget = "";
double g_transferAmount = 0.0;

// Admin states
bool g_adminCreateIsSavings = false;
int g_adminAccountPage = 0;
string g_selectedAdminAccount = "";

// Dynamic widgets
vector<InputField> g_activeInputs;
InputField* g_focusedInput = nullptr;
vector<CustomButton> g_activeButtons;
POINT g_mousePos = {0, 0};

// Loaded Database Cache (fetched from server)
vector<Account> g_accounts;
vector<Transaction> g_transactions;
vector<Loan> g_loans;
CashInventory g_cash;
mutex g_dataMutex;

// Toast notification
string g_toastMessage = "";
bool g_toastSuccess = true;
DWORD g_toastExpiry = 0;

// Custom Fonts (Clean Corporate Segoe UI)
HFONT hFontTitle = NULL;
HFONT hFontHeader = NULL;
HFONT hFontNormal = NULL;
HFONT hFontConsole = NULL;

// --- Forward Declarations ---
void SetupInputs();
void SetToast(const string& msg, bool success, int durationMs = 4000);
void RefreshStateAsync();
void TriggerActionAsync(const string& url, const string& jsonBody, ButtonId btnId);
void ParseStateJson(const string& json);
void AddLog(const string& msg);

// --- String & JSON Parsing Helpers ---
static string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

static string jsonStr(const string& body, const string& key) {
    string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == string::npos) return "";
    size_t colon = body.find(':', k + pat.size());
    if (colon == string::npos) return "";
    size_t q1 = body.find('"', colon + 1);
    if (q1 == string::npos) return "";
    size_t q2 = body.find('"', q1 + 1);
    if (q2 == string::npos) return "";
    return body.substr(q1 + 1, q2 - q1 - 1);
}

static double jsonNum(const string& body, const string& key) {
    string pat = "\"" + key + "\"";
    size_t k = body.find(pat);
    if (k == string::npos) return 0;
    size_t colon = body.find(':', k + pat.size());
    if (colon == string::npos) return 0;
    size_t i = colon + 1;
    while (i < body.size() && (body[i] == ' ' || body[i] == '\t' || body[i] == '"')) i++;
    size_t start = i;
    while (i < body.size() && (isdigit((unsigned char)body[i]) || body[i] == '.' || body[i] == '-' || body[i] == '+')) i++;
    if (start == i) return 0;
    try { return stod(body.substr(start, i - start)); } catch (...) { return 0; }
}

static vector<string> parseJsonArray(const string& json, const string& arrayKey) {
    vector<string> items;
    string searchKey = "\"" + arrayKey + "\"";
    size_t pos = json.find(searchKey);
    if (pos == string::npos) return items;
    size_t startBracket = json.find('[', pos);
    if (startBracket == string::npos) return items;
    
    size_t endBracket = string::npos;
    int depth = 1;
    for (size_t i = startBracket + 1; i < json.size(); ++i) {
        if (json[i] == '[') depth++;
        else if (json[i] == ']') {
            depth--;
            if (depth == 0) {
                endBracket = i;
                break;
            }
        }
    }
    if (endBracket == string::npos) return items;
    
    string arrayContent = json.substr(startBracket + 1, endBracket - startBracket - 1);
    
    size_t idx = 0;
    while (true) {
        size_t openObj = arrayContent.find('{', idx);
        if (openObj == string::npos) break;
        size_t closeObj = arrayContent.find('}', openObj);
        if (closeObj == string::npos) break;
        items.push_back(arrayContent.substr(openObj, closeObj - openObj + 1));
        idx = closeObj + 1;
    }
    return items;
}

Account parseAccount(const string& s) {
    Account a;
    a.accountNumber = jsonStr(s, "accountNumber");
    a.name = jsonStr(s, "name");
    a.cnic = jsonStr(s, "cnic");
    a.phone = jsonStr(s, "phone");
    a.type = jsonStr(s, "type");
    a.balance = jsonNum(s, "balance");
    a.status = jsonStr(s, "status");
    return a;
}

// --- Data Cache Lookups ---
Account* findAccount(const string& accNo) {
    lock_guard<mutex> lk(g_dataMutex);
    for (auto& a : g_accounts) {
        if (a.accountNumber == accNo) return &a;
    }
    return nullptr;
}

Loan* findActiveLoan(const string& accNo) {
    lock_guard<mutex> lk(g_dataMutex);
    for (auto& l : g_loans) {
        if (l.accountNumber == accNo && l.status == "ACTIVE") return &l;
    }
    return nullptr;
}

// --- Networking Workers ---
struct AsyncRequestResult {
    bool ok;
    string url;
    string response;
    ButtonId triggerBtn;
};

void RefreshStateAsync() {
    if (g_isCheckingConnection) return;
    g_isCheckingConnection = true;
    
    thread([]() {
        httplib::Client cli("127.0.0.1", 8080);
        cli.set_connection_timeout(0, 800); // 800ms timeout
        cli.set_read_timeout(2, 0);
        
        auto res = cli.Get("/api/state");
        
        PostMessage(g_hwnd, WM_USER + 100, (WPARAM)(res && res->status == 200), (LPARAM)(res ? new string(res->body) : nullptr));
    }).detach();
}

void TriggerActionAsync(const string& url, const string& jsonBody, ButtonId btnId) {
    thread([url, jsonBody, btnId]() {
        httplib::Client cli("127.0.0.1", 8080);
        cli.set_connection_timeout(0, 1000); // 1 sec
        cli.set_read_timeout(3, 0);
        
        AsyncRequestResult* result = new AsyncRequestResult();
        result->url = url;
        result->triggerBtn = btnId;
        
        auto res = cli.Post(url, jsonBody, "application/json");
        if (res && res->status == 200) {
            result->ok = true;
            result->response = res->body;
        } else {
            result->ok = false;
            result->response = "{\"ok\":false,\"message\":\"Connection failed: Check if server is running.\"}";
        }
        
        PostMessage(g_hwnd, WM_USER + 101, (WPARAM)result, 0);
    }).detach();
}

void ParseStateJson(const string& json) {
    lock_guard<mutex> lk(g_dataMutex);
    
    // Accounts
    g_accounts.clear();
    vector<string> accountsRaw = parseJsonArray(json, "accounts");
    for (const string& s : accountsRaw) {
        g_accounts.push_back(parseAccount(s));
    }
    
    // Transactions
    g_transactions.clear();
    vector<string> txsRaw = parseJsonArray(json, "transactions");
    for (const string& s : txsRaw) {
        Transaction t;
        t.id = jsonStr(s, "id");
        t.accountNumber = jsonStr(s, "accountNumber");
        t.type = jsonStr(s, "type");
        t.amount = jsonNum(s, "amount");
        t.dateTime = jsonStr(s, "dateTime");
        t.balanceAfter = jsonNum(s, "balanceAfter");
        g_transactions.push_back(t);
    }
    
    // Loans
    g_loans.clear();
    vector<string> loansRaw = parseJsonArray(json, "loans");
    for (const string& s : loansRaw) {
        Loan l;
        l.loanId = jsonStr(s, "loanId");
        l.accountNumber = jsonStr(s, "accountNumber");
        l.principal = jsonNum(s, "principal");
        l.totalPayable = jsonNum(s, "totalPayable");
        l.paid = jsonNum(s, "paid");
        l.remaining = jsonNum(s, "remaining");
        l.status = jsonStr(s, "status");
        l.dateIssued = jsonStr(s, "dateIssued");
        g_loans.push_back(l);
    }
    
    // Cash
    size_t cashPos = json.find("\"cash\"");
    if (cashPos != string::npos) {
        size_t openBrace = json.find('{', cashPos);
        size_t closeBrace = json.find('}', cashPos);
        if (openBrace != string::npos && closeBrace != string::npos) {
            string cashStr = json.substr(openBrace, closeBrace - openBrace + 1);
            g_cash.notes5000 = (int)jsonNum(cashStr, "notes5000");
            g_cash.notes1000 = (int)jsonNum(cashStr, "notes1000");
            g_cash.notes500 = (int)jsonNum(cashStr, "notes500");
            g_cash.notes100 = (int)jsonNum(cashStr, "notes100");
            g_cash.total = jsonNum(cashStr, "total");
        }
    }
}

void SetToast(const string& msg, bool success, int durationMs) {
    g_toastMessage = msg;
    g_toastSuccess = success;
    g_toastExpiry = GetTickCount() + durationMs;
}

// --- Custom GUI Drawing Utilities (CLEAN CORPORATE STYLE) ---
void DrawRoundedRect(HDC hdc, int x, int y, int w, int h, int rx, int ry, COLORREF fillColor, COLORREF borderColor, int borderWidth = 1) {
    HBRUSH hBrush = CreateSolidBrush(fillColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hPen = CreatePen(PS_SOLID, borderWidth, borderColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    RoundRect(hdc, x, y, x + w, y + h, rx, ry);
    
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void DrawButton(HDC hdc, const CustomButton& b) {
    COLORREF fillCol, borderCol, txtCol;
    
    bool isSidebarBtn = (b.x < 250 && g_currentScreen >= SCR_ATM_DASHBOARD);
    if (isSidebarBtn) {
        // Sidebar button styles
        bool isActive = false;
        if (g_currentScreen == SCR_ATM_DASHBOARD) {
            if (b.id == BTN_ATM_TAB_OVERVIEW && g_currentAtmTab == "overview") isActive = true;
            else if (b.id == BTN_ATM_TAB_DEPOSIT && g_currentAtmTab == "deposit") isActive = true;
            else if (b.id == BTN_ATM_TAB_WITHDRAW && g_currentAtmTab == "withdraw") isActive = true;
            else if (b.id == BTN_ATM_TAB_TRANSFER && g_currentAtmTab == "transfer") isActive = true;
            else if (b.id == BTN_ATM_TAB_LOANS && g_currentAtmTab == "loans") isActive = true;
            else if (b.id == BTN_ATM_TAB_SECURITY && g_currentAtmTab == "security") isActive = true;
        } else if (g_currentScreen == SCR_ADMIN_DASHBOARD) {
            if (b.id == BTN_ADMIN_TAB_ACCOUNTS && g_currentAdminTab == "accounts") isActive = true;
            else if (b.id == BTN_ADMIN_TAB_CREATE && g_currentAdminTab == "create") isActive = true;
            else if (b.id == BTN_ADMIN_TAB_REFILL && g_currentAdminTab == "refill") isActive = true;
            else if (b.id == BTN_ADMIN_TAB_REPORTS && g_currentAdminTab == "reports") isActive = true;
            else if (b.id == BTN_ADMIN_TAB_LEDGER && g_currentAdminTab == "ledger") isActive = true;
        }
        
        if (b.id == BTN_ATM_LOGOUT || b.id == BTN_ADMIN_LOGOUT) {
            fillCol = b.isHovered ? RGB(140, 20, 20) : COLOR_RED_NEON;
            borderCol = fillCol;
            txtCol = COLOR_TEXT_WHITE;
        }
        else if (isActive) {
            fillCol = COLOR_PINK_NEON; // Gold/orange active indicator
            borderCol = fillCol;
            txtCol = COLOR_TEXT_WHITE;
        } else {
            fillCol = b.isHovered ? COLOR_CYAN_DARK : COLOR_CYAN_NEON; // corporate blue sidebar tabs
            borderCol = fillCol;
            txtCol = COLOR_TEXT_WHITE;
        }
    } else {
        // Standard panel buttons
        if (b.isCyberPink) {
            fillCol = b.isHovered ? COLOR_PINK_DARK : COLOR_PINK_NEON;
            borderCol = fillCol;
            txtCol = COLOR_TEXT_WHITE;
        } else {
            fillCol = b.isHovered ? COLOR_CYAN_DARK : COLOR_CYAN_NEON;
            borderCol = fillCol;
            txtCol = COLOR_TEXT_WHITE;
        }
    }
    
    DrawRoundedRect(hdc, b.x, b.y, b.w, b.h, 6, 6, fillCol, borderCol, 1);
    
    RECT rectText = { b.x, b.y + (b.h - 14) / 2, b.x + b.w, b.y + b.h };
    SetTextColor(hdc, txtCol);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFontNormal);
    DrawText(hdc, b.label.c_str(), -1, &rectText, DT_CENTER | DT_SINGLELINE);
}

void AddButton(ButtonId id, const string& label, int x, int y, int w, int h, bool isPink = false) {
    CustomButton b;
    b.id = id;
    b.label = label;
    b.x = x;
    b.y = y;
    b.w = w;
    b.h = h;
    b.isHovered = (g_mousePos.x >= x && g_mousePos.x <= x + w && g_mousePos.y >= y && g_mousePos.y <= y + h);
    b.isCyberPink = isPink;
    g_activeButtons.push_back(b);
}

// Draw toast notification on center screen
void DrawToast(HDC hdc, int w) {
    if (GetTickCount() > g_toastExpiry) return;
    
    int toastW = 480;
    int toastH = 40;
    int toastX = (w - toastW) / 2;
    int toastY = 80;
    
    COLORREF border = g_toastSuccess ? COLOR_GREEN_NEON : COLOR_RED_NEON;
    COLORREF bg = COLOR_PANEL_BG;
    
    DrawRoundedRect(hdc, toastX, toastY, toastW, toastH, 6, 6, bg, border, 2);
    
    RECT rText = { toastX + 15, toastY + (toastH - 14)/2, toastX + toastW - 15, toastY + toastH };
    SetTextColor(hdc, g_toastSuccess ? COLOR_GREEN_NEON : COLOR_RED_NEON);
    SelectObject(hdc, hFontNormal);
    DrawText(hdc, g_toastMessage.c_str(), -1, &rText, DT_CENTER | DT_SINGLELINE);
}

// Setup responsive text input controls coordinates
void SetupInputs() {
    g_activeInputs.clear();
    g_focusedInput = nullptr;
    
    int w = g_winWidth;
    int h = g_winHeight;
    
    if (g_currentScreen == SCR_ATM_LOGIN) {
        int formW = 340;
        int formH = 320;
        int formX = (w - formW) / 2;
        int formY = (h - formH) / 2;
        if (formY < 80) formY = 80;
        
        g_activeInputs.push_back({ IN_ATM_ACC_NO, "Account Number", "10017", "", formX + 50, formY + 90, 240, 32, false, true, false, 10 });
        g_activeInputs.push_back({ IN_ATM_PIN, "Security PIN", "••••", "", formX + 50, formY + 160, 240, 32, false, true, true, 4 });
    }
    else if (g_currentScreen == SCR_ADMIN_LOGIN) {
        int formW = 340;
        int formH = 260;
        int formX = (w - formW) / 2;
        int formY = (h - formH) / 2;
        if (formY < 80) formY = 80;
        
        g_activeInputs.push_back({ IN_ADMIN_PASS, "Security Password", "admin123", "", formX + 50, formY + 110, 240, 32, false, false, true, 20 });
    }
    else if (g_currentScreen == SCR_ATM_DASHBOARD) {
        int mainX = 260;
        int mainY = 90;
        int mainW = w - 280;
        
        if (g_currentAtmTab == "deposit") {
            g_activeInputs.push_back({ IN_ATM_DEPOSIT_AMT, "Enter Deposit Amount (Rs.)", "0.00", "", mainX + (mainW - 240)/2, mainY + 120, 240, 32, false, true, false, 10 });
        }
        else if (g_currentAtmTab == "withdraw") {
            g_activeInputs.push_back({ IN_ATM_WITHDRAW_AMT, "Enter Withdrawal Amount (Rs.)", "0.00", "", mainX + (mainW - 240)/2, mainY + 120, 240, 32, false, true, false, 10 });
        }
        else if (g_currentAtmTab == "transfer") {
            int cX = mainX + (mainW - 240)/2;
            if (g_transferNeedOtp) {
                g_activeInputs.push_back({ IN_ATM_TRANSFER_OTP, "Enter One-Time Password (OTP)", "Check otp.txt / console", "", cX, mainY + 140, 240, 32, false, true, false, 6 });
            } else {
                g_activeInputs.push_back({ IN_ATM_TRANSFER_TO, "Target Account Number", "e.g. 10025", "", cX, mainY + 90, 240, 32, false, true, false, 10 });
                g_activeInputs.push_back({ IN_ATM_TRANSFER_AMT, "Transfer Amount (Rs.)", "0.00", "", cX, mainY + 160, 240, 32, false, true, false, 10 });
            }
        }
        else if (g_currentAtmTab == "loans") {
            Loan* activeL = findActiveLoan(g_currentAtmAccount);
            int cX = mainX + (mainW - 240)/2;
            if (activeL) {
                g_activeInputs.push_back({ IN_ATM_LOAN_REPAY_AMT, "Repayment Amount (Rs.)", "0.00", "", cX, mainY + 210, 240, 32, false, true, false, 10 });
            } else {
                g_activeInputs.push_back({ IN_ATM_LOAN_APPLY_AMT, "Loan Amount Requested (Rs.)", "0.00", "", cX, mainY + 120, 240, 32, false, true, false, 10 });
            }
        }
        else if (g_currentAtmTab == "security") {
            int cX = mainX + (mainW - 200)/2;
            g_activeInputs.push_back({ IN_ATM_OLD_PIN, "Current Security PIN", "••••", "", cX, mainY + 90, 200, 32, false, true, true, 4 });
            g_activeInputs.push_back({ IN_ATM_NEW_PIN, "New Security PIN", "••••", "", cX, mainY + 155, 200, 32, false, true, true, 4 });
            g_activeInputs.push_back({ IN_ATM_CONFIRM_PIN, "Confirm New PIN", "••••", "", cX, mainY + 220, 200, 32, false, true, true, 4 });
        }
    }
    else if (g_currentScreen == SCR_ADMIN_DASHBOARD) {
        int mainX = 260;
        int mainY = 90;
        
        if (g_currentAdminTab == "create") {
            g_activeInputs.push_back({ IN_ADMIN_CREATE_NAME, "Customer Full Name", "Sajjad Gill", "", mainX + 40, mainY + 70, 250, 30, false, false, false, 40 });
            g_activeInputs.push_back({ IN_ADMIN_CREATE_CNIC, "CNIC Number (no dashes)", "3520112345679", "", mainX + 40, mainY + 125, 250, 30, false, true, false, 15 });
            g_activeInputs.push_back({ IN_ADMIN_CREATE_PHONE, "Contact Mobile Number", "03001234567", "", mainX + 40, mainY + 180, 250, 30, false, true, false, 12 });
            g_activeInputs.push_back({ IN_ADMIN_CREATE_PIN, "Initial 4-Digit PIN", "1234", "", mainX + 40, mainY + 235, 250, 30, false, true, true, 4 });
            g_activeInputs.push_back({ IN_ADMIN_CREATE_OPENING, "Opening Balance (Rs.)", "1000.00", "", mainX + 40, mainY + 290, 250, 30, false, true, false, 10 });
        }
        else if (g_currentAdminTab == "refill") {
            g_activeInputs.push_back({ IN_ADMIN_REFILL_5000, "Refill notes of Rs. 5000", "0", "", mainX + 40, mainY + 90, 220, 30, false, true, false, 4 });
            g_activeInputs.push_back({ IN_ADMIN_REFILL_1000, "Refill notes of Rs. 1000", "0", "", mainX + 40, mainY + 145, 220, 30, false, true, false, 4 });
            g_activeInputs.push_back({ IN_ADMIN_REFILL_500,  "Refill notes of Rs. 500",  "0", "", mainX + 40, mainY + 200, 220, 30, false, true, false, 4 });
            g_activeInputs.push_back({ IN_ADMIN_REFILL_100,  "Refill notes of Rs. 100",  "0", "", mainX + 40, mainY + 255, 220, 30, false, true, false, 4 });
        }
    }
}

// Drawing inputs inside layout
void DrawActiveInputs(HDC hdc) {
    for (auto& in : g_activeInputs) {
        // Label
        RECT rLabel = { in.x, in.y - 18, in.x + in.w, in.y };
        SetTextColor(hdc, COLOR_TEXT_DARK);
        SelectObject(hdc, hFontConsole);
        DrawText(hdc, in.label.c_str(), -1, &rLabel, DT_LEFT | DT_TOP);
        
        // Input container box
        COLORREF outline = in.isFocused ? COLOR_CYAN_NEON : COLOR_BORDER;
        DrawRoundedRect(hdc, in.x, in.y, in.w, in.h, 6, 6, COLOR_PANEL_BG, outline, in.isFocused ? 2 : 1);
        
        // Input text string
        RECT rText = { in.x + 8, in.y + (in.h - 14)/2, in.x + in.w - 8, in.y + in.h };
        if (in.value.empty()) {
            SetTextColor(hdc, COLOR_TEXT_MUTED);
            SelectObject(hdc, hFontNormal);
            DrawText(hdc, in.placeholder.c_str(), -1, &rText, DT_LEFT | DT_TOP);
        } else {
            SetTextColor(hdc, COLOR_TEXT_DARK);
            SelectObject(hdc, hFontNormal);
            string disp = in.value;
            if (in.isMasked) disp = string(in.value.size(), 'x'); // clean x placeholder instead of star
            DrawText(hdc, disp.c_str(), -1, &rText, DT_LEFT | DT_TOP);
        }
        
        // Blinking cursor
        if (in.isFocused && (GetTickCount() / 500) % 2 == 0) {
            SIZE s;
            string disp = in.value;
            if (in.isMasked) disp = string(in.value.size(), 'x');
            SelectObject(hdc, hFontNormal);
            GetTextExtentPoint32(hdc, disp.c_str(), (int)disp.size(), &s);
            
            int curX = in.x + 8 + s.cx;
            HPEN hPen = CreatePen(PS_SOLID, 1, COLOR_CYAN_NEON);
            HPEN hOld = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, curX, in.y + 6, NULL);
            LineTo(hdc, curX, in.y + in.h - 6);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);
        }
    }
}

// --- Window Paint Engine (RESPONSIVE CORPORATE LIGHT THEME) ---
void PaintWindow(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    int w = g_winWidth;
    int h = g_winHeight;
    
    // Double buffering initialization
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ hOld = SelectObject(hdcMem, hbmMem);
    
    // 1. Clear background (corporate light blue-grey background)
    HBRUSH hBgBrush = CreateSolidBrush(COLOR_BG);
    RECT rectClient = {0, 0, w, h};
    FillRect(hdcMem, &rectClient, hBgBrush);
    DeleteObject(hBgBrush);
    
    // 2. Draw subtle layout grid lines
    HPEN hGridPen = CreatePen(PS_SOLID, 1, COLOR_GRID);
    HPEN hOldPen = (HPEN)SelectObject(hdcMem, hGridPen);
    for (int x = 0; x < w; x += 40) {
        MoveToEx(hdcMem, x, 0, NULL);
        LineTo(hdcMem, x, h);
    }
    for (int y = 0; y < h; y += 40) {
        MoveToEx(hdcMem, 0, y, NULL);
        LineTo(hdcMem, w, y);
    }
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hGridPen);
    
    // Reset active buttons vector layout
    g_activeButtons.clear();
    
    // 3. Render Header Banner (Clean flat container)
    DrawRoundedRect(hdcMem, 20, 20, w - 40, 50, 8, 8, COLOR_PANEL_BG, COLOR_BORDER, 1);
    
    // Draw Askari Bank logo and header text
    SetTextColor(hdcMem, COLOR_CYAN_NEON);
    SetBkMode(hdcMem, TRANSPARENT);
    SelectObject(hdcMem, hFontTitle);
    RECT rTitle = { 35, 30, w - 100, 60 };
    DrawText(hdcMem, "ASKARI BANK // DIGITAL TELLER PORTAL", -1, &rTitle, DT_LEFT | DT_TOP);
    
    // Server connection indicator
    RECT rStatus = { w - 240, 35, w - 35, 60 };
    if (g_isConnected) {
        SetTextColor(hdcMem, COLOR_GREEN_NEON);
        SelectObject(hdcMem, hFontConsole);
        DrawText(hdcMem, "[SYSTEM ACTIVE]", -1, &rStatus, DT_RIGHT);
    } else {
        SetTextColor(hdcMem, COLOR_RED_NEON);
        SelectObject(hdcMem, hFontConsole);
        DrawText(hdcMem, "[SYSTEM OFFLINE]", -1, &rStatus, DT_RIGHT);
    }
    
    // Render specific screens
    if (g_currentScreen == SCR_CONNECTING) {
        SetTextColor(hdcMem, COLOR_TEXT_DARK);
        SelectObject(hdcMem, hFontHeader);
        RECT rC = { 50, h/2 - 50, w - 50, h/2 - 10 };
        DrawText(hdcMem, "Connecting to Askari Banking Core Gateways...", -1, &rC, DT_CENTER);
        
        SetTextColor(hdcMem, COLOR_TEXT_MUTED);
        SelectObject(hdcMem, hFontNormal);
        RECT rSub = { 50, h/2 - 10, w - 50, h/2 + 60 };
        DrawText(hdcMem, "Querying REST gateway node http://127.0.0.1:8080/api/state.\nPlease ensure the C++ server backend (server.exe) is running in the background.", -1, &rSub, DT_CENTER);
    }
    else if (g_currentScreen == SCR_GATEWAY) {
        int panelW = (w - 240) / 2;
        int panelH = h - 200;
        if (panelH < 260) panelH = 260;
        
        // ATM Client launch panel
        DrawRoundedRect(hdcMem, 80, 100, panelW, panelH, 12, 12, COLOR_PANEL_BG, COLOR_BORDER, 1);
        SetTextColor(hdcMem, COLOR_CYAN_NEON);
        SelectObject(hdcMem, hFontHeader);
        RECT rAtmH = { 80, 130, 80 + panelW, 160 };
        DrawText(hdcMem, "ATM Customer Gateway", -1, &rAtmH, DT_CENTER);
        
        SetTextColor(hdcMem, COLOR_TEXT_DARK);
        SelectObject(hdcMem, hFontNormal);
        RECT rAtmD = { 105, 175, 80 + panelW - 25, 100 + panelH - 80 };
        DrawText(hdcMem, "Self-service client banking terminal for verified account holders.\n\n* Balances inquiry\n* Cash deposits\n* Secure wire transfers with OTP tokens\n* Loan applications and repayment logs\n* Modify authentication PIN keys", -1, &rAtmD, DT_LEFT | DT_WORDBREAK);
        AddButton(BTN_LAUNCH_ATM, "CUSTOMER LOGIN", 80 + (panelW - 240)/2, 100 + panelH - 60, 240, 40, false);
        
        // Admin Terminal launch panel
        DrawRoundedRect(hdcMem, w / 2 + 40, 100, panelW, panelH, 12, 12, COLOR_PANEL_BG, COLOR_BORDER, 1);
        SetTextColor(hdcMem, COLOR_PINK_NEON);
        SelectObject(hdcMem, hFontHeader);
        RECT rAdmH = { w / 2 + 40, 130, w / 2 + 40 + panelW, 160 };
        DrawText(hdcMem, "Staff Administration Portal", -1, &rAdmH, DT_CENTER);
        
        SetTextColor(hdcMem, COLOR_TEXT_DARK);
        SelectObject(hdcMem, hFontNormal);
        RECT rAdmD = { w / 2 + 65, 175, w / 2 + 40 + panelW - 25, 100 + panelH - 80 };
        DrawText(hdcMem, "Teller and administrative console for database configuration.\n\n* Create new customer banking records\n* Refill physical ATM vault reserves\n* Freeze / Unlock locked user accounts\n* Execute interest profit allocations\n* Access live transaction audit books", -1, &rAdmD, DT_LEFT | DT_WORDBREAK);
        AddButton(BTN_LAUNCH_ADMIN, "TELLER LOGIN", w / 2 + 40 + (panelW - 240)/2, 100 + panelH - 60, 240, 40, true);
    }
    else if (g_currentScreen == SCR_ATM_LOGIN) {
        int formW = 340;
        int formH = 320;
        int formX = (w - formW) / 2;
        int formY = (h - formH) / 2;
        if (formY < 80) formY = 80;
        
        DrawRoundedRect(hdcMem, formX, formY, formW, formH, 10, 10, COLOR_PANEL_BG, COLOR_BORDER, 1);
        SetTextColor(hdcMem, COLOR_CYAN_NEON);
        SelectObject(hdcMem, hFontHeader);
        RECT rLog = { formX, formY + 20, formX + formW, formY + 50 };
        DrawText(hdcMem, "ATM CUSTOMER LOGIN", -1, &rLog, DT_CENTER);
        
        DrawActiveInputs(hdcMem);
        AddButton(BTN_ATM_LOGIN_SUBMIT, "LOGIN", formX + 50, formY + 215, 240, 38, false);
        AddButton(BTN_NAV_GATEWAY, "← BACK TO HOME", formX + 50, formY + 265, 240, 32, true);
    }
    else if (g_currentScreen == SCR_ATM_DASHBOARD) {
        Account* activeAcc = findAccount(g_currentAtmAccount);
        int sidebarW = 220;
        int sidebarH = h - 110;
        if (sidebarH < 260) sidebarH = 260;
        
        // Navigation side column (Navy Blue Panel)
        DrawRoundedRect(hdcMem, 20, 90, sidebarW, sidebarH, 8, 8, COLOR_CYAN_NEON, COLOR_CYAN_NEON, 1);
        
        vector<pair<string, ButtonId>> tabs = {
            {"OVERVIEW STATEMENT", BTN_ATM_TAB_OVERVIEW},
            {"DEPOSIT CASH", BTN_ATM_TAB_DEPOSIT},
            {"WITHDRAW CASH", BTN_ATM_TAB_WITHDRAW},
            {"EXPRESS FUNDS TRANSFER", BTN_ATM_TAB_TRANSFER},
            {"CREDIT LOAN MANAGER", BTN_ATM_TAB_LOANS},
            {"SECURITY KEY CONFIG", BTN_ATM_TAB_SECURITY}
        };
        
        int btnY = 110;
        for (auto& tab : tabs) {
            AddButton(tab.second, tab.first, 35, btnY, 190, 35, false);
            btnY += 45;
        }
        AddButton(BTN_ATM_LOGOUT, "LOGOUT", 35, 90 + sidebarH - 50, 190, 35, true);
        
        // Main detail panel (stretches to take up remaining screen width/height)
        int mainX = 260;
        int mainY = 90;
        int mainW = w - 280;
        int mainH = h - 110;
        if (mainH < 260) mainH = 260;
        
        DrawRoundedRect(hdcMem, mainX, mainY, mainW, mainH, 8, 8, COLOR_PANEL_BG, COLOR_BORDER, 1);
        
        if (activeAcc) {
            // Customer greeting banner
            DrawRoundedRect(hdcMem, mainX + 20, mainY + 20, mainW - 40, 65, 6, 6, COLOR_CYAN_TRANSP, COLOR_CYAN_NEON, 1);
            
            // Name / Account / Status
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rG = { mainX + 35, mainY + 30, mainX + mainW - 300, mainY + 55 };
            DrawText(hdcMem, activeAcc->name.c_str(), -1, &rG, DT_LEFT);
            
            SetTextColor(hdcMem, COLOR_TEXT_MUTED);
            SelectObject(hdcMem, hFontNormal);
            RECT rSub = { mainX + 35, mainY + 55, mainX + mainW - 300, mainY + 75 };
            string subText = "A/C: " + activeAcc->accountNumber + " | SCHEME: " + activeAcc->type + " | STATE: " + activeAcc->status;
            DrawText(hdcMem, subText.c_str(), -1, &rSub, DT_LEFT);
            
            // Balance Panel
            SetTextColor(hdcMem, COLOR_TEXT_DARK);
            SelectObject(hdcMem, hFontNormal);
            RECT rBalL = { mainX + mainW - 280, mainY + 30, mainX + mainW - 35, mainY + 50 };
            DrawText(hdcMem, "AVAILABLE BALANCE", -1, &rBalL, DT_RIGHT);
            
            ostringstream ssBal;
            ssBal << "Rs. " << fixed << setprecision(2) << activeAcc->balance;
            SetTextColor(hdcMem, COLOR_GREEN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rBalV = { mainX + mainW - 280, mainY + 50, mainX + mainW - 35, mainY + 75 };
            DrawText(hdcMem, ssBal.str().c_str(), -1, &rBalV, DT_RIGHT);
            
            // Draw active tab views
            if (g_currentAtmTab == "overview") {
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontHeader);
                RECT rSecH = { mainX + 20, mainY + 105, mainX + mainW - 20, mainY + 125 };
                DrawText(hdcMem, "LEDGER ACCOUNT ACTIVITY STATEMENT", -1, &rSecH, DT_LEFT);
                
                // Transactions table
                int tY = mainY + 130;
                DrawRoundedRect(hdcMem, mainX + 20, tY, mainW - 40, 30, 4, 4, COLOR_BG, COLOR_BORDER, 1);
                
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontConsole);
                
                int colW = (mainW - 80) / 5;
                RECT rCol1 = { mainX + 30, tY + 8, mainX + 30 + colW, tY + 28 }; DrawText(hdcMem, "TX-ID", -1, &rCol1, DT_LEFT);
                RECT rCol2 = { mainX + 30 + colW, tY + 8, mainX + 30 + colW * 2, tY + 28 }; DrawText(hdcMem, "DATE & TIME", -1, &rCol2, DT_LEFT);
                RECT rCol3 = { mainX + 40 + colW * 2, tY + 8, mainX + 40 + colW * 3, tY + 28 }; DrawText(hdcMem, "TYPE", -1, &rCol3, DT_LEFT);
                RECT rCol4 = { mainX + 40 + colW * 3, tY + 8, mainX + 40 + colW * 4, tY + 28 }; DrawText(hdcMem, "AMOUNT", -1, &rCol4, DT_RIGHT);
                RECT rCol5 = { mainX + 40 + colW * 4, tY + 8, mainX + mainW - 30, tY + 28 }; DrawText(hdcMem, "BALANCE", -1, &rCol5, DT_RIGHT);
                
                lock_guard<mutex> lk(g_dataMutex);
                vector<Transaction> localTx;
                for (auto& tx : g_transactions) {
                    if (tx.accountNumber == activeAcc->accountNumber) {
                        localTx.push_back(tx);
                    }
                }
                
                // Show rows matching screen height capacity
                int drawn = 0;
                int maxRows = (mainH - 180) / 30;
                if (maxRows < 2) maxRows = 2;
                if (maxRows > 15) maxRows = 15;
                
                int idx = (int)localTx.size() - 1;
                tY += 35;
                while (idx >= 0 && drawn < maxRows) {
                    Transaction& tx = localTx[idx];
                    
                    // Zebra striping colors
                    COLORREF rowBg = (drawn % 2 == 0) ? COLOR_PANEL_BG : COLOR_ROW_ALT;
                    DrawRoundedRect(hdcMem, mainX + 20, tY, mainW - 40, 25, 4, 4, rowBg, COLOR_GRID_LINE, 1);
                    
                    SetTextColor(hdcMem, COLOR_TEXT_DARK);
                    SelectObject(hdcMem, hFontConsole);
                    
                    RECT r1 = { mainX + 30, tY + 5, mainX + 30 + colW, tY + 25 }; DrawText(hdcMem, tx.id.c_str(), -1, &r1, DT_LEFT);
                    RECT r2 = { mainX + 30 + colW, tY + 5, mainX + 30 + colW * 2, tY + 25 }; DrawText(hdcMem, tx.dateTime.c_str(), -1, &r2, DT_LEFT);
                    
                    if (tx.type == "WITHDRAW" || tx.type == "TRANSFER_OUT") SetTextColor(hdcMem, COLOR_RED_NEON);
                    else SetTextColor(hdcMem, COLOR_GREEN_NEON);
                    
                    RECT r3 = { mainX + 40 + colW * 2, tY + 5, mainX + 40 + colW * 3, tY + 25 }; DrawText(hdcMem, tx.type.c_str(), -1, &r3, DT_LEFT);
                    
                    ostringstream ssAmt, ssBalAfter;
                    ssAmt << (tx.type == "WITHDRAW" || tx.type == "TRANSFER_OUT" ? "- " : "+ ") << tx.amount;
                    ssBalAfter << tx.balanceAfter;
                    
                    RECT r4 = { mainX + 40 + colW * 3, tY + 5, mainX + 40 + colW * 4, tY + 25 }; DrawText(hdcMem, ssAmt.str().c_str(), -1, &r4, DT_RIGHT);
                    SetTextColor(hdcMem, COLOR_TEXT_DARK);
                    RECT r5 = { mainX + 40 + colW * 4, tY + 5, mainX + mainW - 30, tY + 25 }; DrawText(hdcMem, ssBalAfter.str().c_str(), -1, &r5, DT_RIGHT);
                    
                    tY += 28;
                    drawn++;
                    idx--;
                }
                
                if (localTx.empty()) {
                    SetTextColor(hdcMem, COLOR_TEXT_MUTED);
                    SelectObject(hdcMem, hFontNormal);
                    RECT rEmpty = { mainX + 20, mainY + 200, mainX + mainW - 20, mainY + 240 };
                    DrawText(hdcMem, "No historical transactions detected on this account node.", -1, &rEmpty, DT_CENTER);
                }
            }
            else if (g_currentAtmTab == "deposit") {
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontHeader);
                RECT rDep = { mainX + 20, mainY + 105, mainX + mainW - 20, mainY + 130 };
                DrawText(hdcMem, "DEPOSIT CASH RESERVES", -1, &rDep, DT_LEFT);
                
                DrawActiveInputs(hdcMem);
                int cX = mainX + (mainW - 240) / 2;
                AddButton(BTN_ATM_DEPOSIT_SUBMIT, "DEPOSIT CASH", cX, mainY + 175, 240, 35, false);
            }
            else if (g_currentAtmTab == "withdraw") {
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontHeader);
                RECT rWit = { mainX + 20, mainY + 105, mainX + mainW - 20, mainY + 130 };
                DrawText(hdcMem, "WITHDRAW PHYSICAL CASH", -1, &rWit, DT_LEFT);
                
                int cX = mainX + (mainW - 240) / 2;
                DrawActiveInputs(hdcMem);
                
                SetTextColor(hdcMem, COLOR_TEXT_MUTED);
                SelectObject(hdcMem, hFontNormal);
                RECT rRules = { mainX + 40, mainY + 230, mainX + mainW - 40, mainY + 310 };
                DrawText(hdcMem, "* Notes must be in multiples of 100, 500, 1000, 5000.\n* Daily transaction maximum: Rs. 50,000.\n* System conducts physical checks on note stock levels before execution.", -1, &rRules, DT_LEFT);
                
                AddButton(BTN_ATM_WITHDRAW_SUBMIT, "WITHDRAW CASH", cX, mainY + 175, 240, 35, false);
            }
            else if (g_currentAtmTab == "transfer") {
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontHeader);
                RECT rTr = { mainX + 20, mainY + 105, mainX + mainW - 20, mainY + 130 };
                DrawText(hdcMem, "EXPRESS FUNDS TRANSFER", -1, &rTr, DT_LEFT);
                
                DrawActiveInputs(hdcMem);
                int cX = mainX + (mainW - 240)/2;
                if (g_transferNeedOtp) {
                    AddButton(BTN_ATM_TRANSFER_CONFIRM_OTP, "VERIFY OTP", cX, mainY + 200, 240, 35, false);
                    AddButton(BTN_ATM_TRANSFER_CANCEL_OTP, "CANCEL", cX, mainY + 245, 240, 32, true);
                } else {
                    AddButton(BTN_ATM_TRANSFER_SUBMIT, "TRANSFER FUNDS", cX, mainY + 230, 240, 35, false);
                }
            }
            else if (g_currentAtmTab == "loans") {
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontHeader);
                RECT rLn = { mainX + 20, mainY + 105, mainX + mainW - 20, mainY + 130 };
                DrawText(hdcMem, "LOAN MANAGEMENT PORTAL", -1, &rLn, DT_LEFT);
                
                Loan* activeL = findActiveLoan(g_currentAtmAccount);
                int cX = mainX + (mainW - 240)/2;
                if (activeL) {
                    // Show active loan details in clean box
                    DrawRoundedRect(hdcMem, mainX + 40, mainY + 130, 260, 140, 6, 6, COLOR_PINK_TRANSP, COLOR_PINK_NEON, 1);
                    SetTextColor(hdcMem, COLOR_PINK_NEON);
                    SelectObject(hdcMem, hFontNormal);
                    RECT rLnH = { mainX + 50, mainY + 140, mainX + 280, mainY + 160 }; DrawText(hdcMem, "ACTIVE LOAN BALANCE", -1, &rLnH, DT_LEFT);
                    
                    SetTextColor(hdcMem, COLOR_TEXT_DARK);
                    SelectObject(hdcMem, hFontConsole);
                    
                    ostringstream ssP, ssPay, ssPaid, ssRem;
                    ssP << "Principal: Rs. " << activeL->principal;
                    ssPay << "Payable:   Rs. " << activeL->totalPayable;
                    ssPaid << "Paid:      Rs. " << activeL->paid;
                    ssRem << "Remaining: Rs. " << activeL->remaining;
                    
                    RECT r1 = { mainX + 50, mainY + 170, mainX + 280, mainY + 190 }; DrawText(hdcMem, ssP.str().c_str(), -1, &r1, DT_LEFT);
                    RECT r2 = { mainX + 50, mainY + 190, mainX + 280, mainY + 210 }; DrawText(hdcMem, ssPay.str().c_str(), -1, &r2, DT_LEFT);
                    RECT r3 = { mainX + 50, mainY + 210, mainX + 280, mainY + 230 }; DrawText(hdcMem, ssPaid.str().c_str(), -1, &r3, DT_LEFT);
                    
                    SetTextColor(hdcMem, COLOR_RED_NEON);
                    RECT r4 = { mainX + 50, mainY + 235, mainX + 280, mainY + 260 }; DrawText(hdcMem, ssRem.str().c_str(), -1, &r4, DT_LEFT);
                    
                    DrawActiveInputs(hdcMem);
                    AddButton(BTN_ATM_LOAN_REPAY, "REPAY LOAN", cX, mainY + 260, 240, 35, false);
                } else {
                    SetTextColor(hdcMem, COLOR_TEXT_MUTED);
                    SelectObject(hdcMem, hFontNormal);
                    RECT rNoLn = { mainX + 40, mainY + 170, mainX + mainW - 40, mainY + 220 };
                    DrawText(hdcMem, "No outstanding bank loans detected.\nInterest rate scheme standard: 10% flat.\nCredit funds are credited directly to available balance.", -1, &rNoLn, DT_LEFT);
                    
                    DrawActiveInputs(hdcMem);
                    AddButton(BTN_ATM_LOAN_APPLY, "APPLY FOR LOAN", cX, mainY + 180, 240, 35, false);
                }
            }
            else if (g_currentAtmTab == "security") {
                SetTextColor(hdcMem, COLOR_CYAN_NEON);
                SelectObject(hdcMem, hFontHeader);
                RECT rSec = { mainX + 20, mainY + 105, mainX + mainW - 20, mainY + 130 };
                DrawText(hdcMem, "UPDATE SECURITY PIN", -1, &rSec, DT_LEFT);
                
                DrawActiveInputs(hdcMem);
                int cX = mainX + (mainW - 200)/2;
                AddButton(BTN_ATM_CHANGE_PIN, "UPDATE PIN", cX, mainY + 280, 200, 35, false);
            }
        }
    }
    else if (g_currentScreen == SCR_ADMIN_LOGIN) {
        int formW = 340;
        int formH = 260;
        int formX = (w - formW) / 2;
        int formY = (h - formH) / 2;
        if (formY < 80) formY = 80;
        
        DrawRoundedRect(hdcMem, formX, formY, formW, formH, 10, 10, COLOR_PANEL_BG, COLOR_BORDER, 1);
        SetTextColor(hdcMem, COLOR_PINK_NEON);
        SelectObject(hdcMem, hFontHeader);
        RECT rLog = { formX, formY + 20, formX + formW, formY + 50 };
        DrawText(hdcMem, "STAFF ADMINISTRATIVE LOGIN", -1, &rLog, DT_CENTER);
        
        DrawActiveInputs(hdcMem);
        AddButton(BTN_ADMIN_LOGIN_SUBMIT, "LOGIN", formX + 50, formY + 160, 240, 38, true);
        AddButton(BTN_NAV_GATEWAY, "← BACK TO HOME", formX + 50, formY + 210, 240, 32, false);
    }
    else if (g_currentScreen == SCR_ADMIN_DASHBOARD) {
        int sidebarW = 220;
        int sidebarH = h - 110;
        if (sidebarH < 260) sidebarH = 260;
        
        // Staff Navigation Sidebar (Navy Blue Panel)
        DrawRoundedRect(hdcMem, 20, 90, sidebarW, sidebarH, 8, 8, COLOR_CYAN_NEON, COLOR_CYAN_NEON, 1);
        
        vector<pair<string, ButtonId>> tabs = {
            {"USER ACCOUNTS DIRECTORY", BTN_ADMIN_TAB_ACCOUNTS},
            {"REGISTER CUSTOMER NODE", BTN_ADMIN_TAB_CREATE},
            {"REFILL ATM CASH RESERVES", BTN_ADMIN_TAB_REFILL},
            {"CORE SYSTEM METRICS", BTN_ADMIN_TAB_REPORTS},
            {"SYSTEM WIRE LEDGER", BTN_ADMIN_TAB_LEDGER}
        };
        
        int btnY = 110;
        for (auto& tab : tabs) {
            AddButton(tab.second, tab.first, 35, btnY, 190, 35, true);
            btnY += 45;
        }
        AddButton(BTN_ADMIN_LOGOUT, "LOGOUT", 35, 90 + sidebarH - 50, 190, 35, false);
        
        // Main detail panel
        int mainX = 260;
        int mainY = 90;
        int mainW = w - 280;
        int mainH = h - 110;
        if (mainH < 260) mainH = 260;
        
        DrawRoundedRect(hdcMem, mainX, mainY, mainW, mainH, 8, 8, COLOR_PANEL_BG, COLOR_BORDER, 1);
        
        if (g_currentAdminTab == "accounts") {
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rTitle = { mainX + 20, mainY + 15, mainX + 500, mainY + 40 };
            DrawText(hdcMem, "CUSTOMER ACCOUNTS DIRECTORY", -1, &rTitle, DT_LEFT);
            
            // Columns headers
            int tY = mainY + 45;
            DrawRoundedRect(hdcMem, mainX + 20, tY, mainW - 40, 28, 4, 4, COLOR_BG, COLOR_BORDER, 1);
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontConsole);
            
            int colW = (mainW - 80) / 6;
            RECT rC1 = { mainX + 30, tY + 6, mainX + 30 + colW, tY + 26 }; DrawText(hdcMem, "ACC-NO", -1, &rC1, DT_LEFT);
            RECT rC2 = { mainX + 30 + colW, tY + 6, mainX + 30 + colW * 2, tY + 26 }; DrawText(hdcMem, "CUSTOMER NAME", -1, &rC2, DT_LEFT);
            RECT rC3 = { mainX + 40 + colW * 2, tY + 6, mainX + 40 + colW * 3, tY + 26 }; DrawText(hdcMem, "CNIC", -1, &rC3, DT_LEFT);
            RECT rC4 = { mainX + 40 + colW * 3, tY + 6, mainX + 40 + colW * 4, tY + 26 }; DrawText(hdcMem, "TYPE", -1, &rC4, DT_LEFT);
            RECT rC5 = { mainX + 40 + colW * 4, tY + 6, mainX + 40 + colW * 5, tY + 26 }; DrawText(hdcMem, "BALANCE", -1, &rC5, DT_RIGHT);
            RECT rC6 = { mainX + 40 + colW * 5, tY + 6, mainX + mainW - 30, tY + 26 }; DrawText(hdcMem, "STATUS", -1, &rC6, DT_CENTER);
            
            lock_guard<mutex> lk(g_dataMutex);
            
            // Calculate dynamic rows capacity
            int pageSize = (mainH - 260) / 28;
            if (pageSize < 3) pageSize = 3;
            if (pageSize > 15) pageSize = 15;
            
            int totalAccs = (int)g_accounts.size();
            int maxPages = max(1, (totalAccs + pageSize - 1) / pageSize);
            if (g_adminAccountPage >= maxPages) g_adminAccountPage = maxPages - 1;
            if (g_adminAccountPage < 0) g_adminAccountPage = 0;
            
            int startIdx = g_adminAccountPage * pageSize;
            int endIdx = min(totalAccs, startIdx + pageSize);
            
            tY += 32;
            for (int i = startIdx; i < endIdx; ++i) {
                Account& a = g_accounts[i];
                bool isSelected = (a.accountNumber == g_selectedAdminAccount);
                
                COLORREF rowBg = isSelected ? COLOR_CYAN_TRANSP : ((i % 2 == 0) ? COLOR_PANEL_BG : COLOR_ROW_ALT);
                COLORREF rowBorder = isSelected ? COLOR_CYAN_NEON : COLOR_GRID_LINE;
                
                DrawRoundedRect(hdcMem, mainX + 20, tY, mainW - 40, 24, 4, 4, rowBg, rowBorder);
                
                SetTextColor(hdcMem, COLOR_TEXT_DARK);
                SelectObject(hdcMem, hFontConsole);
                
                RECT r1 = { mainX + 30, tY + 4, mainX + 30 + colW, tY + 24 }; DrawText(hdcMem, a.accountNumber.c_str(), -1, &r1, DT_LEFT);
                RECT r2 = { mainX + 30 + colW, tY + 4, mainX + 30 + colW * 2, tY + 24 }; DrawText(hdcMem, a.name.c_str(), -1, &r2, DT_LEFT);
                RECT r3 = { mainX + 40 + colW * 2, tY + 4, mainX + 40 + colW * 3, tY + 24 }; DrawText(hdcMem, a.cnic.c_str(), -1, &r3, DT_LEFT);
                RECT r4 = { mainX + 40 + colW * 3, tY + 4, mainX + 40 + colW * 4, tY + 24 }; DrawText(hdcMem, a.type.c_str(), -1, &r4, DT_LEFT);
                
                ostringstream ssB; ssB << fixed << setprecision(2) << a.balance;
                RECT r5 = { mainX + 40 + colW * 4, tY + 4, mainX + 40 + colW * 5, tY + 24 }; DrawText(hdcMem, ssB.str().c_str(), -1, &r5, DT_RIGHT);
                
                if (a.status == "ACTIVE") SetTextColor(hdcMem, COLOR_GREEN_NEON);
                else if (a.status == "LOCKED") SetTextColor(hdcMem, COLOR_PINK_NEON);
                else SetTextColor(hdcMem, COLOR_RED_NEON);
                
                RECT r6 = { mainX + 40 + colW * 5, tY + 4, mainX + mainW - 30, tY + 24 }; DrawText(hdcMem, a.status.c_str(), -1, &r6, DT_CENTER);
                
                tY += 28;
            }
            
            // Table pagination buttons
            int pagY = mainY + mainH - 180;
            AddButton(BTN_ADMIN_PREV_PAGE, "<< PREV", mainX + 20, pagY, 100, 26, true);
            
            ostringstream ssPage;
            ssPage << "PAGE " << (g_adminAccountPage + 1) << " / " << maxPages;
            SetTextColor(hdcMem, COLOR_TEXT_DARK);
            SelectObject(hdcMem, hFontNormal);
            RECT rPgTxt = { mainX + 130, pagY + 6, mainX + 240, pagY + 26 };
            DrawText(hdcMem, ssPage.str().c_str(), -1, &rPgTxt, DT_CENTER);
            
            AddButton(BTN_ADMIN_NEXT_PAGE, "NEXT >>", mainX + 250, pagY, 100, 26, true);
            
            // Selected account actions box anchored at the bottom
            int boxH = 135;
            int boxY = mainY + mainH - boxH - 20;
            
            DrawRoundedRect(hdcMem, mainX + 20, boxY, mainW - 40, boxH, 6, 6, COLOR_ROW_ALT, COLOR_BORDER, 1);
            if (!g_selectedAdminAccount.empty()) {
                Account* sel = nullptr;
                for (auto& acc : g_accounts) {
                    if (acc.accountNumber == g_selectedAdminAccount) {
                        sel = &acc;
                        break;
                    }
                }
                
                if (sel) {
                    SetTextColor(hdcMem, COLOR_CYAN_NEON);
                    SelectObject(hdcMem, hFontNormal);
                    RECT rSelH = { mainX + 35, boxY + 15, mainX + mainW - 35, boxY + 35 };
                    string actText = "SELECTED ACCOUNT: " + sel->name + " (A/C: " + sel->accountNumber + ")";
                    DrawText(hdcMem, actText.c_str(), -1, &rSelH, DT_LEFT);
                    
                    // Render action triggers
                    if (sel->status == "ACTIVE") {
                        AddButton(BTN_ADMIN_FREEZE, "FREEZE ACCOUNT", mainX + 35, boxY + 55, 160, 32, true);
                    } else if (sel->status == "LOCKED") {
                        AddButton(BTN_ADMIN_UNLOCK, "UNLOCK ACCOUNT", mainX + 35, boxY + 55, 160, 32, false);
                    }
                    AddButton(BTN_ADMIN_DELETE, "DELETE ACCOUNT", mainX + 215, boxY + 55, 190, 32, true);
                }
            } else {
                SetTextColor(hdcMem, COLOR_TEXT_MUTED);
                SelectObject(hdcMem, hFontNormal);
                RECT rPrompt = { mainX + 20, boxY + 55, mainX + mainW - 20, boxY + 95 };
                DrawText(hdcMem, "Click an account row in the directory above to display security control actions.", -1, &rPrompt, DT_CENTER);
            }
        }
        else if (g_currentAdminTab == "create") {
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rC = { mainX + 20, mainY + 15, mainX + mainW - 20, mainY + 40 };
            DrawText(hdcMem, "REGISTER CUSTOMER ACCOUNT", -1, &rC, DT_LEFT);
            
            DrawActiveInputs(hdcMem);
            
            // Current / Savings toggle button
            SetTextColor(hdcMem, COLOR_TEXT_DARK);
            SelectObject(hdcMem, hFontConsole);
            RECT rTypeL = { mainX + 340, mainY + 70, mainX + 600, mainY + 90 };
            DrawText(hdcMem, "ACCOUNT TYPE", -1, &rTypeL, DT_LEFT);
            
            string schemeLabel = g_adminCreateIsSavings ? "SAVINGS ACCOUNT" : "CURRENT ACCOUNT";
            AddButton(BTN_ADMIN_CREATE_TOGGLE_TYPE, schemeLabel, mainX + 340, mainY + 90, 240, 32, true);
            
            AddButton(BTN_ADMIN_CREATE_SUBMIT, "CREATE ACCOUNT", mainX + 340, mainY + 285, 240, 40, true);
        }
        else if (g_currentAdminTab == "refill") {
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rRef = { mainX + 20, mainY + 15, mainX + mainW - 20, mainY + 40 };
            DrawText(hdcMem, "PHYSICAL ATM VAULT REFILL SYSTEM", -1, &rRef, DT_LEFT);
            
            // Vault inventory breakdown card
            int cardW = 260;
            int cardX = mainX + mainW - cardW - 40;
            if (cardX < mainX + 280) cardX = mainX + 280;
            
            DrawRoundedRect(hdcMem, cardX, mainY + 70, cardW, 215, 8, 8, COLOR_PINK_TRANSP, COLOR_PINK_NEON, 1);
            SetTextColor(hdcMem, COLOR_PINK_NEON);
            SelectObject(hdcMem, hFontNormal);
            RECT rVh = { cardX + 15, mainY + 85, cardX + cardW, mainY + 110 }; DrawText(hdcMem, "ATM VAULT STATUS", -1, &rVh, DT_LEFT);
            
            lock_guard<mutex> lk(g_dataMutex);
            SetTextColor(hdcMem, COLOR_TEXT_DARK);
            SelectObject(hdcMem, hFontConsole);
            
            ostringstream s5, s1, s05, s01, sTot;
            s5 << "Rs. 5000: " << g_cash.notes5000 << " notes";
            s1 << "Rs. 1000: " << g_cash.notes1000 << " notes";
            s05 << "Rs.  500: " << g_cash.notes500 << " notes";
            s01 << "Rs.  100: " << g_cash.notes100 << " notes";
            sTot << "Total Vault: Rs. " << g_cash.total;
            
            RECT r1 = { cardX + 15, mainY + 120, cardX + cardW, mainY + 136 }; DrawText(hdcMem, s5.str().c_str(), -1, &r1, DT_LEFT);
            RECT r2 = { cardX + 15, mainY + 140, cardX + cardW, mainY + 156 }; DrawText(hdcMem, s1.str().c_str(), -1, &r2, DT_LEFT);
            RECT r3 = { cardX + 15, mainY + 160, cardX + cardW, mainY + 176 }; DrawText(hdcMem, s05.str().c_str(), -1, &r3, DT_LEFT);
            RECT r4 = { cardX + 15, mainY + 180, cardX + cardW, mainY + 196 }; DrawText(hdcMem, s01.str().c_str(), -1, &r4, DT_LEFT);
            
            SetTextColor(hdcMem, COLOR_GREEN_NEON);
            RECT r5 = { cardX + 15, mainY + 215, cardX + cardW, mainY + 245 }; DrawText(hdcMem, sTot.str().c_str(), -1, &r5, DT_LEFT);
            
            DrawActiveInputs(hdcMem);
            AddButton(BTN_ADMIN_REFILL_SUBMIT, "REFILL VAULT", mainX + 40, mainY + 310, 220, 35, true);
        }
        else if (g_currentAdminTab == "reports") {
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rRep = { mainX + 20, mainY + 15, mainX + mainW - 20, mainY + 40 };
            DrawText(hdcMem, "SYSTEM STATISTICS & AUDIT", -1, &rRep, DT_LEFT);
            
            lock_guard<mutex> lk(g_dataMutex);
            double totalDeposits = 0;
            double activeLoans = 0;
            int customersCount = (int)g_accounts.size();
            for (auto& a : g_accounts) totalDeposits += a.balance;
            for (auto& l : g_loans) {
                if (l.status == "ACTIVE") activeLoans += l.remaining;
            }
            
            DrawRoundedRect(hdcMem, mainX + 20, mainY + 50, mainW - 40, 200, 8, 8, COLOR_ROW_ALT, COLOR_BORDER, 1);
            
            SetTextColor(hdcMem, COLOR_TEXT_DARK);
            SelectObject(hdcMem, hFontNormal);
            
            ostringstream sc, sd, sl, sv;
            sc << "Registered Customers Node:     " << customersCount;
            sd << "Combined User Ledger Deposits: Rs. " << fixed << setprecision(2) << totalDeposits;
            sl << "Active Outstanding Bank Loans: Rs. " << fixed << setprecision(2) << activeLoans;
            sv << "Physical ATM Cash Reserves:    Rs. " << fixed << setprecision(2) << g_cash.total;
            
            RECT r1 = { mainX + 40, mainY + 70, mainX + mainW - 40, mainY + 95 }; DrawText(hdcMem, sc.str().c_str(), -1, &r1, DT_LEFT);
            RECT r2 = { mainX + 40, mainY + 100, mainX + mainW - 40, mainY + 125 }; DrawText(hdcMem, sd.str().c_str(), -1, &r2, DT_LEFT);
            RECT r3 = { mainX + 40, mainY + 130, mainX + mainW - 40, mainY + 155 }; DrawText(hdcMem, sl.str().c_str(), -1, &r3, DT_LEFT);
            RECT r4 = { mainX + 40, mainY + 160, mainX + mainW - 40, mainY + 185 }; DrawText(hdcMem, sv.str().c_str(), -1, &r4, DT_LEFT);
            
            // Run monthly savings profit calculations trigger
            SetTextColor(hdcMem, COLOR_TEXT_MUTED);
            RECT rInf = { mainX + 40, mainY + 270, mainX + mainW - 40, mainY + 320 };
            DrawText(hdcMem, "* Applying savings profits accrues 10% monthly interest into SAVINGS accounts.\n* Process logs will record immediately in backend ledger audits.", -1, &rInf, DT_LEFT);
            
            AddButton(BTN_ADMIN_APPLY_PROFIT, "APPLY MONTHLY SAVINGS INTEREST (10%)", mainX + 40, mainY + 330, 480, 40, true);
        }
        else if (g_currentAdminTab == "ledger") {
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontHeader);
            RECT rTr = { mainX + 20, mainY + 15, mainX + mainW - 20, mainY + 40 };
            DrawText(hdcMem, "SYSTEM TRANSACTION LEDGER", -1, &rTr, DT_LEFT);
            
            int tY = mainY + 45;
            DrawRoundedRect(hdcMem, mainX + 20, tY, mainW - 40, 28, 4, 4, COLOR_BG, COLOR_BORDER, 1);
            SetTextColor(hdcMem, COLOR_CYAN_NEON);
            SelectObject(hdcMem, hFontConsole);
            RECT rC1 = { mainX + 30, tY + 6, mainX + 110, tY + 26 }; DrawText(hdcMem, "TX-ID", -1, &rC1, DT_LEFT);
            RECT rC2 = { mainX + 120, tY + 6, 200, tY + 26 }; DrawText(hdcMem, "A/C NO", -1, &rC2, DT_LEFT);
            RECT rC3 = { mainX + 210, tY + 6, mainX + 330, tY + 26 }; DrawText(hdcMem, "DATE & TIME", -1, &rC3, DT_LEFT);
            RECT rC4 = { mainX + 340, tY + 6, mainX + 460, tY + 26 }; DrawText(hdcMem, "OPERATION TYPE", -1, &rC4, DT_LEFT);
            RECT rC5 = { mainX + 470, tY + 6, mainX + 590, tY + 26 }; DrawText(hdcMem, "AMOUNT", -1, &rC5, DT_RIGHT);
            RECT rC6 = { mainX + 600, tY + 6, mainX + mainW - 30, tY + 26 }; DrawText(hdcMem, "POST BAL", -1, &rC6, DT_RIGHT);
            
            lock_guard<mutex> lk(g_dataMutex);
            int pageSize = (mainH - 180) / 25;
            if (pageSize < 4) pageSize = 4;
            if (pageSize > 20) pageSize = 20;
            
            int totalTxs = (int)g_transactions.size();
            int maxPages = max(1, (totalTxs + pageSize - 1) / pageSize);
            if (g_adminAccountPage >= maxPages) g_adminAccountPage = maxPages - 1;
            if (g_adminAccountPage < 0) g_adminAccountPage = 0;
            
            int startIdx = max(0, totalTxs - (g_adminAccountPage + 1) * pageSize);
            int endIdx = max(0, totalTxs - g_adminAccountPage * pageSize);
            
            tY += 32;
            int rowCount = 0;
            // Draw in reverse chronological order
            for (int i = endIdx - 1; i >= startIdx; --i) {
                Transaction& tx = g_transactions[i];
                
                COLORREF rowBg = (rowCount % 2 == 0) ? COLOR_PANEL_BG : COLOR_ROW_ALT;
                DrawRoundedRect(hdcMem, mainX + 20, tY, mainW - 40, 22, 4, 4, rowBg, COLOR_GRID_LINE, 1);
                
                SetTextColor(hdcMem, COLOR_TEXT_DARK);
                SelectObject(hdcMem, hFontConsole);
                
                RECT r1 = { mainX + 30, tY + 3, mainX + 110, tY + 23 }; DrawText(hdcMem, tx.id.c_str(), -1, &r1, DT_LEFT);
                RECT r2 = { mainX + 120, tY + 3, 200, tY + 23 }; DrawText(hdcMem, tx.accountNumber.c_str(), -1, &r2, DT_LEFT);
                RECT r3 = { mainX + 210, tY + 3, mainX + 330, tY + 23 }; DrawText(hdcMem, tx.dateTime.c_str(), -1, &r3, DT_LEFT);
                
                if (tx.type == "WITHDRAW" || tx.type == "TRANSFER_OUT") SetTextColor(hdcMem, COLOR_RED_NEON);
                else SetTextColor(hdcMem, COLOR_GREEN_NEON);
                RECT r4 = { mainX + 340, tY + 3, mainX + 460, tY + 23 }; DrawText(hdcMem, tx.type.c_str(), -1, &r4, DT_LEFT);
                
                ostringstream ssA, ssB;
                ssA << tx.amount;
                ssB << tx.balanceAfter;
                
                RECT r5 = { mainX + 470, tY + 3, mainX + 590, tY + 23 }; DrawText(hdcMem, ssA.str().c_str(), -1, &r5, DT_RIGHT);
                SetTextColor(hdcMem, COLOR_TEXT_DARK);
                RECT r6 = { mainX + 600, tY + 3, mainX + mainW - 30, tY + 23 }; DrawText(hdcMem, ssB.str().c_str(), -1, &r6, DT_RIGHT);
                
                tY += 25;
                rowCount++;
            }
            
            // Pagination controls
            int pagY = mainY + mainH - 50;
            AddButton(BTN_ADMIN_PREV_PAGE, "<< PREV", mainX + 20, pagY, 100, 26, true);
            
            ostringstream ssPage;
            ssPage << "PAGE " << (g_adminAccountPage + 1) << " / " << maxPages;
            SetTextColor(hdcMem, COLOR_TEXT_DARK);
            SelectObject(hdcMem, hFontNormal);
            RECT rPgTxt = { mainX + 130, pagY + 6, mainX + 240, pagY + 26 };
            DrawText(hdcMem, ssPage.str().c_str(), -1, &rPgTxt, DT_CENTER);
            
            AddButton(BTN_ADMIN_NEXT_PAGE, "NEXT >>", mainX + 250, pagY, 100, 26, true);
        }
    }
    
    // 5. Draw toast if active
    DrawToast(hdcMem, w);
    
    // 6. Draw active buttons
    for (const auto& btn : g_activeButtons) {
        DrawButton(hdcMem, btn);
    }
    
    // Blit from back buffer to screen DC
    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
    
    // Clean up graphics buffers
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    
    EndPaint(hwnd, &ps);
}

// --- Window Input and Event Processing ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hwnd = hwnd;
            
            // Create UI clean modern sans-serif fonts (Segoe UI)
            hFontTitle = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontHeader = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontNormal = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontConsole = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            
            AddLog("SYS: Askari Bank Client Initialized.");
            AddLog("NET: Querying core bank node database at localhost:8080...");
            
            // Periodic network polling timer (every 2.5 seconds)
            SetTimer(hwnd, 1, 2500, NULL);
            // Refresh frame update timer (60 FPS - approx 16ms)
            SetTimer(hwnd, 2, 16, NULL);
            
            RECT rect;
            GetClientRect(hwnd, &rect);
            g_winWidth = rect.right - rect.left;
            g_winHeight = rect.bottom - rect.top;
            
            RefreshStateAsync();
            SetupInputs();
            break;
        }
        
        case WM_SIZE: {
            int nw = LOWORD(lParam);
            int nh = HIWORD(lParam);
            if (nw > 100 && nh > 100) {
                g_winWidth = nw;
                g_winHeight = nh;
                SetupInputs(); // recalculate input bounds responsively
            }
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        case WM_PAINT: {
            PaintWindow(hwnd);
            break;
        }
        
        case WM_TIMER: {
            if (wParam == 1) {
                RefreshStateAsync();
            }
            else if (wParam == 2) {
                // Update animation tick
                g_animationPhase += 0.05f;
                // Redraw
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        
        case WM_MOUSEMOVE: {
            g_mousePos.x = GET_X_LPARAM(lParam);
            g_mousePos.y = GET_Y_LPARAM(lParam);
            break;
        }
        
        case WM_LBUTTONDOWN: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            
            // Check input focus clicks
            bool clickedInput = false;
            for (auto& in : g_activeInputs) {
                if (mx >= in.x && mx <= in.x + in.w && my >= in.y && my <= in.y + in.h) {
                    // Focus this input
                    for (auto& other : g_activeInputs) other.isFocused = false;
                    in.isFocused = true;
                    g_focusedInput = &in;
                    clickedInput = true;
                    AddLog("UI: Focus set to " + in.label);
                    break;
                }
            }
            
            if (!clickedInput) {
                if (g_focusedInput) {
                    g_focusedInput->isFocused = false;
                    g_focusedInput = nullptr;
                }
            }
            
            // Check buttons click
            for (const auto& btn : g_activeButtons) {
                if (mx >= btn.x && mx <= btn.x + btn.w && my >= btn.y && my <= btn.y + btn.h) {
                    SendMessage(hwnd, WM_COMMAND, btn.id, 0);
                    break;
                }
            }
            
            // Admin directory selection click
            if (g_currentScreen == SCR_ADMIN_DASHBOARD && g_currentAdminTab == "accounts") {
                int mainX = 260;
                int mainY = 90;
                int mainW = g_winWidth - 280;
                int mainH = g_winHeight - 110;
                if (mainH < 260) mainH = 260;
                
                int tY = mainY + 45 + 32; // Row start Y
                
                // Calculate dynamic rows capacity
                int pageSize = (mainH - 260) / 28;
                if (pageSize < 3) pageSize = 3;
                if (pageSize > 15) pageSize = 15;
                
                lock_guard<mutex> lk(g_dataMutex);
                int totalAccs = (int)g_accounts.size();
                
                int startIdx = g_adminAccountPage * pageSize;
                int endIdx = min(totalAccs, startIdx + pageSize);
                
                for (int i = startIdx; i < endIdx; ++i) {
                    if (mx >= mainX + 20 && mx <= mainX + mainW - 20 && my >= tY && my <= tY + 24) {
                        g_selectedAdminAccount = g_accounts[i].accountNumber;
                        AddLog("UI: Selected account " + g_selectedAdminAccount);
                        break;
                    }
                    tY += 28;
                }
            }
            
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        case WM_CHAR: {
            if (g_focusedInput) {
                if (wParam == VK_BACK) {
                    // Backspace
                    if (!g_focusedInput->value.empty()) {
                        g_focusedInput->value.pop_back();
                    }
                }
                else if (wParam == VK_RETURN) {
                    // Submit active form
                    if (g_currentScreen == SCR_ATM_LOGIN) SendMessage(hwnd, WM_COMMAND, BTN_ATM_LOGIN_SUBMIT, 0);
                    else if (g_currentScreen == SCR_ADMIN_LOGIN) SendMessage(hwnd, WM_COMMAND, BTN_ADMIN_LOGIN_SUBMIT, 0);
                    else if (g_currentScreen == SCR_ATM_DASHBOARD) {
                        if (g_currentAtmTab == "deposit") SendMessage(hwnd, WM_COMMAND, BTN_ATM_DEPOSIT_SUBMIT, 0);
                        else if (g_currentAtmTab == "withdraw") SendMessage(hwnd, WM_COMMAND, BTN_ATM_WITHDRAW_SUBMIT, 0);
                        else if (g_currentAtmTab == "transfer") {
                            if (g_transferNeedOtp) SendMessage(hwnd, WM_COMMAND, BTN_ATM_TRANSFER_CONFIRM_OTP, 0);
                            else SendMessage(hwnd, WM_COMMAND, BTN_ATM_TRANSFER_SUBMIT, 0);
                        }
                        else if (g_currentAtmTab == "loans") {
                            Loan* activeL = findActiveLoan(g_currentAtmAccount);
                            if (activeL) SendMessage(hwnd, WM_COMMAND, BTN_ATM_LOAN_REPAY, 0);
                            else SendMessage(hwnd, WM_COMMAND, BTN_ATM_LOAN_APPLY, 0);
                        }
                        else if (g_currentAtmTab == "security") SendMessage(hwnd, WM_COMMAND, BTN_ATM_CHANGE_PIN, 0);
                    }
                    else if (g_currentScreen == SCR_ADMIN_DASHBOARD) {
                        if (g_currentAdminTab == "create") SendMessage(hwnd, WM_COMMAND, BTN_ADMIN_CREATE_SUBMIT, 0);
                        else if (g_currentAdminTab == "refill") SendMessage(hwnd, WM_COMMAND, BTN_ADMIN_REFILL_SUBMIT, 0);
                    }
                }
                else {
                    // Normal characters
                    char c = (char)wParam;
                    if (g_focusedInput->value.size() < g_focusedInput->maxLen) {
                        if (g_focusedInput->isNumeric) {
                            if (isdigit(c) || c == '.') {
                                g_focusedInput->value.push_back(c);
                            }
                        } else {
                            if (isprint(c)) {
                                g_focusedInput->value.push_back(c);
                            }
                        }
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        
        // Asynchronous REST API update reply
        case WM_USER + 100: {
            g_isCheckingConnection = false;
            bool ok = (wParam != 0);
            g_isConnected = ok;
            
            if (ok && lParam) {
                string* rawState = (string*)lParam;
                ParseStateJson(*rawState);
                delete rawState;
                
                if (g_currentScreen == SCR_CONNECTING) {
                    g_currentScreen = SCR_GATEWAY;
                    AddLog("NET: Connection to core Askari Bank nodes established.");
                }
            } else {
                if (g_currentScreen != SCR_CONNECTING) {
                    g_currentScreen = SCR_CONNECTING;
                    AddLog("NET: Link lost. Re-establishing link...");
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        // Asynchronous trigger action reply
        case WM_USER + 101: {
            AsyncRequestResult* res = (AsyncRequestResult*)wParam;
            if (res) {
                AddLog("[API] Received response from " + res->url);
                bool ok = jsonNum(res->response, "ok") != 0 || res->response.find("\"ok\":true") != string::npos;
                string msg = jsonStr(res->response, "message");
                
                SetToast(msg, ok);
                
                if (res->triggerBtn == BTN_ATM_LOGIN_SUBMIT) {
                    if (ok) {
                        string accNo = jsonStr(res->response, "accountNumber");
                        if (accNo.empty()) {
                            // Extract from inner account block
                            size_t p = res->response.find("\"account\":");
                            if (p != string::npos) {
                                accNo = jsonStr(res->response.substr(p), "accountNumber");
                            }
                        }
                        g_currentAtmAccount = accNo;
                        g_currentScreen = SCR_ATM_DASHBOARD;
                        g_currentAtmTab = "overview";
                        SetupInputs();
                        AddLog("AUTH: User logged in: Account " + accNo);
                    }
                }
                else if (res->triggerBtn == BTN_ADMIN_LOGIN_SUBMIT) {
                    if (ok) {
                        g_currentScreen = SCR_ADMIN_DASHBOARD;
                        g_currentAdminTab = "accounts";
                        g_adminAccountPage = 0;
                        g_selectedAdminAccount = "";
                        SetupInputs();
                        AddLog("AUTH: Administrative session initialized.");
                    }
                }
                else if (res->triggerBtn == BTN_ATM_TRANSFER_SUBMIT) {
                    // Check if server prompted for OTP
                    if (res->response.find("\"needOtp\":true") != string::npos) {
                        g_transferNeedOtp = true;
                        // Find values from input cache
                        string target = "";
                        double amt = 0.0;
                        for (auto& in : g_activeInputs) {
                            if (in.id == IN_ATM_TRANSFER_TO) target = in.value;
                            if (in.id == IN_ATM_TRANSFER_AMT) amt = stod(in.value.empty() ? "0" : in.value);
                        }
                        g_transferTarget = target;
                        g_transferAmount = amt;
                        SetupInputs();
                        AddLog("AUTH: Transaction exceeds threshold limit. OTP authentication token required.");
                    } else {
                        if (ok) {
                            g_transferNeedOtp = false;
                            SetupInputs();
                            AddLog("TXN: Funds transferred successfully.");
                        }
                    }
                }
                else if (res->triggerBtn == BTN_ATM_TRANSFER_CONFIRM_OTP) {
                    if (ok) {
                        g_transferNeedOtp = false;
                        g_transferTarget = "";
                        g_transferAmount = 0.0;
                        SetupInputs();
                        AddLog("TXN: OTP verification successful. Funds dispatched.");
                    }
                }
                else if (res->triggerBtn == BTN_ATM_DEPOSIT_SUBMIT || res->triggerBtn == BTN_ATM_WITHDRAW_SUBMIT ||
                         res->triggerBtn == BTN_ATM_LOAN_APPLY || res->triggerBtn == BTN_ATM_LOAN_REPAY ||
                         res->triggerBtn == BTN_ATM_CHANGE_PIN) {
                    if (ok) {
                        SetupInputs();
                        AddLog("SYS: Operation executed on customer core.");
                    }
                }
                else if (res->triggerBtn == BTN_ADMIN_CREATE_SUBMIT) {
                    if (ok) {
                        SetupInputs();
                        AddLog("SYS: User account registration committed.");
                    }
                }
                else if (res->triggerBtn == BTN_ADMIN_REFILL_SUBMIT || res->triggerBtn == BTN_ADMIN_APPLY_PROFIT ||
                         res->triggerBtn == BTN_ADMIN_FREEZE || res->triggerBtn == BTN_ADMIN_UNLOCK ||
                         res->triggerBtn == BTN_ADMIN_DELETE) {
                    if (ok) {
                        if (res->triggerBtn == BTN_ADMIN_DELETE) {
                            g_selectedAdminAccount = "";
                        }
                        SetupInputs();
                        AddLog("SYS: Command committed to central banking core.");
                    }
                }
                
                delete res;
                RefreshStateAsync();
            }
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        
        // Button operations clicks dispatching
        case WM_COMMAND: {
            ButtonId btnId = (ButtonId)wParam;
            
            if (btnId == BTN_NAV_GATEWAY) {
                g_currentScreen = SCR_GATEWAY;
                SetupInputs();
                AddLog("UI: Navigation to portal gateway.");
            }
            else if (btnId == BTN_LAUNCH_ATM) {
                g_currentScreen = SCR_ATM_LOGIN;
                SetupInputs();
                AddLog("UI: Loading ATM customer portal login.");
            }
            else if (btnId == BTN_LAUNCH_ADMIN) {
                g_currentScreen = SCR_ADMIN_LOGIN;
                SetupInputs();
                AddLog("UI: Loading administrative portal login.");
            }
            else if (btnId == BTN_ATM_LOGOUT) {
                g_currentAtmAccount = "";
                g_currentScreen = SCR_GATEWAY;
                g_transferNeedOtp = false;
                SetupInputs();
                SetToast("ATM Session Logged Out.", true);
                AddLog("AUTH: ATM Customer Session Terminated.");
            }
            else if (btnId == BTN_ADMIN_LOGOUT) {
                g_currentScreen = SCR_GATEWAY;
                g_selectedAdminAccount = "";
                SetupInputs();
                SetToast("Admin Terminal Session Locked.", true);
                AddLog("AUTH: Administrative Console Locked.");
            }
            // Tab changes ATM
            else if (btnId >= BTN_ATM_TAB_OVERVIEW && btnId <= BTN_ATM_TAB_SECURITY) {
                g_transferNeedOtp = false;
                if (btnId == BTN_ATM_TAB_OVERVIEW) g_currentAtmTab = "overview";
                else if (btnId == BTN_ATM_TAB_DEPOSIT) g_currentAtmTab = "deposit";
                else if (btnId == BTN_ATM_TAB_WITHDRAW) g_currentAtmTab = "withdraw";
                else if (btnId == BTN_ATM_TAB_TRANSFER) g_currentAtmTab = "transfer";
                else if (btnId == BTN_ATM_TAB_LOANS) g_currentAtmTab = "loans";
                else if (btnId == BTN_ATM_TAB_SECURITY) g_currentAtmTab = "security";
                SetupInputs();
                AddLog("UI: Tab switched to ATM " + g_currentAtmTab);
            }
            // Tab changes Admin
            else if (btnId >= BTN_ADMIN_TAB_ACCOUNTS && btnId <= BTN_ADMIN_TAB_LEDGER) {
                if (btnId == BTN_ADMIN_TAB_ACCOUNTS) { g_currentAdminTab = "accounts"; g_adminAccountPage = 0; }
                else if (btnId == BTN_ADMIN_TAB_CREATE) g_currentAdminTab = "create";
                else if (btnId == BTN_ADMIN_TAB_REFILL) g_currentAdminTab = "refill";
                else if (btnId == BTN_ADMIN_TAB_REPORTS) g_currentAdminTab = "reports";
                else if (btnId == BTN_ADMIN_TAB_LEDGER) { g_currentAdminTab = "ledger"; g_adminAccountPage = 0; }
                SetupInputs();
                AddLog("UI: Tab switched to Admin " + g_currentAdminTab);
            }
            
            // Submissions & REST Calls
            else if (btnId == BTN_ATM_LOGIN_SUBMIT) {
                string acc = "", pin = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_ACC_NO) acc = in.value;
                    if (in.id == IN_ATM_PIN) pin = in.value;
                }
                
                if (acc.empty() || pin.empty()) {
                    SetToast("Account and PIN fields cannot be empty.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(acc) << "\",\"pin\":\"" << jsonEscape(pin) << "\"}";
                    TriggerActionAsync("/api/login", ss.str(), BTN_ATM_LOGIN_SUBMIT);
                }
            }
            else if (btnId == BTN_ADMIN_LOGIN_SUBMIT) {
                string pass = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ADMIN_PASS) pass = in.value;
                }
                
                ostringstream ss;
                ss << "{\"password\":\"" << jsonEscape(pass) << "\"}";
                TriggerActionAsync("/api/admin/login", ss.str(), BTN_ADMIN_LOGIN_SUBMIT);
            }
            else if (btnId == BTN_ATM_DEPOSIT_SUBMIT) {
                string amtStr = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_DEPOSIT_AMT) amtStr = in.value;
                }
                double amt = amtStr.empty() ? 0.0 : stod(amtStr);
                if (amt <= 0) {
                    SetToast("Please enter a positive deposit amount.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"amount\":" << amt << "}";
                    TriggerActionAsync("/api/deposit", ss.str(), BTN_ATM_DEPOSIT_SUBMIT);
                }
            }
            else if (btnId == BTN_ATM_WITHDRAW_SUBMIT) {
                string amtStr = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_WITHDRAW_AMT) amtStr = in.value;
                }
                double amt = amtStr.empty() ? 0.0 : stod(amtStr);
                if (amt <= 0) {
                    SetToast("Please enter a positive withdrawal amount.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"amount\":" << amt << "}";
                    TriggerActionAsync("/api/withdraw", ss.str(), BTN_ATM_WITHDRAW_SUBMIT);
                }
            }
            else if (btnId == BTN_ATM_TRANSFER_SUBMIT) {
                string to = "", amtStr = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_TRANSFER_TO) to = in.value;
                    if (in.id == IN_ATM_TRANSFER_AMT) amtStr = in.value;
                }
                double amt = amtStr.empty() ? 0.0 : stod(amtStr);
                if (to.empty() || amt <= 0) {
                    SetToast("Please specify a valid target account and positive amount.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"from\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"to\":\"" << jsonEscape(to)
                       << "\",\"amount\":" << amt << ",\"otp\":\"\"}";
                    TriggerActionAsync("/api/transfer", ss.str(), BTN_ATM_TRANSFER_SUBMIT);
                }
            }
            else if (btnId == BTN_ATM_TRANSFER_CONFIRM_OTP) {
                string otp = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_TRANSFER_OTP) otp = in.value;
                }
                if (otp.empty()) {
                    SetToast("Please enter the verification OTP token.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"from\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"to\":\"" << jsonEscape(g_transferTarget)
                       << "\",\"amount\":" << g_transferAmount << ",\"otp\":\"" << jsonEscape(otp) << "\"}";
                    TriggerActionAsync("/api/transfer", ss.str(), BTN_ATM_TRANSFER_CONFIRM_OTP);
                }
            }
            else if (btnId == BTN_ATM_TRANSFER_CANCEL_OTP) {
                g_transferNeedOtp = false;
                g_transferTarget = "";
                g_transferAmount = 0.0;
                SetupInputs();
                SetToast("Transfer cancelled.", true);
                AddLog("TXN: Transfer verification abandoned.");
            }
            else if (btnId == BTN_ATM_LOAN_APPLY) {
                string amtStr = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_LOAN_APPLY_AMT) amtStr = in.value;
                }
                double amt = amtStr.empty() ? 0.0 : stod(amtStr);
                if (amt <= 0) {
                    SetToast("Please enter a positive requested credit amount.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"amount\":" << amt << "}";
                    TriggerActionAsync("/api/loan/apply", ss.str(), BTN_ATM_LOAN_APPLY);
                }
            }
            else if (btnId == BTN_ATM_LOAN_REPAY) {
                string amtStr = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_LOAN_REPAY_AMT) amtStr = in.value;
                }
                double amt = amtStr.empty() ? 0.0 : stod(amtStr);
                if (amt <= 0) {
                    SetToast("Please enter a positive repayment allocation.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"amount\":" << amt << "}";
                    TriggerActionAsync("/api/loan/repay", ss.str(), BTN_ATM_LOAN_REPAY);
                }
            }
            else if (btnId == BTN_ATM_CHANGE_PIN) {
                string oldPin = "", newPin = "", confPin = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ATM_OLD_PIN) oldPin = in.value;
                    if (in.id == IN_ATM_NEW_PIN) newPin = in.value;
                    if (in.id == IN_ATM_CONFIRM_PIN) confPin = in.value;
                }
                
                if (oldPin.size() != 4 || newPin.size() != 4 || confPin.size() != 4) {
                    SetToast("All PIN codes must be exactly 4 digits.", false);
                } else if (newPin != confPin) {
                    SetToast("New PIN entries do not match.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(g_currentAtmAccount) << "\",\"oldPin\":\"" << jsonEscape(oldPin)
                       << "\",\"newPin\":\"" << jsonEscape(newPin) << "\"}";
                    TriggerActionAsync("/api/changepin", ss.str(), BTN_ATM_CHANGE_PIN);
                }
            }
            
            // Admin inputs submissions
            else if (btnId == BTN_ADMIN_CREATE_TOGGLE_TYPE) {
                g_adminCreateIsSavings = !g_adminCreateIsSavings;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (btnId == BTN_ADMIN_CREATE_SUBMIT) {
                string name = "", cnic = "", phone = "", pin = "", openingStr = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ADMIN_CREATE_NAME) name = in.value;
                    if (in.id == IN_ADMIN_CREATE_CNIC) cnic = in.value;
                    if (in.id == IN_ADMIN_CREATE_PHONE) phone = in.value;
                    if (in.id == IN_ADMIN_CREATE_PIN) pin = in.value;
                    if (in.id == IN_ADMIN_CREATE_OPENING) openingStr = in.value;
                }
                
                double opening = openingStr.empty() ? 0.0 : stod(openingStr);
                
                if (name.empty() || cnic.empty() || phone.empty() || pin.size() != 4 || opening < 0) {
                    SetToast("Please fill all fields accurately. PIN must be 4 digits.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"name\":\"" << jsonEscape(name) << "\",\"cnic\":\"" << jsonEscape(cnic)
                       << "\",\"phone\":\"" << jsonEscape(phone) << "\",\"pin\":\"" << jsonEscape(pin)
                       << "\",\"type\":\"" << (g_adminCreateIsSavings ? "SAVINGS" : "CURRENT")
                       << "\",\"opening\":" << opening << "}";
                    TriggerActionAsync("/api/admin/create", ss.str(), BTN_ADMIN_CREATE_SUBMIT);
                }
            }
            else if (btnId == BTN_ADMIN_REFILL_SUBMIT) {
                string n5k = "", n1k = "", n500 = "", n100 = "";
                for (auto& in : g_activeInputs) {
                    if (in.id == IN_ADMIN_REFILL_5000) n5k = in.value;
                    if (in.id == IN_ADMIN_REFILL_1000) n1k = in.value;
                    if (in.id == IN_ADMIN_REFILL_500) n500 = in.value;
                    if (in.id == IN_ADMIN_REFILL_100) n100 = in.value;
                }
                
                int c5000 = n5k.empty() ? 0 : stoi(n5k);
                int c1000 = n1k.empty() ? 0 : stoi(n1k);
                int c500 = n500.empty() ? 0 : stoi(n500);
                int c100 = n100.empty() ? 0 : stoi(n100);
                
                if (c5000 < 0 || c1000 < 0 || c500 < 0 || c100 < 0) {
                    SetToast("Refill values cannot be negative.", false);
                } else {
                    ostringstream ss;
                    ss << "{\"notes5000\":" << c5000 << ",\"notes1000\":" << c1000
                       << ",\"notes500\":" << c500 << ",\"notes100\":" << c100 << "}";
                    TriggerActionAsync("/api/admin/cash", ss.str(), BTN_ADMIN_REFILL_SUBMIT);
                }
            }
            else if (btnId == BTN_ADMIN_APPLY_PROFIT) {
                TriggerActionAsync("/api/admin/profit", "{}", BTN_ADMIN_APPLY_PROFIT);
            }
            else if (btnId == BTN_ADMIN_FREEZE || btnId == BTN_ADMIN_UNLOCK) {
                if (!g_selectedAdminAccount.empty()) {
                    string targetStatus = (btnId == BTN_ADMIN_FREEZE) ? "LOCKED" : "ACTIVE";
                    ostringstream ss;
                    ss << "{\"accountNumber\":\"" << jsonEscape(g_selectedAdminAccount) << "\",\"status\":\"" << targetStatus << "\"}";
                    TriggerActionAsync("/api/admin/status", ss.str(), btnId);
                }
            }
            else if (btnId == BTN_ADMIN_DELETE) {
                if (!g_selectedAdminAccount.empty()) {
                    if (MessageBox(hwnd, "Are you sure you want to CLOSE and DELETE this customer account forever?", "Security Authorization Required", MB_YESNO | MB_ICONWARNING) == IDYES) {
                        ostringstream ss;
                        ss << "{\"accountNumber\":\"" << jsonEscape(g_selectedAdminAccount) << "\"}";
                        TriggerActionAsync("/api/admin/delete", ss.str(), BTN_ADMIN_DELETE);
                    }
                }
            }
            // Directory pagination
            else if (btnId == BTN_ADMIN_PREV_PAGE) {
                if (g_adminAccountPage > 0) {
                    g_adminAccountPage--;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            else if (btnId == BTN_ADMIN_NEXT_PAGE) {
                int totalAccs = 0;
                {
                    lock_guard<mutex> lk(g_dataMutex);
                    totalAccs = (int)g_accounts.size();
                }
                int mainH = g_winHeight - 110;
                int pageSize = (g_currentAdminTab == "ledger") ? ((mainH - 180)/25) : ((mainH - 260)/28);
                if (pageSize < 3) pageSize = 3;
                
                int maxPages = max(1, (totalAccs + pageSize - 1) / pageSize);
                if (g_adminAccountPage < maxPages - 1) {
                    g_adminAccountPage++;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            
            break;
        }
        
        case WM_DESTROY: {
            // Delete Fonts GDI references
            DeleteObject(hFontTitle);
            DeleteObject(hFontHeader);
            DeleteObject(hFontNormal);
            DeleteObject(hFontConsole);
            
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Dummy logger method
void AddLog(const string& msg) {}

// --- Application Core Entry Point ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register Window Class
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // Double buffered draws background
    wc.lpszClassName = "AskariBankUIClass";
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failure!", "Fatal Error", MB_ICONERROR);
        return 1;
    }
    
    // Create Desktop Window with Resizing styles (WS_OVERLAPPEDWINDOW)
    HWND hwnd = CreateWindowEx(
        0,
        "AskariBankUIClass",
        "ASKARI BANK // DIGITAL DESKTOP PORTAL",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 720,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hwnd) {
        MessageBox(NULL, "Window Creation Failure!", "Fatal Error", MB_ICONERROR);
        return 1;
    }
    
    // Center Window on Screen
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;
    int x = (screenW - winW) / 2;
    int y = (screenH - winH) / 2;
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Run Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
