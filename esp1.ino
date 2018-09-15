
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#define HOSTNAME "WJ-HOME-SmartGarden-OTA-" ///< Hostename. The setup function adds the Chip ID at the end.
#include <BlynkSimpleEsp8266.h>
#define TRIGGERPIN D1
#define ECHOPIN    D2
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <SimpleTimer.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
//char auth[] = "c803cba4d6ad41aeac7c676df5ab5a51";
char blynk_token[34] = "c803cba4d6ad41aeac7c676df5ab5a51";
SimpleTimer timer1;

WidgetRTC rtc;
WidgetTerminal terminal(V9);

int min1;
int max1;
long startTimeInSecs;
int duration = 20;
// Your WiFi credentials.
// Set password to "" for open networks.
//char ssid[] = "WJ-HOME";
//char pass[] = "home1411";
bool shouldSaveConfig = false;
bool toggled = false;
//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
String hostname(HOSTNAME);
BlynkTimer timer;
BLYNK_CONNECTED() {
  //get data stored in virtual pin V0 from server
  Blynk.syncVirtual(V4);
  Blynk.syncVirtual(V5);
  Blynk.syncVirtual(V8);
  Blynk.syncVirtual(V10);
}
BLYNK_WRITE(V4)
{
  //restoring int value
  max1 = param.asInt();
}
BLYNK_WRITE(V10)
{
  //restoring int value
  duration = param.asInt();
  Serial.println(duration);
}
BLYNK_WRITE(V8) {
  startTimeInSecs = param[0].asLong();
  Serial.println(startTimeInSecs);
  Serial.println();
}
BLYNK_WRITE(V5)
{
  //restoring int value
  min1 = param.asInt();
}
void ota_init() {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

int wifisignal() {
  return map(WiFi.RSSI(), -105, -40, 0, 100);
}
void clockvalue() // Digital clock display of the time
{

  int gmthour = hour();
  if (gmthour == 24) {
    gmthour = 0;
  }
  String displayhour =   String(gmthour, DEC);
  int hourdigits = displayhour.length();
  if (hourdigits == 1) {
    displayhour = "0" + displayhour;
  }
  String displayminute = String(minute(), DEC);
  int minutedigits = displayminute.length();
  if (minutedigits == 1) {
    displayminute = "0" + displayminute;
  }
  String displaysecond = String(second(), DEC);
  int seconddigits = displaysecond.length();
  if (seconddigits == 1) {
    displaysecond = "0" + displaysecond;
  }


  Blynk.virtualWrite(V11, displayhour + ":" + displayminute + ":" + displaysecond );
  Blynk.virtualWrite(V12, wifisignal() );

}
class tt {
  public:
    int h = 0;
    int m = 0;
    int s = 0;
    tt(int hh, int mm, int ss)
    { h = hh;
      m = mm;
      s = ss;
    }
    void to_F(void) {
      to_s();
      m = s / 60; //convert seconds to minutes
      h = m / 60; //convert minutes to hours
      s = s - (m * 60); //subtract the coverted seconds to minutes in order to display 59 secs max
      m = m - (h * 60); //subtract the coverted minutes to hours in order to display 59 minutes max
    }
    void to_s() {
      s += m * 60;
      s += h * 60 * 60;
      m = 0;
      h = 0;
    }
};
void turnon()
{
  tt t(0, 0, startTimeInSecs);
  t.to_F();
  Serial.println(String(t.h, DEC) + ':' + String(t.m, DEC) + ':' + String(t.s, DEC) + '\t' + String(hour(), DEC) + ':' + String(minute(), DEC) + ':' + String(second(), DEC));
  if (hour() == t.h && minute() == t.m && second() == t.s) {
    Blynk.virtualWrite(D0, LOW) ;
    Serial.println("Watering");
    digitalWrite(D0, LOW);
    toggled = true;
  }
}

void turnoff()
{
  tt t(0, 0, startTimeInSecs + duration);
  t.to_F();
  if (hour() == t.h && minute() == t.m && second() == t.s) {
    Blynk.virtualWrite(D0, HIGH) ;
    Serial.println("stop");
    digitalWrite(D0, HIGH);
    toggled = false;
  }
}



void level_2()
{
  int lev2 = 100 * ((digitalRead(D5) * 0.34) + (digitalRead(D6) * 0.34) + (digitalRead(D7) * 0.34) + (digitalRead(D8) * 0.34));
  Blynk.virtualWrite(V2, lev2 );
  if (lev2 > 99) {
    digitalWrite(D0, HIGH);
    Blynk.virtualWrite(D0, HIGH);
  }
  clockvalue();
  //turnon();

}
void level_1()
{
  long duration, distance;
  digitalWrite(TRIGGERPIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIGGERPIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(TRIGGERPIN, LOW);
  duration = pulseIn(ECHOPIN, HIGH);
  distance = (duration / 2) / 29.1;
  Blynk.virtualWrite(V3, int(distance));
  //Serial.println(int(duration));
  Blynk.virtualWrite(V1, map(distance, max1, min1, 100, 0));
  turnon();
  turnoff();
  if (map(distance, max1, min1, 100, 0) <= 0) {
    digitalWrite(D0, HIGH);
    Blynk.virtualWrite(D0, HIGH);
  }
}
void pin_init() {
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);
  pinMode(TRIGGERPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  pinMode(D8, INPUT);
}
void setup()
{
  // Debug console
  Serial.begin(115200);
  Serial.println("Booting");


  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
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

          //strcpy(mqtt_server, json["mqtt_server"]);
          //strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);

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
  //WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  //WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 33);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  //wifiManager.addParameter(&custom_mqtt_server);
  //wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_blynk_token);

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
  wifiManager.autoConnect("WJ-HOME-SmartGarden");
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  //strcpy(mqtt_server, custom_mqtt_server.getValue());
  //strcpy(mqtt_port, custom_mqtt_porgetValue());
  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    //json["mqtt_server"] = mqtt_server;
    //json["mqtt_port"] = mqtt_port;
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Blynk.config(blynk_token);
  pin_init();
  Blynk.begin(blynk_token, WiFi.SSID().c_str(), WiFi.psk().c_str());
  rtc.begin();
  ota_init();
  //  BLYNK_LOG("Current time: %02d:%02d:%02d %02d %02d %d",hour(), minute(), second(),day(), month(), year());
  //  BLYNK_LOG("local ip:",String(WiFi.localIP())," ssid:",WiFi.SSID().c_str());
  //Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, level_2);
  timer.setInterval(1000L, level_1);
}

void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
}
