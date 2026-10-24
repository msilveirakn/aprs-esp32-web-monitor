## V3 — Integração com APRS-IS

A V3 conecta o ESP32 à rede APRS-IS pela internet, recebendo linhas reais de pacotes através de TCP.

### Funcionalidades

- Wi-Fi doméstico para acesso à internet.
- Ponto de acesso do ESP32 para acessar o painel local.
- Recepção APRS-IS com filtro de pacotes de posição.
- Histórico circular dos últimos 10 pacotes.
- Painel web com atualização automática.
- API JSON de status e histórico.
- Exibição do estado da conexão no OLED SSD1306.
- Reconexão automática quando a conexão cai.

### Endpoints

| Endpoint | Função |
|---|---|
| `/` | Painel web |
| `/api/status` | Status do sistema e da conexão |
| `/api/packets` | Histórico dos pacotes recebidos |

### Configuração

Configure `WIFI_SSID` e `WIFI_PASSWORD` no arquivo do firmware antes de compilar.

A conexão APRS-IS utiliza `soam.aprs2.net:14580`, login somente de recepção com `pass -1` e filtro `t/p`.

**Importante:** esta versão recebe dados pela internet. Ela não recebe diretamente sinais do Baofeng UV-5R e não transmite pacotes para APRS-IS.

Consulte [`docs/v3.md`](docs/v3.md) para instruções de instalação, testes e limitações.

### Roadmap

- [x] V1 — Painel web e OLED
- [x] V2 — Simulação e histórico de pacotes
- [x] V3 — Recepção APRS-IS pela internet
- [ ] V4 — Decodificação estruturada dos pacotes recebidos
- [ ] Versões futuras — GPS e integração com rádio/TNC
