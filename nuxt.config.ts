// https://nuxt.com/docs/api/configuration/nuxt-config
import tailwindcss from "@tailwindcss/vite";
import { JoystickManager } from "nipplejs";

export default defineNuxtConfig({
  compatibilityDate: '2024-11-01',
  devtools: { enabled: true },
  css: ['~/assets/css/main.css'],

  app: {
    head: {
      title: 'Proyecto Autito ICC',
      meta: [
        { name: 'description', content: 'Control y monitoreo de autito ICC por WiFi' }
      ],
      link: [
        { rel: 'icon', type: 'image/png', href: '/auto.png' } // Asegúrate de tener favicon.png en /public
      ]
    }
  },

  vite: {
    plugins: [
      tailwindcss(),
      {
        name: 'nipplejs-plugin',
        transform(code, id) {
          if (id.endsWith('nipplejs.js')) {
            return code.replace(/JoystickManager/g, 'JoystickManager');
          }
          return code;
        }
      }
    ],
    optimizeDeps: {
      needsInterop: ['vue-joystick']
    }
  }
})

