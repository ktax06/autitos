<!-- carControl.vue -->
<template>
  <div class="min-h-screen flex flex-col items-center bg-gray-100 p-4 space-y-4">
    <h1 class="text-2xl font-bold text-center">Proyecto autito ICC</h1>

    <div class="flex flex-col-reverse md:flex-row w-full max-w-6xl gap-6 mt-4 items-center md:items-start">
      <!-- CONTROLES -->
      <div class="flex flex-col items-center md:items-start md:w-1/3 space-y-10 p-6">
        <!-- Botón Flash -->
        <button
          @click="toggleFlash"
          class="w-48 px-6 py-3 bg-yellow-400 hover:bg-yellow-500 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition-all duration-150"
        >
          {{ flashOn ? 'Apagar Flash' : 'Encender Flash' }}
        </button>

        <!-- Joystick -->
        <div class="w-40 h-40 mr-20">
          <JoystickComponent @move="handleJoystickMove" />
        </div>

        <!-- Botón Conectar Autito -->
        <button
          @click="ipModalOpen = true"
          class="w-48 px-6 py-3 mt-20 bg-indigo-600 hover:bg-indigo-700 active:scale-95 text-white font-semibold rounded-2xl shadow-md transition-all duration-150"
        >
          Conectar Autito
        </button>
        
        <!-- Indicador de conexión Bluetooth -->
        <div v-if="bluetoothDevice" class="text-sm text-green-600 text-center">
          🔵 Bluetooth: {{ bluetoothDevice.name }}
        </div>
      </div>

      <!-- CÁMARAS -->
      <div class="w-full md:flex-1">
        <!-- Vista de cámara -->
        <div class="aspect-video bg-black rounded-2xl overflow-hidden shadow-lg w-full">
          <img 
            v-if="cameraUrl" 
            :src="cameraUrl" 
            alt="Cámara" 
            class="w-full h-full object-cover" 
          />
          <div v-else class="flex items-center justify-center h-full text-white">
            Esperando señal...
          </div>
        </div>
      </div>
    </div>

    <!-- MODAL Conexión IP -->
    <div v-if="ipModalOpen" class="fixed inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50">
      <div class="bg-white p-6 rounded-2xl shadow-xl w-96 space-y-4 max-h-[90vh] overflow-y-auto">
        <h2 class="text-xl font-bold text-center">Conectar Autito</h2>
        
        <!-- Sección Bluetooth -->
        <div class="border-b pb-4">
          <h3 class="text-lg font-semibold mb-3 text-blue-600">🔵 Conexión Bluetooth</h3>
          
          <div v-if="!bluetoothSupported" class="text-red-500 text-sm mb-3">
            ⚠️ Bluetooth no está disponible en este dispositivo/navegador
          </div>
          
          <button
            v-if="bluetoothSupported"
            @click="connectBluetooth"
            :disabled="bluetoothConnecting"
            :class="[
              'w-full py-3 mb-3 font-semibold rounded-lg shadow-md transition-all',
              bluetoothConnecting
                ? 'bg-gray-400 text-white cursor-not-allowed'
                : bluetoothDevice
                ? 'bg-green-500 hover:bg-green-600 text-white'
                : 'bg-blue-500 hover:bg-blue-600 text-white'
            ]"
          >
            {{ bluetoothConnecting ? 'Conectando...' : bluetoothDevice ? `Conectado: ${bluetoothDevice.name}` : 'Conectar por Bluetooth' }}
          </button>

          <div v-if="bluetoothDevice && !obtainedIp" class="text-center">
            <button
              @click="getIpFromBluetooth"
              :disabled="gettingIp"
              class="w-full py-2 bg-indigo-500 hover:bg-indigo-600 text-white font-semibold rounded-lg"
            >
              {{ gettingIp ? 'Obteniendo IP...' : 'Obtener IP del Autito' }}
            </button>
          </div>

          <div v-if="obtainedIp" class="text-center text-green-600 font-semibold">
            ✅ IP obtenida: {{ obtainedIp }}
          </div>
        </div>

        <!-- Sección Manual -->
        <div class="pt-4">
          <h3 class="text-lg font-semibold mb-3 text-gray-600">📝 Conexión Manual</h3>
          
          <label class="block">
            <span class="text-sm font-medium text-gray-700">Dirección IP del autito</span>
            <input
              v-model="manualIp"
              :class="[
                'mt-1 w-full px-4 py-2 border rounded-lg shadow-sm focus:outline-none focus:ring',
                ipIsValid ? 'border-gray-300 focus:ring-indigo-200' : 'border-red-500 focus:ring-red-200'
              ]"
              placeholder="ej: 192.168.1.123"
            />
          </label>

          
        </div>

        <!-- Estado de conexión -->
        <div v-if="connectionStatus" class="text-center text-sm p-2 rounded-lg bg-gray-100">
          Estado: {{ connectionStatus }}
        </div>

        <!-- Botones principales -->
        <div class="flex justify-end gap-4 pt-4 border-t">
          <button
            @click="connectToESP()"
            :disabled="searching || bluetoothConnecting || gettingIp"
            :class="[
              'px-6 py-2 rounded-lg font-semibold shadow-md',
              (searching || bluetoothConnecting || gettingIp)
                ? 'bg-gray-400 text-white cursor-not-allowed'
                : 'bg-indigo-500 hover:bg-indigo-600 text-white'
            ]"
          >
            Conectar WebSocket
          </button>
          <button
            @click="closeModal"
            :disabled="searching || bluetoothConnecting || gettingIp"
            :class="[
              'px-6 py-2 rounded-lg font-semibold shadow-md',
              (searching || bluetoothConnecting || gettingIp)
                ? 'bg-gray-400 text-white cursor-not-allowed'
                : 'bg-gray-300 hover:bg-gray-400 text-gray-800'
            ]"
          >
            Cancelar
          </button>
        </div>
      </div>
    </div>

    <!-- Componente WebSocket (invisible) -->
    <ESP32WebSocket ref="webSocketComponent" />
    
  </div>
</template>

<script setup>
  import JoystickComponent from './joystickComponent.vue'
  import { onMounted, ref, onBeforeUnmount, computed} from 'vue'
  import ESP32WebSocket from '~/components/websocket.vue'

  const flashOn = ref(false)
  const ipModalOpen = ref(false)
  const cameraUrl = ref('') // URL de la cámara (puerto 82)
  const manualIp = ref('')
  const connectionStatus = ref('')
  const searching = ref(false)
  const webSocketComponent = ref(null)

  // Variables Bluetooth
  const bluetoothSupported = ref(false)
  const bluetoothDevice = ref(null)
  const bluetoothCharacteristic = ref(null)
  const bluetoothConnecting = ref(false)
  const obtainedIp = ref('')
  const gettingIp = ref(false)

  // Constantes Bluetooth
  const ESP32_SERVICE_UUID = '12345678-1234-1234-1234-123456789abc'
  const IP_CHARACTERISTIC_UUID = '87654321-4321-4321-4321-cba987654321'

  const toggleFlash = () => {
    flashOn.value = !flashOn.value
    sendCommand(flashOn.value ? 'FLASH_ON' : 'FLASH_OFF')
  }

  const ipIsValid = ref(true)

  // Verificar soporte Bluetooth
  const checkBluetoothSupport = () => {
    bluetoothSupported.value = 'bluetooth' in navigator && 'requestDevice' in navigator.bluetooth
  }

  // Conectar dispositivo Bluetooth
  const connectBluetooth = async () => {
    if (!bluetoothSupported.value) {
      connectionStatus.value = 'Bluetooth no soportado'
      return
    }

    bluetoothConnecting.value = true
    connectionStatus.value = 'Buscando dispositivos Bluetooth...'

    try {
      // Solicitar dispositivo Bluetooth
      const device = await navigator.bluetooth.requestDevice({
        filters: [
          { namePrefix: 'ESP32' },
          { namePrefix: 'Autito' },
          { services: [ESP32_SERVICE_UUID] }
        ],
        optionalServices: [ESP32_SERVICE_UUID]
      })

      connectionStatus.value = 'Conectando al dispositivo...'
      
      // Conectar al servidor GATT
      const server = await device.gatt.connect()
      
      // Obtener el servicio
      const service = await server.getPrimaryService(ESP32_SERVICE_UUID)
      
      // Obtener la característica para obtener IP
      const characteristic = await service.getCharacteristic(IP_CHARACTERISTIC_UUID)
      
      bluetoothDevice.value = device
      bluetoothCharacteristic.value = characteristic
      connectionStatus.value = `Conectado a ${device.name}`
      
      // Listener para desconexión
      device.addEventListener('gattserverdisconnected', onBluetoothDisconnected)
      
    } catch (error) {
      console.error('Error conectando Bluetooth:', error)
      connectionStatus.value = 'Error conectando Bluetooth: ' + error.message
      bluetoothDevice.value = null
      bluetoothCharacteristic.value = null
    }
    
    bluetoothConnecting.value = false
  }

  // Obtener IP desde Bluetooth
  const getIpFromBluetooth = async () => {
    if (!bluetoothCharacteristic.value) {
      connectionStatus.value = 'No hay conexión Bluetooth activa'
      return
    }

    gettingIp.value = true
    connectionStatus.value = 'Obteniendo IP del dispositivo...'

    try {
      // Leer la característica que contiene la IP
      const value = await bluetoothCharacteristic.value.readValue()
      const ip = new TextDecoder().decode(value)
      
      if(ip) {
        obtainedIp.value = ip
        manualIp.value = ip
        connectionStatus.value = `IP obtenida: ${ip}`
      } else {
        connectionStatus.value = 'IP recibida no es válida: ' + ip
      }
      
    } catch (error) {
      console.error('Error obteniendo IP:', error)
      connectionStatus.value = 'Error obteniendo IP: ' + error.message
    }
    
    gettingIp.value = false
  }

  // Manejar desconexión Bluetooth
  const onBluetoothDisconnected = () => {
    console.log('Dispositivo Bluetooth desconectado')
    bluetoothDevice.value = null
    bluetoothCharacteristic.value = null
    obtainedIp.value = ''
    connectionStatus.value = 'Dispositivo Bluetooth desconectado'
  }

  // Seleccionar dispositivo encontrado en la búsqueda automática
  const selectFoundDevice = (ip) => {
    manualIp.value = ip
    ipIsValid.value = true
  }

  // Cerrar modal
  const closeModal = () => {
    ipModalOpen.value = false
    connectionStatus.value = ''
  }

  const connectToESP = async (ip = null) => {
    const targetIp = ip || manualIp.value
    try {
      connectionStatus.value = 'Conectando...'
      
      // Desconectar el WebSocket existente
      if (webSocketComponent.value) {
        webSocketComponent.value.disconnectWebSocket()
      }
      
      // Actualizar URL de la cámara en el puerto 82
      cameraUrl.value = `http://${targetIp}:82/stream`
      
      // Verificar conexión a la cámara
      try {
        const response = await fetch(`http://${targetIp}:82/`, { mode: 'no-cors' })
        console.log('Cámara disponible en puerto 82: ', response)
      } catch (err) {
        console.warn('No se pudo conectar a la cámara en puerto 82:', err)
        cameraUrl.value = ''
      }
      
      // Crear una nueva conexión WebSocket con el componente
      webSocketComponent.value.connectWebSocket(`ws://${targetIp}:81/`)
      
      connectionStatus.value = 'Conectado'
      ipModalOpen.value = false

    } catch (err) {
      connectionStatus.value = 'Error al conectar'
      console.error(err)
    }
  }

  const sendCommand = (cmd, params = {}) => {
    if (webSocketComponent.value) {
      webSocketComponent.value.sendMessage(JSON.stringify({ 
        type: 'command', 
        value: cmd,
        ...params
      }))
    } else {
      console.warn('Componente WebSocket no inicializado')
    }
  }

  // Manejo del joystick
  const joystickThreshold = 0.15;
  const joystickCommand = ref('STOP');
  const joystickTimer = ref(null);

  const handleJoystickMove = (position) => {
    if (joystickTimer.value) {
      clearTimeout(joystickTimer.value);
    }
    
    const { x, y } = position;
    let command = 'STOP';
    let speed = 0;
    let turn = 0;
    
    const magnitude = Math.sqrt(x * x + y * y);
    
    if (magnitude < joystickThreshold) {
      command = 'STOP';
    } else {
      let angle = Math.atan2(y, x) * (180 / Math.PI);
      if (angle < 0) angle += 360;
      
      if (angle >= 315 || angle < 45) {
        command = 'RIGHT';
        turn = magnitude;
      } else if (angle >= 45 && angle < 135) {
        command = 'FORWARD';
        speed = magnitude;
      } else if (angle >= 135 && angle < 225) {
        command = 'LEFT';
        turn = magnitude;
      } else if (angle >= 225 && angle < 315) {
        command = 'BACKWARD';
        speed = magnitude;
      }
    }
    
    if (command !== joystickCommand.value || (magnitude > joystickThreshold)) {
      joystickCommand.value = command;
      
      sendCommand(command, { 
        speed: Math.round(speed * 100), 
        turn: Math.round(turn * 100)
      });
      
      if (command !== 'STOP') {
        joystickTimer.value = setTimeout(() => {
          sendCommand('STOP');
          joystickCommand.value = 'STOP';
        }, 200);
      }
    }
  }

  const checkCameraStatus = () => {
    if (!manualIp.value) {
      return;
    }

    fetch(`http://${manualIp.value}:82/`, { mode: 'no-cors' })
      .then(() => {
        if (!cameraUrl.value) {
          cameraUrl.value = `http://${manualIp.value}:82/stream`;
        }
      })
      .catch(() => {
        cameraUrl.value = '';
      });
  };

  const cameraCheckInterval = ref(null);

  onMounted(() => {
    checkBluetoothSupport()
    cameraCheckInterval.value = setInterval(checkCameraStatus, 10000);
  });

  onBeforeUnmount(() => {
    if (joystickTimer.value) clearTimeout(joystickTimer.value)
    if (cameraCheckInterval.value) clearInterval(cameraCheckInterval.value)
    if (webSocketComponent.value) webSocketComponent.value.disconnectWebSocket()
    
    // Desconectar Bluetooth si está conectado
    if (bluetoothDevice.value && bluetoothDevice.value.gatt.connected) {
      bluetoothDevice.value.gatt.disconnect()
    }
  })

</script>

<style scoped>
</style>