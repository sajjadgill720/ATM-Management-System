#ifndef LUHN_H
#define LUHN_H

#include <string>

namespace luhn {

inline bool isValid(const std::string& numStr) {
    if (numStr.empty()) return false;
    int nDigits = numStr.length();
    int sum = 0;
    bool isSecond = false;
    for (int i = nDigits - 1; i >= 0; i--) {
        char c = numStr[i];
        if (c < '0' || c > '9') return false; // Contains non-digit
        int d = c - '0';
        if (isSecond == true) {
            d = d * 2;
        }
        sum += d / 10;
        sum += d % 10;
        isSecond = !isSecond;
    }
    return (sum % 10 == 0);
}

inline std::string generateNext(int seq) {
    std::string base = std::to_string(seq);
    for (int d = 0; d <= 9; ++d) {
        std::string candidate = base + std::to_string(d);
        if (isValid(candidate)) {
            return candidate;
        }
    }
    return base + "0";
}

} // namespace luhn

#endif // LUHN_H
