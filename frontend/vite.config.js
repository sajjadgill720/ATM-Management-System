import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// The React app talks to the C++ HTTP server for all data. We proxy /api to it
// so the browser makes same-origin calls (no CORS needed).
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    open: false,
    proxy: {
      '/api': {
        target: 'http://localhost:8080', // the C++ server
        changeOrigin: true,
      },
    },
  },
})
