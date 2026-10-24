
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// CONFIGURACAO
// =====================================================

const char* WIFI_SSID = "Marcos";
const char* WIFI_PASSWORD = "48773354";

const char* AP_SSID = "APRS-ESP32-V3";
const char* AP_PASSWORD = "aprs12345";

const char* APRS_SERVER = "soam.aprs2.net";
const uint16_t APRS_PORT = 14580;

// Recepcao somente. -1 nao autoriza transmissao.
const char* APRS_CALLSIGN = "N0CALL";
const char* APRS_FILTER = "t/p";

constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr size_t MAX_PACKETS = 10;

constexpr unsigned long WIFI_RETRY_INTERVAL = 15000;
constexpr unsigned long APRS_RETRY_INTERVAL = 15000;
constexpr unsigned long LOGIN_TIMEOUT = 3000;
constexpr unsigned long OLED_INTERVAL = 1000;

Adafruit_SSD1306 display(
  OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET
);

WebServer server(80);
WiFiClient aprsClient;

// =====================================================
// ESTRUTURA E HISTORICO
// =====================================================

struct AprsPacket {
  String callsign;
  String raw;
  unsigned long receivedAt;
};

AprsPacket packets[MAX_PACKETS];

size_t packetHead = 0;
size_t packetCount = 0;

unsigned long totalPacketsReceived = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastAprsAttempt = 0;
unsigned long aprsConnectedAt = 0;
unsigned long lastOledUpdate = 0;

bool oledAvailable = false;
bool aprsLoginSent = false;

String connectionMessage = "Aguardando Wi-Fi";

// =====================================================
// FUNCOES AUXILIARES
// =====================================================

String jsonEscape(const String& input) {
  String output;
  output.reserve(input.length() + 8);

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];

    if (c == '"' || c == '\\') {
      output += '\\';
      output += c;
    } else if (c == '\n') {
      output += "\\n";
    } else if (c == '\r') {
      output += "\\r";
    } else if (c == '\t') {
      output += "\\t";
    } else if ((uint8_t)c >= 0x20) {
      output += c;
    }
  }

  return output;
}

String getNewestPacket(size_t index) {
  if (index >= packetCount) {
    return "";
  }

  size_t position =
    (packetHead + MAX_PACKETS - 1 - index) % MAX_PACKETS;

  return packets[position].raw;
}

void addPacket(const String& line) {
  if (line.length() == 0 || line.startsWith("#")) {
    return;
  }

  int separator = line.indexOf('>');
  if (separator <= 0) {
    return;
  }

  // Armazena a linha recebida, sem inventar ou simular dados.
  packets[packetHead].callsign = line.substring(0, separator);
  packets[packetHead].raw = line;
  packets[packetHead].receivedAt = millis();

  packetHead = (packetHead + 1) % MAX_PACKETS;

  if (packetCount < MAX_PACKETS) {
    packetCount++;
  }

  totalPacketsReceived++;
}

String getNewestCallsign() {
  if (packetCount == 0) {
    return "Nenhum";
  }

  size_t position =
    (packetHead + MAX_PACKETS - 1) % MAX_PACKETS;

  return packets[position].callsign;
}

// =====================================================
// WI-FI
// =====================================================

void startWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);

  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("=== APRS Web Monitor V3 ===");
  Serial.print("Ponto de acesso: ");
  Serial.println(apStarted ? "iniciado" : "falhou");
  Serial.print("IP do painel: ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWiFiAttempt = millis();
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  if (millis() - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) {
    lastWiFiAttempt = millis();
    Serial.println("Tentando reconectar ao Wi-Fi...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

// =====================================================
// APRS-IS: CONEXAO SOMENTE RECEPCAO
// =====================================================

void disconnectAprs(const String& reason) {
  if (aprsClient.connected()) {
    aprsClient.stop();
  }

  aprsLoginSent = false;
  connectionMessage = reason;
  Serial.println(reason);
}

void connectAprs() {
  if (WiFi.status() != WL_CONNECTED) {
    connectionMessage = "Sem internet: Wi-Fi";
    return;
  }

  Serial.print("Conectando ao APRS-IS: ");
  Serial.println(APRS_SERVER);

  if (!aprsClient.connect(APRS_SERVER, APRS_PORT)) {
    connectionMessage = "Falha na conexao APRS-IS";
    Serial.println(connectionMessage);
    lastAprsAttempt = millis();
    return;
  }

  aprsClient.setNoDelay(true);
  aprsClient.setTimeout(20);

  aprsConnectedAt = millis();
  lastAprsAttempt = millis();
  aprsLoginSent = false;
  connectionMessage = "TCP conectado; aguardando servidor";

  Serial.println(connectionMessage);
}

void sendAprsLogin() {
  if (!aprsClient.connected() || aprsLoginSent) {
    return;
  }

  String login = "user ";
  login += APRS_CALLSIGN;
  login += " pass -1 vers APRS_ESP32_WebMonitor 3.0 filter ";
  login += APRS_FILTER;
  login += "\r\n";

  aprsClient.print(login);
  aprsLoginSent = true;
  connectionMessage = "Login enviado; recepcao";

  Serial.print("Login APRS-IS enviado: ");
  Serial.println(login);
}

void maintainAprs() {
  if (WiFi.status() != WL_CONNECTED) {
    if (aprsClient.connected()) {
      disconnectAprs("Wi-Fi desconectado");
    } else {
      connectionMessage = "Aguardando Wi-Fi";
    }
    return;
  }

  if (aprsClient.connected()) {
    // Alguns servidores enviam uma linha de identificacao
    // iniciada por # antes da linha de login do cliente.
    if (!aprsLoginSent &&
        millis() - aprsConnectedAt >= LOGIN_TIMEOUT) {
      sendAprsLogin();
    }

    while (aprsClient.available()) {
      String line = aprsClient.readStringUntil('\n');
      line.trim();

      if (line.length() == 0) {
        continue;
      }

      Serial.println(line);

      if (line.startsWith("#")) {
        // Envia o login ao receber a identificacao do servidor.
        if (!aprsLoginSent) {
          sendAprsLogin();
        }

        // Guarda mensagens de estado relevantes.
        if (line.indexOf("logresp") >= 0) {
          connectionMessage = line;
        }
        continue;
      }

      if (!aprsLoginSent) {
        sendAprsLogin();
      }

      addPacket(line);
    }

    if (!aprsClient.connected()) {
      disconnectAprs("Conexao APRS-IS encerrada");
    }

    return;
  }

  if (millis() - lastAprsAttempt >= APRS_RETRY_INTERVAL) {
    lastAprsAttempt = millis();
    connectAprs();
  }
}

// =====================================================
// API JSON
// =====================================================

void handleStatus() {
  String json = "{";

  json += "\"version\":\"3.0\",";
  json += "\"wifiConnected\":";
  json += (WiFi.status() == WL_CONNECTED) ? "true," : "false,";

  json += "\"wifiIP\":\"";
  json += jsonEscape(
    WiFi.status() == WL_CONNECTED
      ? WiFi.localIP().toString()
      : String("desconectado")
  );
  json += "\",";

  json += "\"apIP\":\"";
  json += WiFi.softAPIP().toString();
  json += "\",";

  json += "\"aprsConnected\":";
  json += aprsClient.connected() ? "true," : "false,";

  json += "\"loginSent\":";
  json += aprsLoginSent ? "true," : "false,";

  json += "\"message\":\"";
  json += jsonEscape(connectionMessage);
  json += "\",";

  json += "\"callsign\":\"";
  json += jsonEscape(APRS_CALLSIGN);
  json += "\",";

  json += "\"filter\":\"";
  json += jsonEscape(APRS_FILTER);
  json += "\",";

  json += "\"totalPackets\":";
  json += String(totalPacketsReceived);
  json += ",";

  json += "\"historyCount\":";
  json += String(packetCount);
  json += ",";

  json += "\"newestCallsign\":\"";
  json += jsonEscape(getNewestCallsign());
  json += "\",";

  json += "\"uptimeSeconds\":";
  json += String(millis() / 1000);
  json += "}";

  server.send(200, "application/json", json);
}

void handlePackets() {
  String json = "{\"packets\":[";

  for (size_t i = 0; i < packetCount; i++) {
    if (i > 0) {
      json += ",";
    }

    size_t position =
      (packetHead + MAX_PACKETS - 1 - i) % MAX_PACKETS;

    json += "{\"callsign\":\"";
    json += jsonEscape(packets[position].callsign);
    json += "\",\"raw\":\"";
    json += jsonEscape(packets[position].raw);
    json += "\",\"ageSeconds\":";
    json += String(
      (millis() - packets[position].receivedAt) / 1000
    );
    json += "}";
  }

  json += "]}";
  server.send(200, "application/json", json);
}

// =====================================================
// PAINEL WEB
// =====================================================

void handleRoot() {
  String html = R"HTML(
<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>APRS ESP32 V3</title>
<style>
body{font-family:system-ui,sans-serif;max-width:1000px;margin:24px auto;
padding:0 14px;background:#111827;color:#f3f4f6}
.card{background:#1f2937;padding:16px;border-radius:12px;margin:12px 0}
h1{font-size:1.5rem} .muted{color:#9ca3af}
code{overflow-wrap:anywhere;white-space:pre-wrap}
table{width:100%;border-collapse:collapse}
td,th{text-align:left;padding:9px;border-bottom:1px solid #374151}
.status{font-weight:bold} .scroll{overflow-x:auto}
</style>
</head>
<body>
<h1>APRS ESP32 Web Monitor — V3</h1>
<p class="muted">Recepcao APRS-IS via internet; modo somente leitura.</p>
<div class="card">
<h2>Estado do sistema</h2>
<div id="status">Carregando...</div>
</div>
<div class="card">
<h2>Pacotes recebidos</h2>
<p>Total nesta sessao: <strong id="total">0</strong></p>
<div class="scroll">
<table>
<thead><tr><th>Indicativo</th><th>Pacote APRS-IS</th><th>Idade</th></tr></thead>
<tbody id="packets"></tbody>
</table>
</div>
</div>
<script>
async function refresh(){
  try{
    const s=await (await fetch('/api/status')).json();
    document.getElementById('status').innerHTML =
      '<p>Wi-Fi internet: <span class="status">'+
      (s.wifiConnected?'Conectado':'Desconectado')+'</span></p>'+
      '<p>IP da rede Wi-Fi: '+s.wifiIP+'</p>'+
      '<p>IP do ponto de acesso: '+s.apIP+'</p>'+
      '<p>APRS-IS TCP: '+(s.aprsConnected?'Conectado':'Desconectado')+'</p>'+
      '<p>Login enviado: '+(s.loginSent?'Sim':'Aguardando')+'</p>'+
      '<p>Estado: '+s.message+'</p>'+
      '<p>Filtro: '+s.filter+'</p>'+
      '<p>Tempo ligado: '+s.uptimeSeconds+' s</p>';

    document.getElementById('total').textContent=s.totalPackets;
    const p=await (await fetch('/api/packets')).json();
    const body=document.getElementById('packets');
    body.replaceChildren();

    for(const item of p.packets){
      const tr=document.createElement('tr');
      for(const value of [item.callsign,item.raw,item.ageSeconds+' s']){
        const td=document.createElement('td');
        const code=document.createElement('code');
        code.textContent=value;
        td.appendChild(code);
        tr.appendChild(td);
      }
      body.appendChild(tr);
    }
  }catch(e){
    document.getElementById('status').textContent='Falha ao consultar a API';
  }
}
refresh();
setInterval(refresh,3000);
</script>
</body>
</html>
)HTML";

  server.send(200, "text/html; charset=utf-8", html);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/packets", HTTP_GET, handlePackets);

  server.begin();
  Serial.println("Servidor web iniciado na porta 80");
}

// =====================================================
// OLED
// =====================================================

void updateOled() {
  if (!oledAvailable) {
    return;
  }

  if (millis() - lastOledUpdate < OLED_INTERVAL) {
    return;
  }

  lastOledUpdate = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("APRS Monitor V3");
  display.println("----------------");

  display.print("WiFi: ");
  display.println(
    WiFi.status() == WL_CONNECTED ? "OK" : "OFF"
  );

  display.print("APRS-IS: ");
  display.println(
    aprsClient.connected() ? "TCP OK" : "OFF"
  );

  display.print("Login: ");
  display.println(aprsLoginSent ? "enviado" : "aguardando");

  display.print("Pacotes: ");
  display.println(totalPacketsReceived);

  display.print("Ultimo: ");
  display.println(getNewestCallsign());

  display.print("AP IP: ");
  display.println(WiFi.softAPIP());

  display.display();
}

// =====================================================
// SETUP / LOOP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(21, 22);

  oledAvailable = display.begin(
    SSD1306_SWITCHCAPVCC, OLED_ADDRESS
  );

  if (oledAvailable) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("APRS Monitor V3");
    display.println("Inicializando...");
    display.display();
  } else {
    Serial.println("OLED nao encontrado; continuando sem tela.");
  }

  startWiFi();
  startWebServer();
}

void loop() {
  maintainWiFi();
  maintainAprs();

  server.handleClient();
  updateOled();

  delay(2);
}
