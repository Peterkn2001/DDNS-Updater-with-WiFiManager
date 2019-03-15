#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <SimpleTimer.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson Expects version 5.13.5 - DOES NOT WORK WITH V6.xx

#include <EasyDDNS.h>             //For NoIP Updater https://github.com/ayushsharma82/EasyDDNS
WiFiServer server(80);            //For NoIP Updater 

//define your default values here, if there are different values in config.json, they are overwritten.
char NoIP_Domain[40] = "";
char NoIP_Username[40]= "";
char NoIP_Password[40]= "";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(74880);
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format(); //***************************************** Un-Comment this and run once if you need to format SPIFFS *****************************************

  //read configuration from FS json
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
          strcpy(NoIP_Domain, json["NoIP_Domain"]);
          strcpy(NoIP_Username, json["NoIP_Username"]);          
          strcpy(NoIP_Password, json["NoIP_Password"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_NoIP_Domain("NoIP Domain", "NoIP Domain", NoIP_Domain, 40);
  WiFiManagerParameter custom_NoIP_Username("NoIP Username", "NoIP Username", NoIP_Username, 40);
  WiFiManagerParameter custom_NoIP_Password("NoIP Password", "NoIP Password", NoIP_Password, 40);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_NoIP_Domain);
  wifiManager.addParameter(&custom_NoIP_Username);
  wifiManager.addParameter(&custom_NoIP_Password);
    
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("NoIP Updater")) {   // Satrt portal with SSID "NoIP Updater" and no password
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(NoIP_Domain, custom_NoIP_Domain.getValue());
  strcpy(NoIP_Username, custom_NoIP_Username.getValue());
  strcpy(NoIP_Password, custom_NoIP_Password.getValue());
    
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["NoIP_Domain"] = NoIP_Domain;
    json["NoIP_Username"] = NoIP_Username;
    json["NoIP_Password"] = NoIP_Password;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  Serial.println();
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("NoIP_Domain: ");
  Serial.println(NoIP_Domain);
  Serial.print("NoIP_Username: "); 
  Serial.println(NoIP_Username);
  Serial.print("NoIP_Password: ");    
  Serial.println(NoIP_Password);  

  // Set-up the services needed for the No-IP Updater routine....
  WiFi.mode(WIFI_STA);
  server.begin();
  EasyDDNS.service("noip");    // DDNS Service Name - "noip"
  EasyDDNS.client(NoIP_Domain, NoIP_Username, NoIP_Password);    // Enter ddns Domain & Username & Password | Example - "esp.duckdns.org","username", "password"
}


void loop()
{
  EasyDDNS.update(10000); // Check for New Ip Every 10 Seconds.
}
