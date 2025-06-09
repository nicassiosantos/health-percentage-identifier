#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"

// Dados do Wi-Fi
const char* ssid = "";
const char* password = "";
const char* serverName = ""; // URL do endpoint

void setup() {
  Serial.begin(115200);

  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado!");

  // Inicializar câmera
  camera_config_t config = {
    // configuração da câmera
  };
  esp_camera_init(&config);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    camera_fb_t *fb = esp_camera_fb_get();

    if (fb) {
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "image/jpeg");
      int httpResponseCode = http.POST(fb->buf, fb->len);
      
      Serial.printf("Resposta do servidor: %d\n", httpResponseCode);
      esp_camera_fb_return(fb);
    }

    delay(10000);  // Espera 10 segundos antes da próxima foto
  }
}
