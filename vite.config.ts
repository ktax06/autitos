import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import tailwindcss from '@tailwindcss/vite'
import { VitePWA } from 'vite-plugin-pwa'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    tailwindcss(),
    VitePWA({
      includeAssets: ['favicon.ico', 'auto.png', 'auto.svg'],
      registerType: 'autoUpdate',
      workbox: {
        navigateFallbackDenylist: [/^\/ws/, /^\/websocket/],
        runtimeCaching: [
          {
            urlPattern: /^https:\/\/fonts\.googleapis\.com\/.*/i,
            handler: 'CacheFirst',
            options: {
              cacheName: 'google-fonts-cache',
              expiration: {
                maxEntries: 10,
                maxAgeSeconds: 60 * 60 * 24 * 365 // <== 365 days
              }
            }
          }
        ]
      },
      manifest: {
        name: 'Proyecto Autito ICC',
        short_name: 'Autito',
        description: 'Control y monitoreo de autito ICC por WiFi',
        theme_color: '#e82c2c',
        background_color: '#ffffff',
        display: 'standalone',
        orientation: 'portrait',
        icons: [
          {
            src: 'auto-192.png',
            sizes: '192x192',
            type: 'image/png',
            purpose: 'any maskable'
          },
          {
            src: 'auto-512.png',
            sizes: '512x512',
            type: 'image/png',
            purpose: 'any maskable'
          }
        ],
      screenshots: [
        {
          src: "screenshot-desktop.png",
          sizes: "1080x720",
          type: "image/png",
          form_factor: "wide"
        },
        {
          src: "screenshot-mobile.png",
          sizes: "480x800",
          type: "image/png",
          form_factor: "narrow"
        }
      ]
      }
    })
  ],
  server: {
    host: '0.0.0.0',
    port: 5173,
    proxy: {
      '/ws': {
        target: 'ws://192.168.1.26:81',
        ws: true,
        changeOrigin: true,
      }
    }
  },
})
