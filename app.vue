<template>
  <div class="container">
    <h2>Control de Línea 1</h2>
    <div class="button-group">
      <button @click="sendCommand('linea1/avanzar')">Avanzar</button>
      <button @click="sendCommand('linea1/detener')">Detener</button>
    </div>

    <h2>Control de Línea 2</h2>
    <div class="button-group">
      <button @click="sendCommand('linea2/avanzar')">Avanzar</button>
      <button @click="sendCommand('linea2/detener')">Detener</button>
    </div>
  </div>
</template>

<script>
export default {
  data() {
    return {
      esp32Ip: 'http://192.168.1.100' // Cambia esto por la IP real del ESP32
    };
  },
  methods: {
    async sendCommand(path) {
      try {
        const response = await fetch(`${this.esp32Ip}/${path}`);
        const text = await response.text();
        console.log('Respuesta ESP32:', text);
      } catch (err) {
        console.error('Error enviando comando:', err);
      }
    }
  }
};
</script>

<style scoped>
.container {
  padding: 20px;
  font-family: sans-serif;
  background-color: #f0f4f8;
  min-height: 100vh;
}

h2 {
  margin-top: 20px;
  color: #333;
}

.button-group {
  margin-bottom: 20px;
}

button {
  margin: 10px;
  padding: 12px 24px;
  font-size: 16px;
  background-color: #1976d2;
  color: white;
  border: none;
  border-radius: 8px;
  box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.1);
  cursor: pointer;
  transition: background-color 0.2s ease;
}

button:hover {
  background-color: #1565c0;
}
</style>