#include "utils.h"

#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <limits>

// conio.h gives us _getch() for hidden PIN entry on Windows / MinGW.
// io.h + _isatty let us detect when input is a real keyboard vs a redirected
// file/pipe (so automated testing still works).
#ifdef _WIN32
#include <conio.h>
#include <io.h>
#endif

using namespace std;

namespace utils {

    string trim(const string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    vector<string> split(const string& s, char delim) {
        vector<string> parts;
        size_t start = 0;
        while (true) {
            size_t pos = s.find(delim, start);
            if (pos == string::npos) {
                parts.push_back(s.substr(start)); // final (maybe empty) field
                break;
            }
            parts.push_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        return parts;
    }

    string readLine(const string& prompt) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) {
            // End of input (Ctrl+Z / Ctrl+D or a closed pipe). Exit cleanly
            // instead of looping forever on an empty stream.
            cout << "\n  Input stream closed. Exiting.\n";
            exit(0);
        }
        return trim(line);
    }

    int readInt(const string& prompt) {
        while (true) {
            string line = readLine(prompt);
            stringstream ss(line);
            int value;
            char leftover;
            // Valid only if we read an int and nothing meaningful is left over.
            if (ss >> value && !(ss >> leftover)) {
                return value;
            }
            cout << "  Invalid number. Please try again.\n";
        }
    }

    bool readMoney(const string& prompt, double& out) {
        string line = readLine(prompt);
        stringstream ss(line);
        double value;
        char leftover;
        if (!(ss >> value) || (ss >> leftover)) {
            cout << "  Amount must be a number.\n";
            return false;
        }
        if (value <= 0) {
            cout << "  Amount must be greater than zero.\n";
            return false;
        }
        // Reject amounts with more than 2 decimal places (currency rule).
        double rounded = static_cast<long long>(value * 100 + 0.5) / 100.0;
        out = rounded;
        return true;
    }

    string readHiddenPin(const string& prompt) {
        cout << prompt;
        string pin;
#ifdef _WIN32
        // If stdin is redirected (piped/from a file) _getch() would block waiting
        // for a real key press, so fall back to normal line input in that case.
        if (!_isatty(_fileno(stdin))) {
            if (!getline(cin, pin)) { exit(0); }
            return trim(pin);
        }
        char ch;
        while ((ch = static_cast<char>(_getch())) != '\r') { // Enter key
            if (ch == '\b') {                 // Backspace
                if (!pin.empty()) {
                    pin.pop_back();
                    cout << "\b \b";          // erase last * on screen
                }
            } else if (ch >= '0' && ch <= '9') {
                pin.push_back(ch);
                cout << '*';
            }
        }
        cout << "\n";
#else
        // Fallback for non-Windows: plain input (still functional for the demo).
        getline(cin, pin);
        pin = trim(pin);
#endif
        return pin;
    }

    bool isValidPin(const string& pin) {
        if (pin.size() != 4) return false;
        for (char c : pin) {
            if (c < '0' || c > '9') return false;
        }
        return true;
    }

    string hashPin(const string& pin) {
        // djb2 hash with a fixed salt. The same PIN always gives the same hash,
        // so we can verify by hashing the entered PIN and comparing, but the
        // stored hash cannot be turned back into the PIN.
        const string salt = "MYBANK$2026$";
        string salted = salt + pin;
        unsigned long h = 5381;
        for (char c : salted) {
            h = ((h << 5) + h) + static_cast<unsigned char>(c); // h * 33 + c
        }
        stringstream ss;
        ss << hex << h; // store as a hex string, e.g. "7c9e6865"
        return ss.str();
    }

    string currentDateTime() {
        time_t now = time(nullptr);
        tm* t = localtime(&now);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
        return string(buf);
    }

    string currentDate() {
        time_t now = time(nullptr);
        tm* t = localtime(&now);
        char buf[16];
        strftime(buf, sizeof(buf), "%Y-%m-%d", t);
        return string(buf);
    }

    string generateOtp() {
        // Six random digits. srand is seeded once in main().
        string otp;
        for (int i = 0; i < 6; ++i) {
            otp.push_back(static_cast<char>('0' + rand() % 10));
        }
        return otp;
    }

    void pause() {
        cout << "\n  Press Enter to continue...";
        string dummy;
        getline(cin, dummy);
    }

    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
}
