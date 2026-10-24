# APRS ESP32 Web Monitor

Projeto de aprendizado com ESP32, display OLED SSD1306 e uma interface web local para monitorar pacotes APRS recebidos pela internet por meio do serviço **APRS-IS**.

O firmware evolui em versões incrementais, começando por um servidor web local e avançando até a conexão ao APRS-IS e à interpretação de posições APRS não comprimidas.

> **Estado do projeto:** a V4 adiciona a interpretação de posições APRS não comprimidas ao monitor. Compile e teste o firmware no seu hardware antes de considerar a versão validada ou criar uma tag de lançamento.

## Objetivos

- Aprender a desenvolver aplicações IoT com ESP32.
- Disponibilizar uma interface web local para acompanhar o estado do dispositivo.
- Exibir informações básicas no display OLED.
- Receber linhas de dados do APRS-IS pela internet.
- Manter um histórico limitado de pacotes recebidos.
- Interpretar posições APRS não comprimidas e apresentar latitude e longitude em formato decimal.

## Hardware

- ESP32 compatível com Arduino IDE.
- Display OLED SSD1306 I2C, resolução 128 × 64, endereço I2C `0x3C`.
- Cabos de ligação.
- Rede Wi-Fi com acesso à internet para a conexão APRS-IS.

### Ligação do OLED

| OLED SSD1306 | ESP32 |
|---|---|
| VCC | Conforme a tensão aceita pelo módulo |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

Confira a tensão aceita pelo seu módulo OLED antes de alimentar o display.

## Software e bibliotecas

- Arduino IDE.
- Pacote de placas ESP32 da Espressif Systems.
- `Adafruit GFX Library`.
- `Adafruit SSD1306`.

As bibliotecas podem ser instaladas pelo gerenciador de bibliotecas da Arduino IDE.

## Versões

### V1 — Servidor web local

- Cria uma rede Wi-Fi própria (SoftAP).
- Disponibiliza uma página web local.
- Exibe informações básicas do sistema no OLED.
- Expõe o endpoint `/api/status`.

Rede de exemplo:
- SSID: `APRS-ESP32-V1`
- Senha: `aprs12345`
- Endereço do painel: `http://192.168.4.1`

Documentação: [`docs/v1.md`](docs/v1.md)

### V2 — Pacotes simulados

- Mantém o painel web local.
- Gera pacotes APRS simulados para testar a interface sem conexão ao APRS-IS.
- Mantém um histórico circular de até 10 pacotes.
- Disponibiliza os endpoints `/api/status` e `/api/packets`.
- Atualiza o painel periodicamente e exibe informações no OLED.

Rede de exemplo:
- SSID: `APRS-ESP32-V2`
- Senha: `aprs12345`
- Endereço do painel: `http://192.168.4.1`

**Importante:** os pacotes desta versão são simulados; não representam recepção real de rádio nem dados recebidos do APRS-IS.

Documentação: [`docs/v2.md`](docs/v2.md)

### V3 — Recepção APRS-IS

- Conecta o ESP32 à rede Wi-Fi doméstica (modo Station) para acessar a internet.
- Mantém uma rede Wi-Fi própria para acessar o painel local.
- Abre uma conexão TCP com um servidor APRS-IS.
- Envia a linha de login APRS-IS e recebe linhas de pacotes.
- Mantém um histórico limitado de pacotes recebidos.
- Exibe o estado da conexão e dados resumidos no painel e no OLED.

Configuração APRS-IS usada como referência:
- Servidor: `rotate.aprs2.net`
- Porta TCP: `14580`
- Filtro: `t/p`
- Senha APRS-IS: `-1` para login somente de recepção, quando aceito pelo servidor.

O indicativo `N0CALL` é apenas um valor de exemplo. Substitua-o pelo indicativo que você está autorizado a utilizar. A aceitação do login depende das regras do servidor APRS-IS; consulte a documentação do serviço se houver rejeição.

**Limitação:** a V3 recebe dados pelo APRS-IS via internet. Ela não recebe diretamente transmissões RF do Baofeng UV-5R.

Documentação: [`docs/v3.md`](docs/v3.md)

### V4 — Interpretação de posições APRS

A V4 amplia a V3 com interpretação básica de posições APRS **não comprimidas**.

- Mantém a conexão Wi-Fi, o servidor web local e a conexão TCP ao APRS-IS.
- Identifica o indicativo e o caminho do pacote.
- Classifica tipos básicos de pacote.
- Interpreta formatos comuns de posição não comprimida, com e sem timestamp.
- Converte latitude e longitude para graus decimais quando o pacote contém uma posição reconhecida.
- Mantém o conteúdo bruto do pacote para consulta.
- Apresenta os dados interpretados no painel e no endpoint `/api/packets`.
- Mantém o histórico limitado de pacotes.

Rede local de exemplo:
- SSID: `APRS-ESP32-V4`
- Senha: `aprs12345`
- Endereço do painel: `http://192.168.4.1`

A V4 não pretende interpretar todos os formatos APRS. Posições comprimidas, mensagens, telemetria e outros formatos avançados podem exigir implementações adicionais. A presença de coordenadas depende do formato recebido.

Documentação planejada: [`docs/v4.md`](docs/v4.md)

## Estrutura do repositório

```text
aprs-esp32-web-monitor/
├── firmware/
│   ├── V1/
│   │   └── APRS_WebMonitor_V1/
│   │       └── APRS_WebMonitor_V1.ino
│   ├── V2/
│   │   └── APRS_WebMonitor_V2/
│   │       └── APRS_WebMonitor_V2.ino
│   ├── V3/
│   │   └── APRS_WebMonitor_V3/
│   │       └── APRS_WebMonitor_V3.ino
│   └── V4/
│       └── APRS_WebMonitor_V4/
│           └── APRS_WebMonitor_V4.ino
├── docs/
│   ├── v1.md
│   ├── v2.md
│   ├── v3.md
│   └── v4.md
└── README.md
```

Se algum arquivo ainda não tiver sido criado ou enviado ao GitHub, ajuste a estrutura para refletir os arquivos realmente existentes no repositório.

## Configuração da V4

Abra o arquivo:

`firmware/V4/APRS_WebMonitor_V4/APRS_WebMonitor_V4.ino`

Configure as credenciais da rede Wi-Fi que fornece acesso à internet:

```cpp
const char* WIFI_SSID = "NOME_DA_SUA_REDE";
const char* WIFI_PASSWORD = "SENHA_DA_SUA_REDE";
```

Confira também o indicativo de chamada configurado para o APRS-IS. Não publique senhas pessoais ou outras credenciais privadas no repositório.

Após gravar o firmware:

1. Abra o Monitor Serial e confira a inicialização e as mensagens de conexão.
2. Conecte o computador ou celular à rede `APRS-ESP32-V4`.
3. Acesse `http://192.168.4.1`.
4. Confira o estado da conexão APRS-IS e os pacotes recebidos.
5. Verifique se pacotes que contêm posições não comprimidas apresentam latitude e longitude interpretadas.
6. Teste os endpoints `/api/status` e `/api/packets`.

O endereço local pode variar se a configuração de rede for alterada.

## Endpoints HTTP

| Endpoint | Finalidade |
|---|---|
| `/` | Painel web |
| `/api/status` | Estado resumido do sistema e das conexões |
| `/api/packets` | Histórico de pacotes e, na V4, campos interpretados disponíveis |

Os campos exatos do JSON podem variar conforme a versão do firmware. Consulte o código e a documentação da versão usada.

## Como funciona a conexão APRS-IS

O ESP32 usa a rede Wi-Fi doméstica para acessar um servidor APRS-IS por TCP. A rede criada pelo próprio ESP32 permite abrir o painel local. Assim, o dispositivo utiliza simultaneamente a conexão Wi-Fi de acesso à internet e o ponto de acesso local.

A porta TCP `14580` é comumente usada para conexões APRS-IS sem TLS. O projeto não deve ser tratado como uma implementação de comunicação segura por TLS.

O filtro `t/p` é um exemplo de filtro APRS-IS; consulte a documentação do APRS-IS para entender seu alcance e ajustar os dados recebidos.

## Limitações conhecidas

- A recepção de pacotes das versões V3 e V4 é via internet/APRS-IS, não diretamente por RF.
- O projeto não transforma o ESP32 em um transceptor.
- O firmware não substitui um TNC ou uma interface de rádio apropriada para receber APRS por RF.
- A V4 interpreta apenas formatos de posição não comprimida contemplados no parser.
- O histórico é limitado e mantido em memória; não é um banco de dados persistente.
- Reiniciar ou desligar o ESP32 apaga o histórico mantido em RAM.
- O painel só pode receber dados enquanto o firmware estiver em execução e as conexões necessárias estiverem funcionando.

## Versionamento e GitHub

As versões devem permanecer em pastas separadas para facilitar a comparação e evitar perder versões anteriores.

### Conferir as alterações

```bash
git status
git diff --check
git diff
```

### Adicionar os arquivos da V4

Depois de conferir e testar os arquivos:

```bash
git add README.md docs/v4.md firmware/V4/
git diff --cached
git commit -m "feat: add APRS position parsing in V4"
git push origin main
```

Se `docs/v4.md` ainda não existir, crie-o antes de executar `git add`.

### Criar uma tag de versão

Crie uma tag somente depois de compilar e testar o firmware e revisar a documentação. Confira primeiro as tags existentes:

```bash
git tag --list
```

Se `v4.0.0` ainda não existir e a versão estiver validada:

```bash
git tag -a v4.0.0 -m "APRS ESP32 Web Monitor V4"
git push origin v4.0.0
```

Não reutilize nem mova uma tag já publicada. Tags apontam para commits específicos; alterações posteriores no `main` não modificam o conteúdo de uma tag antiga.

## Próximas melhorias possíveis

- Suporte a posições APRS comprimidas.
- Interpretação de mensagens e telemetria.
- Melhorias de robustez no processamento de linhas TCP.
- Exibição de símbolos APRS e detalhes adicionais no painel.
- Armazenamento persistente de pacotes.
- Integração com um receptor de rádio/TNC apropriado para APRS por RF, em uma etapa separada.

## Aviso

Este projeto é educacional. Valide o firmware no hardware antes de publicar uma versão de lançamento. Respeite as regras aplicáveis ao uso de indicativos, radioamadorismo e serviços APRS.