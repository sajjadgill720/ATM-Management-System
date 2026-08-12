import { useState, useCallback } from 'react'

// ---------------------------------------------------------------------------
// The data layer. Every read and write goes to the C++ HTTP server (proxied at
// /api), so the web app and the C++ files share ONE live state. Nothing is kept
// only in the browser — this is a genuine two-way connection to the C++ backend.
//
// Every action returns { ok, message, ...extra } from the server so the UI can
// show a toast. Actions are async (they await the server), and after any write
// we re-fetch /api/state so the dashboards reflect the new truth.
// ---------------------------------------------------------------------------

const EMPTY = {
  accounts: [],
  transactions: [],
  loans: [],
  cash: { notes5000: 0, notes1000: 0, notes500: 0, notes100: 0, total: 0 },
  connected: false,
}

export function cashTotal(cash) {
  return cash.notes5000 * 5000 + cash.notes1000 * 1000 + cash.notes500 * 500 + cash.notes100 * 100
}

export function useBank() {
  const [state, setState] = useState(EMPTY)

  // Pull the whole live state from the C++ server.
  const refresh = useCallback(async () => {
    try {
      const res = await fetch('/api/state', { cache: 'no-store' })
      if (!res.ok) throw new Error('bad status')
      const data = await res.json()
      setState({
        accounts: data.accounts || [],
        transactions: data.transactions || [],
        loans: data.loans || [],
        cash: data.cash || EMPTY.cash,
        connected: true,
      })
      return true
    } catch {
      setState((s) => ({ ...s, connected: false }))
      return false
    }
  }, [])

  // POST a request, then refresh the state, and hand the server's reply back.
  const post = useCallback(async (url, body) => {
    let res
    try {
      res = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
      })
    } catch {
      return { ok: false, message: 'Cannot reach the C++ server. Is it running (make serve)?' }
    }
    let data
    try { data = await res.json() } catch { return { ok: false, message: 'Unexpected server response.' } }
    await refresh() // reflect the new truth after any write
    return data
  }, [refresh])

  // ----- lookups (read from the last fetched state) -----
  const find = useCallback((accNo) => state.accounts.find((a) => a.accountNumber === accNo), [state])
  const activeLoan = useCallback((accNo) =>
    (state.loans || []).find((l) => l.accountNumber === accNo && l.status === 'ACTIVE'), [state])

  // ----- customer actions -----
  const login = useCallback((accNo, pin) => post('/api/login', { accountNumber: accNo, pin }), [post])
  const deposit = useCallback((accNo, amount) => post('/api/deposit', { accountNumber: accNo, amount }), [post])
  const withdraw = useCallback((accNo, amount) => post('/api/withdraw', { accountNumber: accNo, amount }), [post])
  // otp is '' on the first call; the server replies needOtp + otp for large amounts.
  const transfer = useCallback((from, to, amount, otp = '') => post('/api/transfer', { from, to, amount, otp }), [post])
  const changePin = useCallback((accNo, oldPin, newPin) => post('/api/changepin', { accountNumber: accNo, oldPin, newPin }), [post])
  const applyLoan = useCallback((accNo, amount) => post('/api/loan/apply', { accountNumber: accNo, amount }), [post])
  const repayLoan = useCallback((accNo, amount) => post('/api/loan/repay', { accountNumber: accNo, amount }), [post])

  // ----- staff actions -----
  const adminLogin = useCallback((password) => post('/api/admin/login', { password }), [post])
  const createAccount = useCallback((data) => post('/api/admin/create', data), [post])
  const setStatus = useCallback((accNo, status) => post('/api/admin/status', { accountNumber: accNo, status }), [post])
  const removeAccount = useCallback((accNo) => post('/api/admin/delete', { accountNumber: accNo }), [post])
  const refillCash = useCallback((add) => post('/api/admin/cash', add), [post])
  const applyProfit = useCallback(() => post('/api/admin/profit', {}), [post])

  return {
    state, refresh, find, activeLoan,
    login, deposit, withdraw, transfer, changePin, applyLoan, repayLoan,
    adminLogin, createAccount, setStatus, removeAccount, refillCash, applyProfit,
  }
}
