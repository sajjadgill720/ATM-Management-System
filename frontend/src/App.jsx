import { useState, useCallback, useEffect } from 'react'
import { BrowserRouter, Routes, Route, Navigate, Link, useNavigate, useLocation } from 'react-router-dom'
import { useBank } from './store'
import { Brand, Toast } from './ui'
import AtmView from './AtmView'
import AdminView from './AdminView'

// ---------------------------------------------------------------------------
// The app is split into SEPARATE ENDPOINTS:
//   /        customer landing page
//   /atm     customer ATM (login + self-service)
//   /staff   banking-staff administration console
// Customers never see the staff console and vice-versa. All routes share one
// bank store, and the app auto-connects to the C++ backend on load.
// ---------------------------------------------------------------------------
export default function App() {
  const bank = useBank()
  const [toast, setToast] = useState(null)
  const [syncing, setSyncing] = useState(false)

  const notify = useCallback((ok, message) => setToast({ ok, message, t: Date.now() }), [])

  // Connect to the C++ server when the app opens, and keep the live state fresh.
  useEffect(() => {
    let cancelled = false
    ;(async () => {
      const ok = await bank.refresh()
      if (!cancelled) notify(ok, ok ? 'Connected to C++ server — live data loaded.'
                                     : 'C++ server not reachable. Start it with: make serve')
    })()
    return () => { cancelled = true }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  const doSync = async () => {
    setSyncing(true)
    const ok = await bank.refresh()
    setSyncing(false)
    notify(ok, ok ? 'Refreshed from C++ server.' : 'C++ server not reachable.')
  }

  return (
    <BrowserRouter>
      <div className="app">
        <Topbar syncing={syncing} onSync={doSync} />
        <main className="container">
          {!bank.state.connected && (
            <div className="offline-banner">
              ⚠ Not connected to the C++ server. Start it with <code>cd backend &amp;&amp; make serve</code> (or <code>./server</code>), then click <b>Sync C++ data</b>.
            </div>
          )}
          <Routes>
            <Route path="/" element={<CustomerHome bank={bank} />} />
            <Route path="/atm" element={
              <>
                <BackLink to="/">← Back to home</BackLink>
                <AtmView bank={bank} notify={notify} />
              </>
            } />
            <Route path="/staff" element={
              <>
                <BackLink to="/">← Customer site</BackLink>
                <AdminView bank={bank} notify={notify} />
              </>
            } />
            <Route path="*" element={<Navigate to="/" replace />} />
          </Routes>
        </main>
        <Toast toast={toast} onClose={() => setToast(null)} />
      </div>
    </BrowserRouter>
  )
}

// Shared top bar. Shows a "Staff Portal" tag on the staff endpoint.
function Topbar({ syncing, onSync }) {
  const navigate = useNavigate()
  const { pathname } = useLocation()
  const isStaff = pathname.startsWith('/staff')
  return (
    <header className="topbar">
      <button onClick={() => navigate('/')} style={{ background: 'none', border: 'none', cursor: 'pointer' }}>
        <Brand />
      </button>
      <div className="topbar-right">
        {isStaff && <span className="pill staff-pill">STAFF PORTAL</span>}
        <button
          className="btn small ghost sync-btn"
          onClick={onSync}
          disabled={syncing}
          title="Load the latest data written by the C++ program"
        >
          <span className={syncing ? 'spin' : ''}>⟳</span> {syncing ? 'Syncing…' : 'Sync C++ data'}
        </button>
        <span className="pill"><span className="dot" /> System Online</span>
      </div>
    </header>
  )
}

function BackLink({ to, children }) {
  return <Link className="back" to={to}>{children}</Link>
}

// -------- Customer landing page (endpoint: /) — no staff console here --------
function CustomerHome({ bank }) {
  const navigate = useNavigate()

  // Real figures pulled from the live data.
  const accs = bank.state.accounts
  const totalDeposits = accs.reduce((s, a) => s + a.balance, 0)
  const txns = bank.state.transactions.length
  const cash = bank.state.cash
  const cashTotal = cash.notes5000 * 5000 + cash.notes1000 * 1000 + cash.notes500 * 500 + cash.notes100 * 100

  const features = [
    { icon: '💳', title: 'Check & Manage', text: 'See your balance and a full history of every deposit, withdrawal and transfer, any time.' },
    { icon: '💵', title: 'Deposit & Withdraw', text: 'Add funds or take out cash. The ATM dispenses an exact note breakdown and respects your daily limit.' },
    { icon: '🔁', title: 'Transfers with OTP', text: 'Send money to another account instantly. Large transfers are confirmed with a one-time password.' },
    { icon: '🔒', title: 'PIN & Auto-Lock', text: 'Your hidden 4-digit PIN is kept separate from your details, and your card locks after 3 wrong attempts.' },
    { icon: '🧾', title: 'Transaction Ledger', text: 'Every action is recorded with an ID, timestamp and resulting balance — nothing is ever lost.' },
    { icon: '📈', title: 'Savings Profit', text: 'Savings accounts earn monthly profit, applied automatically across your balance.' },
  ]

  const steps = [
    { n: '01', title: 'Insert your card', text: 'Sign in to the ATM with your account number and 4-digit PIN.' },
    { n: '02', title: 'Bank in seconds', text: 'Check your balance, deposit, withdraw cash, or transfer funds to anyone.' },
    { n: '03', title: 'Always up to date', text: 'Each action is saved instantly, so your balance and history are always accurate.' },
  ]

  return (
    <>
      <section className="hero">
        <div className="hero-inner">
          <div className="eyebrow">Online Banking</div>
          <h1>Your money,<br /><span>always within reach.</span></h1>
          <p>Check your balance, withdraw cash, and move money in seconds. Sign in to the ATM to get started.</p>
          <button className="btn primary hero-cta" onClick={() => navigate('/atm')}>Sign in to ATM →</button>
        </div>
      </section>

      <section className="statband">
        <div className="statband-item"><div className="v">{accs.length}</div><div className="k">Accounts</div></div>
        <div className="statband-item"><div className="v">Rs. {totalDeposits.toLocaleString()}</div><div className="k">Total Deposits</div></div>
        <div className="statband-item"><div className="v">Rs. {cashTotal.toLocaleString()}</div><div className="k">ATM Cash</div></div>
        <div className="statband-item"><div className="v">{txns}</div><div className="k">Transactions</div></div>
      </section>

      <section className="home-section">
        <div className="section-head">
          <div className="eyebrow">What you can do</div>
          <h2>Everything your account needs</h2>
        </div>
        <div className="feature-grid">
          {features.map((f) => (
            <div className="feature" key={f.title}>
              <div className="f-icon">{f.icon}</div>
              <h4>{f.title}</h4>
              <p>{f.text}</p>
            </div>
          ))}
        </div>
      </section>

      <section className="home-section">
        <div className="section-head">
          <div className="eyebrow">How it works</div>
          <h2>Three steps to bank</h2>
        </div>
        <div className="steps">
          {steps.map((s) => (
            <div className="step" key={s.n}>
              <div className="s-n">{s.n}</div>
              <h4>{s.title}</h4>
              <p>{s.text}</p>
            </div>
          ))}
        </div>
        <div style={{ textAlign: 'center', marginTop: 26 }}>
          <button className="btn primary" style={{ width: 'auto' }} onClick={() => navigate('/atm')}>Sign in to ATM →</button>
        </div>
      </section>

      <footer className="home-footer">
        <div>
          <div className="brand-mini">MY<b>BANK</b></div>
          <p>Banking, the simple way. Available whenever you need it.</p>
        </div>
        <div className="foot-creds">
          <span className="hint">© {new Date().getFullYear()} MYBANK</span>
          <Link className="staff-link" to="/staff">Banking staff → Staff portal</Link>
        </div>
      </footer>
    </>
  )
}
