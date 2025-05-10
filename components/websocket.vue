<script setup>
import { onMounted, onUnmounted, ref } from 'vue';

const websocket = ref(null);
const isConnected = ref(false);

// Eventos personalizados
const emit = defineEmits(['connected', 'disconnected', 'message', 'error']);

// Parámetros opcionales
const props = defineProps({
  autoConnect: {
    type: Boolean,
    default: false
  },
  defaultUrl: {
    type: String,
    default: ''
  }
});

onMounted(() => {
  if (props.autoConnect && props.defaultUrl) {
    connectWebSocket(props.defaultUrl);
  }
});

onUnmounted(() => {
  disconnectWebSocket();
});

const connectWebSocket = (url) => {
  // Cerrar cualquier conexión existente primero
  disconnectWebSocket();
  
  try {
    websocket.value = new WebSocket(url);

    websocket.value.onopen = () => {
      console.log('Conectado al WebSocket del ESP32');
      isConnected.value = true;
      emit('connected');
    };

    websocket.value.onmessage = (event) => {
      console.log('Mensaje recibido:', event.data);
      emit('message', event.data);
    };

    websocket.value.onclose = () => {
      console.log('Desconectado del WebSocket del ESP32');
      isConnected.value = false;
      emit('disconnected');
    };

    websocket.value.onerror = (error) => {
      console.error('Error en el WebSocket:', error);
      isConnected.value = false;
      emit('error', error);
    };
  } catch (error) {
    console.error('Error al crear la conexión WebSocket:', error);
    isConnected.value = false;
    emit('error', error);
  }
};

const disconnectWebSocket = () => {
  if (websocket.value && websocket.value.readyState === WebSocket.OPEN) {
    websocket.value.close();
    isConnected.value = false;
  }
};

const sendMessage = (message) => {
  if (websocket.value && websocket.value.readyState === WebSocket.OPEN && message) {
    websocket.value.send(message);
    console.log('Mensaje enviado:', message);
    return true;
  }
  return false;
};

// Exponer métodos para que el componente padre pueda utilizarlos
defineExpose({
  connectWebSocket,
  disconnectWebSocket,
  sendMessage,
  isConnected
});
</script>

<template>
  <!-- Este componente no tiene representación visual -->
</template>