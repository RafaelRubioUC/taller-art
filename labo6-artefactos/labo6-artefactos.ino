#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "iPhone de Isolina";
const char* WIFI_PASSWORD = "rafa123456";


// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "172.20.10.3";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

// (Estudiante)
// Definir los pines y objetos necesarios para los sensores.
#define LDR_PIN 32
#define DHT_PIN 33
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);



WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {
  float valor_LDR = analogRead(LDR_PIN);
  float porcentaje_luz = map(valor_LDR, 0, 4094, 0,100);
  delay(2000);
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  mqttClient.publish(TOPIC_HUMEDAD, String(humedad).c_str());
  mqttClient.publish(TOPIC_TEMPERATURA, String(temperatura).c_str());
  mqttClient.publish(TOPIC_LUZ, String(porcentaje_luz).c_str());

  Serial.println();
  Serial.print("valor ldr: ");
  Serial.println( valor_LDR);
  Serial.print("Porcentaje de luz: ");
  Serial.println(porcentaje_luz);
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println("%");
  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println("C°");
  delay(2000);
}

void setup() {

  Serial.begin(115200);
  pinMode(LDR_PIN, INPUT);
  dht.begin();

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  publicarSensores();

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();


  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}