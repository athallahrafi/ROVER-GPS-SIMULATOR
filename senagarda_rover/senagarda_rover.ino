#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <WiFiClientSecure.h>

// Update ini sesuai dengan jaringan wifi harianmu
const char* ssid = "Redmi Note 11";
const char* password = "12345678";
const char* mqtt_server = "931710ae580f46e3b0f17196c1c834f8.s1.eu.hivemq.cloud";

WiFiClientSecure espClient;
PubSubClient * client; 

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];

// Interval pengiriman data tracking (1000 ms = 1 detik sekali)
// Kamu bisa sesuaikan, untuk robot bergerak cepat biasanya 500ms - 1000ms adalah ideal
const unsigned long intervalTracking = 1000; 

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("\nWiFi connected");
}

void setDateTime() {
  configTime(25200, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Tempat menerima instruksi balik dari Base Station ke Robot jika ada
}

void reconnect() {
  while (!client->connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "Senagarda-Rover-" + String(random(0, 0xffff), HEX);
    
    if (client->connect(clientId.c_str(), "rover_user", "Senagarda1")) {
      Serial.println("connected");
      // Subscribe ke topik perintah jika robot butuh dikendalikan jarak jauh lewat MQTT
      client->subscribe("rover/command"); 
    } else {
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  delay(500);
  Serial.begin(115200); 
  delay(500);

  setup_wifi();
  setDateTime();

  espClient.setInsecure(); 

  client = new PubSubClient(espClient);
  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
}

void loop() {
  if (!client->connected()) {
    reconnect();
  }
  client->loop();

  unsigned long now = millis();
  
  // Masuk ke fase pengiriman kontinu untuk tracking setiap interval tercapai
  if (now - lastMsg > intervalTracking) {
    lastMsg = now;

    // ------------------------------------------------======================
    // TIP ROBOTIKA: Ganti nilai statis ini dengan pembacaan sensor GPS aslimu
    // Misalnya: latitude = gps.location.lat();
    // ------------------------------------------------======================
    float latitude  = -7.801389;  
    float longitude = 110.384444;
    float altitude  = 114.50;
    
    // Membungkus data koordinat ke format JSON string
    snprintf(msg, MSG_BUFFER_SIZE, 
            "{\"lat\":%.6f,\"lng\":%.6f,\"alt\":%.2f}", 
            latitude, longitude, altitude);
            
    // Cetak ke Serial Monitor untuk memantau pergerakan data sebelum dikirim
    Serial.print("Live Tracking -> ");
    Serial.println(msg);
    
    // Kirim secara real-time ke topik khusus lokasi robot
    client->publish("rover/present_location", msg);
  }
}