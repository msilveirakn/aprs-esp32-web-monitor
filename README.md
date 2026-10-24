# APRS ESP32 Web Monitor

Painel de monitoramento experimental para projetos APRS (*Automatic Packet Reporting System*), desenvolvido com ESP32 e display OLED SSD1306, utilizando a Arduino IDE.

O objetivo do projeto é construir, passo a passo, uma plataforma de monitoramento APRS que futuramente poderá integrar dados recebidos pela internet (APRS-IS) e explorar a comunicação via rádio com um Baofeng UV-5R.

## Funcionalidades atuais

A versão V1 implementa:

- Access Point Wi-Fi próprio utilizando o ESP32.
- Servidor web HTTP hospedado localmente.
- Painel web responsivo para celular e computador.
- API HTTP para consulta do estado do sistema.
- Exibição do endereço IP e quantidade de clientes conectados.
- Contador de tempo de funcionamento.
- Interface OLED SSD1306 de 128 × 64 pixels.
- Atualização automática dos dados do painel a cada 3 segundos.

**Importante:** a V1 é uma base de interface e monitoramento. Ainda não recebe pacotes APRS reais, não se conecta ao APRS-IS e não transmite sinais de rádio.

## Hardware

- ESP32 DevKit ou placa ESP32 compatível.
- Display OLED SSD1306 com interface I²C, resolução de 128 × 64 pixels.
- Cabo USB para programação e alimentação.

O Baofeng UV-5R será utilizado em etapas futuras, quando o projeto avançar para a integração com comunicação APRS por rádio.

## Ligações do OLED

Configuração padrão para ESP32 DevKit clássico:

| OLED SSD1306 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

Verifique a tensão admitida pelo seu módulo OLED antes de conectá-lo.

O endereço I²C padrão configurado no código é `0x3C`. Alguns módulos utilizam `0x3D`.

## Tecnologias e bibliotecas

- C++ para Arduino.
- Arduino IDE.
- ESP32 Arduino Core.
- Wi-Fi SoftAP.
- Servidor HTTP com `WebServer.h`.
- `Adafruit SSD1306`.
- `Adafruit GFX Library`.

As bibliotecas `WiFi.h`, `WebServer.h` e `Wire.h` são fornecidas pelo ambiente de desenvolvimento ESP32.

## Estrutura do projeto

```text
aprs-esp32-web-monitor/
├── firmware/
│   └── V1/
│       └── APRS_WebMonitor_V1/
│           └── APRS_WebMonitor_V1.ino
├── docs/
│   └── v1.md
└── README.md
```

## Instalação e configuração

### 1. Preparar a Arduino IDE

Instale a Arduino IDE e configure o suporte para placas ESP32 da Espressif.

Selecione a placa correspondente ao seu hardware e a porta serial correta.

### 2. Instalar as bibliotecas

Abra o Gerenciador de Bibliotecas da Arduino IDE e instale:

- `Adafruit SSD1306`
- `Adafruit GFX Library`

### 3. Carregar o firmware

Abra o arquivo:

`firmware/V1/APRS_WebMonitor_V1/APRS_WebMonitor_V1.ino`

Compile o código e carregue o firmware no ESP32.

### 4. Conectar à rede Wi-Fi

Após iniciar, o ESP32 criará uma rede Wi-Fi própria.

| Parâmetro | Valor padrão |
|---|---|
| SSID | `APRS-ESP32-V1` |
| Senha | `aprs12345` |
| Endereço IP | `192.168.4.1` |
| Porta HTTP | `80` |

Conecte o celular ou computador à rede criada pelo ESP32. Caso o dispositivo informe que a rede não possui acesso à internet, permaneça conectado.

### 5. Abrir o painel web

Acesse pelo navegador:

http://192.168.4.1

O painel exibirá o estado do sistema, o tempo ligado, o endereço IP e o número de clientes conectados.

## API HTTP

A V1 disponibiliza os seguintes endpoints:

| Endpoint | Método | Descrição |
|---|---|---|
| `/` | GET | Página principal do painel web |
| `/api/status` | GET | Retorna os dados de estado do ESP32 |

Exemplo de resposta de `/api/status`:

```json
{
  "state": "AP ativo",
  "uptime": "0d 00:05:32",
  "ip": "192.168.4.1",
  "clients": 1
}
```

Os valores são ilustrativos. A resposta real depende do estado do dispositivo no momento da consulta.

## Limitações da V1

Esta versão ainda não implementa:

- Conexão do ESP32 a uma rede Wi-Fi externa.
- Integração com APRS-IS.
- Recepção ou transmissão de pacotes APRS por rádio.
- Decodificação de áudio AFSK ou quadros AX.25.
- Rastreamento por GPS.
- Telemetria de sensores.
- Armazenamento persistente de pacotes.

O sistema utiliza dados locais de funcionamento para validar a interface e a comunicação HTTP.

## Roadmap

### V1.0.0 — Painel web básico

- [x] Access Point Wi-Fi.
- [x] Servidor HTTP.
- [x] Painel web responsivo.
- [x] API de estado.
- [x] Exibição no OLED SSD1306.
- [x] Documentação inicial.

### V2.0.0 — Estrutura de dados APRS

- [ ] Criar estruturas para representar pacotes APRS.
- [ ] Implementar dados simulados para testes.
- [ ] Organizar a lógica APRS em módulos separados.
- [ ] Exibir informações simuladas no painel e no OLED.

### V3.0.0 — Integração com APRS-IS

- [ ] Adicionar modo Wi-Fi cliente.
- [ ] Implementar conexão TCP/IP com APRS-IS.
- [ ] Interpretar linhas recebidas do servidor.
- [ ] Exibir informações de estações recebidas.
- [ ] Tratar falhas de conexão e reconexão.

### V4.0.0 — GPS e rastreamento

- [ ] Integrar módulo GPS/GNSS.
- [ ] Interpretar coordenadas e dados de localização.
- [ ] Exibir informações de posição.
- [ ] Preparar estrutura para relatórios APRS de posição.

### Versões futuras — Rádio e telemetria

- [ ] Explorar integração de áudio com o Baofeng UV-5R.
- [ ] Estudar modulação AFSK e protocolo AX.25.
- [ ] Implementar recepção e decodificação de pacotes com hardware compatível.
- [ ] Integrar sensores para telemetria.
- [ ] Avaliar transmissão APRS conforme os requisitos técnicos e regulatórios aplicáveis.

O roadmap é um plano de desenvolvimento e não significa que essas funcionalidades já estejam implementadas.

## Versionamento com Git

O projeto utiliza Git para controlar o histórico de desenvolvimento.

Convenção sugerida para commits:

- `feat:` novas funcionalidades.
- `fix:` correções.
- `docs:` documentação.
- `refactor:` reorganização do código sem alteração funcional.
- `test:` testes e validações.

Exemplos:

```bash
git add firmware/V1 docs/v1.md
git commit -m "feat: add APRS web monitor V1"

git add README.md
git commit -m "docs: add project README"
```

Para marcar uma versão:

```bash
git tag -a v1.0.0 -m "APRS Web Monitor V1"
git push origin main
git push origin v1.0.0
```

## Segurança

A senha Wi-Fi definida no código é apenas um valor padrão para testes. Altere-a antes de utilizar o dispositivo em ambientes compartilhados.

Não exponha credenciais, dados pessoais ou informações sensíveis no repositório público.

## Objetivo educacional

Este projeto tem como objetivo estudar sistemas embarcados, redes Wi-Fi, servidores HTTP, interfaces OLED, organização de firmware e, progressivamente, protocolos de comunicação utilizados no APRS.

A implementação é incremental: cada versão amplia as capacidades da anterior, mantendo o código organizado e o desenvolvimento documentado.

## Licença

...