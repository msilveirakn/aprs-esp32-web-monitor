#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// APRS ESP32 WEB MONITOR - V4
// Recepcao APRS-IS, parser basico de pacotes e painel web.
// Nao transmite pacotes para APRS-IS.
// =====================================================

// Wi-Fi domestico: preencha antes de compilar.
const char* WIFI_SSID = "Marcos";
const char* WIFI_PASSWORD = "48773354";

// Ponto de acesso local do ESP32.
const char* AP_SSID = "APRS-ESP32-V4";
const char* AP_PASSWORD = "aprs12345";

// APRS-IS. pass -1 e usado para recepcao sem autenticacao.
const char* APRS_SERVER = "rotate.aprs2.net";
const uint16_t APRS_PORT = 14580;
const char* APRS_CALLSIGN = "N0CALL"; // Identificador demonstrativo
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

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);
WiFiClient aprsClient;

struct AprsPacket {
  String callsign;
  String path;
  String raw;
  String type;
  bool hasPosition;
  double latitude;
  double longitude;
  char symbolTable;
  char symbolCode;
  unsigned long receivedAt;
};

AprsPacket packets[MAX_PACKETS];
size_t packetHead = 0;  // Proxima posicao em que gravar
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
// AUXILIARES
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

bool isDigits(const String& value) {
  if (value.length() == 0) {
    return false;
  }

  for (size_t i = 0; i < value.length(); i++) {
    if (value[i] < '0' || value[i] > '9') {
      return false;
    }
  }

  return true;
}

// Formatos de entrada:
// Latitude:  DDMM.hhN ou DDMM.hhS (8 caracteres)
// Longitude: DDDMM.hhE ou DDDMM.hhW (9 caracteres)
bool parseAprsCoordinate(
  const String& value,
  bool isLatitude,
  double& result
) {
  const int degreeDigits = isLatitude ? 2 : 3;
  const int expectedLength = degreeDigits + 6; // graus + MM.hh + hemisferio

  if (value.length() != expectedLength) {
    return false;
  }

  String degreesText = value.substring(0, degreeDigits);
  String minutesText = value.substring(degreeDigits, degreeDigits + 5);
  char hemisphere = value[expectedLength - 1];

  if (!isDigits(degreesText)) {
    return false;
  }

  if (minutesText[0] < '0' || minutesText[0] > '9' ||
      minutesText[1] < '0' || minutesText[1] > '9' ||
      minutesText[2] != '.' ||
      minutesText[3] < '0' || minutesText[3] > '9' ||
      minutesText[4] < '0' || minutesText[4] > '9') {
    return false;
  }

  if (isLatitude) {
    if (hemisphere != 'N' && hemisphere != 'S') {
      return false;
    }
  } else {
    if (hemisphere != 'E' && hemisphere != 'W') {
      return false;
    }
  }

  int degrees = degreesText.toInt();
  double minutes = minutesText.toFloat();
  const int maxDegrees = isLatitude ? 90 : 180;

  if (minutes >= 60.0 || degrees > maxDegrees) {
    return false;
  }

  if (degrees == maxDegrees && minutes > 0.0) {
    return false;
  }

  result = degrees + minutes / 60.0;

  if (hemisphere == 'S' || hemisphere == 'W') {
    result = -result;
  }

  return true;
}

String classifyAprsPacket(char dataType) {
  switch (dataType) {
    case '!':
    case '=':
    case '/':
    case '@':
      return "Posicao";
    case ':':
      return "Mensagem";
    case '>':
      return "Status";
    case '_':
      return "Meteorologia";
    case 'T':
      return "Telemetria";
    case ';':
      return "Objeto";
    case ')':
      return "Item";
    case '$':
      return "Dados GPS/NMEA";
    case '?':
      return "Consulta";
    case '{':
      return "Extraneous";
    default:
      return "Outro";
  }
}

// Decodifica posicoes APRS nao comprimidas.
// Suporta ! e = sem timestamp, e / e @ com timestamp de 7 caracteres.
// Posicoes comprimidas e formatos especiais ficam como texto bruto.
bool parseAprsPosition(
  const String& payload,
  double& latitude,
  double& longitude,
  char& symbolTable,
  char& symbolCode
) {
  if (payload.length() < 19) {
    return false;
  }

  char dataType = payload[0];
  int positionOffset = 0;

  if (dataType == '!' || dataType == '=') {
    positionOffset = 1;
  } else if (dataType == '/' || dataType == '@') {
    positionOffset = 8; // DTI + timestamp de 7 caracteres
  } else {
    return false;
  }

  // Latitude (8) + tabela (1) + longitude (9) + simbolo (1)
  if (payload.length() < (size_t)(positionOffset + 19)) {
    return false;
  }

  String latText = payload.substring(positionOffset, positionOffset + 8);
  symbolTable = payload[positionOffset + 8];
  String lonText = payload.substring(positionOffset + 9, positionOffset + 18);
  symbolCode = payload[positionOffset + 18];

  if (!parseAprsCoordinate(latText, true, latitude)) {
    return false;
  }

  if (!parseAprsCoordinate(lonText, false, longitude)) {
    return false;
  }

  return true;
}

String getNewestCallsign() {
  if (packetCount == 0) {
    return "Nenhum";
  }

  size_t position = (packetHead + MAX_PACKETS - 1) % MAX_PACKETS;
  return packets[position].callsign;
}

// =====================================================
// ARMAZENAMENTO E DECODIFICACAO
// =====================================================

void addPacket(const String& line) {
  if (line.length() == 0 || line.startsWith("#")) {
    return;
  }

  int colon = line.indexOf(':');
  int separator = line.indexOf('>');

  if (separator <= 0 || colon <= separator + 1) {
    Serial.println("Linha ignorada: cabecalho APRS invalido.");
    return;
  }

  String header = line.substring(0, colon);
  String payload = line.substring(colon + 1);

  int headerSeparator = header.indexOf('>');
  if (headerSeparator <= 0 || payload.length() == 0) {
    return;
  }

  AprsPacket packet;
  packet.callsign = header.substring(0, headerSeparator);
  packet.path = header.substring(headerSeparator + 1);
  packet.raw = line;
  packet.type = classifyAprsPacket(payload[0]);
  packet.hasPosition = false;
  packet.latitude = 0.0;
  packet.longitude = 0.0;
  packet.symbolTable = '\0';
  packet.symbolCode = '\0';
  packet.receivedAt = millis();

  if (packet.type == "Posicao") {
    packet.hasPosition = parseAprsPosition(
      payload,
      packet.latitude,
      packet.longitude,
      packet.symbolTable,
      packet.symbolCode
    );

    if (!packet.hasPosition) {
      packet.type = "Posicao nao decodificada";
    }
  }

  packets[packetHead] = packet;
  packetHead = (packetHead + 1) % MAX_PACKETS;

  if (packetCount < MAX_PACKETS) {
    packetCount++;
  }

  totalPacketsReceived++;

  Serial.print("Pacote [");
  Serial.print(packet.type);
  Serial.print("] ");
  Serial.println(packet.callsign);

  if (packet.hasPosition) {
    Serial.print("  Latitude: ");
    Serial.println(packet.latitude, 6);
    Serial.print("  Longitude: ");
    Serial.println(packet.longitude, 6);
  }
}

// =====================================================
// WI-FI
// =====================================================

void startWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);

  bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("=== APRS Web Monitor V4 ===");
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
// APRS-IS: SOMENTE RECEPCAO
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
  login += " pass -1 vers APRS_ESP32_WebMonitor 4.0 filter ";
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
        if (line.indexOf("Login by user not allowed") >= 0) {
          disconnectAprs("Servidor rejeitou o tipo de login");
          lastAprsAttempt = millis();
          return;
        }

        if (line.indexOf("logresp") >= 0) {
          connectionMessage = line;

          if (line.indexOf("unverified") >= 0 &&
              line.indexOf("verified") < 0) {
            Serial.println("Servidor informou login nao verificado.");
          }
        }

        if (!aprsLoginSent) {
          sendAprsLogin();
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

  json += "\"version\":\"4.0\",";
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

    size_t position = (packetHead + MAX_PACKETS - 1 - i) % MAX_PACKETS;
    const AprsPacket& packet = packets[position];

    json += "{\"callsign\":\"";
    json += jsonEscape(packet.callsign);
    json += "\",\"path\":\"";
    json += jsonEscape(packet.path);
    json += "\",\"type\":\"";
    json += jsonEscape(packet.type);
    json += "\",\"hasPosition\":";
    json += packet.hasPosition ? "true" : "false";

    if (packet.hasPosition) {
      json += ",\"latitude\":";
      json += String(packet.latitude, 6);
      json += ",\"longitude\":";
      json += String(packet.longitude, 6);
      json += ",\"symbolTable\":\"";
      json += jsonEscape(String(packet.symbolTable));
      json += "\",\"symbolCode\":\"";
      json += jsonEscape(String(packet.symbolCode));
      json += "\"";
    }

    json += ",\"raw\":\"";
    json += jsonEscape(packet.raw);
    json += "\",\"ageSeconds\":";
    json += String((millis() - packet.receivedAt) / 1000);
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
<title>APRS ESP32 V4</title>
<style>
body{font-family:system-ui,sans-serif;max-width:1100px;margin:24px auto;
padding:0 14px;background:#111827;color:#f3f4f6}
.card{background:#1f2937;padding:16px;border-radius:12px;margin:12px 0}
h1{font-size:1.5rem}.muted{color:#9ca3af}
code{overflow-wrap:anywhere;white-space:pre-wrap}
table{width:100%;border-collapse:collapse}
td,th{text-align:left;padding:9px;border-bottom:1px solid #374151;vertical-align:top}
.scroll{overflow-x:auto}.status{font-weight:bold}
</style>
</head>
<body>
<h1>APRS ESP32 Web Monitor — V4</h1>
<p class="muted">Recepcao APRS-IS e decodificacao basica de posicoes.</p>
<div class="card">
<h2>Estado do sistema</h2>
<div id="status">Carregando...</div>
</div>
<div class="card">
<h2>Pacotes recebidos</h2>
<p>Total nesta sessao: <strong id="total">0</strong></p>
<div class="scroll">
<table>
<thead><tr><th>Indicativo</th><th>Tipo</th><th>Posicao</th><th>Simbolo</th><th>Pacote original</th><th>Idade</th></tr></thead>
<tbody id="packets"></tbody>
</table>
</div>
</div>
<script>
async function refresh(){
  try{
    const s = await (await fetch('/api/status')).json();
    const status = document.getElementById('status');
    status.replaceChildren();

    const lines = [
      'Wi-Fi internet: ' + (s.wifiConnected ? 'Conectado' : 'Desconectado'),
      'IP da rede Wi-Fi: ' + s.wifiIP,
      'IP do ponto de acesso: ' + s.apIP,
      'APRS-IS TCP: ' + (s.aprsConnected ? 'Conectado' : 'Desconectado'),
      'Login enviado: ' + (s.loginSent ? 'Sim' : 'Aguardando'),
      'Estado: ' + s.message,
      'Filtro: ' + s.filter,
      'Tempo ligado: ' + s.uptimeSeconds + ' s'
    ];

    for (const text of lines) {
      const p = document.createElement('p');
      p.textContent = text;
      status.appendChild(p);
    }

    document.getElementById('total').textContent = s.totalPackets;

    const p = await (await fetch('/api/packets')).json();
    const body = document.getElementById('packets');
    body.replaceChildren();

    for (const item of p.packets) {
      const tr = document.createElement('tr');
      const position = item.hasPosition
        ? item.latitude.toFixed(6) + ', ' + item.longitude.toFixed(6)
        : 'Indisponivel';
      const symbol = item.hasPosition
        ? item.symbolTable + ' ' + item.symbolCode
        : '-';

      const values = [
        item.callsign, item.type, position, symbol,
        item.raw, item.ageSeconds + ' s'
      ];

      for (const value of values) {
        const td = document.createElement('td');
        const code = document.createElement('code');
        code.textContent = value;
        td.appendChild(code);
        tr.appendChild(td);
      }
      body.appendChild(tr);
    }
  } catch (e) {
    document.getElementById('status').textContent =
      'Falha ao consultar a API';
  }
}
refresh();
setInterval(refresh, 3000);
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
  if (!oledAvailable || millis() - lastOledUpdate < OLED_INTERVAL) {
    return;
  }

  lastOledUpdate = millis();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("APRS Monitor V4");
  display.println("----------------");

  display.print("WiFi: ");
  display.println(WiFi.status() == WL_CONNECTED ? "OK" : "OFF");

  display.print("APRS-IS: ");
  display.println(aprsClient.connected() ? "TCP OK" : "OFF");

  display.print("Pacotes: ");
  display.println(totalPacketsReceived);

  if (packetCount > 0) {
    size_t position = (packetHead + MAX_PACKETS - 1) % MAX_PACKETS;
    const AprsPacket& packet = packets[position];

    display.print(packet.callsign);
    display.print(" ");
    display.println(packet.type);

    if (packet.hasPosition) {
      display.print(packet.latitude, 2);
      display.print(",");
      display.println(packet.longitude, 2);
    } else {
      display.println("Sem posicao decod.");
    }
  } else {
    display.println("Aguardando pacotes");
  }

  display.display();
}

// =====================================================
// SETUP / LOOP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(21, 22);

  oledAvailable = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);

  if (oledAvailable) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("APRS Monitor V4");
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