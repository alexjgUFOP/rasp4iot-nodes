
/*
  Rasp4IoT Node
  
  Compilador: Arduino 1.8.19

  HTTP
  
*/

// ========== HABILITANDO MODO DEBUG ========

#define DEBUG_LORA

// ========== INCLUSAO DAS BIBLIOTECAS ========

#include<ESP8266WiFi.h>                 // lib. para conexao WI-FI
#include<ESP8266HTTPClient.h>           // lib. para conexao HTTP
#include<string.h>

// ========== Macros e constantes ==========

#define MBUFFER           256                     // BUFFER para as mensagens 

#define INTERFACE_NAP     "b36f57ad-0345-4643-be9f-9d4395d4e91c"
#define NODE_NID          "LgMm"
#define NODE_GW           "wOyo"
#define NODE_ADDR1        "R57o"
#define NODE_ADDR2        "FakW"

#define WIFI_SSID         "ssid"                  // SSID da rede
#define WIFI_PASSWD       "password"              // senha da rede

#define HTTP_SERVER       "http://192.168.3.11"
#define HTTP_PORT         3000
#define HTTP_CMD_SEND     "/devices/send"
#define HTTP_CMD_RECEIVE  "/devices/receive/http"

// ========== Mapeamento de portas =========


// ========== Conf. de hardware ============



// ========== redefinicao de tipo ==========
typedef unsigned char u_int8;                                         // var. int. de  8 bits nao sinalizada
typedef unsigned char u_int16;                                        // var. int. de 16 bits nao sinalizada
typedef unsigned long u_int32;                                        // var. int. de 32 bits nao sinalizada

// ========== estruturas ===================
enum NodeAddr{GATEWAY,NODE1,NODE2};                                   // enumeracao de enderecos

// ========== constantes ===================
const char HTTP_SEND[] = HTTP_CMD_SEND;
const char HTTP_RECEIVE[] = HTTP_CMD_RECEIVE;
const char NAP[] = INTERFACE_NAP;
const char NID[] = NODE_NID;
const char NODE[][5]= {NODE_GW, NODE_ADDR1, NODE_ADDR2};
const int PORT = HTTP_PORT;
const char SERVER[] = HTTP_SERVER;

// ========== Variaveis globais =============
u_int32     runtime = 0, timecheck = 0;
char        message[MBUFFER];

// ========== Prototipos das Funcoes ========

u_int8 wifiConnect();                                                 // valida conexao do wi-fi
void httpGet(WiFiClient wifi, char *httpPath);
void httpPost(WiFiClient wifi, char *httpPath, char *httpBody);
void httpReceive(WiFiClient wifi, const char *interface_nap, const char *nid_origin);
void httpSend(WiFiClient wifi, const char *interface_nap, const char *nid_origin, const char *nid_destiny, char *message);

// ========== Declaracao dos objetos ========
WiFiClient espWFClient;                                               // objeto da classe WiFiClient
HTTPClient http;

// ========== Configuracoes iniciais ========
void setup() {
  
  Serial.begin(9600);
  wifiConnect();                                                    // chama a funcao que conecta ESP na rede Wi-Fi
}

// ========== Codigo principal ==============
void loop() {
  
  if(millis() - runtime > 30000){
    
    runtime = millis();
    
    snprintf(message, MBUFFER, "%d", random(100));
    httpReceive(espWFClient, NAP, NID);
    httpSend(espWFClient, NAP, NID, NODE[GATEWAY], message);
    httpSend(espWFClient, NAP, NID, NODE[NODE1], message);
    httpSend(espWFClient, NAP, NID, NODE[NODE2], message);
  }

  if(millis() - timecheck > 5000){

    timecheck = millis();
    httpReceive(espWFClient, NAP, NID);
  }

}

// ========== Rotinas de interrupcao ========

// ========== Desenvolv. das funcoes ========
u_int8 wifiConnect(){

  static u_int8  wifiStatus = 0, statusChange = 0;                                  // status da conexao
  static u_int32 runtime = millis();                                                // tempo atual para controlar envio do log
    
  wifiStatus = ((WiFi.status() != WL_CONNECTED) ?  0 : 1);                           // se nao conectado, wifiStatus = 0
  
  if(!wifiStatus){

    WiFi.begin(WIFI_SSID, WIFI_PASSWD);                                             // passando o usuario e senha de rede wi-fi
    
    if(!wifiStatus && (millis() - runtime > 1000)){                                     // mostra status da conexao a cada 1s
    
        Serial.println("[WiFi] Status: falha na conexao - nova tentativa...");
        runtime = millis();                                                         // atualiza temporizador
        statusChange = 0;                                                           // monitora se wi-fi mudou de estado
    }    
  }
  else if((statusChange != wifiStatus)){                                            // se saiu de desconectado para conectado alerta
                                                                                    // se wi-fi conectou
    statusChange = wifiStatus;
    Serial.print("[WiFi] Status: Conectado - IP: ");
    Serial.println(WiFi.localIP());
    delay(1500);
  }    
  return wifiStatus;                                                                // retorna status da conexao        
}


void httpReceive(WiFiClient wifi, const char *interface_nap, const char *nid_origin){
  char  httpCommand[MBUFFER] = "";
  
  snprintf(httpCommand, MBUFFER, "%s:%d%s?interface_nap=%s&node_nid=%s", SERVER, PORT, HTTP_RECEIVE, interface_nap, nid_origin);
  
  httpGet(wifi, httpCommand);
}

void httpSend(WiFiClient wifi, const char *interface_nap, const char *nid_origin, const char *nid_destiny, char *message){
  char  httpCommand[MBUFFER] = "",
        httpBody[MBUFFER] = "";
  
  snprintf(httpCommand, MBUFFER, "%s:%d%s?interface_nap=%s", SERVER, PORT, HTTP_SEND, interface_nap);
  snprintf(httpBody, MBUFFER, "node_nid_origin=%s&node_nid_destiny=%s&node_message=%s", nid_origin, nid_destiny, message);
  
  httpPost(wifi, httpCommand, httpBody);
}

void httpGet(WiFiClient wifi, char *httpPath){
  
  if(WiFi.status()){
    
    HTTPClient http;
    String payload;
    char packet[3][MBUFFER] = {"", "", ""};
    u_int16 httpCode;

      
    http.begin(wifi, httpPath);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpCode = http.GET();
  
    // httpCode will be negative on error
    if (httpCode > 0){
      payload = http.getString();
      if(payload != "" && payload != NULL){
        Serial.printf("[HTTP] Status: requisição GET enviada com código %d\n", httpCode);
        payload = (String) http.getString();
        Serial.printf("\tHTTP_PATH: %s\n", httpPath);
        Serial.print("\tMENSAGEM: ");
        Serial.println(payload);
        Serial.println();
      }
    }
    else{
      Serial.printf("[HTTP] Status: GET... falhou com erro: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}

void httpPost(WiFiClient wifi, char *httpPath, char *httpBody){
  
  if(WiFi.status()){
    
    HTTPClient http;
    u_int16 httpCode;
      
    http.begin(wifi, httpPath);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpCode = http.POST(httpBody);
  
    // httpCode will be negative on error
    if (httpCode > 0) {
      Serial.printf("[HTTP] Status: requisição POST enviada com código %d\n", httpCode);
      Serial.printf("\tHTTP_PATH: %s\n", httpPath);
      Serial.printf("\tHTTP_BODY: %s\n\n", httpBody);
    }
    else{
      Serial.printf("[HTTP] Status: POST... falhou com erro: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}



/*
 * 
 * HTTP 
 * https://randomnerdtutorials.com/esp8266-nodemcu-http-get-post-arduino/
 * https://esp32io.com/tutorials/esp32-http-request#google_vignette
 * *
 * STRING
 * https://petbcc.ufscar.br/stringfuncoes/
 * https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
 * 
 */
