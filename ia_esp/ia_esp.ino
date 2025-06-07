#include <math.h>

// Defina os pinos dos potenciômetros
#define TEMPERATURE_PIN 12    // Potenciômetro para temperatura
#define HUMIDITY_PIN 13       // Potenciômetro para umidade
#define GAS_PIN 15            // Potenciômetro para gás
#define LIGHT_PIN 14          // Potenciômetro para luz

// Parâmetros da rede neural
#define INPUT_SIZE 4
#define HIDDEN_SIZE 8
#define OUTPUT_SIZE 1

// Valores mínimos dos sensores reais (temperatura, umidade, gás, luz)
const float sensor_min[INPUT_SIZE] = {
    5.793663f,      // Temperatura mínima
    20.596114f,     // Umidade mínima
    9.414635f,      // Gás mínimo
    160.582696f     // Luz mínima
};

// Valores máximos dos sensores reais (temperatura, umidade, gás, luz)
const float sensor_max[INPUT_SIZE] = {
    41.263657f,     // Temperatura máxima
    81.931076f,     // Umidade máxima
    217.787131f,    // Gás máximo
    1086.463945f    // Luz máxima
};

// Faixa de saída (chance_vida_%)
const float output_min = 27.8333;
const float output_max = 75.1331;

// Pesos e bias da rede (mantidos do código anterior)
const float input_weights[HIDDEN_SIZE][INPUT_SIZE] = {
     {-1.585901f, -3.851513f, -2.323728f, -0.228413f},
    {0.290427f, 0.014248f, 0.095358f, -4.617404f},
    {2.325055f, 0.909094f, -3.530610f, 2.585164f},
    {4.785251f, 0.045489f, -0.005723f, -0.187794f},
    {-0.058725f, 5.988096f, -0.001366f, 0.012601f},
    {-0.027096f, 0.051076f, -0.090247f, 3.419779f},
    {-4.493089f, 0.019802f, 0.085434f, -0.383548f},
    {-1.960262f, -1.836490f, 2.190576f, -3.523955f}
};

const float hidden_weights[OUTPUT_SIZE][HIDDEN_SIZE] = {
    {-0.023949f, -0.264223f, 0.031412f, -0.479339f, 0.217386f, -0.525979f, -0.496713f, 0.008342f}
};

const float hidden_bias[HIDDEN_SIZE] = {
    6.545727f, 0.496819f, -2.521009f, -2.943459f, 
    -2.838308f, -2.974795f, 1.388760f, -0.147514f
};

const float output_bias[OUTPUT_SIZE] = {
    -0.509851f
};

void setup() {
  Serial.begin(115200);
  
  pinMode(TEMPERATURE_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  
  Serial.println("Sistema MLP com mapeamento de potenciômetros para sensores");
}

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

void loop() {
  // Lê os valores brutos dos potenciômetros
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
  float final_output = denormalize_output(output);
  
  // Exibe os resultados
  Serial.print("Valores mapeados: [");
  Serial.print("Temp: "); Serial.print(sensor_values[0]); Serial.print("°C, ");
  Serial.print("Umid: "); Serial.print(sensor_values[1]); Serial.print("%, ");
  Serial.print("Gas: "); Serial.print(sensor_values[2]); Serial.print("ppm, ");
  Serial.print("Luz: "); Serial.print(sensor_values[3]); Serial.print("lux");
  Serial.print("] => ");
  
  Serial.print("Chance de vida: ");
  Serial.print(final_output);
  Serial.println("%");
  
  delay(500);
}