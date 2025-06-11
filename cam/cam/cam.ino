#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>


// Pinos dos potenciômetros
#define TEMPERATURE_PIN 12
#define HUMIDITY_PIN 13
#define GAS_PIN 15
#define LIGHT_PIN 14
#define INPUT_SIZE 4

// Parâmetros da rede neural
#define INPUT_SIZE 4
#define HIDDEN_SIZE 8
#define OUTPUT_SIZE 1

const char* ssid = "GalaxyF41";
const char* password = "nqxy7683";

// MQTT
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "UEFS_TEC_470/medidas";
const char * mqtt_user = "nicassio";
const char* mqtt_client_id = "dkadmakmsdka";
const char* mqtt_password = "gA123456";
WiFiClient espClient;
PubSubClient client(espClient);

bool wifiLigado = false;

// Valores mínimos dos sensores reais (temperatura, umidade, gás, luz)
const float sensor_min[INPUT_SIZE] = {
    0,      // Temperatura mínima
    10,     // Umidade mínima
    5,      // Gás mínimo
    10     // Luz mínima
};

// Valores máximos dos sensores reais (temperatura, umidade, gás, luz)
const float sensor_max[INPUT_SIZE] = {
    70,     // Temperatura máxima
    90,     // Umidade máxima
    250,    // Gás máximo
    1200   // Luz máxima
};

// Faixa de saída (chance_vida_%)
const float output_min = 0;
const float output_max = 100;

// Pesos e bias da rede (mantidos do código anterior)
const float input_weights[HIDDEN_SIZE][INPUT_SIZE] = {
    {-1.676144f, -4.192115f, -1.940467f, 0.342898f},
    {1.776931f, 0.179876f, 0.087885f, -4.758552f},
    {2.203135f, 1.034753f, -3.410160f, 2.743835f},
    {5.130609f, 0.060461f, -0.025356f, -0.443203f},
    {-0.148497f, 5.770133f, 0.007369f, 0.081489f},
    {-0.193404f, 0.070040f, -0.095037f, 3.248849f},
    {-3.875859f, 0.051763f, 0.069566f, -0.984084f},
    {-1.853075f, -2.282710f, 2.240063f, -3.274500f}
};

const float hidden_weights[OUTPUT_SIZE][HIDDEN_SIZE] = {
  {-0.005424f, -0.089901f, 0.017923f, -0.222324f, 0.107147f, -0.272823f, -0.265463f, -0.031131f}
};

const float hidden_bias[HIDDEN_SIZE] = {
    6.572955f, -0.433129f,  -2.585646f,  -3.004882f, 
   -2.730221f, -2.638584f, 1.441663f, -0.362875f
};

const float output_bias[OUTPUT_SIZE] = {
    0.008817f
};


float activation(float x, const char* type) {
    if (strcmp(type, "tansig") == 0) {
        // Implementação precisa da tanh
        if (x > 4.0) return 1.0;
        if (x < -4.0) return -1.0;
        float x2 = x * x;
        float a = x * (135135.0f + x2 * (17325.0f + x2 * 378.0f));
        float b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
        return a / b;
    }
    return x;
}

// Mapeia o valor do potenciômetro (0-1023) para a faixa do sensor real
void map_to_sensor_range(const int raw_input[INPUT_SIZE], float mapped[INPUT_SIZE]) {
    for (int i = 0; i < INPUT_SIZE; i++) {
        // Primeiro normaliza para 0-1, depois mapeia para a faixa do sensor
        mapped[i] = sensor_min[i] + (sensor_max[i] - sensor_min[i]) * (raw_input[i] / 4095.0f);
    }
}

// Normaliza os valores dos sensores para 0-1 (como foi feito no MATLAB)
void normalize_input(const float sensor_values[INPUT_SIZE], float normalized[INPUT_SIZE]) {
    for (int i = 0; i < INPUT_SIZE; i++) {
        normalized[i] = (sensor_values[i] - sensor_min[i]) / (sensor_max[i] - sensor_min[i]);
    }
}

float denormalize_output(float normalized) {
    return normalized * (output_max - output_min) + output_min;
}

void mlp_predict(const float input[INPUT_SIZE], float output[OUTPUT_SIZE]) {
    float hidden[HIDDEN_SIZE];
    
    // Camada oculta
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        hidden[i] = hidden_bias[i];
        for (int j = 0; j < INPUT_SIZE; j++) {
            hidden[i] += input_weights[i][j] * input[j];
        }
        hidden[i] = activation(hidden[i], "tansig");
    }
    
    // Camada de saída
    output[0] = output_bias[0];
    for (int j = 0; j < HIDDEN_SIZE; j++) {
        output[0] += hidden_weights[0][j] * hidden[j];
    }
    // Sem ativação (purelin)
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
    const char* willTopic = "status/esp32cam"; // Tópico da mensagem pós-morte

    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
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

  pinMode(TEMPERATURE_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);

  //setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  
  int raw_input[INPUT_SIZE] = {
    analogRead(TEMPERATURE_PIN),
    analogRead(HUMIDITY_PIN),
    analogRead(GAS_PIN),
    analogRead(LIGHT_PIN)
  };

  // Mapeia para as faixas reais dos sensores
  float sensor_values[INPUT_SIZE];
  map_to_sensor_range(raw_input, sensor_values);
  
  // Normaliza para 0-1 (como no MATLAB)
  float normalized_input[INPUT_SIZE];
  normalize_input(sensor_values, normalized_input);
  
  // Faz a predição
  float output;
  mlp_predict(normalized_input, &output);
  
  // Desnormaliza a saída
  float chanceVida = denormalize_output(output);
  
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

  if(wifiLigado){ 
    Serial.println("Deligando Wifi");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF); 
    wifiLigado = false;
  }
  delay(200);
}
