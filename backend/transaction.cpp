#include "transaction.h"
#include "utils.h"

#include <sstream>
#include <vector>

using namespace std;

Transaction::Transaction()
    : amount(0.0), balanceAfter(0.0) {}

Transaction::Transaction(string id, string acc, string type,
                         double amount, string dateTime, double balanceAfter)
    : id(move(id)), accountNumber(move(acc)), type(move(type)),
      amount(amount), dateTime(move(dateTime)), balanceAfter(balanceAfter) {}

string Transaction::toFileLine() const {
    stringstream ss;
    ss << id << '|' << accountNumber << '|' << type << '|'
       << amount << '|' << dateTime << '|' << balanceAfter;
    return ss.str();
}

bool Transaction::fromFileLine(const string& line, Transaction& out) {
    // Split the line on '|' into fields (keeping any empty trailing field).
    vector<string> parts = utils::split(line, '|');
    if (parts.size() != 6) return false; // corrupt / wrong format

    out.id            = parts[0];
    out.accountNumber = parts[1];
    out.type          = parts[2];
    out.dateTime      = parts[4];
    try {
        out.amount       = stod(parts[3]);
        out.balanceAfter = stod(parts[5]);
    } catch (...) {
        return false;
    }
    return true;
}
