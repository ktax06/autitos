<template>
  <div 
    ref="joystickContainer" 
    class="joystick-container w-40 h-40 bg-gray-800 rounded-full relative border-4 border-gray-700 shadow-lg"
    @mousedown="startDrag"
    @touchstart="startDrag"
  >
    <div 
      ref="joystickHandle" 
      class="joystick-handle w-16 h-16 bg-indigo-500 rounded-full absolute top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 cursor-grab shadow-md"
      :style="{ 
        transform: `translate(${position.x}px, ${position.y}px)` 
      }"
    ></div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onUnmounted } from 'vue';

const emit = defineEmits(['move']);

const joystickContainer = ref(null);
const joystickHandle = ref(null);
const isDragging = ref(false);
const position = reactive({ x: 0, y: 0 });
const containerRect = reactive({ width: 0, height: 0, centerX: 0, centerY: 0 });
const maxDistance = ref(0);

// Coordenadas normalizadas entre -1 y 1
const normalizedPosition = reactive({ x: 0, y: 0 });

onMounted(() => {
  updateContainerDimensions();
  window.addEventListener('resize', updateContainerDimensions);
  document.addEventListener('mousemove', handleDrag);
  document.addEventListener('touchmove', handleDrag, { passive: false });
  document.addEventListener('mouseup', stopDrag);
  document.addEventListener('touchend', stopDrag);
});

onUnmounted(() => {
  window.removeEventListener('resize', updateContainerDimensions);
  document.removeEventListener('mousemove', handleDrag);
  document.removeEventListener('touchmove', handleDrag);
  document.removeEventListener('mouseup', stopDrag);
  document.removeEventListener('touchend', stopDrag);
});

const updateContainerDimensions = () => {
  if (!joystickContainer.value) return;
  
  const rect = joystickContainer.value.getBoundingClientRect();
  containerRect.width = rect.width;
  containerRect.height = rect.height;
  containerRect.centerX = rect.width / 2;
  containerRect.centerY = rect.height / 2;
  
  // El radio máximo es la mitad del ancho menos el radio del joystick
  maxDistance.value = (Math.min(containerRect.width, containerRect.height) / 2) - 
                     (joystickHandle.value.offsetWidth / 2);
};

const startDrag = (event) => {
  event.preventDefault();
  isDragging.value = true;
  joystickHandle.value.style.cursor = 'grabbing';
  
  // Ya que acabamos de empezar, procesamos un evento de arrastre
  // para posicionar inmediatamente el joystick
  handleDrag(event);
};

const handleDrag = (event) => {
  if (!isDragging.value) return;
  event.preventDefault();
  
  const clientX = event.touches ? event.touches[0].clientX : event.clientX;
  const clientY = event.touches ? event.touches[0].clientY : event.clientY;
  
  const rect = joystickContainer.value.getBoundingClientRect();
  
  // Posición relativa al centro del joystick
  let deltaX = clientX - (rect.left + containerRect.centerX);
  let deltaY = clientY - (rect.top + containerRect.centerY);
  
  // Distancia desde el centro
  const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
  
  // Si estamos más allá del límite, normalizar
  if (distance > maxDistance.value) {
    const ratio = maxDistance.value / distance;
    deltaX *= ratio;
    deltaY *= ratio;
  }
  
  // Actualizar posición
  position.x = deltaX;
  position.y = deltaY;
  
  // Calcular posiciones normalizadas (-1 a 1)
  normalizedPosition.x = parseFloat((deltaX / maxDistance.value).toFixed(2));
  normalizedPosition.y = parseFloat((deltaY / maxDistance.value).toFixed(2));
  
  // Emitir evento con posiciones normalizadas
  emit('move', {
    x: normalizedPosition.x,
    y: normalizedPosition.y,
    angle: Math.atan2(normalizedPosition.y, normalizedPosition.x)
  });
};

const stopDrag = () => {
  if (!isDragging.value) return;
  
  isDragging.value = false;
  joystickHandle.value.style.cursor = 'grab';
  
  // Regresar al centro
  position.x = 0;
  position.y = 0;
  normalizedPosition.x = 0;
  normalizedPosition.y = 0;
  
  // Emitir posición centrada
  emit('move', { x: 0, y: 0, angle: 0 });
};
</script>

<style scoped>
.joystick-container {
  touch-action: none;
  user-select: none;
}

.joystick-handle {
  transition: transform 0.05s ease;
}
</style>