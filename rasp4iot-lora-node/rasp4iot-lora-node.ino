
/*
  Rasp4IoT Node
  
  Compilador: Arduino 1.8.19

  Lora: TX/RX
  
*/

// ========== HABILITANDO MODO DEBUG ========

#define DEBUG_LORA

// ========== INCLUSAO DAS BIBLIOTECAS ========

#include<SPI.h>                         // lib. para barramento SPI
#include<LoRa.h>                      // lib. para conexao LoRa
#include<string.h>

// ========== Macros e constantes ==========
                                            // referentes ao sensor anemometro

#define MBUFFER         50                  // BUFFER para as mensagens 

#define NODE_NID        "yKNA"
#define NODE_GW         "YyeM"
#define NODE_ADDR1      "6123"

#define LORA_GAIN       20                  // potencia do sinal em dBm
#define LORA_FREQ_TX    915E6               // canal de op. em MHz
#define LORA_FREQ_RX    868E6               // canal de op. em MHz

// ========== Mapeamento de portas =========

                                            // referentes ao barramento SPI e dispositivos - IMPORTANTE: conferir pinagem do SPI do kit usado
#define SPI_SCK     14
#define SPI_MISO    12
#define SPI_MOSI    13

#define I2C_SDC      5
#define I2C_SDA      4

#define LORA_SS     15
#define LORA_RST     2
#define LORA_DIO0   16

// ========== Conf. de hardware ============



// ========== redefinicao de tipo ==========
typedef unsigned char u_int8;                                                                           // var. int. de  8 bits nao sinalizada
typedef unsigned char u_int16;                                                                          // var. int. de 16 bits nao sinalizada
typedef unsigned long u_int32;                                                                          // var. int. de 32 bits nao sinalizada

// ========== estruturas ===================
enum NodeAddr{GATEWAY,NODE1,NODE2};                                                                     // enumeracao de enderecos

// ========== constantes ===================;
const char NID[] = NODE_NID;
const char NODE[][5]= {NODE_GW, NODE_ADDR1};

// ========== Variaveis globais ============
u_int32     runtime = 0;
char        message[MBUFFER];

// ========== Prototipos das Funcoes ========


void    loraSetup();                                                                                   // configuracao da rede
u_int8  loraConnect(u_int32 freq);                                                                     // valida conexao do LORALORA
void    loraFilterMessage(char *message);
void loraSend(const char *nid_origin, const char *gateway, const char *nid_destiny, char *mesg);       // realiza conexao LORA
u_int8 loraReceive(char *mensg);                                                                       // valida se recebeu mensagem na LORA

// ========== Declaracao dos objetos ========

// ========== Configuracoes iniciais ========
void setup() {
  
  Serial.begin(9600);
  loraSetup();                                                                                          // chama a funcao que configura ESP32 na rede LORA
}

// ========== Codigo principal ==============
void loop() {
  
  loraConnect(LORA_FREQ_RX);                                                                            // chama a funcao que conecta ESP32 na rede MQTT e verifica se mantem conectado
  if(loraReceive(message)){
    //Serial.println(message);
    loraFilterMessage(message);    
  }
  
  if(millis() - runtime > 30000){
    runtime = millis();
    snprintf(message, MBUFFER, "%d", random(100));
    // repeater mode    | node - gw - node
    // gateway destiny  | node - gw - gw
    loraSend(NID, NODE[GATEWAY], NODE[GATEWAY], message);
    loraSend(NID, NODE[GATEWAY], NODE[NODE1], message);
  }                      
}

// ========== Rotinas de interrupcao ========

// ========== Desenvolv. das funcoes ========

void loraFilterMessage(char *message){
  char    nidGateway[5],
          nidOrigin[5],
          nidDestiny[5],
          data[100];
  u_int8  posNidGateway = 0,
          posNidOrigin = 0,
          posNidDestiny = 0,
          posData = 0;
  u_int16 messageSize = strlen(message);
        
  if(messageSize <= 12){
    Serial.println("[LoRa] Status: Mensagem inválida");
  }
  else{

    for(int i=0; i<messageSize; i++){
      
      if(i >= 0 && i < 4){
        nidGateway[posNidGateway] = message[i];
        posNidGateway++;
      }
      if(i >= 4 && i < 8){
        nidOrigin[posNidOrigin] = message[i];
        posNidOrigin++;
      }
      if(i >= 8 && i < 12){
        nidDestiny[posNidDestiny] = message[i];
        posNidDestiny++;
      }
      if(i >= 12){
        data[posData] = message[i];
        posData++;
      }
    }

    nidGateway[posNidGateway] = '\0';
    nidOrigin[posNidOrigin] = '\0';
    nidDestiny[posNidDestiny] = '\0';
    data[posData] = '\0';    
  }

  if(!strcmp(nidDestiny,NODE_NID) && !strcmp(nidGateway, NODE_GW)){
    Serial.println("[LoRa] Status Mensagem recebida");
    Serial.printf("\tNID origin: %s\n", nidOrigin);
    Serial.printf("\tNID gateway: %s\n", nidGateway);
    Serial.printf("\tNID destiny: %s\n", nidDestiny);
    Serial.printf("\tData: %s\n", data);
  }
  else{
    Serial.println("[LoRa] Status: pacote de origem inválida, descartado");
  }
}

void loraSetup(){   
  Serial.println("[LoRa] Configuracao LoRa...");                                                        // indica tentativa de fechar enlace Lora
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);                                                           // associando os pinos do SPI com o radio LORA
  Serial.println("[LoRa] Configurado");
  LoRa.setTxPower(LORA_GAIN);                                                                           // seta ganho do radio LoRa - 20dBm é o máximo
}
                                                                                                        // habilita conexao LoRa
u_int8 loraConnect(u_int32 freq){                                                  

  static u_int8  loraStatus = 0;                                                                        // status da conexao
  static u_int32 runtime = millis();                                                                    // tempo atual para controlar envio do log      
  
  if(!loraStatus){                                                                     
    if(!LoRa.begin(LORA_FREQ_RX)){                                                                      // verifica se habilitou para conexao    
      
        loraStatus = 0;                                                                                 // falha na conexao
            
        if(millis() - runtime > 1000){                                                                  // mostra status da conexao a cada 1s
    
            Serial.println("[LoRa] Status: falha na inicializacao - nova tentativa... ");
            runtime = millis();                                                                         // atualiza temporizador
        }        
    }
    else{
        Serial.println("[LoRa] Status: modulo inicializado ");
        loraStatus = 1;                                                                                 // sucesso na conexao
            
    }        
  }
  return loraStatus;                                                                                    // retorna status
}
    
void loraSend(const char *nid_origin, const char *gateway, const char *nid_destiny, char *mesg){       // funcao para envio de pacotes pela LORA
    char networkAddress[15] = "";
    LoRa.setFrequency(LORA_FREQ_TX);
    if(loraConnect(LORA_FREQ_TX)){
        snprintf(networkAddress, 15, "%s%s%s", nid_origin, gateway, nid_destiny);
        LoRa.beginPacket();                                                                             // inicio de pacote LORA
        LoRa.print(networkAddress);                                                                     // enviando endereco para tratamento de origem das mensagens
        LoRa.print(mesg);                                                                               // corpo da mensagem
        LoRa.endPacket();                                                                               // finalizando envio de pacote LORA
        Serial.printf("[LoRa] Status: mensagem enviada: \n\tADDR: \"%s\" \n\tDATA: \"%s\"\n", networkAddress,mesg);
    }
    else{
        Serial.println("[LoRa] Status: Erro no envio");
    }
    LoRa.setFrequency(LORA_FREQ_RX);
}
                                                                                
u_int8 loraReceive(char *mensg){                                                                        // funcao para recebimento de pacotes pela LORA


  u_int8  mensgStatus = 0;                                                                              // status se recebeu mensagem
  String  pacRec;                                                                                       // pacote recebido
  u_int32 pacSize = 0;                                                                                  // tamanho do pac.
  

  if(loraConnect(LORA_FREQ_RX)){

    pacSize = LoRa.parsePacket();                                                                       // verifica se tem pacotes disponiveis
  
    if(pacSize){                                                                                        // caso tenha recibido pacote

      mensgStatus = 1;                                                                                  // recebeu mensagem
    
      //Serial.print("Mensagem recebida: ");

      while(LoRa.available()){                                                                          // recebe mensagem enquanto tiver dados disponiveis
          pacRec = LoRa.readString();
          //Serial.print(pacRec);
      }
      strcpy(mensg, pacRec.c_str());                                                                    // passando mensagem para var. enviada como parametro
    }
    else{
      //Serial.print("Sem mensagem recebida ");
      mensgStatus = 0;                                                                                  // nao recebeu mensagem
    }  
  }    
  return mensgStatus;                                                                                   // retorna se recebeu ou nao uma mensagem
}


/* 
 * LORA
 * https://www.filipeflop.com/blog/comunicacao-lora-ponto-a-ponto-com-modulos-esp32-lora/
 * https://github.com/sandeepmistry/arduino-LoRa#readme
 * https://www.fernandok.com/2019/03/instalacao-do-esp32-lora-na-arduino-ide.html * 
 * https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md
 * https://randomnerdtutorials.com/esp32-lora-rfm95-transceiver-arduino-ide/
 */
