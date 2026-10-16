#include <ESP8266WiFi.h>          // Gunakan <WiFi.h> jika memakai ESP32
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

// --- Config Wi-Fi ---
const char* ssid     = "SSID_WIFI_KAMU";
const char* password = "PASSWORD_WIFI_KAMU";

// --- Config DHT ---
#define DHTPIN 2        // Pin D4 pada ESP8266
#define DHTTYPE DHT11   // DHT11 atau DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- Variabel Telemetri & State ---
String currentTemp = "--";
String currentHum  = "--";
String ledState    = "OFF";

unsigned long previousMillis = 0;
const long interval = 2000; // Timer non-blocking 2 detik

// --- Objek WebServer & WebSocket ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ============================================================================
// FRONT-END: HTML, CSS, & JavaScript (Disimpan di PROGMEM)
// ============================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Dashboard Telemetri ESP</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f9; margin: 0; padding: 20px; }
    h1 { color: #333; }
    .card { background: white; padding: 20px; margin: 15px auto; max-width: 300px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    .card h2 { margin: 0; color: #007bff; }
    .value { font-weight: bold; }
  </style>
</head>
<body>
  <h1>Dashboard Telemetri IoT</h1>

  <!-- Kartu Telemetri Suhu -->
  <div class="card">
    <h2>Suhu: <span id="tempValue" class="value">--</span> &deg;C</h2>
  </div>

  <!-- Kartu Telemetri Kelembapan -->
  <div class="card">
    <h2>Kelembapan: <span id="humValue" class="value">--</span> %</h2>
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    window.addEventListener('load', initWebSocket);

    function initWebSocket() {
      console.log('Mencoba membuka koneksi WebSocket...');
      websocket = new WebSocket(gateway);
      websocket.onopen    = onOpen;
      websocket.onclose   = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) {
      console.log('Koneksi WebSocket Terbuka');
    }

    function onClose(event) {
      console.log('Koneksi WebSocket Terputus, mencoba lagi...');
      setTimeout(initWebSocket, 2000);
    }

    // Callback saat menerima data JSON dari ESP (Back-End)
    function onMessage(event) {
      console.log("Data diterima: " + event.data);
      var data = JSON.parse(event.data);

      if (data.suhu !== undefined) {
        document.getElementById('tempValue').innerText = data.suhu;
      }
      if (data.hum !== undefined) {
        document.getElementById('humValue').innerText = data.hum;
      }
    }
  </script>
</body>
</html>
)rawliteral";

// ============================================================================
// BACK-END: Fungsi C++
// ============================================================================

// Fungsi untuk mengirim data JSON ke WebSocket Client
void notifyClients() {
  StaticJsonDocument<200> doc;
  doc["led"]  = ledState;
  doc["suhu"] = currentTemp;
  doc["hum"]  = currentHum;

  String jsonString;
  serializeJson(doc, jsonString);
  ws.textAll(jsonString);
}

// Handler event WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%uint terhubung dari %s\n", client->id(), client->remoteIP().toString().c_str());
    notifyClients(); // Kirim data terkini saat client baru terhubung
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%uint terputus\n", client->id());
  }
}

// Task pembacaan sensor non-blocking
void readSensorTask() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      currentHum  = String(h, 1);
      currentTemp = String(t, 1);
      notifyClients(); // Kirim pembaruan ke Web
    } else {
      Serial.println("Gagal membaca dari sensor DHT!");
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Koneksi ke Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address ESP: ");
  Serial.println(WiFi.localIP());

  // Setup WebSocket & WebServer
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Serve halaman web dari string PROGMEM index_html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}

void loop() {
  readSensorTask();
  ws.cleanupClients(); // Membersihkan client WebSocket yang menggantung
}