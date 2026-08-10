#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

// ---------------------------------------------------------------------------
// Transaction  --  One financial event on an account.
// Every deposit, withdrawal or transfer creates one of these records so we
// have a permanent, auditable history saved to transactions.txt.
// ---------------------------------------------------------------------------
class Transaction {
public:
    std::string id;            // e.g. TXN0001
    std::string accountNumber; // which account this belongs to
    std::string type;          // DEPOSIT / WITHDRAW / TRANSFER-IN / TRANSFER-OUT
    double      amount;        // how much money moved
    std::string dateTime;      // when it happened
    double      balanceAfter;  // resulting balance on the account

    Transaction();
    Transaction(std::string id, std::string acc, std::string type,
                double amount, std::string dateTime, double balanceAfter);

    // Convert to a single '|' separated line for the text file.
    std::string toFileLine() const;

    // Rebuild a Transaction from a saved file line. Returns false if malformed.
    static bool fromFileLine(const std::string& line, Transaction& out);
};

#endif // TRANSACTION_H
