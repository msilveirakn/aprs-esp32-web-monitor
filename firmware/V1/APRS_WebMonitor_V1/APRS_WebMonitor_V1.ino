
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== Configuracao do projeto =====
const char* AP_SSID = "APRS-ESP32-V1";
const char* AP_PASSWORD = "aprs12345";

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_SSD1306 display(
  OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET
);

WebServer server(80);

unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000;

// ===== Pagina web =====
const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>APRS ESP32 - V1</title>
<style>
*{box-sizing:border-box}
body{
  margin:0;padding:20px;
  font-family:Arial,sans-serif;
  background:#101820;color:#eef4f8
}
main{max-width:650px;margin:auto}
h1{color:#71d6b0;font-size:25px}
.card{
  background:#1c2935;padding:18px;
  border-radius:12px;margin:14px 0
}
.label{color:#aebdca;font-size:14px}
.value{font-size:20px;margin-top:6px;overflow-wrap:anywhere}
button{
  border:0;border-radius:8px;padding:12px 18px;
  background:#71d6b0;color:#10251e;font-weight:bold
}
small{color:#aebdca}
</style>
</head>
<body>
<main>
<h1>APRS ESP32</h1>
<p>Web Monitor - Versao 1.0.0</p>

<div class="card">
  <div class="label">Estado do sistema</div>
  <div class="value" id="state">Carregando...</div>
</div>

<div class="card">
  <div class="label">Tempo ligado</div>
  <div class="value" id="uptime">--</div>
</div>

<div class="card">
  <div class="label">Endereco IP</div>
  <div class="value" id="ip">--</div>
</div>

<div class="card">
  <div class="label">Clientes Wi-Fi conectados</div>
  <div class="value" id="clients">--</div>
</div>

<div class="card">
  <div class="label">APRS-IS</div>
  <div class="value">Nao conectado nesta versao</div>
  <small>Esta V1 e somente um painel local.</small>
</div>

<button onclick="refreshStatus()">Atualizar dados</button>
<p><small>Atualizacao automatica a cada 3 segundos.</small></p>
</main>

<script>
async function refreshStatus(){
  try{
    const response = await fetch('/api/status');
    if(!response.ok) throw new Error('HTTP ' + response.status);
    const d = await response.json();

    document.getElementById('state').textContent = d.state;
    document.getElementById('uptime').textContent = d.uptime;
    document.getElementById('ip').textContent = d.ip;
    document.getElementById('clients').textContent = d.clients;
  }catch(e){
    document.getElementById('state').textContent =
      'Falha ao consultar o ESP32';
  }
}
refreshStatus();
setInterval(refreshStatus,3000);
</script>
</body>
</html>
)rawliteral";

// ===== Funcoes auxiliares =====
String getUptime() {
  unsigned long totalSeconds = millis() / 1000;
  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  char buffer[40];
  snprintf(buffer, sizeof(buffer),
           "%lud %02lu:%02lu:%02lu",
           days, hours, minutes, seconds);

  return String(buffer);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("APRS ESP32 - V1");

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  display.setCursor(0, 17);
  display.println("Wi-Fi: ATIVO");

  display.setCursor(0, 29);
  display.print("IP: ");
  display.println(WiFi.softAPIP());

  display.setCursor(0, 41);
  display.print("Clientes: ");
  display.println(WiFi.softAPgetStationNum());

  display.setCursor(0, 53);
  display.print("Ligado: ");
  display.print(millis() / 1000);
  display.print(" s");

  display.display();
}

// ===== Rotas HTTP =====
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

void handleStatus() {
  String json = "{";
  json += "\"state\":\"AP ativo\",";
  json += "\"uptime\":\"" + getUptime() + "\",";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum());
  json += "}";

  server.send(200, "application/json", json);
}

void handleNotFound() {
  server.send(404, "text/plain", "Rota nao encontrada");
}

// ===== Inicializacao =====
void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Erro ao inicializar o OLED");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Iniciando APRS...");
    display.display();
  }

  WiFi.mode(WIFI_AP);

  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (!apStarted) {
    Serial.println("Falha ao iniciar o Access Point");
  }

  Serial.print("Rede Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Servidor HTTP iniciado");
  updateDisplay();
}

// ===== Loop principal =====
void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;
    updateDisplay();
  }
}
