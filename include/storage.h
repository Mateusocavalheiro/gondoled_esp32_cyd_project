#pragma once
#include <Arduino.h>
#include "config.h" // Precisa conhecer as structs Credenciais e Produto

void Storage_Init();
void Storage_SalvarCredenciais(String ssid, String pass, String key);
Credenciais Storage_LerCredenciais();
void Storage_SalvarProduto(Produto p);
Produto Storage_LerProdutoOffline();