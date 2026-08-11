import { useState } from 'react'
import { Field, Select, Stat, Money, TxnTable } from './ui'
import { cashTotal } from './store'

// Bank Administration module: password-gated (server-checked) console for staff.
export default function AdminView({ bank, notify }) {
  const [authed, setAuthed] = useState(false)
  const [tab, setTab] = useState('accounts')

  if (!authed) return <AdminLogin bank={bank} notify={notify} onOk={() => setAuthed(true)} />

  return (
    <div className="stack">
      <Dashboard bank={bank} />
      <div className="tabs">
        {[['accounts', 'Accounts'], ['create', 'New Account'], ['cash', 'ATM Cash'], ['txns', 'Transactions'], ['loans', 'Loans'], ['reports', 'Reports']].map(([id, label]) => (
          <button key={id} className={`tab ${tab === id ? 'active' : ''}`} onClick={() => setTab(id)}>{label}</button>
        ))}
        <button className="tab" onClick={() => setAuthed(false)}>Logout</button>
      </div>

      {tab === 'accounts' && <Accounts bank={bank} notify={notify} />}
      {tab === 'create' && <CreateAccount bank={bank} notify={notify} onDone={() => setTab('accounts')} />}
      {tab === 'cash' && <CashManager bank={bank} notify={notify} />}
      {tab === 'txns' && <Transactions bank={bank} />}
      {tab === 'loans' && <Loans bank={bank} />}
      {tab === 'reports' && <Reports bank={bank} notify={notify} />}
    </div>
  )
}

function AdminLogin({ bank, notify, onOk }) {
  const [pass, setPass] = useState('')
  const [busy, setBusy] = useState(false)
  const submit = async (e) => {
    e.preventDefault()
    setBusy(true)
    const r = await bank.adminLogin(pass) // verified by the C++ server
    setBusy(false)
    notify(r.ok, r.message)
    if (r.ok) onOk()
    setPass('')
  }
  return (
    <div className="center-narrow">
      <div className="card">
        <h2>Administration Login</h2>
        <p className="sub">Restricted to bank staff.</p>
        <form onSubmit={submit}>
          <Field label="Admin Password" type="password" value={pass} onChange={(e) => setPass(e.target.value)} placeholder="••••••••" />
          <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Checking…' : 'Enter Console →'}</button>
        </form>
      </div>
    </div>
  )
}

function Dashboard({ bank }) {
  const accs = bank.state.accounts
  const totalDeposits = accs.reduce((s, a) => s + a.balance, 0)
  const active = accs.filter((a) => a.status === 'ACTIVE').length
  const locked = accs.filter((a) => a.status === 'LOCKED').length
  return (
    <div className="grid-4">
      <Stat k="Accounts" v={accs.length} tone="cyan" />
      <Stat k="Active / Locked" v={`${active} / ${locked}`} />
      <Stat k="Total Deposits" v={<Money value={totalDeposits} />} tone="violet" />
      <Stat k="ATM Cash" v={<Money value={cashTotal(bank.state.cash)} />} tone="cyan" />
    </div>
  )
}

function Accounts({ bank, notify }) {
  const act = async (promise) => { const r = await promise; notify(r.ok, r.message) }
  return (
    <div className="card">
      <h2>All Accounts</h2>
      <p className="sub">Freeze, unlock or delete customer accounts.</p>
      <div className="table-wrap">
        <table>
          <thead>
            <tr><th>Acc #</th><th>Name</th><th>Type</th><th>Balance</th><th>Status</th><th>Actions</th></tr>
          </thead>
          <tbody>
            {bank.state.accounts.map((a) => (
              <tr key={a.accountNumber}>
                <td className="mono">{a.accountNumber}</td>
                <td>{a.name}</td>
                <td>{a.type}</td>
                <td className="mono">{Number(a.balance).toLocaleString()}</td>
                <td><span className={`badge ${a.status}`}>{a.status}</span></td>
                <td>
                  <div className="btn-row">
                    {a.status !== 'ACTIVE'
                      ? <button className="btn small" onClick={() => act(bank.setStatus(a.accountNumber, 'ACTIVE'))}>Unlock</button>
                      : <button className="btn small" onClick={() => act(bank.setStatus(a.accountNumber, 'CLOSED'))}>Freeze</button>}
                    <button className="btn small danger" onClick={() => {
                      if (confirm(`Delete account ${a.accountNumber}? This cannot be undone.`)) act(bank.removeAccount(a.accountNumber))
                    }}>Delete</button>
                  </div>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  )
}

function CreateAccount({ bank, notify, onDone }) {
  const [f, setF] = useState({ name: '', cnic: '', phone: '', type: 'SAVINGS', pin: '', opening: '' })
  const [busy, setBusy] = useState(false)
  const up = (k) => (e) => setF({ ...f, [k]: e.target.value })
  const submit = async (e) => {
    e.preventDefault()
    setBusy(true)
    const r = await bank.createAccount({ ...f, opening: Number(f.opening) || 0 })
    setBusy(false)
    notify(r.ok, r.message)
    if (r.ok) { setF({ name: '', cnic: '', phone: '', type: 'SAVINGS', pin: '', opening: '' }); onDone() }
  }
  return (
    <div className="card">
      <h2>Open New Account</h2>
      <p className="sub">Account number is assigned automatically. CNIC must be unique. PIN is stored hashed by the server.</p>
      <form onSubmit={submit}>
        <div className="grid-2">
          <Field label="Customer Name" value={f.name} onChange={up('name')} placeholder="Full name" />
          <Field label="CNIC / National ID" value={f.cnic} onChange={up('cnic')} placeholder="35201-XXXXXXX-X" />
          <Field label="Phone" value={f.phone} onChange={up('phone')} placeholder="03XX-XXXXXXX" />
          <Select label="Account Type" value={f.type} onChange={up('type')}>
            <option value="SAVINGS">SAVINGS</option>
            <option value="CURRENT">CURRENT</option>
          </Select>
          <Field label="4-digit PIN" type="password" value={f.pin} onChange={up('pin')} maxLength={4} inputMode="numeric" />
          <Field label="Opening Deposit (Rs.)" value={f.opening} onChange={up('opening')} placeholder="0" inputMode="decimal" />
        </div>
        <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Creating…' : 'Create Account'}</button>
      </form>
    </div>
  )
}

function CashManager({ bank, notify }) {
  const cash = bank.state.cash
  const [add, setAdd] = useState({ notes5000: '', notes1000: '', notes500: '', notes100: '' })
  const [busy, setBusy] = useState(false)
  const up = (k) => (e) => setAdd({ ...add, [k]: e.target.value })
  const denoms = [['notes5000', 5000], ['notes1000', 1000], ['notes500', 500], ['notes100', 100]]

  const refill = async (e) => {
    e.preventDefault()
    setBusy(true)
    const r = await bank.refillCash({
      notes5000: Number(add.notes5000) || 0, notes1000: Number(add.notes1000) || 0,
      notes500: Number(add.notes500) || 0, notes100: Number(add.notes100) || 0,
    })
    setBusy(false)
    notify(r.ok, r.message)
    setAdd({ notes5000: '', notes1000: '', notes500: '', notes100: '' })
  }

  return (
    <div className="card">
      <h2>ATM Cash Inventory</h2>
      <p className="sub">Current notes in the machine · total <Money value={cashTotal(cash)} />.</p>
      <div className="notes-grid" style={{ marginBottom: 20 }}>
        {denoms.map(([k, d]) => (
          <div className="note" key={k}>
            <div className="d">Rs {d}</div>
            <div className="c">{cash[k]}</div>
          </div>
        ))}
      </div>
      <div className="section-title">Refill (add notes)</div>
      <form onSubmit={refill}>
        <div className="grid-4">
          {denoms.map(([k, d]) => (
            <Field key={k} label={`+ Rs ${d}`} value={add[k]} onChange={up(k)} placeholder="0" inputMode="numeric" />
          ))}
        </div>
        <button className="btn primary" type="submit" disabled={busy}>{busy ? 'Refilling…' : 'Refill ATM'}</button>
      </form>
    </div>
  )
}

function Transactions({ bank }) {
  const [accNo, setAccNo] = useState('')
  const [type, setType] = useState('')
  let rows = bank.state.transactions.slice().reverse()
  if (accNo.trim()) rows = rows.filter((t) => t.accountNumber === accNo.trim())
  if (type) rows = rows.filter((t) => t.type === type)
  return (
    <div className="card">
      <h2>Transaction Ledger</h2>
      <p className="sub">Search and filter every transaction in the bank.</p>
      <div className="grid-2">
        <Field label="Filter by account" value={accNo} onChange={(e) => setAccNo(e.target.value)} placeholder="e.g. 10017" />
        <Select label="Filter by type" value={type} onChange={(e) => setType(e.target.value)}>
          <option value="">Any type</option>
          {['DEPOSIT', 'WITHDRAW', 'TRANSFER-IN', 'TRANSFER-OUT', 'PROFIT', 'LOAN', 'LOAN-REPAY'].map((t) => <option key={t} value={t}>{t}</option>)}
        </Select>
      </div>
      <TxnTable rows={rows} showAccount />
    </div>
  )
}

function Loans({ bank }) {
  const loans = bank.state.loans || []
  const outstanding = loans.reduce((s, l) => s + (l.totalPayable - l.paid), 0)
  return (
    <div className="card">
      <h2>All Loans</h2>
      <p className="sub">{loans.length} loan(s) · total outstanding <Money value={outstanding} />.</p>
      {loans.length === 0 ? <p className="hint">No loans issued yet.</p> : (
        <div className="table-wrap">
          <table>
            <thead>
              <tr><th>Loan #</th><th>Account</th><th>Principal</th><th>Payable</th><th>Paid</th><th>Remaining</th><th>Status</th></tr>
            </thead>
            <tbody>
              {loans.map((l) => (
                <tr key={l.loanId}>
                  <td className="mono">{l.loanId}</td>
                  <td className="mono">{l.accountNumber}</td>
                  <td className="mono">{Number(l.principal).toLocaleString()}</td>
                  <td className="mono">{Number(l.totalPayable).toLocaleString()}</td>
                  <td className="mono">{Number(l.paid).toLocaleString()}</td>
                  <td className="mono">{Number(l.totalPayable - l.paid).toLocaleString()}</td>
                  <td><span className={`badge ${l.status === 'PAID' ? 'ACTIVE' : 'LOCKED'}`}>{l.status}</span></td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  )
}

function Reports({ bank, notify }) {
  const accs = bank.state.accounts
  const savings = accs.filter((a) => a.type === 'SAVINGS')
  const [busy, setBusy] = useState(false)
  return (
    <div className="card">
      <h2>Reports & Profit Run</h2>
      <p className="sub">Portfolio summary and the monthly savings profit run (5% p.a.).</p>
      <div className="grid-3" style={{ marginBottom: 18 }}>
        <Stat k="Savings Accounts" v={savings.length} tone="cyan" />
        <Stat k="Current Accounts" v={accs.length - savings.length} />
        <Stat k="Transactions" v={bank.state.transactions.length} tone="violet" />
      </div>
      <button className="btn primary" style={{ width: 'auto' }} disabled={busy} onClick={async () => {
        setBusy(true); const r = await bank.applyProfit(); setBusy(false); notify(r.ok, r.message)
      }}>{busy ? 'Running…' : 'Run Monthly Savings Profit'}</button>
    </div>
  )
}
