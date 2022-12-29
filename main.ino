// all testing was done with (reflink); https://opencircuit.nl/product/lilygo-ttgo-t-call-esp32-with-sim800c?cl=DC9ZYNYEF9&cid=SMS%20Proxy
// call forwarding in regards to Dutch perpaid cards; https://forum.kpn.com/prepaid-16/doorschakelen-met-een-prepaid-478998

#define SIM800C_AXP192_VERSION_20200609
#define TINY_GSM_MODEM_SIM800

#include <Arduino.h>
#include <TinyGsmClient.h>
#include "utilities.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMonitor Serial
// Set serial for AT commands (to the module)
#define SerialSIM800  Serial1

TinyGsm modem(SerialSIM800);

String fullsms;
const String PHONE_NUMBER = "+31612345678"; //always with country code like this +31612345678
const char* SSID          = "";
const char* PASSWORD      = "";
String HEARBEAT_FULL_URL  = ""; //Free and good service I personally use for this (hearbeat); https://cronitor.io/
unsigned long HEARTBEAT_INTERVAL     = 600000; // 600000 milliseconds - 10 min
unsigned long HEARTBEAT_LAST_TIME   = 0; //// milliseconds

void setupModem()
{
#ifdef MODEM_RST
  // Keep reset high
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, HIGH);
#endif

  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  // Turn on the Modem power first
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Pull down PWRKEY for more than 1 second according to manual requirements
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);

  // Initialize the indicator as an output
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LED_OFF);
}

void setupSIM800()
{
  // SerialMonitor.println("Configuring SIM800 to redirect phone calls... (method 1)");
  // SerialSIM800.println("AT+CCFC=0,3,\"" + PHONE_NUMBER + "\",145");

  // SerialMonitor.println("Configuring SIM800 to redirect phone calls... (method 2)"); // was not able to test, use modem as it's nicer
  // SerialSIM800.println("AT+CUSD=1,\"**21*" + PHONE_NUMBER + "\"#");
  // modem.sendUSSD("**21*" + PHONE_NUMBER + "\"#")

  SerialMonitor.println("Configuring SIM800 to TEXT mode...");
  SerialSIM800.println("AT+CMGF=1"); // Configuring TEXT mode

  SerialMonitor.println("Configuring SIM800 to handle SMS messages...");
  SerialSIM800.println("AT+CNMI=1,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
}

void setupESP(){
  WiFi.begin(SSID, PASSWORD);
  SerialMonitor.println("Connecting to Wi-Fi network");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    SerialMonitor.print(".");
  }
  SerialMonitor.println("");
  SerialMonitor.print("Connected to WiFi network with IP Address: ");
  SerialMonitor.println(WiFi.localIP());
}

void initializeModem(){
  SerialMonitor.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMonitor.print("Modem Info: ");
  SerialMonitor.println(modemInfo);

  SerialMonitor.println("Waiting for network...");
  if (!modem.waitForNetwork(60000L, true)) {
    SerialMonitor.println("Network connection with SIM800 failed - this is possible during the very first setup with a new simcard");
    return;
  }
  SerialMonitor.println("Network connection with SIM800 success");
}

void setup()
{
  // Set console baud rate
  SerialMonitor.begin(115200);

  // Set GSM module baud rate and UART pins
  SerialSIM800.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  setupModem(); // Not using TinyGSM module for setup, as this is module specific (see also utilities.h) - feel free to replace tho if possible

  initializeModem();

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

  setupSIM800();

  setupESP();

  initialHello(); // this will fail for some providers, but that's OK - there is a catch later down in the code
}

void heartbeat(){
  //Send heartbeat only if wifi (internet) is available AND the SIM800 is registered, connected and has a non-zero strength to a network (this should catch power failure, network failure and sim800 failure)
  if(WiFi.status()== WL_CONNECTED && modem.getRegistrationStatus() == 1 && modem.isNetworkConnected() == 1 && modem.getSignalQuality() != 99 && modem.getSignalQuality() != 0){
    SerialMonitor.print("Attempting to send heartbeat...");

    HTTPClient http;

    String serverPath = HEARBEAT_FULL_URL;
    
    http.begin(serverPath.c_str());
    
    // Send HTTP GET request
    int httpResponseCode = http.GET();
    
    if (httpResponseCode>0) {
      SerialMonitor.print("HTTP Response code: ");
      SerialMonitor.println(httpResponseCode);
      String payload = http.getString();
      SerialMonitor.println(payload);
    }
    else {
      SerialMonitor.print("Error code: ");
      SerialMonitor.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    SerialMonitor.println("Heartbeat failure! Either Wi-Fi (internet) or SIM800 is failing");
  }
}

void loop()
{ // Beware that the ESP is single threaded! So if an SMS is being forwarded -> it can't send a heartbeat at the same time. Shouldn't be too big of an issue - as long as your health check allows for minor (minute max) margin.
  readSerial();
  
  if ((millis() - HEARTBEAT_LAST_TIME) > HEARTBEAT_INTERVAL) {
    heartbeat();
    HEARTBEAT_LAST_TIME = millis();
  }
  
}

void readSerial()
{
  while (SerialSIM800.available()) {
    parseData(SerialSIM800.readString());      
  }
  while (SerialMonitor.available()) {
    SerialSIM800.println(SerialMonitor.readString()); 
  }
}

// void updateSerial()
// {
//   while (SerialSIM800.available()) {
//     SerialMonitor.write(SerialSIM800.read());
//   }
//   while (SerialMonitor.available()) {
//     SerialSIM800.write(SerialMonitor.read());
//   }
// }

void parseData(String buff){
  Serial.println(buff);

  unsigned int len, index;

  index = buff.indexOf("\r");
  buff.remove(0, index+2);
  buff.trim();

  if(buff != "OK"){
    index = buff.indexOf(":");
    String cmd = buff.substring(0, index);
    cmd.trim();
    
    buff.remove(0, index+2);
    
    if(cmd == "+CMT"){
      extractSms(buff);
      SerialMonitor.println("Forwarding SMS to owner...");
      sendSms(fullsms);
    }
    else if(cmd == "SMS Ready"){ // depending on the provider - this is not always send when ready - hence the initialHello is at multiple places
      initialHello();
    }
  }
}

void sendSms(String content){
  modem.sendSMS(PHONE_NUMBER, content);
  SerialSIM800.write("AT+CMGDA=\"DEL ALL\""); //delete all messages from sim - this is required to avoid storage fill up
}

void initialHello(){
  SerialMonitor.println("Sending SMS to owner to share phone number and verify connectivity..."); //it's also possible to get it via serial with (lyca mobile - *102# number) AT+CUSD=1,"*102#" - modem.sendUSSD("*111#") - note the code depends on your sim card provider!
  sendSms("Beep beep boop");
}

void extractSms(String buff){
  buff.trim();
  fullsms = buff;
  // fullsms.toLowerCase();
}