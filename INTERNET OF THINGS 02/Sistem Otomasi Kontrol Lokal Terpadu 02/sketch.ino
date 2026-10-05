#include "DHT.h"

// Pemetaan Pin ESP32
#define PIN_LDR 35     // Analog Out (AO) LDR ke GPIO 34
#define PIN_DHT 13     // Data DHT22 ke GPIO 13
#define PIN_RELAY 12   // Input Relay ke GPIO 12
#define PIN_LED 5      // LED Indikator ke GPIO 5

#define DHTTYPE DHT22

DHT dht(PIN_DHT, DHTTYPE);

// Konfigurasi Active LOW Modul Relay
#define RELAY_ON LOW
#define RELAY_OFF HIGH

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  // Matikan relay dan LED saat awal menyala
  digitalWrite(PIN_RELAY, RELAY_OFF);
  digitalWrite(PIN_LED, LOW);

  dht.begin();
  Serial.println("System Initialized: Smart Warehouse Rule Engine Active!");
}

void loop() {
  float suhu = dht.readTemperature();
  int nilaiLDR = analogRead(PIN_LDR);

  if (isnan(suhu)) return;

  // Tentukan status yang SEHARUSNYA (Target State)
  bool butuhAktuator = (suhu > 34.0 || nilaiLDR < 1200);
  int targetRelay = butuhAktuator ? RELAY_ON : RELAY_OFF;

  // Perintahkan Pin
  digitalWrite(PIN_RELAY, targetRelay);
  digitalWrite(PIN_LED, butuhAktuator ? HIGH : LOW);

  // --- CEK DETEKSI KEGAGALAN (Hardware Read-Back) ---
  int statusFisikPin = digitalRead(PIN_RELAY);

  if (statusFisikPin != targetRelay) {
    Serial.println("ALERT ERROR: Aktuator GAGAL Merespons! (Pin Mismatch)");
  } else if (butuhAktuator) {
    Serial.println("Status: Peringatan: Aktuator Aktif Berhasil!");
  } else {
    Serial.println("Status: Kondisi Aman");
  }

  delay(2000);
}