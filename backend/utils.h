#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// utils.h  --  Small helper functions shared across the whole program.
// These cover: input validation, date/time, hidden PIN entry and OTP.
// Keeping them in one place means every module validates data the same way.
// ---------------------------------------------------------------------------
namespace utils {

    // Reads a whole line of text safely (never leaves the stream in a bad state).
    std::string readLine(const std::string& prompt);

    // Reads an integer from the user, re-asking until the input is a valid number.
    int readInt(const std::string& prompt);

    // Reads a money amount (double). Rejects negative numbers and non-numbers.
    // Returns true on success and stores the value in "out".
    bool readMoney(const std::string& prompt, double& out);

    // Reads a PIN without showing it on screen (shows * for each digit).
    // Falls back to normal input on systems without conio.h.
    std::string readHiddenPin(const std::string& prompt);

    // True only if the string is exactly 4 digits (our PIN rule).
    bool isValidPin(const std::string& pin);

    // Turns a PIN into a non-reversible hash string. We store only the hash in
    // pins.txt, never the real PIN, so the raw PIN is never written to disk.
    // (A simple djb2-style hash with a salt — enough to demonstrate the idea in
    // a basic C++ project; a real bank would use a cryptographic hash.)
    std::string hashPin(const std::string& pin);

    // Returns the current date+time, e.g. "2026-08-08 14:35:07".
    std::string currentDateTime();

    // Returns only the current date, e.g. "2026-08-08" (used for daily limits).
    std::string currentDate();

    // Generates a random 6-digit One Time Password as a string.
    std::string generateOtp();

    // Trim spaces from both ends of a string.
    std::string trim(const std::string& s);

    // Split a string on a delimiter, KEEPING empty fields (including a trailing
    // one). We need this so a record like "a|b|" reliably gives 3 fields, not 2.
    std::vector<std::string> split(const std::string& s, char delim);

    // Pause the screen until the user presses Enter (menu-driven UX).
    void pause();

    // Clears the console screen (works on Windows and Unix).
    void clearScreen();
}

#endif // UTILS_H
