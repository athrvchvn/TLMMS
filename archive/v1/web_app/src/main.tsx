import { createRoot } from 'react-dom/client'
import { StrictMode } from 'react';
import App from './App.tsx'
import './index.css'
import { registerSW } from "virtual:pwa-register"

registerSW({
  onNeedRefresh() {
    console.log('New content available, refresh!')
  },
  onOfflineReady() {
    console.log('App ready to work offline!')
  }
})


createRoot(document.getElementById("root")!).render(<StrictMode><App /></StrictMode>);
