#include "network.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "storage.h"
#include "config.h"

DNSServer dnsServer;
AsyncWebServer server(80);

bool Network_ConectarWiFi() {
    Credenciais creds = Storage_LerCredenciais();
    if (creds.ssid == "") {
        Serial.println("-> Nenhuma credencial de WiFi salva na memoria.");
        return false;
    }

    Serial.print("-> Tentando conectar na rede: ");
    Serial.println(creds.ssid);
    
    WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }
    Serial.println(); // Pula linha

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("-> Conectado com sucesso! IP local: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("-> Falha na conexao (Timeout de 10 segundos).");
        return false;
    }
}

void Network_IniciarPortalCativo() {
    String SSID_AP = "ESL-" + macPlaca.substring(macPlaca.length() - 4);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID_AP.c_str());
    dnsServer.start(53, "*", WiFi.softAPIP());

    if(!LittleFS.begin(true)){
        Serial.println("Erro no LittleFS");
        return;
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    // Rota de Processamento do Formulário (POST)
    server.on("/salvar", HTTP_POST, [](AsyncWebServerRequest *request){
        String input_ssid = "";
        String input_pass = "";
        String input_key = "";

        // O 'true' garante que estamos pegando os dados do corpo (BODY) do POST do HTML
        if (request->hasParam("ssid", true)) {
            input_ssid = request->getParam("ssid", true)->value();
        }
        if (request->hasParam("password", true)) {
            input_pass = request->getParam("password", true)->value();
        }
        if (request->hasParam("apikey", true)) {
            input_key = request->getParam("apikey", true)->value();
        }

        // Valida se o SSID e a Chave da API não estão vazios
        if (input_ssid != "" && input_key != "") {
            
            // 1. Grava fisicamente na memória NVS
            Storage_SalvarCredenciais(input_ssid, input_pass, input_key);

            // 2. É OBRIGATÓRIO DAR ESTE RETORNO PARA O NAVEGADOR! (Evita o erro "Handler did not handle")
            String htmlResposta = "<html><body style='text-align:center; margin-top:50px; font-family:sans-serif;'>";
            htmlResposta += "<h2 style='color:green;'>Credenciais Salvas com Sucesso!</h2>";
            htmlResposta += "<p>A placa esta reiniciando e tentara conectar na rede: <b>" + input_ssid + "</b></p>";
            htmlResposta += "</body></html>";
            
            request->send(200, "text/html", htmlResposta);

            // 3. Dá um tempinho para o celular carregar a tela e reinicia o ESP32
            delay(2000);
            ESP.restart();
            
        } else {
            // Se o usuário mandou o formulário em branco
            request->send(400, "text/html", "<h2>Erro: Campos obrigatorios ausentes. Volte e preencha o SSID e a API Key.</h2>");
        }
    });

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });
    
    Serial.println("-> MODO ROTEADOR (AP) ATIVADO!");
    Serial.println("-> Rede WiFi Gerada: " + SSID_AP);
    Serial.println("-> IP do Servidor Web: 192.168.4.1");
    server.begin();
}

void Network_LoopDNS() {
    dnsServer.processNextRequest();
}