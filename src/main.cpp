#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <ArduinoJson.h>      
#include <Preferences.h>      

// --- MAPEAMENTO DE PINOS DO ESP32-2432S028 (CYD) ---
#define TFT_CS   15
#define TFT_DC   2
#define TFT_MOSI 13
#define TFT_CLK  14
#define TFT_MISO 12
#define TFT_RST  -1
#define TFT_BL   21 

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

// --- CORES DO NOVO LAYOUT ---
#define COR_FUNDO       0xFFFF
#define COR_TEXTO       0x0000
#define COR_TEXTO_FRACO tft.color565(120, 120, 120)
#define COR_LARANJA     tft.color565(240, 120, 0)
#define COR_TEXTO_SOBRE_LARANJA 0xFFFF

// --- CONFIGURAÇÕES DE REDE E SERVIDOR ---
const char* ssid = "NET_2G772290";       
const char* password = "4F772290";

const char* apiKey = "Senha_2026!"; 
const String serverURL_verificar = "https://gondoled003-gvdqfgduhpg5f0eg.brazilsouth-01.azurewebsites.net/esp32/verificar/"; 
const String serverURL_registrar = "https://gondoled003-gvdqfgduhpg5f0eg.brazilsouth-01.azurewebsites.net/esp32/registrar"; 

// --- VARIÁVEIS DE CONTROLE ---
Preferences preferences;

const unsigned long updateInterval = 30000; // 1 minutos
unsigned long lastUpdate = 0;
String macDoEsp;
bool layoutDesenhado = false;

// --- PROTÓTIPOS DAS FUNÇÕES ---
void conectarWiFi();
void verificarBancoDeDados(String macDoEsp);
void registrarEtiqueta(String macDoEsp);
void desenharEtiqueta(Adafruit_ST7789 &tft, const String &nomeProduto, const String &precoComum, const String &precoClube);
void carregarDadosOffline();
String formatarPrecoReal(float preco);

void setup() {
  Serial.begin(115200);

  preferences.begin("gondoled", false);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  tft.init(240, 320);
  
  // CORREÇÃO: Força a inversão de cores para corrigir o padrão do ST7789
  tft.invertDisplay(0); 
  
  tft.setRotation(1); 

  tft.fillScreen(COR_FUNDO);
  tft.setTextColor(COR_TEXTO);
  tft.setTextSize(2);
  tft.setCursor(10, 100);
  tft.print("Conectando Wi-Fi...");

  conectarWiFi();

  macDoEsp = WiFi.macAddress();
  Serial.print("MAC Address da Placa: ");
  Serial.println(macDoEsp);

  if (WiFi.status() == WL_CONNECTED) {
    tft.fillRect(0, 80, 320, 50, COR_FUNDO); 
    tft.setCursor(10, 100);
    tft.print("Sincronizando Azure...");

    verificarBancoDeDados(macDoEsp);
  } 
  
  if (!layoutDesenhado) {
    carregarDadosOffline();
  }
}

void loop() {
  if (millis() - lastUpdate >= updateInterval) {
    Serial.println("\nChecando atualizações no servidor (Heartbeat)...");
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi desconectado. Tentando reconectar...");
      conectarWiFi();
    }

    if (WiFi.status() == WL_CONNECTED) {
      verificarBancoDeDados(macDoEsp);
    }
    
    lastUpdate = millis();
  }
}

void conectarWiFi() {
  Serial.print("Conectando-se a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
  } else {
    Serial.println("\nFalha ao conectar.");
  }
}

// --- FUNÇÃO AUXILIAR: Formata número para "R$ X,XX" ---
String formatarPrecoReal(float preco) {
  char buf[15];
  sprintf(buf, "R$ %.2f", preco);
  String str = String(buf);
  str.replace(".", ","); // Troca ponto por vírgula no padrão brasileiro
  return str;
}

void registrarEtiqueta(String macDoEsp) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Iniciando requisição POST...");
    
    WiFiClientSecure *client = new WiFiClientSecure; 
    if (client) {
      client->setInsecure(); 
      
      HTTPClient http;
      http.begin(*client, serverURL_registrar);
      http.addHeader("X-API-Key", apiKey);
      http.addHeader("Content-Type", "application/json");
      
      JsonDocument doc;
      doc["mac"] = macDoEsp;
      doc["nome_produto"] = "Novo Produto";
      doc["preco"] = 0.00;
      doc["preco_clube"] = 0.00;
      doc["prazo_validade"] = "2026-12-31";

      String jsonPayload;
      serializeJson(doc, jsonPayload);

      int httpResponseCode = http.POST(jsonPayload);
      
      if (httpResponseCode == 200 || httpResponseCode == 201) {
        Serial.println("Registrado com sucesso na Azure!");
        preferences.putBool("registrado", true);
        verificarBancoDeDados(macDoEsp);
      } else {
        Serial.println("Erro no registro. Codigo HTTP: " + String(httpResponseCode));
      }
      http.end();
      delete client; 
    }
  }
}

void verificarBancoDeDados(String macDoEsp) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Iniciando requisição GET para " + macDoEsp + " ...");
    
    WiFiClientSecure *client = new WiFiClientSecure; 
    if (client) {
      client->setInsecure(); 
      
      HTTPClient http;
      http.begin(*client, serverURL_verificar + macDoEsp);
      http.addHeader("X-API-Key", apiKey);
      
      int httpResponseCode = http.GET();
      
      if (httpResponseCode == 200) {
        String payload = http.getString(); 
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          // Checa se os campos de preço e nome existem no JSON
          if (doc["nome_produto"].is<String>() && doc["preco"].is<float>() && doc["preco_clube"].is<float>()) {
            
            String srvNome = doc["nome_produto"].as<String>();
            float srvPreco = doc["preco"].as<float>();
            float srvPrecoClube = doc["preco_clube"].as<float>();

            String nvsNome = preferences.getString("nome", "");
            float nvsPreco = preferences.getFloat("preco", -1.0);
            float nvsPrecoClube = preferences.getFloat("preco_clube", -1.0);

            // Se qualquer valor for diferente do que está salvo, ele atualiza a tela
            if (srvNome != nvsNome || srvPreco != nvsPreco || srvPrecoClube != nvsPrecoClube || !layoutDesenhado) {
              Serial.println("Alteração detectada! Atualizando Tela e Memória.");
              
              preferences.putString("nome", srvNome);
              preferences.putFloat("preco", srvPreco);
              preferences.putFloat("preco_clube", srvPrecoClube);
              
              String strPreco = formatarPrecoReal(srvPreco);
              String strPrecoClube = formatarPrecoReal(srvPrecoClube);

              // CHAMA O SEU NOVO LAYOUT!
              desenharEtiqueta(tft, srvNome, strPreco, strPrecoClube);
              layoutDesenhado = true;
            } else {
              Serial.println("Dados idênticos. Mantendo display atual (Heartbeat enviado).");
            }
          } else {
             Serial.println("ALERTA: O JSON chegou, mas faltam chaves (nome_produto, preco ou preco_clube)!");
          }
        } else {
          Serial.println("Erro ao interpretar o JSON: " + String(error.c_str()));
        }
      } 
      else if (httpResponseCode == 404) {
        Serial.println("ALERTA 404: Etiqueta NAO ENCONTRADA no banco. Iniciando rotina de cadastro...");
        registrarEtiqueta(macDoEsp);
      } 
      else if (httpResponseCode == 403) {
        Serial.println("ERRO 403: API Key recusada pelo servidor Azure.");
      } 
      else if (httpResponseCode < 0) {
        Serial.println("Falha severa na conexão HTTPS.");
      }
      http.end();
      delete client; 
    }
  }
}

void carregarDadosOffline() {
  Serial.println("Carregando dados da memoria interna (NVS)...");
  String nomeOffline = preferences.getString("nome", "Aguardando");
  float precoOffline = preferences.getFloat("preco", 0.0);
  float precoClubeOffline = preferences.getFloat("preco_clube", 0.0);
  
  String strPreco = formatarPrecoReal(precoOffline);
  String strPrecoClube = formatarPrecoReal(precoClubeOffline);

  desenharEtiqueta(tft, nomeOffline, strPreco, strPrecoClube);
  layoutDesenhado = true;
}

// ==========================================
// SEU NOVO LAYOUT DE ETIQUETA
// ==========================================
void desenharEtiqueta(Adafruit_ST7789 &tft, const String &nomeProduto, const String &precoComum, const String &precoClube) {
  // 1. Pinta o fundo
  tft.fillScreen(COR_FUNDO);

  // ==========================================
  //                TÍTULO
  // ==========================================
  tft.setTextColor(COR_TEXTO);
  tft.setTextSize(3); 
  tft.setCursor(20, 10); 
  
  // Corta o nome se for muito grande para não quebrar a tela
  String nomeExibicao = nomeProduto;
  if(nomeExibicao.length() > 14) nomeExibicao = nomeExibicao.substring(0, 14);
  tft.println(nomeExibicao);

  // ==========================================
  //        BLOCO ESQUERDO: PREÇO COMUM
  // ==========================================
  // Label "PRECO COMUM"
  tft.setTextColor(COR_TEXTO_FRACO);
  tft.setTextSize(1);
  tft.setCursor(10, 80); 
  tft.print("PRECO COMUM");

  // Preço regular
  tft.setTextColor(COR_TEXTO);
  tft.setTextSize(3); 
  tft.setCursor(10, 100);
  tft.print(precoComum);

  // ==========================================
  //        BLOCO DIREITO: PREÇO CLUBE
  // ==========================================
  // Fundo da etiqueta "NO CLUBE"
  tft.fillRect(170, 75, 140, 25, COR_LARANJA); 
  
  // Texto "NO CLUBE"
  tft.setTextColor(COR_TEXTO_SOBRE_LARANJA);
  tft.setTextSize(2); 
  tft.setCursor(185, 80);
  tft.println("NO CLUBE");

  // Preço clube
  tft.setTextColor(COR_LARANJA);
  tft.setTextSize(3);
  tft.setCursor(170, 105); 
  tft.print(precoClube);
}