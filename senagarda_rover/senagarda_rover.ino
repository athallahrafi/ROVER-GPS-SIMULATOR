#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "BSI-UAD";

// Global Variabel untuk menyimpan state dan data antar-loop
String dynamicURL = "";
bool statusInternetTerakhir = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid);
  
  Serial.println("\n=======================================");
  Serial.print("[1/2] Menghubungkan ke Radio Wi-Fi: ");
  Serial.println(ssid);
}

void loop() {
  // 1. Pastikan koneksi fisik Wi-Fi aman
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    statusInternetTerakhir = false;
    dynamicURL = ""; // Reset URL jika koneksi putus
    delay(1000);
    return;
  }

  // 2. Cek Akses Internet Nyata (Ping Test)
  bool statusInternetSekarang = cekAksesInternet();

  // JIKA TERJEBAK DI LOGIN PAGE (Internet mati, tapi fisik konek)
  if (!statusInternetSekarang) {
    statusInternetTerakhir = false;
    
    // STATE A: Jika belum punya URL dinamis, ambil dulu token segarnya
    if (dynamicURL == "") {
      Serial.println("\n[2/2] >>> STATUS: TERTINGGAL DI LOGIN PAGE! <<<");
      ambilURLDinamis();
      delay(2000); // Beri jeda waktu yang aman bagi FreeRTOS untuk membersihkan queue socket
    } 
    // STATE B: Jika URL dinamis sudah siap di memori, eksekusi nembak payload
    else {
      Serial.println("[PROSES] Menembak payload menggunakan URL Dinamis Baru...");
      tembakPayloadLogin();
      delay(4000); // Tunggu proses autentikasi sistem kampus
    }
  } 
  // JIKA BYPASS SUKSES & INTERNET AKTIF
  else {
    if (statusInternetTerakhir == false) {
      Serial.println("\n>>> STATUS: BYPASS SUKSES! INTERNET AKTIF <<<");
      Serial.print("[INFO] IP Address ESP32: ");
      Serial.println(WiFi.localIP());
      statusInternetTerakhir = true;
    }
    delay(5000); // Jika sudah internetan, cek ulang berkala setiap 5 detik saja
  }
}

// FUNGSI PING INTERNET
bool cekAksesInternet() {
  HTTPClient http;
  http.begin("http://connectivitycheck.gstatic.com/generate_204");
  http.setTimeout(1500);
  int httpCode = http.GET();
  http.end();
  return (httpCode == 204);
}

// FUNGSI AMBIL URL DAN TOKEN (FASE 1)
void ambilURLDinamis() {
  HTTPClient http;
  Serial.println("[PROSES] Mengambil token baru dari gateway...");
  http.begin("http://2.2.2.2/");
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  
  const char* headerKeys[] = {"Location"};
  http.collectHeaders(headerKeys, 1);
  
  int initialCode = http.GET();
  if (initialCode == 302 && http.hasHeader("Location")) {
    dynamicURL = http.header("Location");
    Serial.println("[SUKSES] Mendapatkan URL Dinamis Baru.");
  } else {
    Serial.println("[GAGAL] Gateway tidak merespon 302. Mencoba lagi loop depan.");
  }
  http.end();
}

// FUNGSI TEMBAK PAYLOAD LOGIN (FASE 2)
void tembakPayloadLogin() {
  HTTPClient http;
  http.begin(dynamicURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS); // Matikan follow redirect demi hemat RAM
  
  String httpRequestData = "RequestType=Login"
                           "&UE-Username=2315022033"
                           "&UE-Password=uputuput"
                           "&button=Sign+In";
  
  int httpResponseCode = http.POST(httpRequestData);
  
  if (httpResponseCode > 0) {
    Serial.print("[RESPON SERVER KAMPUS]: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("[ERROR HTTP]: ");
    Serial.println(http.errorToString(httpResponseCode).c_str());
  }
  
  http.end();
  dynamicURL = ""; // Reset kembali setelah digunakan agar loop depan mengambil token baru jika gagal
}