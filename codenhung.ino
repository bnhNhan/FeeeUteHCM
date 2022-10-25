#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WebSocketsServer.h>
#include <Firebase_ESP_Client.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include "OLED.h"
#include <time.h>

int relay = 12;
PZEM004Tv30 pzem(&Serial);
OLED display(4, 5);

int timezone = 7*3600;
int dst=0;

WebSocketsServer webSocket = WebSocketsServer(81);
const IPAddress apIP(192, 168, 4, 1);
const char* apSSID = "FeeeUteHCM";

int indexthang;
int indexbang;
int indexva;
int indexacong;
String inputString = ""; 
String username = "";
String password = "";
String Token = "";
String Phong = "";
String Phong1 = "";

String Phong3 = "";
String Phong4 = "";
String Token1 = "";
String Token3 = "";
String Token4 = "";
String ssid = "";
String pass = "";
bool stringComplete = false;
boolean receivedata;

// button
static const int buttonPin = 13;                    
int buttonStatePrevious = LOW;                      
unsigned long minButtonLongPressDuration = 5000;    
unsigned long buttonLongPressMillis;                
bool buttonStateLongPress = false;                  
const int intervalButton = 50;                      
unsigned long previousButtonMillis;                 
unsigned long buttonPressDuration;                  
unsigned long currentMillis; 

// Firesotre
#include "addons/TokenHelper.h"

#define API_KEY "xxxxxx"

#define FIREBASE_PROJECT_ID "hieunhan-8d2a7"

#define USER_EMAIL "nhan@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long dataMillis = 0;
int count = 0;
boolean sendfirestore;
boolean connectfiretore;
boolean sendok;
bool value = false;
bool resetpz = false;
bool taskcomplete = false;   
bool relayfirestore = false;
bool relayfirestore1 = false;

void setup() {
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  pinMode(buttonPin, INPUT);
  Serial.begin(9600);
  EEPROM.begin(512);
  display.begin();
  display.clear();
  delay(100);       
  if (restoreConfig()) {                      
    checkConnection();           
  }
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
// firestore

}

void loop() {
  readButtonState();
  webSocket.loop();
  if(sendfirestore){
    if(connectfiretore){
      configTime(timezone, dst, "pool.ntp.org","time.nist.gov");
      Serial.println("Witting Connect Timezone");
      while(!time(nullptr)){
        Serial.print("*");
        delay(500);
      }
      Serial.println("OK!");
      Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
      config.api_key = API_KEY;
      auth.user.email = USER_EMAIL;
      auth.user.password = USER_PASSWORD;
      config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
      Firebase.begin(&config, &auth);
      display.clear();   
      Firebase.reconnectWiFi(true);
      connectfiretore = false;   
    }
    firestorepzem();
  }
}

// read EEPROM
boolean restoreConfig() {
  if (EEPROM.read(0) != 0) {                           //neu duu lieu doc ra tu EEPROM khac 0 thi doc du lieui  
    for (int i = 0; i < 31; ++i) {                //32 o nho dau tieu la chua ten mang wifi SSID
      ssid += char(EEPROM.read(i));
    }
    Serial.println("Reading username to EEPROM: "); 
    Serial.println(ssid); 
    for (int i = 32; i < 95; ++i) {               //o nho tu 32 den 96 la chua PASSWORD
      pass += char(EEPROM.read(i));
    }
    Serial.println("Reading Password to EEPROM: "); 
    Serial.println(pass);
    for (int i = 96; i < 499; ++i) {                //32 o nho dau tieu la chua ten mang wifi SSID
      Token1 += char(EEPROM.read(i));
    }
    Serial.println("Reading Token to EEPROM: "); 
    Serial.println(Token1);
    delay(100);
    String Token2 = "%" + Token1 + "$" ;
    int indextien;
    int indexphantram;
    indexphantram = Token2.indexOf("%");
    indextien = Token2.indexOf("$");
    Token3 = Token2.substring(indexphantram+1, indextien);
    Token4 = Token2.substring(indextien+1);
    for (int i = 500; i < 512; ++i) {                //32 o nho dau tieu la chua ten mang wifi SSID
      Phong1 += char(EEPROM.read(i));
    }
    Serial.println("Reading Token to EEPROM: "); 
    Serial.println(Phong1);
    delay(100);
    String Phong2 = "^" + Phong1 + "*" ;
    int indexmu;
    int indexsao;
    indexmu = Phong2.indexOf("^");
    indexsao = Phong2.indexOf("*");
    Phong3 = Phong2.substring(indexmu+1, indexsao);
    Phong4 = Phong2.substring(indexsao+1);
    display.clear();
    WiFi.begin(ssid.c_str(), pass.c_str());
    return true;
  }
  else {
    Serial.println("Config not found.");
    setupMode();
    return false;
  }
}
// check connect WiFi
boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {      //neu ket noi thanh cong thi in ra connected!
      Serial.println();
      Serial.println("Connected!");
      Serial.println(WiFi.localIP()); 
      delay(1000);
      sendfirestore = true;    
      connectfiretore = true;
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  setupMode();
  return false;
}
// che do Access point
void setupMode(){
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);               
  WiFi.softAP(apSSID);             
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
  display.clear();
  display.print("WiFi: OFF", 0, 3);
  receivedata = true ;
}
// nhan giu lieu tu Client
void webSocketEvent(uint8_t num, WStype_t type,
                    uint8_t * payload,
                    size_t length)
{
  String payloadString = "";
  payloadString = (const char *)payload;
  sendok = true;
  if(sendok){
    webSocket.broadcastTXT("OK");
    sendok = false;
  }
  inputString = payloadString;
  if(receivedata){
    for(int n = 10; n < inputString.length() ; n++ ){
    if(n >= ( inputString.length()-1) ){stringComplete = true;}
    }
    if (stringComplete) {
      indexthang = inputString.indexOf("/");
      indexbang = inputString.indexOf("=");
      indexva = inputString.indexOf("&");
      indexacong = inputString.indexOf("@");
      username = inputString.substring(indexthang+1,indexbang);
      password = inputString.substring(indexbang+1,indexva);
      Token = inputString.substring(indexva+1, indexacong);
      Phong = inputString.substring(indexacong+1);
      Serial.println(username);
      Serial.println(password);
      Serial.println(Token);
      Serial.println(Phong);
      inputString = "";
      display.clear();
      display.print("Waiting Connect", 3, 1);
      display.print("WiFi..... ", 4, 3);
      WiFi.mode(WIFI_STA);            //che do hoat dong la May Tram Station
      for (int i = 0; i < 512; ++i) {
        EEPROM.write(i, 0);               //xoa bo nho EEPROM
        delay(10);
      }
      Serial.println("Writing username to EEPROM...");
      for (int i = 0; i < username.length(); ++i) {
        EEPROM.write(i, username[i]);
      }
      Serial.println(username);
      Serial.println("Writing Password to EEPROM...");
          for (int i = 0; i < password.length(); ++i) {
            EEPROM.write(32 + i, password[i]);
      }
      Serial.println(password);
      Serial.println("Writing ToKen to EEPROM...");
          for (int i = 0; i < Token.length(); ++i) {
            EEPROM.write(96 + i, Token[i]);
      }
      Serial.println(Token);
      Serial.println("Writing ToKen to EEPROM...");
          for (int i = 0; i < Phong.length(); ++i) {
            EEPROM.write(500 + i, Phong[i]);
      }
      Serial.println(Phong);
      EEPROM.commit();
      stringComplete = false;
      receivedata = false ;
      if (restoreConfig()) {
        checkConnection();
      }  
    }
  }
}
// Button reset EEPROM 5s
void readButtonState() {
  currentMillis = millis(); 
  if(currentMillis - previousButtonMillis > intervalButton) {
    int buttonState = digitalRead(buttonPin);   
    if (buttonState == HIGH && buttonStatePrevious == LOW && !buttonStateLongPress) {
      buttonLongPressMillis = currentMillis;
      buttonStatePrevious = HIGH;
    }
    buttonPressDuration = currentMillis - buttonLongPressMillis;
    if (buttonState == HIGH && !buttonStateLongPress && buttonPressDuration >= minButtonLongPressDuration) {
      buttonStateLongPress = true;
      for (int i = 0; i < 512; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      Serial.println("Reset EEPROM");
      Token1.remove(0);
      Phong1.remove(0);
      ssid.remove(0);
      pass.remove(0);
      display.clear();
      WiFi.disconnect();
      Serial.println("Disconnect WiFi");
      taskcomplete = false;   
      sendfirestore = false;
      delay(500);
      setupMode();
    }
    if (buttonState == LOW && buttonStatePrevious == HIGH) {
      buttonStatePrevious = LOW;
      buttonStateLongPress = false;
    }
    previousButtonMillis = currentMillis;
  }
}

void firestorepzem(){
    time_t now=time(nullptr);
    struct tm* p_tm=localtime(&now);
    Serial.print(p_tm ->tm_mday);
    Serial.print("/");
    Serial.print(p_tm ->tm_mon + 1);
    Serial.print("/");
    Serial.print(p_tm ->tm_year + 1900);
    Serial.print("   ");
    Serial.print(p_tm ->tm_hour);
    Serial.print(":");
    Serial.print(p_tm ->tm_min);
    Serial.print(":");
    Serial.print(p_tm ->tm_sec);
    Serial.print("\n");
    display.print("WiFi: ON", 0, 3);
  // ngay
    char day[3];
    String strday;
    strday += p_tm ->tm_mday;
    strday.toCharArray(day, 3);
    display.print(day, 1, 3);
  // thang
    char mon[2];
    String strmon;
    strmon += p_tm ->tm_mon + 1;
    strmon.toCharArray(mon, 2);
    display.print("/", 1, 5);
    display.print(mon, 1, 6);
  // nam
    char year[5];
    String stryear;
    stryear += p_tm ->tm_year + 1900;
    stryear.toCharArray(year, 5);
    display.print("/", 1, 8);
    display.print(year, 1, 9);
  // gio 
    char hour[3];
    String strhour;
    strhour += p_tm ->tm_hour;
    strhour.toCharArray(hour, 3);
    display.print(hour, 2, 5);
  // phut
    char min[3]; 
    String strmin;
    strmin += p_tm ->tm_min;
    strmin.toCharArray(min, 3);
    display.print(":", 2, 7);
    display.print(min, 2, 8);
//  // giay
//    char sec[3];
//    String strsec;
//    strsec += p_tm ->tm_sec;
//    strsec.toCharArray(sec, 3);
//    display.print(":", 2, 9);
//    display.print(sec, 2, 10);
  // dien ap 
    float voltage = pzem.voltage();
    if( !isnan(voltage) ){
      char charvol[7];
      String strvol;
      strvol += voltage;
      strvol.toCharArray(charvol, 7);
      display.print("V:", 3, 1);
      display.print(charvol, 3, 4);
      display.print("V", 3, 10);
    }
  // dong dien
    float current = pzem.current();
    if( !isnan(current) ){
      char charcur[7];
      String strcur;
      strcur += current;
      strcur.toCharArray(charcur, 7);
      display.print("I:", 4, 1);
      display.print(charcur, 4, 4);
      display.print("A", 4, 9);
    }
  // cong suat
    float power = pzem.power();
    if( !isnan(power) ){
      char charpow[6];
      String strpow;
      strpow += power;
      strpow.toCharArray(charpow, 6);
      display.print("P:", 5, 1);
      display.print(charpow, 5, 5);
      display.print("W", 5, 9); 
    }
  // cong suat tieu thu
    float energy = pzem.energy();
    if( !isnan(energy) ){
      char charene[7];
      String strene;
      strene += energy;
      strene.toCharArray(charene, 7);
      display.print("E:", 6, 1);
      display.print(charene, 6, 5);
      display.print("kWh", 6, 10);
    }
  // tan so
    float frequency = pzem.frequency();
     if( !isnan(frequency) ){
      char charfre[5];
      String strfre;
      strfre += frequency;
      strfre.toCharArray(charfre, 5);
      display.print("F:", 7, 1);
      display.print(charfre, 7, 4);
      display.print("Hz", 7, 8); 
     }
    display.on();
    delay(500);
     String firestoretoken = "energy/" + Token3 + "/" + stryear + "/" + strmon + "/" + strday + "/" ;
     String firestoretoken1 = "energy/" + Token3 + "/" + stryear + "/" + strmon + "/mony/" + Phong3;
      if(!taskcomplete){ 
        taskcomplete = true;          
        String content;
        FirebaseJson js;
        String documentPath = firestoretoken; 
        js.set("fields/Key/stringValue", Phong3);
        js.set("fields/value/booleanValue", true);
        js.set("fields/resetpz/booleanValue", false);
        js.set("fields/offpz/booleanValue", false);
        js.set("fields/warningPower/doubleValue", 10000);
        js.toString(content);
        Serial.print("Create a document... ");
        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.c_str()))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
           Serial.println(fbdo.errorReason());
        delay(1000);
        
        String documentPath1 = firestoretoken1;
        String content1;   
        js.toString(content1);
        Serial.print("Create a document... ");
        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath1.c_str(), content1.c_str()))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
           Serial.println(fbdo.errorReason());
      }   
// firestore
    if (Firebase.ready() && (millis() - dataMillis > 1000 || dataMillis == 0))
    {
       dataMillis = millis();
        String content;
        FirebaseJson js;        
        String documentPath = firestoretoken1; 
        float current = pzem.current();  
        float power = pzem.power();
        float pf = pzem.pf();
        float frequency = pzem.frequency();
        float energy = pzem.energy();
        float voltage = pzem.voltage();
        if( !isnan(current) ){
          js.set("fields/Current/doubleValue", current);
          if( !isnan(power) ){
            js.set("fields/Power/doubleValue", power);
          }
          if( !isnan(pf) ){
            js.set("fields/PowerFactor/doubleValue", pf);
          }
          if( !isnan(frequency) ){
            js.set("fields/Frequency/doubleValue", frequency);
          }
          if( !isnan(energy) ){
            js.set("fields/TotalEnergy/doubleValue", energy);
          }
          if( !isnan(voltage) ){
            js.set("fields/Voltage/doubleValue", voltage);
          }
          js.toString(content);     
           Serial.print("Update a document... ");
          if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.c_str(), "Current,Power,PowerFactor,Frequency,TotalEnergy,Voltage" /* updateMask */))
              Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
          else
              Serial.println(fbdo.errorReason());

//          String documentPath1 = firestoretoken1; 
//          js.toString(content); 
//          Serial.print("Update a document... ");
//          if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath1.c_str(), content.c_str(), "Current,Power,PowerFactor,Frequency,TotalEnergy,Voltage" /* updateMask */))
//              Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//          else
//              Serial.println(fbdo.errorReason());
        }
        else{
            Serial.println("Not update firebase");
        }
        
//        String documentPath1 = firestoretoken1; 
//        if( !isnan(current) ){
//          js.set("fields/Current/doubleValue", current);
//          if( !isnan(power) ){
//            js.set("fields/Power/doubleValue", power);
//          }
//          if( !isnan(pf) ){
//            js.set("fields/PowerFactor/doubleValue", pf);
//          }
//          if( !isnan(frequency) ){
//            js.set("fields/Frequency/doubleValue", frequency);
//          }
//          if( !isnan(energy) ){
//            js.set("fields/TotalEnergy/doubleValue", energy);
//          }
//          if( !isnan(voltage) ){
//            js.set("fields/Voltage/doubleValue", voltage);
//          }
//          js.toString(content); 
//           Serial.print("Update a document... ");
//          if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath1.c_str(), content.c_str(), "Current,Power,PowerFactor,Frequency,TotalEnergy,Voltage" /* updateMask */))
//              Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//          else
//              Serial.println(fbdo.errorReason());
//        }
//        else{
//            Serial.println("Not update firebase");
//        }
    }
    Serial.print("Get a document... ");
    String documentPath = firestoretoken1; 
    String content;
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.c_str())) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      // Create a FirebaseJson object and set content with received payload
      FirebaseJson payload;
      payload.setJsonData(fbdo.payload().c_str());
      FirebaseJsonData jsonData;
      payload.get(jsonData,"fields/value/booleanValue", true); 
      bool value = jsonData.boolValue;
      Serial.println(value);          
      if(value == true){
        digitalWrite(relay, HIGH);
      }
      if(value == false){
        digitalWrite(relay, LOW);
      }
     payload.get(jsonData,"fields/resetpz/booleanValue", true);
     Serial.println(jsonData.boolValue); 
     if(jsonData.boolValue == true){
        delay(1000);
        pzem.resetEnergy();
        delay(1000);
     }  
     payload.get(jsonData,"fields/offpz/booleanValue", true);
     bool offpz = jsonData.boolValue;
     Serial.println(offpz); 
     int power = pzem.power();
     Serial.println(power);   
     payload.get(jsonData,"fields/warningPower/integerValue", true);
     Serial.println(jsonData.intValue);            
     if(jsonData.intValue <= power && offpz == true && value == true ){
          digitalWrite(relay, LOW);
          Serial.println("ngat tai");   
          String content;
          FirebaseJson js;       
          String documentPath2 = firestoretoken1;
          js.set("fields/value/booleanValue", false);
          js.toString(content); 
          if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath2.c_str(), content.c_str(), "value" ))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
          else
            Serial.println(fbdo.errorReason());
        }
//     if(jsonData.intValue >= power && offpz == false && value == false){
//            digitalWrite(relay, HIGH);
//            Serial.println("bat tai");   
//            String content;
//            FirebaseJson js;       
//            String documentPath2 = firestoretoken1;
//            js.set("fields/value/booleanValue", true);
//            js.toString(content); 
//            if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath2.c_str(), content.c_str(), "value"))
//              Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
//            else
//              Serial.println(fbdo.errorReason());
//         }
    }
}
