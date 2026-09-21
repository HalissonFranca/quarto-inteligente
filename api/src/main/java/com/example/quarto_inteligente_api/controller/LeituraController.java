package com.example.quarto_inteligente_api.controller;

import com.example.quarto_inteligente_api.model.Leitura;
import com.example.quarto_inteligente_api.repository.LeituraRepository;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.List;

@RestController
@RequestMapping("/api/leituras")
public class LeituraController {

    private final LeituraRepository repository;

    @Value("${app.api-key}")
    private String apiKeyEsperada;

    public LeituraController(LeituraRepository repository) {
        this.repository = repository;
    }

    @PostMapping
    public ResponseEntity<?> receberLeitura(
            @RequestBody Leitura leitura,
            @RequestHeader("X-API-Key") String apiKeyRecebida) {

        if (!apiKeyEsperada.equals(apiKeyRecebida)) {
            return ResponseEntity.status(401).body("API key inválida");
        }

        leitura.setDataHora(LocalDateTime.now());
        Leitura salva = repository.save(leitura);

        return ResponseEntity.ok(salva);
    }

    @GetMapping
    public List<Leitura> listarLeituras() {
        return repository.findAll();
    }
}