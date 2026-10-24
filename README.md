# APRS ESP32 Web Monitor

Projeto de aprendizado e desenvolvimento de um monitor APRS utilizando ESP32, display OLED SSD1306 e interface web. O objetivo é evoluir gradualmente de um painel local para um sistema capaz de receber e apresentar informações reais da rede APRS-IS.

O desenvolvimento é organizado por versões, mantendo o firmware e a documentação de cada etapa.

## Objetivos do projeto

- Desenvolver uma interface web hospedada no ESP32.
- Exibir informações do sistema em um display OLED SSD1306.
- Aprender a trabalhar com Wi-Fi, TCP/IP, APIs HTTP e JSON.
- Simular pacotes APRS e, posteriormente, receber dados reais.
- Explorar possibilidades de integração futura com GPS e rádio amador.
- Praticar organização de código, documentação e versionamento com Git e GitHub.

## Hardware

- ESP32.
- Display OLED SSD1306 I2C, resolução 128 × 64 pixels.
- Computador com Arduino IDE para desenvolvimento e gravação.

### Ligações do OLED

| OLED SSD1306 | ESP32 |
|---|---|
| VCC | Conforme a especificação do módulo |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

O endereço I2C configurado no firmware é `0x3C`.

## Tecnologias e bibliotecas

- Arduino C++.
- Arduino IDE.
- ESP32 Arduino Core.
- Wi-Fi e TCP/IP.
- Servidor HTTP e APIs JSON.
- HTML, CSS e JavaScript.
- Adafruit SSD1306.
- Adafruit GFX Library.
- Git e GitHub.

As bibliotecas `WiFi.h`, `WiFiClient.h`, `WebServer.h` e `Wire.h` são fornecidas pelo pacote de placas ESP32.

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
│   └── V3/
│       └── APRS_WebMonitor_V3/
│           └── APRS_WebMonitor_V3.ino
├── docs/
│   ├── v1.md
│   ├── v2.md
│   └── v3.md
└── README.md
```

Cada versão possui seu próprio diretório de firmware e documentação, permitindo consultar e recuperar implementações anteriores.

## Versões do projeto

### V1.0.0 — Painel web e OLED

**Objetivo:** criar a estrutura inicial do sistema.

Funcionalidades:

- Ponto de acesso Wi-Fi criado pelo ESP32.
- Servidor web HTTP local.
- Interface web de monitoramento.
- API de estado do sistema.
- Exibição de informações básicas no OLED.
- Monitoramento do tempo de funcionamento e dos dispositivos conectados.

Configuração inicial:

- SSID: `APRS-ESP32-V1`
- Senha: `aprs12345`
- Endereço do painel: `http://192.168.4.1`

Documentação: [`docs/v1.md`](docs/v1.md).

### V2.0.0 — Simulação e histórico de pacotes

**Objetivo:** desenvolver a estrutura de armazenamento e visualização de pacotes semelhantes aos utilizados no APRS.

Funcionalidades:

- Estrutura de dados para pacotes.
- Histórico circular limitado a 10 registros.
- Dados de demonstração e geração periódica de pacotes simulados.
- Painel web com atualização automática.
- API JSON para consultar estado e histórico.
- Exibição do último indicativo e da quantidade de registros no OLED.

Configuração inicial:

- SSID: `APRS-ESP32-V2`
- Senha: `aprs12345`
- Endereço do painel: `http://192.168.4.1`

**Importante:** a V2 simula pacotes. Ela não recebe dados reais de rádio nem da rede APRS-IS.

Documentação: [`docs/v2.md`](docs/v2.md).

### V3.0.0 — Recepção APRS-IS pela internet

**Objetivo:** conectar o ESP32 à rede APRS-IS e receber linhas reais de pacotes pela internet.

Funcionalidades:

- Wi-Fi em modo Station para acessar a internet.
- Ponto de acesso próprio para continuar acessando o painel.
- Conexão TCP com APRS-IS.
- Login de recepção utilizando `pass -1`.
- Filtro APRS-IS configurável.
- Histórico circular dos últimos 10 pacotes recebidos.
- API JSON para estado e histórico.
- Painel web com atualização automática.
- Informações de conexão no OLED.
- Tentativas de reconexão após falhas.

Configuração do ponto de acesso:

- SSID: `APRS-ESP32-V3`
- Senha: `aprs12345`
- Endereço do painel: `http://192.168.4.1`

Configuração APRS-IS utilizada:

- Servidor: `rotate.aprs2.net`
- Porta: `14580`
- Senha de recepção: `-1`
- Filtro inicial: `t/p`

O servidor e o filtro podem ser alterados no firmware conforme a disponibilidade do serviço e as necessidades do projeto.

#### Endpoints da API

| Endpoint | Descrição |
|---|---|
| `/` | Painel web |
| `/api/status` | Estado do Wi-Fi, APRS-IS, contadores e tempo ligado |
| `/api/packets` | Histórico dos pacotes recebidos |

#### Limitações da V3

- Recebe dados pela internet, não diretamente pelo Baofeng UV-5R.
- Não transmite pacotes para APRS-IS.
- Armazena as linhas recebidas como texto bruto.
- Ainda não interpreta coordenadas, símbolos e demais campos APRS em estruturas específicas.
- O histórico fica na RAM e é perdido quando o ESP32 reinicia.
- A conexão TCP desta implementação não utiliza TLS.

Documentação: [`docs/v3.md`](docs/v3.md).

## Roadmap

- [x] V1 — Painel web e display OLED.
- [x] V2 — Simulação e histórico de pacotes.
- [x] V3 — Recepção APRS-IS pela internet.
- [ ] V4 — Interpretação estruturada dos pacotes APRS.
- [ ] Versões futuras — Exibição de coordenadas e informações de estações.
- [ ] Versões futuras — Avaliação da integração com GPS.
- [ ] Versões futuras — Pesquisa e desenvolvimento de integração com rádio amador ou TNC.

O roadmap representa a direção planejada, não funcionalidades já implementadas.

## Instalação e execução

### 1. Preparar o Arduino IDE

Instale o pacote de placas ESP32 e as bibliotecas:

- Adafruit SSD1306.
- Adafruit GFX Library.

### 2. Abrir o firmware

Escolha o arquivo `.ino` correspondente à versão desejada dentro de `firmware/`.

### 3. Configurar a versão

Na V1 e na V2, conecte-se ao ponto de acesso criado pelo ESP32.

Na V3, configure as credenciais da rede Wi-Fi no firmware para permitir o acesso à internet. O ESP32 também cria seu próprio ponto de acesso.

**Não publique credenciais pessoais de Wi-Fi em um repositório público.** Antes de executar `git add`, confira as alterações e certifique-se de que o código contém apenas valores de demonstração.

### 4. Compilar e gravar

Selecione a placa ESP32 correta, escolha a porta serial e grave o firmware.

### 5. Acessar o painel

Na V3, conecte o computador ou celular à rede `APRS-ESP32-V3` e acesse:

`http://192.168.4.1`

Também é possível acessar o painel pelo endereço IP do ESP32 na rede doméstica, desde que o dispositivo cliente consiga alcançá-lo.

## Versionamento com Git

O projeto utiliza Git para registrar as alterações e tags para identificar versões estáveis.

### Convenção de commits

| Prefixo | Utilização | Exemplo |
|---|---|---|
| `feat:` | Nova funcionalidade | `feat: add APRS-IS reception` |
| `fix:` | Correção de bug | `fix: correct packet history in V2` |
| `docs:` | Documentação | `docs: update V3 instructions` |
| `refactor:` | Reorganização do código | `refactor: simplify packet handling` |
| `chore:` | Manutenção geral | `chore: update project configuration` |

### Fluxo normal de desenvolvimento

Confira as alterações:

```bash
git status
git diff
git diff --check
```

Adicione os arquivos modificados:

```bash
git add caminho/do/arquivo
```

Registre o commit:

```bash
git commit -m "fix: describe the correction"
```

Envie para o GitHub:

```bash
git push origin main
```

Substitua o caminho e a mensagem pelos dados da alteração real.

### Criar uma nova versão estável

Depois de testar a implementação, crie uma tag anotada. Por exemplo, para uma versão `v3.0.1`:

```bash
git tag -a v3.0.1 -m "APRS Web Monitor V3.0.1"
git push origin v3.0.1
```

Use um número de versão que ainda não exista. As tags publicadas devem identificar commits específicos e não devem ser reutilizadas para apontar para código diferente.

## Manutenção de versões antigas

Como o desenvolvimento principal ocorre na `main`, correções de versões anteriores devem ser feitas em branches de manutenção quando não se deseja incluir funcionalidades mais recentes.

Por exemplo, para trabalhar na V2 a partir da tag `v2.0.0`:

```bash
git switch -c maintenance/v2 v2.0.0
```

Depois de corrigir e testar o firmware:

```bash
git add firmware/V2/APRS_WebMonitor_V2/APRS_WebMonitor_V2.ino
git commit -m "fix: correct packet history in V2"
git tag -a v2.0.1 -m "APRS Web Monitor V2.0.1"
git push -u origin maintenance/v2
git push origin v2.0.1
```

Esse fluxo mantém a correção da V2 separada do desenvolvimento da V3 na `main`.

Se a branch de manutenção já existir, utilize-a em vez de criar outra com o mesmo nome. Caso a documentação da V2 também seja alterada, inclua `docs/v2.md` no commit.

## Objetivos educacionais

Este projeto é desenvolvido como um laboratório prático para estudar:

- Programação embarcada com ESP32.
- Redes Wi-Fi e comunicação TCP/IP.
- Servidores HTTP e APIs JSON.
- Desenvolvimento de interfaces web embarcadas.
- Comunicação I2C com displays OLED.
- Protocolo APRS e acesso à rede APRS-IS.
- Organização de código e documentação técnica.
- Controle de versão com Git e GitHub.

## Segurança e uso responsável

- Não publique senhas de redes Wi-Fi.
- Use identificações e credenciais compatíveis com as regras de acesso ao APRS-IS.
- Mantenha a recepção separada de qualquer implementação de transmissão.
- Observe as normas aplicáveis ao serviço de radioamador antes de desenvolver funcionalidades que envolvam transmissão por rádio.

## Licença

A licença do projeto ainda deve ser definida antes de conceder permissões formais de reutilização e redistribuição. Até que uma licença seja escolhida e adicionada ao repositório, não presuma que o código esteja liberado sob uma licença de código aberto.

---

Projeto desenvolvido de forma incremental para fins educacionais e experimentais.