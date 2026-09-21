# Sistema de Monitoramento e Automação Residencial

Sistema de monitoramento ambiental (temperatura e umidade) com ESP32, comunicação via MQTT, e API em Spring Boot, desenvolvido como projeto de estudo aplicado em IoT e desenvolvimento backend. Preparado para evoluir futuramente para automação residencial (tomada inteligente e iluminação).

## Arquitetura

```
ESP32 (sensor) --[MQTT]--> Broker Mosquitto --[MQTT]--> API Spring Boot --> PostgreSQL
```

A ESP32 publica as leituras de temperatura e umidade em um tópico MQTT. A API Spring Boot assina esse tópico, recebe as mensagens de forma assíncrona e as persiste no banco de dados, disponibilizando-as através de um endpoint REST.

## Estrutura do repositório

- [`firmware/`](./firmware) — Código-fonte da ESP32 (PlatformIO), responsável pela leitura do sensor DHT22 e publicação via MQTT.
- [`api/`](./api) — API REST em Spring Boot, responsável por assinar o broker MQTT, persistir os dados em PostgreSQL e expor o histórico de leituras.
- [`docs/`](./docs) — Documentação completa do projeto: escopo, arquitetura, decisões técnicas e relatório de erros/soluções enfrentados durante o desenvolvimento.

## Tecnologias

| Categoria | Tecnologia |
|---|---|
| Hardware | ESP32 + Sensor DHT22 |
| Firmware | C++ (Arduino Framework), PlatformIO, PubSubClient, ArduinoJson |
| Mensageria | MQTT (Eclipse Mosquitto) |
| Backend | Java, Spring Boot 4, Spring Data JPA, Eclipse Paho |
| Banco de Dados | PostgreSQL |

## Status do projeto

Em desenvolvimento ativo. Veja o [documento de especificação](./docs) para o roadmap completo e o histórico detalhado de decisões técnicas.

## Autor

Desenvolvido por Halisson como projeto pessoal de estudo em IoT e desenvolvimento backend.
