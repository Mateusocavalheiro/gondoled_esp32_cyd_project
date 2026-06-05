#pragma once
#include <Arduino.h>

// Struct para trafegar os dados do produto entre a API, Memória e Tela
struct Produto {
    String nome;
    float preco;
    float precoClube;
};

//Struct para credenciais
struct Credenciais {
    String ssid;
    String password;
    String apiKey;
};

//URL da API

const String serverURL_verificar = "https://gondoled003-gvdqfgduhpg5f0eg.brazilsouth-01.azurewebsites.net/esp32/verificar/"; 
const String serverURL_registrar = "https://gondoled003-gvdqfgduhpg5f0eg.brazilsouth-01.azurewebsites.net/esp32/registrar";

// Flags globais (Usadas para comunicação entre as Threads)
extern bool flag_atualizar_tela;
extern Produto produtoAtual;
extern String macPlaca;