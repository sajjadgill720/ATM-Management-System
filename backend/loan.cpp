#include "loan.h"
#include "utils.h"

#include <sstream>
#include <vector>

using namespace std;

Loan::Loan()
    : principal(0.0), totalPayable(0.0), paid(0.0), status(loanStatus::ACTIVE) {}

double Loan::remaining() const {
    double r = totalPayable - paid;
    return r > 0 ? r : 0.0;
}

string Loan::toFileLine() const {
    stringstream ss;
    ss << loanId << '|' << accountNumber << '|' << principal << '|'
       << totalPayable << '|' << paid << '|' << status << '|' << dateIssued;
    return ss.str();
}

bool Loan::fromFileLine(const string& line, Loan& out) {
    vector<string> p = utils::split(line, '|');
    if (p.size() != 7) return false;

    out.loanId        = p[0];
    out.accountNumber = p[1];
    out.status        = p[5];
    out.dateIssued    = p[6];
    try {
        out.principal    = stod(p[2]);
        out.totalPayable = stod(p[3]);
        out.paid         = stod(p[4]);
    } catch (...) {
        return false;
    }
    return true;
}
