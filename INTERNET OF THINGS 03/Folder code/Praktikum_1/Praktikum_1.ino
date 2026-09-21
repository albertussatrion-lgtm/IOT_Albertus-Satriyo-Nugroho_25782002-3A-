#include <ESP8266WiFi.h>  
#include <ESP8266HTTPClient.h>  
#include <WiFiClient.h>

const char* ssid = "dpinaasns";  
const char* password = "1234567891011";

const char* serverName = "http://172.20.5.3/relay/on";   
const byte ldrPin = A0;

void setup() {  
  Serial.begin(115200);  
  WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }  
  Serial.println("\nClient Terhubung ke Wi-Fi!");  
}

void loop() {  
  int ldrValue = analogRead(ldrPin);  
    
  if ((WiFi.status() == WL_CONNECTED) && (ldrValue < 300)) {  
    WiFiClient client;  
    HTTPClient http;  
      
    http.begin(client, serverName);  
    int httpResponseCode = http.GET();  
      
    Serial.print("HTTP Response code: ");  
    Serial.println(httpResponseCode); 
      
    http.end();  
    delay(10000); 
  }  
  delay(2000);  
}