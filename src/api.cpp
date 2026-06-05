#include "api.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "storage.h"

void API_VerificarBanco() {
    Serial.println("-> [API] Iniciando checagem no banco...");
    Serial.flush(); 

    Credenciais creds = Storage_LerCredenciais();
    
    // Instanciação direta na Stack (sem ponteiros) elimina riscos de Null Pointer
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(10000); // Limite de 10 segundos para não travar a linha
    http.begin(client, serverURL_verificar + macPlaca);
    http.addHeader("X-API-Key", creds.apiKey);

    Serial.println("-> [API] Enviando GET para Azure...");
    Serial.flush();
    
    int code = http.GET();
    Serial.printf("-> [API] Resposta HTTP da Azure: %d\n", code);
    Serial.flush();

    if (code == 200) {
        String payload = http.getString(); 
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            Produto p_novo;
            p_novo.nome = doc["nome_produto"].as<String>();
            p_novo.preco = doc["preco"].as<float>();
            p_novo.precoClube = doc["preco_clube"].as<float>();

            if (p_novo.nome != produtoAtual.nome || p_novo.preco != produtoAtual.preco || p_novo.precoClube != produtoAtual.precoClube) {
                Serial.println("-> [API] Novos valores detectados! Atualizando...");
                produtoAtual = p_novo;
                Storage_SalvarProduto(produtoAtual);
                flag_atualizar_tela = true;
            } else {
                Serial.println("-> [API] Dados identicos aos atuais. Nada a atualizar.");
            }
        } else {
            Serial.print("-> [API] Erro no Parse do JSON: ");
            Serial.println(error.c_str());
        }
    } else if (code == 404) {
        Serial.println("-> [API] Etiqueta nao cadastrada (404). Executando auto-registro...");
        Serial.flush();
        API_RegistrarEtiqueta();
    }
    
    http.end();
}

void API_RegistrarEtiqueta() {
    Serial.println("-> [API POST] Preparando auto-registro...");
    Serial.flush();

    Credenciais creds = Storage_LerCredenciais();
    WiFiClientSecure client; 
    client.setInsecure(); 
      
    HTTPClient http;
    http.setTimeout(10000);
    http.begin(client, serverURL_registrar);
    http.addHeader("X-API-Key", creds.apiKey);
    http.addHeader("Content-Type", "application/json");
      
    JsonDocument doc;
    doc["mac"] = macPlaca;
    doc["nome_produto"] = "Nova Etiqueta CYD";
    doc["preco"] = 0.00;
    doc["preco_clube"] = 0.00;
    doc["prazo_validade"] = "2026-12-31";

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("-> [API POST] Enviando dados de cadastro...");
    Serial.flush();

    int httpResponseCode = http.POST(jsonPayload);
    Serial.printf("-> [API POST] Resposta do Cadastro: %d\n", httpResponseCode);
    Serial.flush();
      
    if (httpResponseCode == 200 || httpResponseCode == 201) {
        Serial.println("-> [API POST] Sucesso! Placa registrada na nuvem.");
    } else {
        Serial.printf("-> [API POST] Erro ao registrar: %d\n", httpResponseCode);
    }
    
    http.end();
}