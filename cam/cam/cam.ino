#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>

// Pinos dos potenciômetros
#define TEMPERATURE_PIN 12
#define HUMIDITY_PIN 13
#define GAS_PIN 15
#define LIGHT_PIN 14
#define INPUT_SIZE 4

const char* ssid = "GalaxyF41";
const char* password = "nqxy7683";

// MQTT
const char* mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensores";
const char* mqtt_client = "user";
const char* mqtt_password = "1234";
WiFiClient espClient;
PubSubClient client(espClient);

bool wifiLigado = false;

const float sensor_min[INPUT_SIZE] = {5.793663f, 20.596114f, 9.414635f, 160.582696f};
const float sensor_max[INPUT_SIZE] = {41.263657f, 81.931076f, 217.787131f, 1086.463945f};
const float output_min = 27.8333;
const float output_max = 75.1331;

void map_to_sensor_range(const int raw_input[INPUT_SIZE], float mapped[INPUT_SIZE]) {
  for (int i = 0; i < INPUT_SIZE; i++) {
    mapped[i] = sensor_min[i] + (sensor_max[i] - sensor_min[i]) * (raw_input[i] / 4095.0f);
  }
}

float denormalize_output(float normalized) {
  return normalized * (output_max - output_min) + output_min;
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 50000) {
    delay(1000);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.println("Hora sincronizada com NTP");
    } else {
      Serial.println("Falha ao sincronizar hora");
    }

    wifiLigado = true;
  } else {
    Serial.println("Falha ao conectar ao Wi-Fi.");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("web-client-01", "user", "1234")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falhou, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  //pinMode(TEMPERATURE_PIN, INPUT);
  //pinMode(HUMIDITY_PIN, INPUT);
  //pinMode(GAS_PIN, INPUT);
  //pinMode(LIGHT_PIN, INPUT);

  //setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  
  int raw_input[INPUT_SIZE] = {
    0,//analogRead(TEMPERATURE_PIN),
    0,//analogRead(HUMIDITY_PIN),
    0,//analogRead(GAS_PIN),
    0,//analogRead(LIGHT_PIN)
  };

  float sensor_values[INPUT_SIZE];
  map_to_sensor_range(raw_input, sensor_values);
  

  float chanceVida = denormalize_output(0.82);  // valor fixo por enquanto

  // Obter data/hora no formato ISO 8601 UTC
  char isoDate[30];
  time_t now;
  time(&now);
  struct tm *utc_time = gmtime(&now);
  strftime(isoDate, sizeof(isoDate), "%Y-%m-%dT%H:%M:%SZ", utc_time);

  // Criar payload JSON
  String payload = "{";
  payload += "\"temperatura\":" + String(sensor_values[0], 2) + ",";
  payload += "\"umidade\":" + String(sensor_values[1], 2) + ",";
  payload += "\"luminosidade\":" + String(sensor_values[3], 2) + ",";
  payload += "\"gas\":" + String(sensor_values[2], 2) + ",";
  payload += "\"chanceVida\":" + String(chanceVida, 2) + ",";
  payload += "\"dataHora\":\"" + String(isoDate) + "\"";
  payload += "}";

  if (!wifiLigado) {
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  Serial.println("Enviando via MQTT:");
  Serial.println(payload);

  client.publish(mqtt_topic, payload.c_str());

  delay(10000);
}
