#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "secrets.h"
#include "certs.h"

// ---------- Configurações do sensor ----------
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------- Configurações do display OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define PINO_SDA 21
#define PINO_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Posição e tamanho dos olhos (aumentados para melhor visibilidade)
const int OLHO_ESQ_X = 34;
const int OLHO_DIR_X = 94;
const int OLHO_Y = 20;
const int RAIO_OLHO = 15;
const int RAIO_PUPILA = 5;

// ---------- Intervalo entre leituras/envios do sensor ----------
const unsigned long INTERVALO_MS = 300000;
unsigned long ultimoEnvio = 0;

float ultimaTemperatura = 25.0;
float ultimaUmidade = 50.0;

// ---------- Estado da animação "idle" (vida do rosto) ----------
bool piscando = false;
unsigned long fimPiscada = 0;
unsigned long proximaPiscada = 0;

int olharDeslocamento = 0;
unsigned long proximaMudancaOlhar = 0;

bool bocejando = false;
unsigned long fimBocejo = 0;
unsigned long proximoBocejo = 0;

unsigned long proximoRedraw = 0;
const unsigned long INTERVALO_REDRAW_MS = 150;

// ---------- Objetos do MQTT ----------
WiFiClientSecure espClient;
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

void conectarMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Conectando ao broker MQTT (TLS)...");
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD))
        {
            Serial.println(" conectado!");
        }
        else
        {
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
    if (!mqttClient.publish(MQTT_TOPIC, payload))
    {
        Serial.println("Falha ao publicar mensagem!");
    }
}

// ---------- Animação "idle": decide quando piscar, olhar e bocejar ----------
void atualizarAnimacaoIdle()
{
    unsigned long agora = millis();

    if (piscando && agora >= fimPiscada)
    {
        piscando = false;
        proximaPiscada = agora + random(2000, 6000);
    }
    else if (!piscando && agora >= proximaPiscada && proximaPiscada != 0)
    {
        piscando = true;
        fimPiscada = agora + 150;
    }
    if (proximaPiscada == 0)
    {
        proximaPiscada = agora + random(2000, 6000);
    }

    if (agora >= proximaMudancaOlhar)
    {
        int opcoes[3] = {-5, 0, 5};
        olharDeslocamento = opcoes[random(0, 3)];
        unsigned long duracao = (olharDeslocamento == 0) ? random(2000, 5000) : random(800, 1800);
        proximaMudancaOlhar = agora + duracao;
    }

    if (bocejando && agora >= fimBocejo)
    {
        bocejando = false;
        proximoBocejo = agora + random(15000, 30000);
    }
    else if (!bocejando && agora >= proximoBocejo && proximoBocejo != 0)
    {
        bocejando = true;
        fimBocejo = agora + 700;
    }
    if (proximoBocejo == 0)
    {
        proximoBocejo = agora + random(15000, 30000);
    }
}

void desenharOlhos()
{
    if (piscando)
    {
        // Pisca: duas linhas grossas (2px de altura) no lugar do círculo
        display.fillRect(OLHO_ESQ_X - RAIO_OLHO, OLHO_Y - 1, RAIO_OLHO * 2, 3, SSD1306_WHITE);
        display.fillRect(OLHO_DIR_X - RAIO_OLHO, OLHO_Y - 1, RAIO_OLHO * 2, 3, SSD1306_WHITE);
        return;
    }

    display.fillCircle(OLHO_ESQ_X, OLHO_Y, RAIO_OLHO, SSD1306_WHITE);
    display.fillCircle(OLHO_DIR_X, OLHO_Y, RAIO_OLHO, SSD1306_WHITE);

    display.fillCircle(OLHO_ESQ_X + olharDeslocamento, OLHO_Y, RAIO_PUPILA, SSD1306_BLACK);
    display.fillCircle(OLHO_DIR_X + olharDeslocamento, OLHO_Y, RAIO_PUPILA, SSD1306_BLACK);
}

void desenharGota(int x, int y)
{
    display.fillCircle(x, y + 4, 4, SSD1306_WHITE);
    display.fillTriangle(x - 4, y + 4, x + 4, y + 4, x, y - 5, SSD1306_WHITE);
}

void desenharBoca(float temperatura)
{
    if (bocejando)
    {
        display.fillRoundRect(48, 38, 32, 20, 6, SSD1306_WHITE);
        return;
    }

    if (temperatura < 18.0)
    {
        // "o" de frio, com traço mais grosso (dois círculos concêntricos)
        display.drawCircle(64, 48, 10, SSD1306_WHITE);
        display.drawCircle(64, 48, 9, SSD1306_WHITE);
    }
    else if (temperatura <= 28.0)
    {
        // Sorriso largo, desenhado em duas camadas para parecer mais grosso
        for (int x = 36; x <= 92; x++)
        {
            int y = 40 + (int)(11 * sin((x - 36) * 3.14 / 56));
            display.drawPixel(x, y, SSD1306_WHITE);
            display.drawPixel(x, y + 1, SSD1306_WHITE);
        }
    }
    else
    {
        // Calor: boca reta e neutra (grossa) + gotas de suor maiores
        display.fillRect(44, 46, 40, 3, SSD1306_WHITE);
        desenharGota(108, 6);
        desenharGota(118, 18);
    }
}

void desenharRosto(float temperatura)
{
    display.clearDisplay();

    desenharOlhos();
    desenharBoca(temperatura);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print(temperatura, 1);
    display.print((char)247);
    display.print("C  ");
    display.print(ultimaUmidade, 0);
    display.print("%");

    display.display();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    randomSeed(analogRead(0));

    dht.begin();

    Wire.begin(PINO_SDA, PINO_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println("Falha ao iniciar o display OLED!");
    }
    else
    {
        display.clearDisplay();
        display.display();
    }

    conectarWiFi();

    espClient.setCACert(ROOT_CA_CERT);
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        conectarWiFi();
    }

    if (!mqttClient.connected())
    {
        conectarMQTT();
    }

    mqttClient.loop();

    unsigned long agora = millis();

    if (agora - ultimoEnvio >= INTERVALO_MS)
    {
        ultimoEnvio = agora;

        float umidade = dht.readHumidity();
        float temperatura = dht.readTemperature();

        if (!isnan(umidade) && !isnan(temperatura))
        {
            ultimaTemperatura = temperatura;
            ultimaUmidade = umidade;

            Serial.printf("Temperatura: %.1f C | Umidade: %.1f%%\n", temperatura, umidade);
            publicarLeitura(temperatura, umidade);
        }
        else
        {
            Serial.println("Falha ao ler o sensor DHT22!");
        }
    }

    atualizarAnimacaoIdle();

    if (agora - proximoRedraw >= INTERVALO_REDRAW_MS || proximoRedraw == 0)
    {
        proximoRedraw = agora;
        desenharRosto(ultimaTemperatura);
    }
}