<script setup>
import { onMounted, onUnmounted, ref } from 'vue';

const websocket = ref(null);
const isConnected = ref(false);
const canSendMessage = ref(true);

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
  if (websocket.value && websocket.value.readyState === WebSocket.OPEN && message && canSendMessage.value) {
    canSendMessage.value = false;
    try {
      websocket.value.send(message);
      console.log('Mensaje enviado:', message);
      setTimeout(() => {
        canSendMessage.value = true;
      }, 500); // Espera 100 ms antes de permitir otro envío
      return true;
    } catch (error) {
      console.error('Error al enviar mensaje:', error);
      canSendMessage.value = true; // Re-habilitar en caso de error
      return false;
    }
  } else if (!canSendMessage.value) {
    console.warn('Envío de mensaje bloqueado: espera 100ms.');
    return false;
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
  </template>