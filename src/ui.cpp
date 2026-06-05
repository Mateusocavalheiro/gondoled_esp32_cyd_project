#include "ui.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "cyd_pins.h"
#include "config.h"
#include <qrcode.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST);

void UI_Init() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    tft.init(240, 320);
    tft.invertDisplay(0); 
    tft.setRotation(1);
}

String formatarPrecoReal(float preco) {
    char buf[15];
    sprintf(buf, "R$ %.2f", preco);
    String str = String(buf);
    str.replace(".", ",");
    return str;
}

void UI_DesenharEtiqueta(Produto p) {
    // 1. Limpa a tela inteira com fundo Branco (Isso mata o "Conectando Wi-Fi...")
    tft.fillScreen(0xFFFF); 

    // 2. Desenha o Nome do Produto (Em Preto)
    tft.setTextColor(0x0000); 
    tft.setTextSize(3); // Fonte grande
    tft.setCursor(10, 20);
    tft.println(p.nome);

    // 3. Desenha o Preço Normal (Em Preto)
    tft.setTextSize(2); // Fonte média
    tft.setCursor(10, 80);
    tft.print("Preco Normal: R$ ");
    tft.println(p.preco);

    // 4. Desenha a Tarja Laranja de Promoção
    // fillRect(X, Y, Largura, Altura, Cor Laranja em RGB565)
    tft.fillRect(10, 130, 300, 50, 0xFD20); 

    // 5. Desenha o Preço do Clube (Em Branco, dentro da tarja)
    tft.setTextColor(0xFFFF); 
    tft.setTextSize(3);
    tft.setCursor(20, 145);
    tft.print("CLUBE: R$ ");
    tft.println(p.precoClube);
}

void UI_DesenharQRCodeConfig(String mac) {
    String uniqueID = mac.substring(mac.length() - 4);
    String qrText = "WIFI:S:ESL-" + uniqueID + ";T:nopass;;";
    
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, qrText.c_str());

    tft.fillScreen(0xFFFF); // Fundo branco
    int scale = 5;
    int offsetX = (320 - (qrcode.size * scale)) / 2;
    int offsetY = (240 - (qrcode.size * scale)) / 2;

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                tft.fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, 0x0000);
            }
        }
    }
    
    tft.setTextColor(0x0000);
    tft.setTextSize(2);
    tft.setCursor(30, offsetY - 30);
    tft.print("Escaneie para Configurar");
}

void UI_TelaCarregamento() {
    tft.fillScreen(0xFFFF); // Pinta de branco
    tft.setTextColor(0x0000); // Texto preto
    tft.setTextSize(2);
    tft.setCursor(30, 150);
    tft.print("Conectando Wi-Fi...");
}