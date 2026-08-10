// Initial seed data used to bootstrap the app on first run. In a real
// deployment this would come from a backend API instead of being bundled.
export const SEED = {
  // Public account data — note there is NO pin field here. Secret PINs live in
  // the separate `credentials` map below (mirroring the backend's pins.txt) and
  // are never included in anything the UI renders.
  accounts: [
    { accountNumber: '1001', name: 'Ali Khan',  cnic: '3520112345678', phone: '03001234567', type: 'SAVINGS', balance: 30000, status: 'ACTIVE', pinAttempts: 0, dailyWithdrawn: 0, lastWithdrawDate: '' },
    { accountNumber: '1002', name: 'Sara Ahmed', cnic: '3520198765432', phone: '03119876543', type: 'CURRENT', balance: 5000,  status: 'ACTIVE', pinAttempts: 0, dailyWithdrawn: 0, lastWithdrawDate: '' },
  ],
  // Kept apart from account data, keyed by account number.
  credentials: { '1001': '1234', '1002': '2000' },
  transactions: [
    { id: 'TXN0001', accountNumber: '1001', type: 'DEPOSIT', amount: 30000, dateTime: '2026-08-08 02:00:00', balanceAfter: 30000 },
    { id: 'TXN0002', accountNumber: '1002', type: 'DEPOSIT', amount: 5000,  dateTime: '2026-08-08 02:00:00', balanceAfter: 5000 },
  ],
  loans: [],
  cash: { notes5000: 20, notes1000: 50, notes500: 50, notes100: 100 },
  nextAccountSeq: 1003,
  nextTxnSeq: 3,
  nextLoanSeq: 1,
}

// Business-rule constants used across the app.
export const RULES = {
  DAILY_WITHDRAW_LIMIT: 50000,
  MAX_PIN_ATTEMPTS: 3,
  OTP_THRESHOLD: 25000,
  LOAN_INTEREST_RATE: 0.10, // 10% flat interest
  MAX_LOAN: 500000,
  ADMIN_PASSWORD: 'admin123',
}
