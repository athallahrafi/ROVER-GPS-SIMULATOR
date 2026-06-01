const path = require('path');

// Used if i modify the .env file to be .server.env to avoid confusion with the .env used by the frontend
require('dotenv').config({ path: path.resolve(__dirname, '.server.env') }); 
// require('dotenv').config(); // The OG path if using the .env

const express = require('express');
const { Pool } = require('pg');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.json());

// Konfigurasi Database PostgreSQL menggunakan Environment Variables
const pool = new Pool({
    user: process.env.PGUSER,
    host: process.env.PGHOST,
    database: process.env.PGDATABASE,
    password: process.env.PGPASSWORD,
    port: 5432,
    // Tambahan konfigurasi SSL khusus untuk koneksi aman ke Cloud Neon
    ssl: process.env.PGSSLMODE === 'require' ? { rejectUnauthorized: false } : false,
});

// Endpoint untuk menyimpan misi baru dari peta
app.post('/api/missions', async (req, res) => {
    const { mission_name, waypoints } = req.body;
    try {
        const result = await pool.query(
            'INSERT INTO mission_tracks (mission_name, waypoints) VALUES ($1, $2) RETURNING *',
            [mission_name, JSON.stringify(waypoints)]
        );
        res.status(201).json(result.rows[0]);
    } catch (err) {
        console.error('Error insert mission:', err);
        res.status(500).send('Database Error');
    }
});

// Endpoint untuk menyimpan log koordinat real-time dari robot
app.post('/api/logs', async (req, res) => {
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

// Menggunakan port dari .env atau default ke 3000
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`GCS Backend berjalan dengan aman di port ${PORT}`));