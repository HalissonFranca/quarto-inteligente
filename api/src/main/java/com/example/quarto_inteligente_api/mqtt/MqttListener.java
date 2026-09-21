package com.example.quarto_inteligente_api.mqtt;

import com.example.quarto_inteligente_api.model.Leitura;
import com.example.quarto_inteligente_api.repository.LeituraRepository;
import jakarta.annotation.PostConstruct;
import tools.jackson.databind.json.JsonMapper;

import org.eclipse.paho.client.mqttv3.*;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.time.LocalDateTime;

@Component
public class MqttListener implements MqttCallback {

    private final LeituraRepository repository;
    private final JsonMapper jsonMapper = JsonMapper.builder().build();

    @Value("${mqtt.username}")
    private String mqttUsername;

    @Value("${mqtt.password}")
    private String mqttPassword;

    @Value("${mqtt.broker-url}")
    private String brokerUrl;

    @Value("${mqtt.topic}")
    private String topico;

    @Value("${mqtt.client-id}")
    private String clientId;

    // Injeção de dependência via construtor, igual fizemos no Controller
    public MqttListener(LeituraRepository repository) {
        this.repository = repository;
    }

    @PostConstruct
    public void conectar() {
        try {
            MqttClient client = new MqttClient(brokerUrl, clientId);
            client.setCallback(this);

            MqttConnectOptions options = new MqttConnectOptions();
            options.setUserName(mqttUsername);
            options.setPassword(mqttPassword.toCharArray());
            options.setCleanSession(true);
            options.setAutomaticReconnect(true);

            client.connect(options);
            client.subscribe(topico);

            System.out.println("Inscrito no tópico MQTT: " + topico);

        } catch (MqttException e) {
            System.err.println("Erro ao conectar no broker MQTT: " + e.getMessage());
        }
    }

    @Override
    public void messageArrived(String topic, MqttMessage message) throws Exception {
        String payload = new String(message.getPayload());
        System.out.println("Mensagem recebida em [" + topic + "]: " + payload);

        Leitura leitura = jsonMapper.readValue(payload, Leitura.class);
        leitura.setDataHora(LocalDateTime.now());

        repository.save(leitura);
    }

    @Override
    public void connectionLost(Throwable cause) {
        System.err.println("Conexão MQTT perdida: " + cause.getMessage());
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // Não utilizado, pois a API apenas consome mensagens, não publica.
    }
}