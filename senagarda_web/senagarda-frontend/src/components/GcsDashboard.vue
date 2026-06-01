<template>
  <div class="gcs-container">
    <div class="control-panel">
      <h3>Senagarda GCS Panel</h3>
      <div class="status-box">
        <p>Status MQTT: <span :class="mqttConnected ? 'online' : 'offline'">{{ mqttConnected ? 'CONNECTED' : 'DISCONNECTED' }}</span></p>
        <p>Live Lat: {{ liveCoords.lat.toFixed(6) }}</p>
        <p>Live Lng: {{ liveCoords.lng.toFixed(6) }}</p>
        <p>Live Alt: {{ liveCoords.alt.toFixed(2) }} m</p>
      </div>
      
      <div class="mission-box">
        <h4>Mission Planner</h4>
        <p>Jumlah Waypoint: {{ missionWaypoints.length }}</p>
        <button @click="sendMission" :disabled="missionWaypoints.length === 0" class="btn-send">
          Kirim Misi ke Robot
        </button>
        <button @click="clearMission" class="btn-clear">Reset Rute</button>
      </div>
    </div>
    
    <div id="map"></div>
  </div>
</template>

<script>
import { onMounted, ref } from 'vue';
import mqtt from 'mqtt';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';

export default {
  name: 'GcsDashboard',
  setup() {
    const mqttConnected = ref(false);
    const liveCoords = ref({ lat: 0, lng: 0, alt: 0 });
    const missionWaypoints = ref([]);
    
    let map = null;
    let roverMarker = null;
    let livePolyline = null;
    let missionPolyline = null;
    let livePathData = []; // Menyimpan histori koordinat pergerakan robot
    let mqttClient = null;

    // 1. Inisialisasi Peta Leaflet saat Komponen Dimuat
    const initMap = () => {
      // Set posisi awal peta ke koordinat kampus UAD Yogyakarta
      map = L.map('map').setView([-7.801389, 110.384444], 16);

      L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '© OpenStreetMap contributors'
      }).addTo(map);

      // Garis pelacak pergerakan nyata robot (Warna Merah)
      livePolyline = L.polyline([], { color: 'red', weight: 4 }).addTo(map);

      // Garis rute rencana misi (Warna Biru Putus-putus)
      missionPolyline = L.polyline([], { color: 'blue', weight: 3, dashArray: '5, 10' }).addTo(map);

      // Event Click: Membuat rute waypoint dengan mengklik peta
      map.on('click', (e) => {
        const { lat, lng } = e.latlng;
        // Pada aplikasi nyata, default altitude bisa ditentukan awal (misal 100m)
        const newWaypoint = { lat: parseFloat(lat.toFixed(6)), lng: parseFloat(lng.toFixed(6)), alt: 100.0 };
        
        missionWaypoints.value.push(newWaypoint);
        missionPolyline.addLatLng([lat, lng]);
        
        // Tambah marker kecil sebagai penanda titik waypoint rute
        L.circleMarker([lat, lng], { radius: 5, color: 'blue' })
         .addTo(map)
         .bindPopup(`WP ${missionWaypoints.value.length}`);
      });
    };

    // 2. Hubungkan ke Broker HiveMQ Cloud via WebSockets (Port 8884)
    const initMQTT = () => {
      const options = {
        connectTimeout: 4000,
        clientId: 'GCS_Vue_App_' + Math.random().toString(16).substr(2, 8),
        username: 'senagarda_app',
        password: 'Senagarda2',
      };

      // Wajib gunakan wss:// dan port 8884 untuk koneksi Web Browser aman
      const host = 'wss://931710ae580f46e3b0f17196c1c834f8.s1.eu.hivemq.cloud:8884/mqtt';
      
      mqttClient = mqtt.connect(host, options);

      mqttClient.on('connect', () => {
        mqttConnected.value = true;
        // Subscribe ke topik tracking koordinat dari ESP32
        mqttClient.subscribe('rover/present_location');
      });

      mqttClient.on('message', (topic, message) => {
        if (topic === 'rover/present_location') {
          try {
            const data = JSON.parse(message.toString());
            liveCoords.value = data;

            const pos = [data.lat, data.lng];

            // Update atau buat marker lokasi posisi aktual robot
            if (!roverMarker) {
              roverMarker = L.marker(pos).addTo(map).bindPopup('Posisi Robot');
            } else {
              roverMarker.setLatLng(pos);
            }

            // Secara otomatis memusatkan kamera peta berdasarkan live_location terbaru
            map.panTo(pos);

            // Gambar garis tracking pergerakan berdasarkan koordinat baru
            livePathData.push(pos);
            livePolyline.setLatLngs(livePathData);

            // Opsional: Kirim data log pergerakan ini ke backend Express untuk disimpan
            saveLogToBackend(data);

          } catch (e) {
            console.error("Gagal membaca payload JSON live_location", e);
          }
        }
      });

      mqttClient.on('close', () => { mqttConnected.value = false; });
    };

    // 3. Mengirimkan Array Semua Waypoint Misi ke Topik "mission"
    const sendMission = async () => {
      if (missionWaypoints.value.length === 0) return;

      const payload = JSON.stringify(missionWaypoints.value);
      
      // Publish rute koordinat ke topik mission dalam bentuk string array JSON
      mqttClient.publish('mission', payload, { qos: 1 });
      alert('Misi berhasil dikirim ke broker HiveMQ!');

      // Simpan rute misi ini ke database melalui backend Express
      try {
        await fetch('http://localhost:3000/api/missions', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            mission_name: 'Misi Jalur ' + new Date().toLocaleTimeString(),
            waypoints: missionWaypoints.value
          })
        });
      } catch (err) {
        console.error('Gagal menyimpan rute ke backend:', err);
      }
    };

    const clearMission = () => {
      missionWaypoints.value = [];
      missionPolyline.setLatLngs([]);
      location.reload(); // Refresh sederhana untuk membersihkan marker rute di peta
    };

    const saveLogToBackend = async (data) => {
      try {
        await fetch('http://localhost:3000/api/logs', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mission_id: 1, lat: data.lat, lng: data.lng, alt: data.alt })
        });
      } catch (err) { /* silent error */ }
    };

    onMounted(() => {
      initMap();
      initMQTT();
    });

    return { mqttConnected, liveCoords, missionWaypoints, sendMission, clearMission };
  }
};
</script>

<style scoped>
.gcs-container {
  display: flex;
  width: 100vw;
  height: 100vh;
}
.control-panel {
  width: 300px;
  background: #2c3e50;
  color: white;
  padding: 20px;
  box-shadow: 2px 0 5px rgba(0,0,0,0.3);
  z-index: 1000;
  display: flex;
  flex-direction: column;
  gap: 20px;
}
#map {
  flex-grow: 1;
  height: 100%;
}
.online { color: #2ecc71; font-weight: bold; }
.offline { color: #e74c3c; font-weight: bold; }
.btn-send {
  background: #2ecc71; color: white; border: none; padding: 10px; width: 100%; cursor: pointer; border-radius: 4px; font-weight: bold;
}
.btn-clear {
  background: #7f8c8d; color: white; border: none; padding: 5px; width: 100%; margin-top: 5px; cursor: pointer; border-radius: 4px;
}
</style>