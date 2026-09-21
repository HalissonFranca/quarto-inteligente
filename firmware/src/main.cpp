#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#include "secrets.h" // copie de secrets.example.h e preencha os dados

// ---------- Configurações do sensor ----------
#define DHTPIN 4 // pino digital onde o DHT22 está ligado
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------- Intervalo entre leituras/envios ----------
const unsigned long INTERVALO_MS = 30000; // 30 segundos
unsigned long ultimoEnvio = 0;

// ---------- Objetos do MQTT ----------
// O WiFiClient cuida da conexão de rede "crua" (TCP).
// O PubSubClient usa esse WiFiClient por baixo dos panos para falar o protocolo MQTT.
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void conectarWiFi()
{
    Serial.print("Conectando ao WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());
}

// Tenta (re)conectar ao broker MQTT. Só retorna quando conseguir conectar.
void conectarMQTT()
{
    // Enquanto não estiver conectado ao broker, fica tentando
    while (!mqttClient.connected())
    {
        Serial.print("Conectando ao broker MQTT...");

        // client.connect(clientId) tenta abrir a conexão MQTT.
        // Retorna true se conseguiu, false se falhou.
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
        {
            Serial.println(" conectado!");
        }
        else
        {
            // client.state() retorna um código numérico indicando o motivo da falha
            Serial.print(" falhou, codigo=");
            Serial.print(mqttClient.state());
            Serial.println(" tentando de novo em 2 segundos");
            delay(2000);
        }
    }
}

void publicarLeitura(float temperatura, float umidade)
{
    JsonDocument doc;
    doc["temperatura"] = temperatura;
    doc["umidade"] = umidade;

    char payload[128];
    serializeJson(doc, payload);

    Serial.print("Publicando em ");
    Serial.print(MQTT_TOPIC);
    Serial.print(": ");
    Serial.println(payload);

    bool sucesso = mqttClient.publish(MQTT_TOPIC, payload);

    if (!sucesso)
    {
        Serial.println("Falha ao publicar mensagem!");
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    dht.begin();
    conectarWiFi();

    // Diz ao PubSubClient qual broker usar (endereço + porta)
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop()
{
    // Garante que WiFi e MQTT continuam conectados antes de qualquer outra coisa
    if (WiFi.status() != WL_CONNECTED)
    {
        conectarWiFi();
    }

    if (!mqttClient.connected())
    {
        conectarMQTT();
    }

    // ESSENCIAL: mantém a conexão MQTT viva e processa mensagens.
    // Precisa ser chamado constantemente, não só quando vamos publicar.
    mqttClient.loop();

    unsigned long agora = millis();

    if (agora - ultimoEnvio >= INTERVALO_MS)
    {
        ultimoEnvio = agora;

        float umidade = dht.readHumidity();
        float temperatura = dht.readTemperature();

        if (isnan(umidade) || isnan(temperatura))
        {
            Serial.println("Falha ao ler o sensor DHT22!");
            return;
        }

        Serial.printf("Temperatura: %.1f C | Umidade: %.1f%%\n", temperatura, umidade);
        publicarLeitura(temperatura, umidade);
    }
}