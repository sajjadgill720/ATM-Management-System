#ifndef LOAN_H
#define LOAN_H

#include <string>

// Loan status constants.
namespace loanStatus {
    const std::string ACTIVE = "ACTIVE"; // still being repaid
    const std::string PAID   = "PAID";   // fully repaid
}

// ---------------------------------------------------------------------------
// Loan  --  Money the bank has lent to one account.
// totalPayable = principal + interest; the customer repays until paid ==
// totalPayable, at which point the loan is PAID.
// ---------------------------------------------------------------------------
class Loan {
public:
    std::string loanId;        // e.g. LN0001
    std::string accountNumber; // which account owns the loan
    double      principal;     // amount borrowed
    double      totalPayable;  // principal + interest
    double      paid;          // repaid so far
    std::string status;        // ACTIVE / PAID
    std::string dateIssued;

    Loan();

    // How much is still owed.
    double remaining() const;

    // Serialize / deserialize for loans.txt.
    std::string toFileLine() const;
    static bool fromFileLine(const std::string& line, Loan& out);
};

#endif // LOAN_H
