import { useState } from 'react'
import { Field, Money, TxnTable } from './ui'
import { RULES } from './seed'

// ATM Customer module: login, then a tabbed dashboard. All actions call the C++
// server (via the store) and await its reply.
export default function AtmView({ bank, notify }) {
  const [accNo, setAccNo] = useState('')
  const [tab, setTab] = useState('overview')

  // The live account is read fresh from the (server-backed) state each render.
  const account = accNo ? bank.find(accNo) : null

  if (!account) {
    return <Login bank={bank} notify={notify} onLogin={(no) => { setAccNo(no); setTab('overview') }} />
  }

  const history = bank.state.transactions
    .filter((t) => t.accountNumber === account.accountNumber)
    .slice().reverse()

  return (
    <div className="stack">
      <div className="balance">
        <div className="chip">•••• {account.accountNumber}</div>
        <div className="k">Available Balance</div>
        <div className="v"><Money value={account.balance} /></div>
        <div className="who">{account.name} · {account.type} · <span className={`badge ${account.status}`}>{account.status}</span></div>
      </div>

      <div className="tabs">
        {['overview', 'deposit', 'withdraw', 'transfer', 'loans', 'security'].map((t) => (
          <button key={t} className={`tab ${tab === t ? 'active' : ''}`} onClick={() => setTab(t)}>
            {t[0].toUpperCase() + t.slice(1)}
          </button>
        ))}
        <button className="tab" onClick={() => { setAccNo(''); notify(true, 'Logged out.') }}>Logout</button>
      </div>

      {tab === 'overview' && (
        <div className="card">
          <h2>Recent Activity</h2>
          <p className="sub">Your latest transactions on account {account.accountNumber}.</p>
          <TxnTable rows={history.slice(0, 8)} />
        </div>
      )}

      {tab === 'deposit' && <Deposit account={account} bank={bank} notify={notify} />}
      {tab === 'withdraw' && <Withdraw account={account} bank={bank} notify={notify} />}
      {tab === 'transfer' && <Transfer account={account} bank={bank} notify={notify} />}
      {tab === 'loans' && <Loans account={account} bank={bank} notify={notify} />}
      {tab === 'security' && <Security account={account} bank={bank} notify={notify} />}
    </div>
  )
}

function Login({ bank, notify, onLogin }) {
  const [accNo, setAccNo] = useState('')
  const [pin, setPin] = useState('')
  const [busy, setBusy] = useState(false)

  const submit = async (e) => {
    e.preventDefault()
    setBusy(true)
    const r = await bank.login(accNo.trim(), pin.trim())
    setBusy(false)
    notify(r.ok, r.message)
    if (r.ok) onLogin(accNo.trim())
    setPin('')
  }

  return (
    <div className="center-narrow">
      <div className="card">
        <h2>ATM Login</h2>
        <p className="sub">Insert your card — enter your account number and 4-digit PIN.</p>
        <form onSubmit={submit}>
          <Field label="Account Number" value={accNo} onChange={(e) => setAccNo(e.target.value)} placeholder="1001" inputMode="numeric" />
          <Field label="PIN" type="password" value={pin} onChange={(e) => setPin(e.target.value)} placeholder="••••" maxLength={4} inputMode="numeric" />
          <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Checking…' : 'Authenticate →'}</button>
        </form>
        <p className="hint" style={{ marginTop: 14 }}>For your security, your card locks after 3 incorrect PIN attempts.</p>
      </div>
    </div>
  )
}

// Reusable amount input whose submit awaits an async handler.
function AmountForm({ label, cta, onSubmit }) {
  const [amount, setAmount] = useState('')
  const [busy, setBusy] = useState(false)
  const submit = async (e) => {
    e.preventDefault()
    setBusy(true)
    await onSubmit(Number(amount))
    setBusy(false)
    setAmount('')
  }
  return (
    <form onSubmit={submit}>
      <Field label={label} value={amount} onChange={(e) => setAmount(e.target.value)} placeholder="0" inputMode="decimal" />
      <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Working…' : cta}</button>
    </form>
  )
}

function Deposit({ account, bank, notify }) {
  return (
    <div className="card">
      <h2>Deposit Cash</h2>
      <p className="sub">Add funds to account {account.accountNumber}.</p>
      <AmountForm label="Deposit amount (Rs.)" cta="Deposit"
        onSubmit={async (amt) => { const r = await bank.deposit(account.accountNumber, amt); notify(r.ok, r.message) }} />
    </div>
  )
}

function Withdraw({ account, bank, notify }) {
  return (
    <div className="card">
      <h2>Withdraw Cash</h2>
      <p className="sub">Multiples of 100 · daily limit <Money value={RULES.DAILY_WITHDRAW_LIMIT} /> · subject to ATM cash.</p>
      <AmountForm label="Withdrawal amount (Rs.)" cta="Withdraw"
        onSubmit={async (amt) => { const r = await bank.withdraw(account.accountNumber, amt); notify(r.ok, r.message) }} />
    </div>
  )
}

function Transfer({ account, bank, notify }) {
  const [to, setTo] = useState('')
  const [amount, setAmount] = useState('')
  const [otpStage, setOtpStage] = useState(null) // { to, amount, otp }
  const [otpInput, setOtpInput] = useState('')
  const [busy, setBusy] = useState(false)

  const start = async (e) => {
    e.preventDefault()
    const amt = Number(amount)
    setBusy(true)
    // First call with no OTP. For large transfers the server replies needOtp.
    const r = await bank.transfer(account.accountNumber, to.trim(), amt)
    setBusy(false)
    if (r.needOtp) { setOtpStage({ to: to.trim(), amount: amt }); return }
    notify(r.ok, r.message)
    if (r.ok) { setTo(''); setAmount('') }
  }

  const confirm = async () => {
    setBusy(true)
    const r = await bank.transfer(account.accountNumber, otpStage.to, otpStage.amount, otpInput.trim())
    setBusy(false)
    notify(r.ok, r.message)
    if (r.ok) { setTo(''); setAmount(''); setOtpStage(null); setOtpInput('') }
  }

  if (otpStage) {
    return (
      <div className="card">
        <h2>Verify Large Transfer</h2>
        <p className="sub">Transfers above <Money value={RULES.OTP_THRESHOLD} /> need a one-time password.</p>
        <p className="hint">An OTP has been sent to the account holder. Find it in <code>backend/otp.txt</code> or the server console, then enter it below.</p>
        <Field label="Enter OTP" value={otpInput} onChange={(e) => setOtpInput(e.target.value)} placeholder="6-digit code" inputMode="numeric" maxLength={6} />
        <div className="btn-row">
          <button className="btn primary" onClick={confirm} disabled={busy}>{busy ? 'Confirming…' : 'Confirm Transfer'}</button>
          <button className="btn ghost" onClick={() => { setOtpStage(null); setOtpInput('') }}>Cancel</button>
        </div>
      </div>
    )
  }

  return (
    <div className="card">
      <h2>Transfer Funds</h2>
      <p className="sub">Send money to another MYBANK account.</p>
      <form onSubmit={start}>
        <Field label="Destination account number" value={to} onChange={(e) => setTo(e.target.value)} placeholder="1002" inputMode="numeric" />
        <Field label="Amount (Rs.)" value={amount} onChange={(e) => setAmount(e.target.value)} placeholder="0" inputMode="decimal" />
        <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Working…' : 'Continue →'}</button>
      </form>
    </div>
  )
}

function Loans({ account, bank, notify }) {
  const loan = bank.activeLoan(account.accountNumber)

  if (loan) {
    const remaining = loan.totalPayable - loan.paid
    return (
      <div className="card">
        <h2>Your Loan</h2>
        <p className="sub">Loan {loan.loanId} · issued {loan.dateIssued}</p>
        <div className="grid-3" style={{ marginBottom: 18 }}>
          <div className="stat"><div className="k">Total Payable</div><div className="v cyan"><Money value={loan.totalPayable} /></div></div>
          <div className="stat"><div className="k">Paid</div><div className="v"><Money value={loan.paid} /></div></div>
          <div className="stat"><div className="k">Remaining</div><div className="v violet"><Money value={remaining} /></div></div>
        </div>
        <AmountForm label="Repay amount (Rs.)" cta="Repay Loan"
          onSubmit={async (amt) => { const r = await bank.repayLoan(account.accountNumber, amt); notify(r.ok, r.message) }} />
      </div>
    )
  }

  return (
    <div className="card">
      <h2>Apply for a Loan</h2>
      <p className="sub">{RULES.LOAN_INTEREST_RATE * 100}% flat interest · up to <Money value={RULES.MAX_LOAN} />. The amount is credited to your balance; you repay the total.</p>
      <AmountForm label="Loan amount (Rs.)" cta="Apply for Loan"
        onSubmit={async (amt) => { const r = await bank.applyLoan(account.accountNumber, amt); notify(r.ok, r.message) }} />
    </div>
  )
}

function Security({ account, bank, notify }) {
  const [oldPin, setOldPin] = useState('')
  const [newPin, setNewPin] = useState('')
  const [confirm, setConfirm] = useState('')
  const [busy, setBusy] = useState(false)

  const submit = async (e) => {
    e.preventDefault()
    if (newPin !== confirm) return notify(false, 'New PINs do not match.')
    setBusy(true)
    const r = await bank.changePin(account.accountNumber, oldPin, newPin)
    setBusy(false)
    notify(r.ok, r.message)
    if (r.ok) { setOldPin(''); setNewPin(''); setConfirm('') }
  }

  return (
    <div className="card">
      <h2>Change PIN</h2>
      <p className="sub">Set a new 4-digit PIN for your card.</p>
      <form onSubmit={submit}>
        <Field label="Current PIN" type="password" value={oldPin} onChange={(e) => setOldPin(e.target.value)} maxLength={4} inputMode="numeric" />
        <div className="grid-2">
          <Field label="New PIN" type="password" value={newPin} onChange={(e) => setNewPin(e.target.value)} maxLength={4} inputMode="numeric" />
          <Field label="Confirm PIN" type="password" value={confirm} onChange={(e) => setConfirm(e.target.value)} maxLength={4} inputMode="numeric" />
        </div>
        <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Updating…' : 'Update PIN'}</button>
      </form>
    </div>
  )
}
