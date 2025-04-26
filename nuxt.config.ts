// https://nuxt.com/docs/api/configuration/nuxt-config
import tailwindcss from "@tailwindcss/vite";
import { JoystickManager } from "nipplejs";

export default defineNuxtConfig({
  compatibilityDate: '2024-11-01',
  devtools: { enabled: true },
  css: ['~/assets/css/main.css'],

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

