const path = require('path');

// Mengarahkan dotenv ke berkas .server.env kustom
require('dotenv').config({ path: path.resolve(__dirname, '.server.env') }); 

const express = require('express');
const { Pool } = require('pg');
const cors = require('cors');
const mqtt = require('mqtt');

const app = express();
app.use(cors());
app.use(express.json());

// PERBAIKAN 1: Menghapus "new Options" menjadi Object Literal biasa
const options = {
    host: process.env.MHOST,
    port: process.env.MPORT,
    protocol: process.env.MPROTOCOL,
    username: process.env.MUSERNAME,
    password: process.env.MPASSWORD
};

// Inisialisasi MQTT client di sisi Express
const mqttClient = mqtt.connect(options);

// Konfigurasi Database PostgreSQL menggunakan Environment Variables
const pool = new Pool({
    user: process.env.PGUSER,
    host: process.env.PGHOST,
    database: process.env.PGDATABASE,
    password: process.env.PGPASSWORD,
    port: 5432,
    ssl: process.env.PGSSLMODE === 'require' ? { rejectUnauthorized: false } : false,
});

// Endpoint untuk menyimpan misi baru dari peta GCS Vue
app.post('/api/missions', async (req, res) => {
    const { mission_name, waypoints } = req.body;
    try {
        // 1. Simpan ke database Neon
        const result = await pool.query(
            'INSERT INTO mission_tracks (mission_name, waypoints) VALUES ($1, $2) RETURNING *',
            [mission_name, JSON.stringify(waypoints)]
        );
        
        const payload = JSON.stringify(waypoints);
        
        // 2. Publish ke robot melalui MQTT HiveMQ
        // Menggunakan QoS 1 sudah sangat cukup dan aman untuk menjamin data sampai
        mqttClient.publish('mission', payload, { qos: 1 }, (err) => {
            if (err) {
                console.error('Gagal publish ke MQTT:', err);
                return res.status(500).json({ success: false, message: 'Gagal mengirim misi ke MQTT' });
            }
            
            console.log(`Misi "${mission_name}" berhasil dipublish oleh Express!`);
            
            // PERBAIKAN 2: Gabungkan pengiriman data database ke dalam response sukses ini
            return res.status(201).json({ 
                success: true, 
                message: 'Misi berhasil disimpan di DB dan dipublish ke Robot!',
                data: result.rows[0]
            });
        });

    } catch (err) {
        console.error('Error insert mission:', err);
        // Pastikan tidak mengirim response jika error terjadi setelah MQTT publish (mencegah double response)
        if (!res.headersSent) {
            res.status(500).send('Database Error');
        }
    }
});

// Endpoint untuk menyimpan log koordinat real-time dari robot
app.post('/api/track', async (req, res) => {
    const { mission_id, lat, lng, alt } = req.body;
    try {
        await pool.query(
            'INSERT INTO mission_logs (mission_id, latitude, longitude, altitude) VALUES ($1, $2, $3, $4)',
            [mission_id, lat, lng, alt]
        );
        
        res.status(201).send('Log tersimpan');
    } catch (err) {
        console.error('Error insert log:', err);
        res.status(500).send('Database Error');
    }
});

// Endpoint untuk menampilkan log koordinat real-time dari robot
app.post('/api/shows', async (req, res) => {
    const { mission_id, lat, lng, alt } = req.body;
    try {
        await pool.query(
            'INSERT INTO mission_logs (mission_id, latitude, longitude, altitude) VALUES ($1, $2, $3, $4)',
            [mission_id, lat, lng, alt]
        );
        
        res.status(201).send('Log tersimpan');
    } catch (err) {
        console.error('Error insert log:', err);
        res.status(500).send('Database Error');
    }
});

mqttClient.on('message', (topic, message) => {
    if (topic === 'rover/present_location') {
        try {
        const data = JSON.parse(message.toString());
        liveCoords.value = data;

        const pos = [data.lat, data.lng];

        } catch (e) {
        console.error("Gagal membaca payload JSON live_location", e);
        }
    }
});

mqttClient.on('close', () => { mqttConnected.value = false; });

mqttClient.on('connect', () => {
    console.log('Backend Express sukses terhubung ke HiveMQ Cloud (Port 8883) 🚀');
    mqttClient.subscribe('rover/present_location');
    const PORT = process.env.PORT || 3000;
    app.listen(PORT, () => {
        console.table({
            'Express Server': `http://localhost:${PORT}`,
            'MQTT Status': mqttClient.connected ? 'Connected' : 'Disconnected',
            'Topic Subscribed': 'rover/present_location',
            'Value': ''
        });
        // console.log(`GCS Backend berjalan dengan aman di port ${PORT}, MQTT Status: ${mqttClient.connected}`);
    });
});

mqttClient.on('error', (err) => {
    console.error('MQTT Backend Error:', err);
});

// Menggunakan port dari .env atau default ke 3000
// const PORT = process.env.PORT || 3000;
// app.listen(PORT, () => console.log(`GCS Backend berjalan dengan aman di port ${PORT},${mqttClient.connected}`));