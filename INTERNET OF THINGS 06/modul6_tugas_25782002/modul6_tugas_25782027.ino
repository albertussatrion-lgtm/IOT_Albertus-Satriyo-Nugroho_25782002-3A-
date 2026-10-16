#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ==========================================
// KREDENSIAL & KONFIGURASI JARINGAN
// ==========================================
const char* ssid = "Infinix GT 30 Pro";
const char* password = "murahcpz";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

// IDENTITAS KLIEN (Wajib Unik, sesuaikan dengan NIM Anda)
const char* clientId = "ESP8266_LabIoT_NIM027_Tugas";

// ==========================================
// DEFINISI TOPIK MQTT (Ganti nim_anda)
// ==========================================
const char* topicTelemetry = "kampus/labiot/25782027/telemetri";
const char* topicControl   = "kampus/labiot/25782027/kendali";
const char* topicAlert     = "kampus/labiot/25782027/alert";
const char* topicStatus    = "kampus/labiot/25782027/status";

// ==========================================
// KONFIGURASI PIN PERANGKAT KERAS
// ==========================================
const byte dhtPin = 2;   // Pin D4 (GPIO 2) untuk Sensor DHT
const byte ledPin = 5;   // Pin D1 (GPIO 5) untuk LED Indikator
#define DHTTYPE DHT22    // Ubah ke DHT22 jika menggunakan modul DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Pengatur Waktu (Non-blocking timer)
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 4000; // Kirim telemetri setiap 4 detik

// Variabel Latch untuk mencegah spamming pesan alert beruntun
bool lastOverheatState = false;

// ==========================================
// FUNGSI CALLBACK (Penerima Perintah Masuk)
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  char incomingBuffer[length + 1];
  memcpy(incomingBuffer, payload, length);
  incomingBuffer[length] = '\0';

  Serial.println("----------------------------------------");
  Serial.printf("[KONTROL DITERIMA] Topik: %s | Payload: %s\n", topic, incomingBuffer);

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, incomingBuffer);
  if (err) {
    Serial.println("[ERROR] Gagal membedah payload JSON!");
    return;
  }

  // Kendali Manual Override LED via Topik Kendali
  if (doc.containsKey("led")) {
    int val = doc["led"];
    digitalWrite(ledPin, val == 1 ? HIGH : LOW);
    Serial.printf("--> [MANUAL OVERRIDE] Status LED diubah: %s\n", val == 1 ? "MENYALA" : "PADAM");
  }
  Serial.println("----------------------------------------");
}

// ==========================================
// FUNGSI RECONNECT & KONFIGURASI LWT
// ==========================================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan kembali ke Broker MQTT...");

    // Parameter Last Will and Testament (LWT)
    const char* willTopic = topicStatus;
    int willQos = 1;
    bool willRetain = true;
    const char* willMessage = "{\"status\":\"OFFLINE\",\"keterangan\":\"Perangkat Terputus Tak Terduga\"}";

    // Hubungkan ke broker dengan menyertakan parameter LWT
    if (client.connect(clientId, willTopic, willQos, willRetain, willMessage)) {
      Serial.println(" Sukses Terhubung!");

      // 1. Daftarkan kembali subskripsi topik kendali manual
      client.subscribe(topicControl);
      Serial.printf("Subskripsi Topik Kendali: %s\n", topicControl);

      // 2. Publikasikan status ONLINE sesaat setelah berhasil terhubung (dengan Retain = true)
      StaticJsonDocument<256> statusDoc;
      statusDoc["status"] = "ONLINE";
      statusDoc["keterangan"] = "Perangkat Beroperasi Normal";
      
      char statusBuffer[256];
      serializeJson(statusDoc, statusBuffer);
      client.publish(topicStatus, statusBuffer, true);
      Serial.println("[STATUS] Pesan ONLINE dipublikasikan ke broker.");

    } else {
      Serial.printf("Gagal terhubung (rc=%d). Coba lagi dalam 4 detik...\n", client.state());
      delay(4000);
    }
  }
}

// ==========================================
// SETUP UTAMA
// ==========================================
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Kondisi awal LED padam

  dht.begin();

  // Koneksi Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terkoneksi!");
  Serial.print("Alamat IP Lokal: ");
  Serial.println(WiFi.localIP());

  // Konfigurasi MQTT Server & Fungsi Callback
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

// ==========================================
// LOOP UTAMA (Non-Blocking)
// ==========================================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Memelihara sesi TCP, keep-alive ping, dan menangkap pesan masuk

  unsigned long now = millis();
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // Validasi pembacaan sensor DHT
    if (isnan(t) || isnan(h)) {
      Serial.println("[PERINGATAN] Gagal membaca data dari sensor DHT!");
      return;
    }

    // 1. Klasifikasi Kondisi Suhu
    String kondisiSuhu = (t > 31.0) ? "PANAS" : "NORMAL";

    // 2. Publikasi Paket Telemetri Rutin (Format JSON)
    StaticJsonDocument<256> docOut;
    docOut["node_id"] = clientId;
    docOut["suhu"] = serialized(String(t, 2));
    docOut["kelembapan"] = serialized(String(h, 2));
    docOut["status_led"] = digitalRead(ledPin);
    docOut["kondisi_suhu"] = kondisiSuhu; // Parameter tambahan tugas

    char buffer[256];
    serializeJson(docOut, buffer);
    
    bool telemetrySuccess = client.publish(topicTelemetry, buffer);
    if (telemetrySuccess) {
      Serial.printf("[TELEMETRI TERKIRIM] %s\n", buffer);
    } else {
      Serial.println("[ERROR] Gagal memublikasikan paket telemetri!");
    }

    // 3. Edge Safety Engine & Dedicated Alert Topic (> 31.0°C)
    bool currentOverheat = (t > 31.0);

    if (currentOverheat) {
      // Otomatis nyalakan indikator LED darurat secara lokal
      digitalWrite(ledPin, HIGH);

      // Gunakan state latch agar pesan alert dikirimkan secara teratur saat pertama kali bahaya terdeteksi (mencegah spamming)
      if (!lastOverheatState) {
        StaticJsonDocument<256> alertDoc;
        alertDoc["status"] = "BAHAYA";
        alertDoc["parameter"] = "OVERHEAT";
        alertDoc["nilai_suhu"] = serialized(String(t, 2));

        char alertBuffer[256];
        serializeJson(alertDoc, alertBuffer);

        bool alertSuccess = client.publish(topicAlert, alertBuffer);
        if (alertSuccess) {
          Serial.printf("[ALERT BAHAYA TERKIRIM] Topik: %s | Payload: %s\n", topicAlert, alertBuffer);
        }
      }
    } else {
      // Jika suhu kembali aman dan sebelumnya dalam keadaan overheat
      if (lastOverheatState) {
        Serial.println("[INFO] Suhu lingkungan kembali turun ke batas aman (NORMAL).");
      }
    }

    // Perbarui status state latch sebelumnya
    lastOverheatState = currentOverheat;
  }
}