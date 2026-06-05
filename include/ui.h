#pragma once
#include <Arduino.h>
#include "config.h"

void UI_Init();
void UI_DesenharEtiqueta(Produto p);
void UI_TelaCarregamento();
void UI_DesenharQRCodeConfig(String mac);
String formatarPrecoReal(float preco);