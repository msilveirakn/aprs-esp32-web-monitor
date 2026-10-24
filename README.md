# APRS ESP32 Web Monitor

Painel experimental de monitoramento APRS (*Automatic Packet Reporting System*), desenvolvido com ESP32, display OLED SSD1306 e Arduino IDE.

O projeto evolui por versões incrementais, explorando sistemas embarcados, redes Wi-Fi, servidores HTTP, estruturas de dados, telemetria e protocolos utilizados no radioamadorismo.

O objetivo futuro é integrar dados do APRS-IS e explorar a comunicação via rádio com um Baofeng UV-5R, utilizando uma interface de áudio apropriada.

## Versão atual

**V2.0.0 — Simulação e histórico de pacotes**

A V2 amplia a interface desenvolvida na V1 com um modelo de dados para pacotes APRS, histórico circular e novas rotas de API.

> **Importante:** os pacotes exibidos na V2 são simulados. O projeto ainda não recebe dados reais do APRS-IS nem transmite ou recebe pacotes APRS por rádio.

## Funcionalidades

### V1.0.0 — Painel web básico

- Access Point Wi-Fi próprio.
- Servidor HTTP hospedado no ESP32.
- Painel web responsivo.
- Endpoint `/api/status`.
- Exibição do endereço IP e quantidade de clientes conectados.
- Contador de tempo de funcionamento.
- Interface OLED SSD1306 de 128 × 64 pixels.

### V2.0.0 — Modelo de dados e simulação

- Estrutura `AprsPacket` para representar registros.
- Histórico circular com capacidade para 10 pacotes.
- Geração de registros simulados a cada 15 segundos.
- Exibição dos registros mais recentes no painel web.
- Informações de indicativo, caminho, tipo e mensagem.
- Idade aproximada dos registros em segundos.
- Endpoint `/api/packets` para consultar o histórico em JSON.
- Estatísticas de registros no painel e no OLED.
- Atualização automática do painel a cada 3 segundos.
- Identificação explícita do modo de simulação.

## Hardware

- ESP32 DevKit ou placa ESP32 compatível.
- OLED SSD1306 com interface I²C e resolução de 128 × 64 pixels.
- Cabo USB para alimentação e programação.

O Baofeng UV-5R será utilizado em etapas futuras, após o desenvolvimento de uma interface adequada para comunicação digital por áudio.

## Ligações do OLED

Configuração de referência para um ESP32 DevKit clássico:

| OLED SSD1306 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

Verifique a tensão admitida pelo módulo OLED e os pinos I²C da sua placa antes de conectar os componentes.

O endereço I²C configurado no firmware é `0x3C`. Alguns módulos utilizam `0x3D`.

## Tecnologias e bibliotecas

- C++ para Arduino.
- Arduino IDE.
- ESP32 Arduino Core.
- Wi-Fi SoftAP.
- Servidor HTTP com `WebServer.h`.
- Comunicação I²C com `Wire.h`.
- `Adafruit SSD1306`.
- `Adafruit GFX Library`.

As bibliotecas `WiFi.h`, `WebServer.h` e `Wire.h` são fornecidas pelo ambiente de desenvolvimento ESP32.

## Estrutura do projeto

```text
aprs-esp32-web-monitor/
├── firmware/
│   ├── V1/
│   │   └── APRS_WebMonitor_V1/
│   │       └── APRS_WebMonitor_V1.ino
│   └── V2/
│       └── APRS_WebMonitor_V2/
│           └── APRS_WebMonitor_V2.ino
├── docs/
│   ├── v1.md
│   └── v2.md
└── README.md
```

Cada versão possui seu próprio firmware, permitindo manter o histórico de desenvolvimento e recuperar versões anteriores.

## Instalação

### 1. Preparar a Arduino IDE

Instale a Arduino IDE e configure o suporte para placas ESP32 da Espressif.

Selecione a placa correspondente ao seu hardware e a porta serial correta.

### 2. Instalar as bibliotecas

No Gerenciador de Bibliotecas, instale:

- `Adafruit SSD1306`
- `Adafruit GFX Library`

### 3. Selecionar a versão

Para executar a V1, abra:

`firmware/V1/APRS_WebMonitor_V1/APRS_WebMonitor_V1.ino`

Para executar a V2, abra:

`firmware/V2/APRS_WebMonitor_V2/APRS_WebMonitor_V2.ino`

Compile e carregue somente o firmware da versão desejada.

### 4. Conectar ao Access Point

A V1 e a V2 utilizam redes Wi-Fi independentes.

| Parâmetro | V1 | V2 |
|---|---|---|
| SSID | `APRS-ESP32-V1` | `APRS-ESP32-V2` |
| Senha padrão | `aprs12345` | `aprs12345` |
| IP padrão | `192.168.4.1` | `192.168.4.1` |

Conecte o celular ou computador à rede criada pelo ESP32. Se o dispositivo avisar que não há acesso à internet, permaneça conectado à rede.

### 5. Abrir o painel

Acesse no navegador:

http://192.168.4.1

## API HTTP

### Endpoint `/api/status`

Método: `GET`

Retorna informações sobre o estado do ESP32.

Na V2, a resposta inclui:

- Estado do Access Point.
- Tempo de funcionamento.
- Endereço IP.
- Quantidade de clientes conectados.
- Total de registros criados.
- Quantidade de registros armazenados.
- Capacidade máxima do histórico.

Exemplo ilustrativo:

```json
{
  "state": "AP ativo - simulacao",
  "uptime": "0d 00:05:32",
  "ip": "192.168.4.1",
  "clients": 1,
  "total": 5,
  "stored": 5,
  "capacity": 10
}
```

### Endpoint `/api/packets`

Método: `GET`

Disponível a partir da V2. Retorna o histórico de registros simulados em formato JSON.

Cada registro contém:

| Campo | Descrição |
|---|---|
| `callsign` | Indicativo de demonstração |
| `path` | Caminho de demonstração |
| `type` | Categoria do registro |
| `message` | Mensagem de demonstração |
| `age` | Idade aproximada em segundos |

A resposta também contém a origem dos dados, a capacidade do histórico e o total de registros criados.

Os valores dos exemplos são ilustrativos e não representam estações reais.

## Histórico de pacotes

A V2 utiliza um histórico circular com capacidade para 10 registros.

Quando a capacidade é atingida, o registro mais antigo é substituído pelo mais recente. A interface apresenta primeiro os registros mais novos.

O histórico fica armazenado na memória RAM e é perdido quando o ESP32 é reiniciado ou desligado. A V2 ainda não implementa armazenamento persistente.

Os registros simulados servem para validar a estrutura de dados, a API e a interface, mas não implementam a interpretação completa do protocolo APRS.

## Roadmap

### V1.0.0 — Painel web básico

- [x] Access Point Wi-Fi.
- [x] Servidor HTTP.
- [x] Painel web responsivo.
- [x] API de estado.
- [x] Exibição no OLED SSD1306.

### V2.0.0 — Estrutura e simulação APRS

- [x] Modelo de dados para registros.
- [x] Histórico circular.
- [x] Pacotes simulados.
- [x] API de histórico em JSON.
- [x] Exibição dos registros no painel.
- [x] Estatísticas no OLED.
- [x] Atualização automática.

### V3.0.0 — Integração com APRS-IS

- [ ] Adicionar modo Wi-Fi cliente.
- [ ] Conectar a uma rede com acesso à internet.
- [ ] Implementar conexão TCP/IP com um servidor APRS-IS.
- [ ] Receber e interpretar linhas de dados APRS.
- [ ] Exibir estações recebidas no painel e no OLED.
- [ ] Implementar tratamento de desconexões e reconexão.
- [ ] Separar a fonte de dados da lógica de apresentação.

### V4.0.0 — GPS e rastreamento

- [ ] Integrar um módulo GPS/GNSS.
- [ ] Interpretar coordenadas e dados de localização.
- [ ] Exibir informações de posição.
- [ ] Preparar a estrutura para relatórios APRS de posição.

### Versões futuras — Rádio e telemetria

- [ ] Explorar a interface de áudio com o Baofeng UV-5R.
- [ ] Estudar AFSK e quadros AX.25.
- [ ] Desenvolver recepção e decodificação de pacotes com hardware compatível.
- [ ] Integrar sensores para telemetria.
- [ ] Avaliar transmissão APRS conforme os requisitos técnicos e regulatórios aplicáveis.

Os itens do roadmap são planejados e não devem ser interpretados como funcionalidades já disponíveis.

## Versionamento com Git

O projeto utiliza Git para registrar alterações e manter as versões do firmware.

Convenção sugerida para mensagens de commit:

- `feat:` novas funcionalidades.
- `fix:` correções.
- `docs:` documentação.
- `refactor:` reorganização do código.
- `test:` testes e validações.

Exemplo de commit da V2:

```bash
git add firmware/V2/APRS_WebMonitor_V2/APRS_WebMonitor_V2.ino
git add docs/v2.md README.md
git commit -m "feat: add APRS packet simulation V2"
```

Para marcar a versão:

```bash
git tag -a v2.0.0 -m "APRS Web Monitor V2"
git push origin main
git push origin v2.0.0
```

Consulte as tags existentes antes de criar uma nova, para evitar duplicação.

## Segurança

A senha Wi-Fi padrão é destinada somente a testes. Altere-a antes de utilizar o dispositivo em ambientes compartilhados.

Não publique credenciais pessoais, senhas de redes externas ou outras informações sensíveis no repositório.

## Objetivo educacional

O projeto é desenvolvido incrementalmente para estudar:

- Sistemas embarcados com ESP32.
- Programação C++ para Arduino.
- Redes Wi-Fi e servidores HTTP.
- APIs REST-like e serialização JSON.
- Displays OLED e comunicação I²C.
- Estruturas de dados e histórico circular.
- Protocolos de comunicação utilizados no APRS.
- Integração entre internet e radioamadorismo.

## Licença

A licença do projeto ainda não foi definida. Uma licença de código aberto poderá ser adicionada posteriormente para esclarecer as condições de uso, modificação e distribuição.
