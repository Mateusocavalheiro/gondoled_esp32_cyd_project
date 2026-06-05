#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "storage.h"
#include "ui.h"
#include "network.h"
#include "api.h"

// Variáveis Globais
bool flag_atualizar_tela = false;
Produto produtoAtual;
String macPlaca;
bool modoPortalCativo = false;

// Task do FreeRTOS para consultar a Azure em segundo plano
void TaskComunicacaoAPI(void *pvParameters) {
    while (true) {
        if (!modoPortalCativo && WiFi.status() == WL_CONNECTED) {
            API_VerificarBanco();
        }
        // Aguarda 30 segundos antes de verificar novamente
        vTaskDelay(30000 / portTICK_PERIOD_MS); 
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Iniciando Sistema GondoLED ---");

    Storage_Init();
    UI_Init();
    
    // 1. Força a tela a limpar qualquer vestígio de QR Code antigo
    UI_TelaCarregamento(); 
    
    macPlaca = WiFi.macAddress();
    Serial.println("-> MAC da Placa: " + macPlaca);

    Serial.println("-> Iniciando Modulo de Rede...");
    if (Network_ConectarWiFi()) {
        modoPortalCativo = false;
        
        // Tenta ler o último produto salvo na memória offline
        produtoAtual = Storage_LerProdutoOffline();
        
        // 2. Se a memória estiver vazia, cria um produto temporário visível
        if (produtoAtual.nome == "") {
            produtoAtual.nome = "Buscando Azure...";
            produtoAtual.preco = 0.00;
            produtoAtual.precoClube = 0.00;
        }
        
        // Atualiza a tela saindo do "Conectando..." para o layout da Etiqueta
        UI_DesenharEtiqueta(produtoAtual); 
        
        Serial.println("-> Iniciando FreeRTOS Task para Azure API...");
        // 3. Memória dobrada (16KB) para aguentar a criptografia SSL da Azure sem travar
        xTaskCreatePinnedToCore(TaskComunicacaoAPI, "TaskAPI", 16384, NULL, 1, NULL, 1); // Core 1 (Seguro e Estável)
    } else {
        modoPortalCativo = true;
        Serial.println("-> Iniciando Portal Cativo para configuracao...");
        Network_IniciarPortalCativo();
        UI_DesenharQRCodeConfig(macPlaca);
    }
}

void loop() {
    if (modoPortalCativo) {
        // Mantém o servidor web e o DNS rodando para o celular conectar
        Network_LoopDNS(); 
    } else {
        // Se a API baixou dados novos na Task paralela, atualiza o display aqui no loop principal
        if (flag_atualizar_tela) {
            Serial.println("-> LOOP: Desenhando nova etiqueta na tela!");
            UI_DesenharEtiqueta(produtoAtual);
            flag_atualizar_tela = false;
        }
    }
}