#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// 1. Konfigurasi Wi-Fi
const char* ssid = "KOST ASSALAM";
const char* password = "hitamputih";

// 2. Konfigurasi Pin Hardware
#define DHTPIN D2     // Pin Data DHT11 3 Kaki
#define DHTTYPE DHT11
#define LED_PIN D1    // Pin LED Eksternal

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// 3. Variabel Penampung Data
String currentHum = "--";
String currentTemp = "--";
String ledState = "OFF";

unsigned long previousMillis = 0;
const long interval = 2000; // Baca sensor tiap 2 detik

// 4. Halaman HTML + CSS + JS (Disimpan dalam PROGMEM / Raw String)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Telemetri Suhu & Kelembapan</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f9; padding: 20px; }
    .card { background: white; padding: 20px; margin: 15px auto; width: 280px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    button { padding: 10px 20px; font-size: 16px; border: none; background: #007bff; color: white; border-radius: 5px; cursor: pointer; }
    button:hover { background: #0056b3; }
  </style>
</head>
<body>

  <h1>Telemetri WebSockets</h1>
  <p>Status Koneksi: <b id="status">Menghubungkan...</b></p>

  <div class="card">
    <h2>Suhu: <span id="tempValue">--</span> &deg;C</h2>
  </div>

  <div class="card">
    <h2>Kelembapan: <span id="humValue">--</span> %</h2>
  </div>

  <div class="card">
    <h2>LED: <span id="state">OFF</span></h2>
    <button onclick="toggleLED()"><span id="btnText">Nyalakan</span></button>
  </div>

  <script>
    // Koneksi WebSocket otomatis menggunakan IP tempat web di-host
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    function initWebSocket() {
      websocket = new WebSocket(gateway);
      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) {
      document.getElementById('status').innerText = "Terhubung";
    }

    function onClose(event) {
      document.getElementById('status').innerText = "Terputus";
      setTimeout(initWebSocket, 2000);
    }

    function onMessage(event) {
      let data = JSON.parse(event.data);

      if (data.suhu !== undefined) {
        document.getElementById('tempValue').innerText = data.suhu;
      }
      if (data.hum !== undefined) {
        document.getElementById('humValue').innerText = data.hum;
      }
      if (data.led !== undefined) {
        document.getElementById('state').innerText = data.led;
        document.getElementById('btnText').innerText = (data.led === "ON") ? "Matikan" : "Nyalakan";
      }
    }

    function toggleLED() {
      websocket.send('toggle');
    }

    window.addEventListener('load', initWebSocket);
  </script>
</body>
</html>
)rawliteral";

// 5. Fungsi Kirim JSON ke Browser
void notifyClients() {
  String jsonPayload = "{\"led\":\"" + ledState + "\",\"suhu\":\"" + currentTemp + "\",\"hum\":\"" + currentHum + "\"}";
  ws.textAll(jsonPayload);
}

// 6. Handler Event WebSocket
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    notifyClients();
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      String message = (char*)data;
      if (message == "toggle") {
        ledState = (ledState == "OFF") ? "ON" : "OFF";
        digitalWrite(LED_PIN, (ledState == "ON") ? HIGH : LOW);
        notifyClients();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  // Koneksi Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Melayani tampilan HTML saat IP NodeMCU diakses dari browser
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}

void loop() {
  ws.cleanupClients();

  // Timing non-blocking menggunakan millis()
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      currentHum = String(h, 1);
      currentTemp = String(t, 1);
      notifyClients();
    }
  }
}

