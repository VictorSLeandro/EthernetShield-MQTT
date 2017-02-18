#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 10, 20, 1, 199 };

EthernetClient client;
PubSubClient mqttClient(client);
IPAddress server(172, 16, 0, 2);
const char topico[] = "victorsamuel/iot";
const char usuario[] = "v4tech_vntpejr";

//SERVE MQTT
char mqttServer[] = "broker.hivemq.com";

// funções
void analisaJson(String json);
void enviaJson();
void ligaLed(int porta);
void desligaLed(int porta);
void inverteLed(int porta);
void atualiza_led();
void atualiza_status();
void setup_ethernet();
void conectaMqtt();

// led pode ser: 0 desligado, 1 ligado, 2 piscando
int ledState = 0;
int intervaloPisca = 1000;  // padrão 1 segundo
int ultimoPisca = 0;

// status da placa
int intervaloStatus = 5000; // padrão 5 segundos
int ultimoStatus = 0;
void setup() {
  Serial.begin(115200);
  delay(2000);  // aguarde porta serial funcionar
  //inicia ethernet;
  setup_ethernet();
  pinMode(7, OUTPUT);//define porta 7 como saida


  // inicia cliente mqtt
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(callback);
}

void loop() {
 
  // reconecte-se ao servidor se MQTT estiver desconectado
  if (!mqttClient.connected()) {
    conectaMqtt();
  }

  // função loop deve ser chamada para receber mensagens
  mqttClient.loop();
  if (Serial.available()){
     String msg = Serial.readStringUntil('\n');
     analisaJson(msg);
  }
 atualiza_led();

} 


void callback(char* topic, byte* payload, unsigned int length) {

  // transforma o payload em string
  char content[200];
  int i = 0;
  for (i = 0; i < length; i++) {
    content[i] = (char) payload[i];
  }
  content[i] = '\0';

  Serial.print("Nova mensagem no topico: ");
  Serial.println(topic);
  Serial.print("Mensagem: ");
  Serial.println(content);

  analisaJson(content);
}


void setup_ethernet() {
  Serial.println();
  Serial.print("Conectando...\n");
  Ethernet.begin(mac, ip);
  Serial.println("");
}

void conectaMqtt() {
  while (!mqttClient.connected()) {
    Serial.println("Tentando conexão com Broker MQTT...");

    // conecta e insere um id qualquer (coloque seu nome, pois o servidor pode bloquear ids duplicados)
    if (mqttClient.connect(usuario)) {
      Serial.println("Conectado!");

      // faz subscribe no topico principal e já envia status
      mqttClient.subscribe(topico);
      enviaJson();
    } else {
      Serial.println("Ocorreu um erro na tentativa de conexão: ");
      Serial.println(mqttClient.state());
      Serial.println("Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void analisaJson(String json) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& buffer = jsonBuffer.parseObject(json);

  if (!buffer.success()) {
    Serial.println("Json apresenta erros..");
    return;
  }

  // atualiza estado do led
  if (buffer.containsKey("status")) {
    // retorna status
    enviaJson();
  }

  // atualiza estado do led
  if (buffer.containsKey("seta_led")) {
    String lamp = buffer["seta_led"];
    if (lamp == "desligado") {
      ledState = 0;
    } else if (lamp == "ligado") {
      ledState = 1;
    } else if (lamp == "piscando") {
      ledState = 2;
    }

    // retorna estado do led
    enviaJson();
  }

  // pisca se for necessário
  if (buffer.containsKey("pisca_led")) {
    intervaloPisca = buffer["pisca_led"];
    ledState = 2;

    // retorna estado do led
    enviaJson();
  }
}
void enviaJson() {
  String lampada = "";

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& buffer = jsonBuffer.createObject();

  // define a string do estado do led
  switch (ledState) {
    case 0:
      lampada = "desligado";
      break;
    case 1:
      lampada = "ligado";
      break;
    case 2:
      lampada = "piscando";
      break;
  }


  buffer["led"] = lampada;

  // envia para broker
  char mensagem[100];
  buffer.printTo(mensagem, sizeof(mensagem));
  mqttClient.publish(topico, mensagem, false);

}
// função liga o led da porta
void ligaLed(int porta) {
  digitalWrite(porta, HIGH);
}

// função desliga o led da porta
void desligaLed(int porta) {
  digitalWrite(porta, LOW);
}

// inverte estado do led da porta
void inverteLed(int porta) {
  digitalWrite(porta, !digitalRead(porta));
}

// liga, desliga ou faz led piscar
void atualiza_led() {

  // define a string do estado do led
  switch (ledState) {
    case 0:
      desligaLed(7);
      break;
    case 1:
      ligaLed(7);
      break;
    case 2:
      if (millis() - ultimoPisca > intervaloPisca) {
        ultimoPisca = millis();
        inverteLed(7);
      }
      break;
  }
}

// envia status para Broker em intervalos de tempo
void atualiza_status() {
  if (millis() - ultimoStatus > intervaloStatus) {
    ultimoStatus = millis();
    enviaJson();
  }
}
