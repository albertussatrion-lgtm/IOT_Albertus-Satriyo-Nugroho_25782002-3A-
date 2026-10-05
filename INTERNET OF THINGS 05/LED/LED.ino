#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "LabIoTMesh"
#define MESH_PASSWORD "iotmeshpassword"
#define MESH_PORT     5555

#define LDRPIN A0      // Pin Analog

Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

void sendMessage() {
  int nilaiADC = analogRead(LDRPIN);

  // Format JSON sesuai permintaan tugas
  StaticJsonDocument<200> doc;
  doc["tipe"] = "cahaya_node";
  doc["adc"] = nilaiADC;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  
  Serial.print("[Node 2 KIRIM] ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}