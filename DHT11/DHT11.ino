#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "LabIoTMesh"
#define MESH_PASSWORD "iotmeshpassword"
#define MESH_PORT     5555

#define LEDPIN 12      // Pin D6 (GPIO 12)

Scheduler userScheduler;
painlessMesh mesh;

// Variabel untuk menyimpan pembacaan terakhir
float suhuTerakhir = 0.0;
int adcTerakhir = 1024; // Default nilai tinggi (terang)

// Fungsi evaluasi kondisi (Rule Engine)
void evaluasiKondisi() {
  // JIKA suhu > 31.0 ATAU adc < 300 (gelap)
  if (suhuTerakhir > 31.0 || adcTerakhir < 300) {
    digitalWrite(LEDPIN, HIGH);
    Serial.println("STATUS: LED MENYALA (Suhu Panas / Kondisi Gelap)");
  } else {
    digitalWrite(LEDPIN, LOW);
    Serial.println("STATUS: LED MATI (Kondisi Normal)");
  }
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (error) return;

  String tipe = doc["tipe"];
  
  if (tipe == "suhu_node") {
    suhuTerakhir = doc["suhu"];
    Serial.printf("[TERIMA DHT] Suhu: %.2f °C | ", suhuTerakhir);
  } 
  else if (tipe == "cahaya_node") {
    adcTerakhir = doc["adc"];
    Serial.printf("[TERIMA LDR] Cahaya (ADC): %d | ", adcTerakhir);
  }
  
  // Panggil Rule Engine tiap kali ada data baru masuk
  evaluasiKondisi();
}

void changedConnectionCallback() {
  Serial.println("--> [INFO] Topologi Jaringan Berubah (Node Masuk/Keluar)");
}

void setup() {
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW); // Pastikan LED mati di awal
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  mesh.onReceive(&receivedCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // taskSendMessage DINONAKTIFKAN pada Node 3
}

void loop() {
  mesh.update();
}