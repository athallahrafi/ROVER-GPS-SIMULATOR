# 🤖 RobotNav — Flutter Robot Path Planning & Monitoring App

Aplikasi Android berbasis Flutter untuk membuat *path planning* robot, memantau posisi robot secara *real-time*, dan menampilkan status sistem robot melalui koneksi MQTT.

---

## 📱 Fitur Utama

- **Path Planning** — Buat dan atur jalur pergerakan robot langsung dari peta interaktif
- **Live Tracking** — Tampilkan posisi robot dan posisi smartphone secara real-time di peta
- **Progress Monitoring** — Pantau persentase penyelesaian path planning oleh robot
- **Status Sistem** — Lihat status komponen robot (baterai, koneksi, sensor, dll.)
- **MQTT Integration** — Terkoneksi ke server MQTT untuk menerima data posisi robot

---

## 🖼️ Screenshot

> *(Tambahkan screenshot aplikasi di sini)*

```
screenshots/
├── path_planning.png
├── live_tracking.png
└── robot_status.png
```

---

## 🏗️ Arsitektur Sistem

```
┌─────────────────┐        MQTT         ┌──────────────┐
│  Flutter App    │ ◄────────────────── │  MQTT Broker │
│  (Android)      │                     │  (Server)    │
│                 │                     └──────┬───────┘
│  - Maps View    │                            │
│  - Path Editor  │                     ┌──────▼───────┐
│  - Status Panel │                     │    Robot     │
└─────────────────┘                     │  (Publisher) │
                                        └──────────────┘
```

**Alur Data:**
1. Robot mengirim string posisi ke MQTT Broker
2. Aplikasi Flutter subscribe ke topic MQTT
3. Posisi di-parse dan ditampilkan di peta
4. Aplikasi mengirim path planning ke robot via MQTT

---

## 🚀 Cara Menjalankan

### Prasyarat

- [Flutter SDK](https://docs.flutter.dev/get-started/install) versi `>=3.0.0`
- Android Studio / VS Code
- Android device atau emulator (API Level 21+)
- Akses ke MQTT Broker (contoh: Mosquitto, HiveMQ)

### Instalasi

```bash
# 1. Clone repositori
git clone https://github.com/username/robotnav-flutter.git
cd robotnav-flutter

# 2. Install dependencies
flutter pub get

# 3. Salin file konfigurasi
cp .env.example .env

# 4. Isi konfigurasi MQTT di file .env (lihat bagian Konfigurasi)

# 5. Jalankan aplikasi
flutter run
```

---

## ⚙️ Konfigurasi

Buat file `.env` di root proyek berdasarkan `.env.example`:

```env
# MQTT Broker
MQTT_BROKER_HOST=192.168.1.100
MQTT_BROKER_PORT=1883
MQTT_CLIENT_ID=robot_nav_app

# MQTT Topics
MQTT_TOPIC_POSITION=robot/position
MQTT_TOPIC_STATUS=robot/status
MQTT_TOPIC_PATH=robot/path_planning

# Maps
MAPS_API_KEY=your_google_maps_api_key
```

### Format Data MQTT

Aplikasi mengharapkan data posisi robot dalam format berikut:

```json
{
  "x": 12.345678,
  "y": 98.765432,
  "heading": 90.0,
  "progress": 75,
  "timestamp": 1716000000
}
```

| Field | Tipe | Keterangan |
|---|---|---|
| `x` | float | Koordinat latitude robot |
| `y` | float | Koordinat longitude robot |
| `heading` | float | Arah hadap robot (derajat) |
| `progress` | int | Persentase path yang sudah diikuti (0–100) |
| `timestamp` | int | Unix timestamp pengiriman data |

---

## 📦 Dependencies

```yaml
dependencies:
  flutter_mqtt_client: ^1.0.0     # Koneksi MQTT
  google_maps_flutter: ^2.5.0     # Tampilan peta
  geolocator: ^10.0.0             # GPS smartphone
  provider: ^6.1.0                # State management
  flutter_dotenv: ^5.1.0          # Konfigurasi .env
```

---

## 📁 Struktur Proyek

```
lib/
├── main.dart
├── config/
│   └── app_config.dart           # Konfigurasi MQTT & Maps
├── models/
│   ├── robot_position.dart       # Model data posisi robot
│   ├── path_plan.dart            # Model path planning
│   └── robot_status.dart         # Model status sistem robot
├── services/
│   ├── mqtt_service.dart         # Koneksi & subscribe MQTT
│   └── location_service.dart     # GPS smartphone
├── screens/
│   ├── map_screen.dart           # Layar peta utama
│   ├── path_planning_screen.dart # Editor path planning
│   └── status_screen.dart        # Status sistem robot
├── widgets/
│   ├── robot_marker.dart         # Marker robot di peta
│   ├── path_overlay.dart         # Overlay path di peta
│   └── status_card.dart          # Kartu status sistem
└── providers/
    ├── robot_provider.dart       # State robot
    └── path_provider.dart        # State path planning
```

---

## 🔌 Menghubungkan ke Robot

1. Pastikan MQTT Broker aktif dan dapat diakses dari jaringan yang sama
2. Atur `MQTT_BROKER_HOST` di file `.env` sesuai IP broker
3. Robot harus mempublish ke topic `robot/position` dengan format JSON di atas
4. Aplikasi akan otomatis subscribe saat pertama kali dibuka

---

## 🧪 Testing

```bash
# Unit test
flutter test

# Integration test
flutter test integration_test/
```

---

## 🐛 Troubleshooting

| Masalah | Solusi |
|---|---|
| Tidak bisa connect ke MQTT | Pastikan IP broker benar dan port 1883 tidak diblokir firewall |
| Peta tidak muncul | Periksa `MAPS_API_KEY` di `.env` sudah valid |
| Posisi robot tidak update | Periksa format JSON dari robot sudah sesuai |
| GPS tidak akurat | Aktifkan GPS dan izinkan akses lokasi di pengaturan Android |

---

## 🤝 Kontribusi

1. Fork repositori ini
2. Buat branch baru: `git checkout -b fitur/nama-fitur`
3. Commit perubahan: `git commit -m 'feat: tambah fitur X'`
4. Push ke branch: `git push origin fitur/nama-fitur`
5. Buat Pull Request

---

## 📄 Lisensi

Proyek ini menggunakan lisensi [MIT](LICENSE).

---

## 👤 Author

**Nama Kamu**
- GitHub: [@username](https://github.com/username)
- Email: email@example.com