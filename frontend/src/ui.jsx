import { useEffect } from 'react'

// Small reusable presentational pieces shared by the ATM and Admin views.

export function Brand() {
  return (
    <div className="brand">
      <div className="logo"><span>M</span></div>
      <div>
        <div className="name">MY<b>BANK</b></div>
        <div className="tag">Online Banking</div>
      </div>
    </div>
  )
}

export function Field({ label, ...props }) {
  return (
    <div className="field">
      {label && <label>{label}</label>}
      <input {...props} />
    </div>
  )
}

export function Select({ label, children, ...props }) {
  return (
    <div className="field">
      {label && <label>{label}</label>}
      <select {...props}>{children}</select>
    </div>
  )
}

export function Stat({ k, v, tone }) {
  return (
    <div className="stat">
      <div className="k">{k}</div>
      <div className={`v ${tone || ''}`}>{v}</div>
    </div>
  )
}

export function Money({ value }) {
  return <>Rs. {Number(value).toLocaleString('en-PK', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}</>
}

// A transaction table used in both modules.
export function TxnTable({ rows, showAccount = false }) {
  if (!rows.length) return <p className="hint">No transactions yet.</p>
  const isIn = (t) => ['DEPOSIT', 'TRANSFER-IN', 'PROFIT', 'LOAN'].includes(t)
  return (
    <div className="table-wrap">
      <table>
        <thead>
          <tr>
            <th>ID</th>
            {showAccount && <th>Account</th>}
            <th>Type</th>
            <th>Amount</th>
            <th>Balance</th>
            <th>Date / Time</th>
          </tr>
        </thead>
        <tbody>
          {rows.map((t) => (
            <tr key={t.id}>
              <td className="mono">{t.id}</td>
              {showAccount && <td className="mono">{t.accountNumber}</td>}
              <td className={`ttype ${isIn(t.type) ? 'tin' : 'tout'}`}>
                {isIn(t.type) ? '▲ ' : '▼ '}{t.type}
              </td>
              <td className="mono">{Number(t.amount).toLocaleString()}</td>
              <td className="mono">{Number(t.balanceAfter).toLocaleString()}</td>
              <td className="mono">{t.dateTime}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  )
}

// Auto-dismissing toast notification.
export function Toast({ toast, onClose }) {
  useEffect(() => {
    if (!toast) return
    const id = setTimeout(onClose, 3200)
    return () => clearTimeout(id)
  }, [toast, onClose])
  if (!toast) return null
  return <div className={`toast ${toast.ok ? 'ok' : 'err'}`}>{toast.ok ? '✓ ' : '⚠ '}{toast.message}</div>
}
