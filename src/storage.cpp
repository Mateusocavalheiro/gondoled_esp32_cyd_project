#include "storage.h"
#include <Preferences.h>
#include "config.h"

Preferences prefs;

void Storage_Init() {
    prefs.begin("gondoled", false);
}

void Storage_SalvarCredenciais(String ssid, String pass, String key) {
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", pass);
    prefs.putString("api_key", key);
}

Credenciais Storage_LerCredenciais() {
    Credenciais creds;
    creds.ssid = prefs.getString("wifi_ssid", "");
    creds.password = prefs.getString("wifi_pass", "");
    creds.apiKey = prefs.getString("api_key", "");
    return creds;
}

void Storage_SalvarProduto(Produto p) {
    prefs.putString("nome", p.nome);
    prefs.putFloat("preco", p.preco);
    prefs.putFloat("preco_clube", p.precoClube);
}

Produto Storage_LerProdutoOffline() {
    Produto p;
    p.nome = prefs.getString("nome", "Aguardando...");
    p.preco = prefs.getFloat("preco", 0.0);
    p.precoClube = prefs.getFloat("preco_clube", 0.0);
    return p;
}