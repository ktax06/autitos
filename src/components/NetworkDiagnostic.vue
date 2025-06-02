<!-- NetworkDiagnostic.vue -->
<template>
  <div class="diagnostic-panel" v-if="showDiagnostic">
    <div class="diagnostic-header">
      <h3>🔍 Diagnóstico de Conexión</h3>
      <button @click="$emit('close')" class="close-btn">✖</button>
    </div>
    
    <div class="diagnostic-content">
      <div class="status-item">
        <span class="label">Modo PWA:</span>
        <span :class="['status', isPWAMode ? 'success' : 'info']">
          {{ isPWAMode ? '✅ Sí' : '🌐 Browser' }}
        </span>
      </div>
      
      <div class="status-item">
        <span class="label">Conexión de Red:</span>
        <span class="status">{{ networkStatus }}</span>
      </div>
      
      <div class="status-item">
        <span class="label">WebSocket Estado:</span>
        <span :class="['status', wsStatusClass]">{{ wsStatus }}</span>
      </div>
      
      <div class="status-item">
        <span class="label">IP Objetivo:</span>
        <span class="status">{{ targetIP }}</span>
      </div>
      
      <div class="status-item">
        <span class="label">Protocolo:</span>
        <span class="status">{{ window.location.protocol }}</span>
      </div>
      
      <div class="diagnostic-actions">
        <button @click="testConnection" class="test-btn" :disabled="testing">
          {{ testing ? '🔄 Probando...' : '🧪 Probar Conexión' }}
        </button>
        <button @click="forceReconnect" class="reconnect-btn">
          🔄 Forzar Reconexión
        </button>
      </div>
      
      <div class="logs" v-if="logs.length > 0">
        <h4>📋 Logs Recientes:</h4>
        <div class="log-entries">
          <div v-for="(log, index) in logs.slice(-10)" :key="index" 
               :class="['log-entry', log.type]">
            <span class="timestamp">{{ log.timestamp }}</span>
            <span class="message">{{ log.message }}</span>
          </div>
        </div>
      </div>
    </div>
  </div>
  
  <!-- Botón flotante de diagnóstico -->
  <button @click="$emit('show-diagnostic')" class="diagnostic-toggle" 
          v-if="!showDiagnostic && showToggle">
    🔍
  </button>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'

const props = defineProps({
  websocketComponent: {
    type: Object,
    default: null
  },
  targetIP: {
    type: String,
    default: ''
  },
  showToggle: {
    type: Boolean,
    default: true
  }
})

const emit = defineEmits(['close'])

const showDiagnostic = ref(false)
const testing = ref(false)
const logs = ref([])
const networkInfo = ref(null)

const isPWAMode = computed(() => {
  return window.matchMedia('(display-mode: standalone)').matches ||
         window.navigator.standalone ||
         document.referrer.includes('android-app://')
})

const networkStatus = computed(() => {
  if (!networkInfo.value) return '❓ Desconocido'
  
  const { type, downlink, rtt, saveData } = networkInfo.value
  return `📶 ${type} | ${downlink}Mbps | ${rtt}ms${saveData ? ' (Ahorro de datos)' : ''}`
})

const wsStatus = computed(() => {
  if (!props.websocketComponent?.websocket?.value) return '❓ No inicializado'
  
  const ws = props.websocketComponent.websocket.value
  if (!ws) return '❌ Desconectado'
  
  switch (ws.readyState) {
    case WebSocket.CONNECTING:
      return '🔄 Conectando...'
    case WebSocket.OPEN:
      return '✅ Conectado'
    case WebSocket.CLOSING:
      return '⏳ Cerrando...'
    case WebSocket.CLOSED:
      return '❌ Cerrado'
    default:
      return '❓ Estado desconocido'
  }
})

const wsStatusClass = computed(() => {
  const ws = props.websocketComponent?.websocket?.value
  if (!ws) return 'error'
  
  switch (ws.readyState) {
    case WebSocket.OPEN:
      return 'success'
    case WebSocket.CONNECTING:
      return 'warning'
    default:
      return 'error'
  }
})

const addLog = (message, type = 'info') => {
  logs.value.push({
    timestamp: new Date().toLocaleTimeString(),
    message,
    type
  })
  
  // Mantener solo los últimos 20 logs
  if (logs.value.length > 20) {
    logs.value = logs.value.slice(-20)
  }
}

const getNetworkInfo = () => {
  if ('connection' in navigator) {
    const conn = navigator.connection || navigator.mozConnection || navigator.webkitConnection
    networkInfo.value = {
      type: conn.effectiveType || conn.type,
      downlink: conn.downlink,
      rtt: conn.rtt,
      saveData: conn.saveData
    }
  }
}

const testConnection = async () => {
  testing.value = true
  addLog('Iniciando prueba de conexión...', 'info')
  
  try {
    // Probar HTTP primero
    const httpUrl = `http://${props.targetIP}:82/`
    const httpResponse = await fetch(httpUrl, { 
      method: 'HEAD',
      mode: 'no-cors',
      timeout: 5000
    })
    addLog('✅ Servidor HTTP accesible', 'success')
    
    // Probar WebSocket
    const wsUrl = `ws://${props.targetIP}:81/`
    const testWs = new WebSocket(wsUrl)
    
    const wsPromise = new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        testWs.close()
        reject(new Error('Timeout'))
      }, 5000)
      
      testWs.onopen = () => {
        clearTimeout(timeout)
        testWs.close()
        resolve('success')
      }
      
      testWs.onerror = (error) => {
        clearTimeout(timeout)
        reject(error)
      }
    })
    
    await wsPromise
    addLog('✅ WebSocket accesible', 'success')
    
  } catch (error) {
    addLog(`❌ Error de conexión: ${error.message}`, 'error')
  } finally {
    testing.value = false
  }
}

const forceReconnect = () => {
  if (props.websocketComponent) {
    addLog('Forzando reconexión...', 'info')
    props.websocketComponent.disconnectWebSocket()
    setTimeout(() => {
      props.websocketComponent.connectWebSocket(`ws://${props.targetIP}:81/`)
    }, 1000)
  }
}

onMounted(() => {
  getNetworkInfo()
  
  // Escuchar cambios de red
  if ('connection' in navigator) {
    navigator.connection.addEventListener('change', getNetworkInfo)
  }
})

onUnmounted(() => {
  if ('connection' in navigator) {
    navigator.connection.removeEventListener('change', getNetworkInfo)
  }
})
</script>

<style scoped>
.diagnostic-panel {
  position: fixed;
  top: 20px;
  right: 20px;
  width: 350px;
  max-height: 80vh;
  background: rgba(0, 0, 0, 0.95);
  color: white;
  border-radius: 12px;
  padding: 20px;
  z-index: 1000;
  overflow-y: auto;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
}

.diagnostic-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
  border-bottom: 1px solid #333;
  padding-bottom: 10px;
}

.close-btn {
  background: #ff4444;
  color: white;
  border: none;
  border-radius: 50%;
  width: 30px;
  height: 30px;
  cursor: pointer;
}

.status-item {
  display: flex;
  justify-content: space-between;
  margin-bottom: 8px;
  padding: 5px 0;
}

.label {
  color: #ccc;
  font-size: 14px;
}

.status {
  font-weight: bold;
  font-size: 14px;
}

.status.success { color: #4CAF50; }
.status.warning { color: #FF9800; }
.status.error { color: #f44336; }
.status.info { color: #2196F3; }

.diagnostic-actions {
  margin: 15px 0;
  display: flex;
  gap: 10px;
}

.test-btn, .reconnect-btn {
  flex: 1;
  padding: 10px;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  font-size: 12px;
}

.test-btn {
  background: #2196F3;
  color: white;
}

.reconnect-btn {
  background: #FF9800;
  color: white;
}

.test-btn:disabled {
  background: #666;
  cursor: not-allowed;
}

.logs {
  margin-top: 15px;
  border-top: 1px solid #333;
  padding-top: 10px;
}

.log-entries {
  max-height: 200px;
  overflow-y: auto;
  font-size: 12px;
}

.log-entry {
  display: flex;
  gap: 10px;
  margin-bottom: 5px;
  padding: 2px 0;
}

.timestamp {
  color: #888;
  min-width: 70px;
}

.message {
  flex: 1;
}

.log-entry.success .message { color: #4CAF50; }
.log-entry.error .message { color: #f44336; }
.log-entry.warning .message { color: #FF9800; }

.diagnostic-toggle {
  position: fixed;
  bottom: 20px;
  right: 20px;
  width: 50px;
  height: 50px;
  border-radius: 50%;
  background: #2196F3;
  color: white;
  border: none;
  font-size: 20px;
  cursor: pointer;
  z-index: 999;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}
</style>