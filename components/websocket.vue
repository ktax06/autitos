<script setup>
import { onMounted, onUnmounted, ref } from 'vue';

const websocket = ref(null);
const messages = ref([]);

onMounted(() => {
  connectWebSocket();
});

onUnmounted(() => {
  disconnectWebSocket();
});

const connectWebSocket = () => {
  websocket.value = new WebSocket('ws://192.168.1.35:81/'); // Reemplaza con la IP y el puerto de tu ESP32

  websocket.value.onopen = () => {
    console.log('Conectado al WebSocket del ESP32');
  };

  websocket.value.onmessage = (event) => {
    console.log('Mensaje recibido:', event.data);
  };

  websocket.value.onclose = () => {
    console.log('Desconectado del WebSocket del ESP32');
    setTimeout(connectWebSocket, 3000);
  };

  websocket.value.onerror = (error) => {
    console.error('Error en el WebSocket:', error);
  };
};

const disconnectWebSocket = () => {
  if (websocket.value && websocket.value.readyState === WebSocket.OPEN) {
    websocket.value.close();
  }
};

const sendMessage = (message) => {
  if (websocket.value && websocket.value.readyState === WebSocket.OPEN && message) {
    websocket.value.send(message);
  }
};

defineExpose({
  sendMessage
});
</script>