import { defineConfig } from "vite";
import react from "@vitejs/plugin-react-swc";
import path from "path";
import { componentTagger } from "lovable-tagger";
import { VitePWA } from "vite-plugin-pwa"
import history from 'connect-history-api-fallback'

// https://vitejs.dev/config/
export default defineConfig(({ mode }) => ({
  server: {
    host: "::",
    port: 8080,
  },
  plugins: [
    react(),
    mode === 'development' &&
    componentTagger(),
    VitePWA({
      registerType: "autoUpdate",
      manifest: false,
    })
  ].filter(Boolean),
  resolve: {
    alias: {
      "@": path.resolve(__dirname, "./src"),
    },
  },
  preview: {
    port: 4173,
    open: true,
    // 👇 Add middleware to serve index.html for unknown routes
    configurePreviewServer(server) {
      server.middlewares.use(
        history({
          index: '/index.html',
          rewrites: []
        })
      )
    }
  }
}));
