
/*
  Rasp4IoT Node
  
  Compilador: Arduino 1.8.19

  MQTT
  
*/

// ========== HABILITANDO MODO DEBUG ========

#define DEBUG_LORA

// ========== INCLUSAO DAS BIBLIOTECAS ========

#include<ESP8266WiFi.h>                 // lib. para conexao WI-FI
#include<PubSubClient.h>                // lib. para conexao MQTT
#include<string.h>

// ========== Macros e constantes ==========
                                            // referentes ao sensor anemometro

#define MBUFFER         256                  // BUFFER para as mensagens 

#define INTERFACE_NAP   "e5ac1a86-b375-419d-ac77-616d8ef274d7"
#define NODE_NID        "J5Hd"
#define NODE_GW         "hiNJ"

#define WIFI_SSID       "R.port_acess"            // SSID da rede
#define WIFI_PASSWD     "tplink123"               // senha da rede

                                                  // referentes ao MQTT-Broker
#define MQTT_CLIENT     "RASP4IOT_MQTT_NODE"      // host como est. met. com esp32 ponta cliente
#define MQTT_USER       ""                        // usuario para conectar na rede MQTT
#define MQTT_PASSWD     ""                        // senha para conectar na rede MQTT
#define MQTT_SERVER     "192.168.3.10"          // endereco IP do servidor MQTT
#define MQTT_PORT       1883                      // porta do servidor MQTT

#define MQTT_CMD_SEND       "send"
#define MQTT_CMD_RECEIVE    "receive"

// ========== Mapeamento de portas =========


// ========== Conf. de hardware ============



// ========== redefinicao de tipo ==========
typedef unsigned char u_int8;                                         // var. int. de  8 bits nao sinalizada
typedef unsigned char u_int16;                                        // var. int. de 16 bits nao sinalizada
typedef unsigned long u_int32;                                        // var. int. de 32 bits nao sinalizada

// ========== estruturas ===================
enum NodeAddr{GATEWAY,NODE1,NODE2};                                   // enumeracao de enderecos

// ========== constantes ===================
const char NAP[] = INTERFACE_NAP;
const char NID[] = NODE_NID;
const char NODE[][5]= {"hiNJ","POw2"};
const char MQTT_SEND[] = MQTT_CMD_SEND;
const char MQTT_RECEIVE[] = MQTT_CMD_RECEIVE;

//<mqtt_interface_nap>/send/<nid_origin>/<nid_destiny>

// ========== Variaveis globais ============
u_int32     runtime = 0;
char        message[MBUFFER],
            mqttTopicReceive[MBUFFER],
            mqttTopicSend[sizeof(NODE)/sizeof(NODE[0])][MBUFFER];

// ========== Prototipos das Funcoes ========

void callback(char* topic, byte* message, unsigned int length);
u_int8 wifiConnect();                                                 // valida conexao do wi-fi
void mqttSetup();                                                     // valida conexao do mqtt
void mqttBuildTopics();
void mqttConnect();                                                   // funcao para conectar o MQTT
void mqttPublish(char *topic, char *payload);

// ========== Declaracao dos objetos ========
WiFiClient espWFClient;                                               // objeto da classe WiFiClient
PubSubClient client(MQTT_SERVER, MQTT_PORT, callback, espWFClient);   // callback: funcao que trata as mensagens recebidas

// ========== Configuracoes iniciais ========
void setup() {
  
  Serial.begin(9600);
  wifiConnect();                                                    // chama a funcao que conecta ESP32 na rede Wi-Fi
  mqttSetup();                                                      // chama a funcao que config. ESP32 na rede MQTT
}

// ========== Codigo principal ==============
void loop() {

  mqttConnect();                                                    // chama a funcao que conecta ESP32 na rede MQTT e verifica se mantem conectado
  
  if(millis() - runtime > 30000){
    
    runtime = millis();
    snprintf(message, MBUFFER, "%d", random(100));
    
    mqttPublish(mqttTopicSend[GATEWAY], message);
    mqttPublish(mqttTopicSend[NODE1], message);
  }
  //Serial.println(message);
  client.loop();
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
}                                                           // configuracao do MQTT
                                                                         
void mqttSetup(){                                               
    client.setServer(MQTT_SERVER, MQTT_PORT);                             // definindo servidor e porta
    client.setCallback(callback);                                         // definindo a funcao que trata os dados recebidos da rede   
}
                                                                          // funcao para conectar o MQTT
void mqttConnect() {
  if(!client.connected()){                                                // testa conexao do MQTT
    do {                                 
      Serial.print("Attempting MQTT connection... ");
      Serial.println(client.state());
      // Attempt to connect
      
      //client.setKeepAlive(0);
      //client.setSocketTimeout(0);
      //if (client.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASSWD)) {        // autenticao MQTT
      
      if (client.connect(MQTT_CLIENT)) {        // autenticao MQTT
        Serial.println("connected");
        mqttBuildTopics();        
      } else {                                                          // caso contrario indica erro e tenta novamente
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(1000);
      }
    } while (!client.connected());
  }
  else{
    client.loop();                                                        // usada para continuar comunicacao com o servidor
  }  
}

void mqttBuildTopics(){
  u_int16 numberDevices = sizeof(mqttTopicSend)/sizeof(mqttTopicSend[0]);
  
  snprintf(mqttTopicReceive, MBUFFER, "%s/%s/+/%s", NAP, MQTT_RECEIVE, NID);
  Serial.println("MQTT[Status] tópicos listados");
  Serial.printf("\t%s\n", mqttTopicReceive);
  
  for(int i = 0; i < numberDevices; i++){
    snprintf(mqttTopicSend[i], MBUFFER, "%s/%s/%s/%s", NAP, MQTT_SEND, NID, NODE[i]);
    Serial.printf("\t%s\n", mqttTopicSend[i]);
  }
  client.subscribe(mqttTopicReceive);
}

void mqttPublish(char *topic, char *payload){
  client.publish(topic, payload);
  Serial.printf("MQTT Status: Enviando mensagem: \n\tTópico: %s \n\tPayload: %s\n", topic, payload);
}

void callback(char* topic, byte* message, unsigned int length) {

  char    *breakStr,
          payload[length+1],
          newStr[9][MBUFFER] = {" ", " ", " ", " ", " "};                          // matriz de strings
  u_int8  i = 0;                                                                    // var. aux. de indices

  breakStr = strtok(topic, "/");                                   // atribuindo ao conteudo a mensagem separada pelo primeiro delimitador
  while(breakStr != NULL){                                               // enquanto ponteiro nao apontar NULL - vazio
      //Serial.println(breakStr);
      strcpy(newStr[i], breakStr);                                      // copiando string delimitada atual na posicao i
      breakStr = strtok(NULL, "/");                                // quebra mensagem incrementa ponteiro                                
      i++;                                                                // incrementando indice para acompanhar indices
  }

  if(!strcmp(newStr[0], NAP)){
    if(!strcmp(newStr[3], NID)){
      if(!strcmp(newStr[1], MQTT_RECEIVE)){
        // se necessario verificar a origem, pode ser adicionado nesta linha
        for (int i = 0; i < length; i++) {
          payload[i] = (char)message[i];
        }
        payload[length] = '\0';
        
        Serial.println("[MQTT] Status: Mensagem recebida");
        Serial.printf("\tNID origin: %s\n", newStr[2]);
        Serial.printf("\tNID destiny: %s\n", newStr[3]);
        Serial.printf("\tData: %s\n", payload);
      }
    }    
  }
}

/*
 * 
 * MQTT https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/
 * BASE: /home/labcam/Arduino/esp01_arduino/esp01_arduino.ino
 * 
 * STRING
 * https://petbcc.ufscar.br/stringfuncoes/
 * https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
 * 
 */
