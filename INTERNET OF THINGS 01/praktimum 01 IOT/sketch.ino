// Definisi Pin ESP32
const int buttonPin = 4; // Pin D4 terhubung ke Push Button (Pull-Down eksternal)
const int ledPin = 5;    // Pin D5 terhubung ke Anoda LED

// Variabel pelacakan status (state tracking)
int buttonState = 0;         // Pembacaan status tombol saat ini
int lastButtonState = LOW;   // Menyimpan status tombol pada iterasi sebelumnya
bool ledState = false;       // Melacak kondisi lampu (false = MATI, true = MENYALA)

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // Kriteria 1: Saat sistem pertama kali dinyalakan, LED dalam keadaan mati
  digitalWrite(ledPin, LOW);
  Serial.println("Sistem ESP32 Siap! Status Awal: LED MATI");
}

void loop() {
  // 1. Baca kondisi logika pin tombol saat ini
  buttonState = digitalRead(buttonPin);

  // 2. Deteksi transisi tepi naik (Edge Detection / State Tracking: LOW -> HIGH)
  // Memastikan sakelar toggle hanya terpicu satu kali saat tombol ditekan singkat,
  // bukan berulang-ulang saat tombol ditahan.
  if (buttonState == HIGH && lastButtonState == LOW) {

    // Logika State Tracking: Membalik status lampu (Toggle Latching)
    ledState = !ledState;

    // Terapkan status baru ke pin fisik LED
    if (ledState == true) {
      digitalWrite(ledPin, HIGH);
      Serial.println("Tombol ditekan 1x -> LED MENYALA (Terkunci ON)");
    } else {
      digitalWrite(ledPin, LOW);
      Serial.println("Tombol ditekan 1x -> LED MATI (Terkunci OFF)");
    }

    // 3. Logika Debounce: Mengatasi pantulan mekanis tombol fisik
    // Jeda 200 ms untuk mencegah fenomena flickering (kedip acak)
    delay(200);
  }

  // 4. Perbarui status tombol sebelumnya untuk siklus loop berikutnya
  lastButtonState = buttonState;
}
