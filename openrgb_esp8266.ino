#include <FS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>


WiFiUDP wifiServer;
unsigned int port = 8898;

int cr = 0, cg = 0, cb = 0;

String deviceID = "DEFAULT";

char customID[40];

bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  analogWrite(D5, 1024);
  analogWrite(D6, 1024);
  analogWrite(D7, 1024);

  Serial.println("mounting FS...");
  
    if (SPIFFS.begin()) {
      Serial.println("mounted file system");
      if (SPIFFS.exists("/config.json")) {
        //file exists, reading and loading
        Serial.println("reading config file");
        File configFile = SPIFFS.open("/config.json", "r");
        if (configFile) {
          Serial.println("opened config file");
          size_t size = configFile.size();
          // Allocate a buffer to store contents of the file.
          std::unique_ptr<char[]> buf(new char[size]);
  
          configFile.readBytes(buf.get(), size);
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(buf.get());
          json.printTo(Serial);
          if (json.success()) {
            Serial.println("\nparsed json");
            strcpy(customID, json["customID"]);
          } else {
            Serial.println("failed to load json config");
          }
        }
      }
    } else {
      Serial.println("failed to mount FS");
    }
    
WiFiManager wifiManager;
//wifiManager.resetSettings();
    
WiFiManagerParameter custom_output("customID", "customID", customID, 40);
wifiManager.setSaveConfigCallback(saveConfigCallback);    
wifiManager.addParameter(&custom_output);
wifiManager.autoConnect("autoconnect");



  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to Wifi, IP: ");
  Serial.println(WiFi.localIP());
  
  rgb(255,0,0);
  rgb(0,255,0);
  rgb(0,0,255);
  rgb(0, 0, 0);

  flash();
  flash();

  if(strlen(custom_output.getValue()) > 0)strcpy(customID, custom_output.getValue());
  if(strlen(customID) > 0) deviceID = customID;
  Serial.println("Device ID is: "+deviceID);


  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["customID"] = customID;
  File configFile = SPIFFS.open("/config.json", "w");
  if(configFile) json.printTo(configFile);

  
  
  wifiServer.begin(port);
}

void loop(){
  String messageToSend = "";
  int packetSize = wifiServer.parsePacket();
  boolean willChangeColor = false;
  int newr, newg, newb;
  if(packetSize){
    char incomingPacket[255];
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, wifiServer.remoteIP().toString().c_str(), wifiServer.remotePort());
      String s = "";
      int len = wifiServer.read(incomingPacket, 255);
      if(len >0){
        incomingPacket[len] = 0;
      }
      Serial.printf("UDP packet contents: %s\n", incomingPacket);
      for(int k=0; k<len; k++){
        s += incomingPacket[k];
      }
      if(s.length()>0) Serial.println("");
      if(s.indexOf("RGBPROTOCOL") != -1){
        String target = s.substring(s.indexOf("RGBPROTOCOL:")+12, s.indexOf("#"));
        String messagetype = s.substring(s.indexOf(target+"#")+target.length()+1, s.indexOf("&"));
        Serial.println("destination: "+target+"; message type: "+messagetype);
        if(target.equals(deviceID) || target.equals("*")){
          if(messagetype.equals("COMMAND")){
            messageToSend = "RGBPROTOCOL:NULL#RESPONSE&OK";
            s = s.substring(s.indexOf("&")+1);
            String kr = s.substring(0, s.indexOf("-"));
            s = s.substring(s.indexOf("-")+1);
            String kg = s.substring(0, s.indexOf("-"));
            s = s.substring(s.indexOf("-")+1);
            String kb = s;
            Serial.println("r="+kr+" g="+kg+" b="+kb);
            willChangeColor = true;
            newr = kr.toInt();
            newg = kg.toInt();
            newb = kb.toInt();
          }
          if(messagetype.equals("PING")){
            Serial.println("PING response sent");
            messageToSend = "RGBPROTOCOL:NULL#ECHO&"+deviceID;
          }
          if(messagetype.equals("DISCOVER")){
            Serial.println("DISCOVER BEACON sent");
            messageToSend = "RGBPROTOCOL:NULL#BEACON&"+deviceID;
          }
        }
      }
      if(messageToSend.length() > 0){
        wifiServer.beginPacket(wifiServer.remoteIP(), wifiServer.remotePort());
        char replyPacket[messageToSend.length()+1];
        messageToSend.toCharArray(replyPacket, messageToSend.length()+1);
        Serial.println(messageToSend+" --> "+replyPacket);
        wifiServer.write(replyPacket);
        wifiServer.endPacket();
      }
      if(willChangeColor){
        rgb(newr, newg, newb);
      }
      delay(10);
  }
}

void rgb(int sr, int sg, int sb){
  int r = (int) (sr / 255.0 * 1024);
  int g = (int) (sg / 255.0 * 1024);
  int b = (int) (sb / 255.0 * 1024);
  while(cr != r || cg != g || cb != b){
    if(cr < r) cr++;
    else if(cr > r) cr--;
    if(cg < g) cg++;
    else if(cg > g) cg--;
    if(cb < b) cb++;
    else if(cb > b) cb--;
    analogWrite(D5, 1024-cr);
    analogWrite(D6, 1024-cg);
    analogWrite(D7, 1024-cb);
    delay(1);
    
  }
}

void flash(){
  delay(100);
  analogWrite(D5, 512);
  analogWrite(D6, 512);
  analogWrite(D7, 512);
  delay(100);
  analogWrite(D5, 1024);
  analogWrite(D6, 1024);
  analogWrite(D7, 1024);
}
