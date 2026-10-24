
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

// =====================================================
// APRS ESP32 Web Monitor - V2.0.0
// Os pacotes desta versao sao simulados.
// Nenhum dado real de radio ou APRS-IS e recebido.
// =====================================================

// ---------- Wi-Fi ----------
const char* AP_SSID = "APRS-ESP32-V2";
const char* AP_PASSWORD = "aprs12345";

// ---------- OLED ----------
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define I2C_SDA 21
#define I2C_SCL 22

Adafruit_SSD1306 display(
  OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET
);

// ---------- Servidor ----------
WebServer server(80);

// ---------- Temporizadores ----------
const unsigned long DISPLAY_INTERVAL = 1000;
const unsigned long SIMULATION_INTERVAL = 15000;

unsigned long lastDisplayUpdate = 0;
unsigned long lastSimulation = 0;

// ---------- Modelo de dados APRS ----------
const int MAX_PACKETS = 10;

struct AprsPacket {
  char callsign[12];
  char path[32];
  char type[20];
  char message[80];
  unsigned long receivedAt;
};

AprsPacket packets[MAX_PACKETS];

int packetCount = 0;
int nextPacketIndex = 0;
unsigned long totalPacketsReceived = 0;

// ---------- Pacotes de demonstracao ----------
struct SimulatedPacket {
  const char* callsign;
  const char* path;
  const char* type;
  const char* message;
};

const SimulatedPacket samplePackets[] = {
  {
    "PY2ABC",
    "WIDE1-1",
    "Position",
    "Estacao de teste"
  },
  {
    "PY2XYZ",
    "WIDE1-1,WIDE2-1",
    "Weather",
    "Temperatura 24 C"
  },
  {
    "PY5DEF",
    "DIRECT",
    "Message",
    "Teste de mensagem APRS"
  },
  {
    "PY2LMN",
    "WIDE1-1",
    "Telemetry",
    "BAT=3.9V;TEMP=28C"
  },
  {
    "PY4RST",
    "DIRECT",
    "Position",
    "Posicao de demonstracao"
  }
};

const size_t SAMPLE_COUNT =
  sizeof(samplePackets) / sizeof(samplePackets[0]);

size_t nextSampleIndex = 0;

// =====================================================
// Utilitarios
// =====================================================

String getUptime() {
  unsigned long total = millis() / 1000;

  unsigned long days = total / 86400;
  unsigned long hours = (total % 86400) / 3600;
  unsigned long minutes = (total % 3600) / 60;
  unsigned long seconds = total % 60;

  char buffer[32];

  snprintf(
    buffer, sizeof(buffer),
    "%lud %02lu:%02lu:%02lu",
    days, hours, minutes, seconds
  );

  return String(buffer);
}

// Escapa caracteres especiais para uma string JSON.
String jsonEscape(const char* input) {
  String output;
  output.reserve(strlen(input) + 8);

  const char* hex = "0123456789ABCDEF";

  while (*input) {
    unsigned char c = (unsigned char)*input++;

    switch (c) {
      case '"':
        output += "\\\"";
        break;

      case '\\':
        output += "\\\\";
        break;

      case '\n':
        output += "\\n";
        break;

      case '\r':
        output += "\\r";
        break;

      case '\t':
        output += "\\t";
        break;

      default:
        if (c < 0x20) {
          output += "\\u00";
          output += hex[(c >> 4) & 0x0F];
          output += hex[c & 0x0F];
        } else {
          output += (char)c;
        }
        break;
    }
  }

  return output;
}

// =====================================================
// Gerenciamento do historico
// =====================================================

void addPacket(
  const char* callsign,
  const char* path,
  const char* type,
  const char* message
) {
  AprsPacket& packet = packets[nextPacketIndex];

  snprintf(
    packet.callsign, sizeof(packet.callsign),
    "%s", callsign
  );

  snprintf(
    packet.path, sizeof(packet.path),
    "%s", path
  );

  snprintf(
    packet.type, sizeof(packet.type),
    "%s", type
  );

  snprintf(
    packet.message, sizeof(packet.message),
    "%s", message
  );

  packet.receivedAt = millis();

  nextPacketIndex =
    (nextPacketIndex + 1) % MAX_PACKETS;

  if (packetCount < MAX_PACKETS) {
    packetCount++;
  }

  totalPacketsReceived++;
}

AprsPacket& getNewestPacket(int position) {
  // position 0 = pacote mais recente.
  int index =
    (nextPacketIndex - 1 - position + MAX_PACKETS * 2)
    % MAX_PACKETS;

  return packets[index];
}

// =====================================================
// Simulador APRS
// =====================================================

void simulatePacket() {
  const SimulatedPacket& sample =
    samplePackets[nextSampleIndex];

  addPacket(
    sample.callsign,
    sample.path,
    sample.type,
    sample.message
  );

  nextSampleIndex =
    (nextSampleIndex + 1) % SAMPLE_COUNT;

  Serial.print("[SIMULADO] ");
  Serial.print(sample.callsign);
  Serial.print(" | ");
  Serial.print(sample.type);
  Serial.print(" | ");
  Serial.println(sample.message);
}

void initializeSampleData() {
  // Gera tres registros iniciais para preencher
  // o painel logo apos a inicializacao.
  for (int i = 0; i < 3; i++) {
    simulatePacket();
  }

  lastSimulation = millis();
}

// =====================================================
// Interface OLED
// =====================================================

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("APRS MONITOR V2");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 14);
  display.print("Wi-Fi: ATIVO");

  display.setCursor(0, 25);
  display.print("Pacotes: ");
  display.print(totalPacketsReceived);

  display.setCursor(0, 36);
  display.print("Historico: ");
  display.print(packetCount);
  display.print("/");
  display.print(MAX_PACKETS);

  display.setCursor(0, 47);

  if (packetCount > 0) {
    AprsPacket& latest = getNewestPacket(0);
    display.print("Ultimo: ");
    display.println(latest.callsign);
  } else {
    display.println("Aguardando dados");
  }

  display.setCursor(0, 57);
  display.print("IP: ");
  display.print(WiFi.softAPIP());

  display.display();
}

// =====================================================
// Pagina web
// =====================================================

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>APRS ESP32 - V2</title>

<style>
*{box-sizing:border-box}
body{
  margin:0;padding:20px;
  font-family:Arial,sans-serif;
  background:#101820;color:#eef4f8
}
main{max-width:900px;margin:auto}
h1{color:#71d6b0}
small,.muted{color:#aebdca}
.grid{
  display:grid;
  grid-template-columns:repeat(auto-fit,minmax(180px,1fr));
  gap:12px
}
.card{
  background:#1c2935;
  padding:16px;border-radius:12px;
  margin:12px 0
}
.value{font-size:21px;margin-top:8px;overflow-wrap:anywhere}
.table-wrap{overflow-x:auto}
table{
  width:100%;border-collapse:collapse;
  min-width:620px
}
th,td{
  padding:10px;text-align:left;
  border-bottom:1px solid #344653;
  vertical-align:top
}
th{color:#71d6b0}
button{
  padding:11px 16px;border:0;border-radius:8px;
  background:#71d6b0;color:#10251e;font-weight:bold;
  cursor:pointer
}
.notice{
  padding:12px;border:1px solid #b68d46;
  border-radius:8px;color:#f0d59c
}
</style>
</head>

<body>
<main>
<h1>APRS ESP32 Web Monitor</h1>
<p>Versao 2.0.0 - Historico de pacotes</p>

<div class="notice">
  MODO DE DEMONSTRACAO: todos os pacotes abaixo
  sao simulados pelo ESP32. Nenhum pacote real foi recebido.
</div>

<div class="grid">
  <div class="card">
    <div class="muted">Estado</div>
    <div class="value" id="state">Carregando...</div>
  </div>

  <div class="card">
    <div class="muted">Total de registros</div>
    <div class="value" id="total">--</div>
  </div>

  <div class="card">
    <div class="muted">Historico armazenado</div>
    <div class="value" id="stored">--</div>
  </div>

  <div class="card">
    <div class="muted">Tempo ligado</div>
    <div class="value" id="uptime">--</div>
  </div>
</div>

<div class="card">
  <div class="muted">Endereco IP</div>
  <div class="value" id="ip">--</div>
  <p class="muted">Clientes Wi-Fi: <span id="clients">--</span></p>
</div>

<div class="card">
  <h2>Ultimos pacotes</h2>
  <button onclick="refreshData()">Atualizar agora</button>
  <p class="muted" id="updated">Aguardando consulta...</p>

  <div class="table-wrap">
    <table>
      <thead>
        <tr>
          <th>Indicativo</th>
          <th>Tipo</th>
          <th>Caminho</th>
          <th>Mensagem</th>
          <th>Idade</th>
        </tr>
      </thead>
      <tbody id="packets">
        <tr><td colspan="5">Carregando...</td></tr>
      </tbody>
    </table>
  </div>
</div>

<p><small>Atualizacao automatica a cada 3 segundos.</small></p>

<script>
async function getJson(url){
  const response = await fetch(url, {cache:'no-store'});
  if(!response.ok) throw new Error('HTTP '+response.status);
  return await response.json();
}

async function refreshData(){
  try{
    const [status, data] = await Promise.all([
      getJson('/api/status'),
      getJson('/api/packets')
    ]);

    document.getElementById('state').textContent = status.state;
    document.getElementById('total').textContent = status.total;
    document.getElementById('stored').textContent =
      data.packets.length + ' / ' + data.capacity;
    document.getElementById('uptime').textContent = status.uptime;
    document.getElementById('ip').textContent = status.ip;
    document.getElementById('clients').textContent = status.clients;

    const tbody = document.getElementById('packets');
    tbody.replaceChildren();

    if(data.packets.length === 0){
      const tr = document.createElement('tr');
      const td = document.createElement('td');
      td.colSpan = 5;
      td.textContent = 'Nenhum registro disponivel';
      tr.appendChild(td);
      tbody.appendChild(tr);
    }

    data.packets.forEach(packet => {
      const tr = document.createElement('tr');

      [
        packet.callsign,
        packet.type,
        packet.path,
        packet.message,
        packet.age + ' s'
      ].forEach(value => {
        const td = document.createElement('td');
        td.textContent = value;
        tr.appendChild(td);
      });

      tbody.appendChild(tr);
    });

    document.getElementById('updated').textContent =
      'Ultima atualizacao: ' + new Date().toLocaleTimeString();

  }catch(error){
    document.getElementById('state').textContent =
      'Falha de comunicacao';
    document.getElementById('updated').textContent =
      'Verifique a conexao com o ESP32.';
  }
}

refreshData();
setInterval(refreshData, 3000);
</script>
</main>
</body>
</html>
)rawliteral";

// =====================================================
// API JSON
// =====================================================

void handleRoot() {
  server.send_P(
    200,
    "text/html; charset=utf-8",
    PAGE_HTML
  );
}

void handleStatus() {
  String json;
  json.reserve(200);

  json = "{";
  json += "\"state\":\"AP ativo - simulacao\",";
  json += "\"uptime\":\"" + getUptime() + "\",";
  json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"total\":" + String(totalPacketsReceived) + ",";
  json += "\"stored\":" + String(packetCount) + ",";
  json += "\"capacity\":" + String(MAX_PACKETS);
  json += "}";

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}

void handlePackets() {
  String json;
  json.reserve(1800);

  json = "{";
  json += "\"source\":\"simulation\",";
  json += "\"capacity\":" + String(MAX_PACKETS) + ",";
  json += "\"total\":" + String(totalPacketsReceived) + ",";
  json += "\"packets\":[";

  for (int i = 0; i < packetCount; i++) {
    AprsPacket& packet = getNewestPacket(i);

    if (i > 0) {
      json += ",";
    }

    unsigned long age =
      (millis() - packet.receivedAt) / 1000;

    json += "{";
    json += "\"callsign\":\"";
    json += jsonEscape(packet.callsign);
    json += "\",";

    json += "\"path\":\"";
    json += jsonEscape(packet.path);
    json += "\",";

    json += "\"type\":\"";
    json += jsonEscape(packet.type);
    json += "\",";

    json += "\"message\":\"";
    json += jsonEscape(packet.message);
    json += "\",";

    json += "\"age\":" + String(age);
    json += "}";
  }

  json += "]}";

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}

void handleNotFound() {
  server.send(
    404,
    "text/plain; charset=utf-8",
    "Rota nao encontrada"
  );
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {
    Serial.println("Erro ao inicializar OLED");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("APRS Monitor V2");
    display.println("Inicializando...");
    display.display();
  }

  WiFi.mode(WIFI_AP);

  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Falha ao iniciar Access Point");
  }

  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/packets", HTTP_GET, handlePackets);
  server.onNotFound(handleNotFound);

  server.begin();

  initializeSampleData();

  Serial.println("Servidor HTTP iniciado");
  Serial.println("Modo de dados: SIMULACAO");
  Serial.println("Acesse http://192.168.4.1");

  updateDisplay();
}

// =====================================================
// Loop
// =====================================================

void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (now - lastSimulation >= SIMULATION_INTERVAL) {
    lastSimulation = now;
    simulatePacket();
  }

  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    lastDisplayUpdate = now;
    updateDisplay();
  }
}
