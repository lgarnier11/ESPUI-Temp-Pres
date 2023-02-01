/*
 This code has been made by Neodyme under the MIT license
 Youtube : https://www.youtube.com/neodymetv
 Twitch : https://www.twitch.tv/ioodyme
 Github : https://github.com/n3odym3
 ESPUI documentation : https://github.com/s00500/ESPUI#readme
*/
#include <Arduino.h>

#include <Adafruit_BMP280.h>

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C

#include <DS18B20.h>

DS18B20 ds(0);
unsigned long last_temp_millis = 0;
unsigned long temp_delay = 1000;

unsigned long last_pres_millis = 0;
unsigned long pres_delay = 1000;

// G22 -> SCL
// G21 -> SDA
// VCC -> 3.3
// GND -> GND

// 5V -> res -> J
// 5V -> R
// GND -> N


//Web server==================================
#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#define LED_BUILTIN 2
#else
#error Architecture unrecognized by this code.
#endif

#include <Preferences.h>
Preferences preferences;
#include <DNSServer.h>
#define DNS_PORT 53
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
const char* hostname = "ESPUI-Temp-Pression-1.0";
bool wificonnected = false;
//Web server==================================

//MQTT=========================
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);
#define mqtt_retry_delay 10000
unsigned long last_millis = 0;
//MQTT=========================

//ESPUI=================================================================================================================
#include <ESPUI.h>
uint16_t wifi_ssid_text, wifi_pass_text, wifi_ssid_timeout_text;
uint16_t mesure_temp_text, mesure_pres_text;
uint16_t mqtt_server_text, mqtt_topic_in_text, mqtt_topic_out_text, mqtt_user_text, mqtt_pass_text, mqtt_enabled_switch;
uint16_t statusLabelId, serialLabelId;
String option;
String stored_ssid, stored_pass;
int stored_ssid_timeout;
String stored_mqtt_server, stored_mqtt_user, stored_mqtt_pass, stored_mqtt_topic_in, stored_mqtt_topic_out;

bool mqtt_enabled = false;
//ESPUI==================================================================================================================

//Custom libraries..............
//Your code HERE !
//Default ESPUI callback======================
void textCallback(Control *sender, int type) {
}
//Default ESPUI callback======================
//Split String===========================================================
String splitString(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//Split String===========================================================

//WiFi settings callback=====================================================
void SaveWifiDetailsCallback(Control *sender, int type) {
  if (type == B_UP) {
    stored_ssid = ESPUI.getControl(wifi_ssid_text)->value;
    stored_pass = ESPUI.getControl(wifi_pass_text)->value;
    stored_ssid_timeout = ESPUI.getControl(wifi_ssid_timeout_text)->value.toInt();
    stored_mqtt_topic_in = ESPUI.getControl(mqtt_topic_in_text)->value;
    stored_mqtt_topic_out = ESPUI.getControl(mqtt_topic_out_text)->value;
    stored_mqtt_server = String(ESPUI.getControl(mqtt_server_text)->value);
    stored_mqtt_user = String(ESPUI.getControl(mqtt_user_text)->value);
    stored_mqtt_pass = String(ESPUI.getControl(mqtt_pass_text)->value);
    mqtt_enabled = ESPUI.getControl(mqtt_enabled_switch)->value.toInt() ? true : false;

    preferences.putString("ssid", stored_ssid);
    preferences.putString("pass", stored_pass);
    preferences.putInt("ssid_timeout", stored_ssid_timeout);
    preferences.putString("mqtt_server", stored_mqtt_server);
    preferences.putString("mqtt_user", stored_mqtt_user);
    preferences.putString("mqtt_pass", stored_mqtt_pass);
    preferences.putString("mqtt_topic_in", stored_mqtt_topic_in);
    preferences.putString("mqtt_topic_out", stored_mqtt_topic_out);
    preferences.putBool("mqtt_enabled", mqtt_enabled);

    Serial.println(stored_ssid);
    Serial.println(stored_pass);
    Serial.println(stored_ssid_timeout);
    Serial.println(stored_mqtt_server);
    Serial.println(stored_mqtt_user);
    Serial.println(stored_mqtt_pass);
    Serial.println(stored_mqtt_topic_in);
    Serial.println(stored_mqtt_topic_out);
    Serial.println(mqtt_enabled);

    Serial.println("Saving settings");
  }
}
//WiFi settings callback=====================================================

//ESP Reset=================================
void ESPReset(Control *sender, int type) {
  if (type == B_UP) {
    //ESPUI.print(mesure_temp_text,)
  }
}
//ESP Reset=================================
//CMD MESURER=================================
void cmdMesurer(Control *sender, int type) {
  if (type == B_UP) {
    ESP.restart();
  }
}
//CMD MESURER=================================

//ESPUI=====================================================================================================================
void espui_init() {
  ESPUI.setVerbosity(Verbosity::Quiet);


  //Custom UI..............................................................................
  //Your code HERE !
  // auto demo_tab = ESPUI.addControl(Tab, "", "demo");
  // auto demo_button = ESPUI.addControl(Button, "", "Button", None, demo_tab, textCallback);
  //Custom UI..............................................................................

//Mesure-------------------------------------------------------------------------------------------------------------------
  auto mesuretab = ESPUI.addControl(Tab, "", "Mesures");
  mesure_temp_text = ESPUI.addControl(Label, "Température", stored_ssid, Alizarin, mesuretab, textCallback);
  mesure_pres_text = ESPUI.addControl(Label, "Pression", stored_pass, Alizarin, mesuretab, textCallback);
  // auto cmdMesure = ESPUI.addControl(Button, "", "Mesurer", None, mesuretab, ESPReset);
  //WiFi-------------------------------------------------------------------------------------------------------------------
  
  //Maintab-----------------------------------------------------------------------------------------------
  auto maintab = ESPUI.addControl(Tab, "", "Debug");
  serialLabelId = ESPUI.addControl(Label, "Serial", "Serial IN", Peterriver, maintab, textCallback);
  statusLabelId = ESPUI.addControl(Label, "", "Serial OUT", Peterriver, serialLabelId, textCallback);
  //Maintab-----------------------------------------------------------------------------------------------

  //WiFi-------------------------------------------------------------------------------------------------------------------
  auto wifitab = ESPUI.addControl(Tab, "", "WiFi");
  wifi_ssid_text = ESPUI.addControl(Text, "SSID", stored_ssid, Alizarin, wifitab, textCallback);
  wifi_pass_text = ESPUI.addControl(Text, "Password", stored_pass, Alizarin, wifitab, textCallback);
  wifi_ssid_timeout_text = ESPUI.addControl(Text, "SSID Timeout", String(stored_ssid_timeout), Alizarin, wifitab, textCallback);
  ESPUI.setInputType(wifi_pass_text, "password");
  ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
  ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
  mqtt_server_text = ESPUI.addControl(Text, "MQTT server", stored_mqtt_server, Alizarin, wifitab, textCallback);
  mqtt_user_text = ESPUI.addControl(Text, "MQTT user", stored_mqtt_user, Alizarin, wifitab, textCallback);
  mqtt_pass_text = ESPUI.addControl(Text, "MQTT password", stored_mqtt_pass, Alizarin, wifitab, textCallback);
  ESPUI.setInputType(mqtt_pass_text, "password");
  mqtt_enabled_switch = ESPUI.addControl(Switcher, "Enable MQTT", String(mqtt_enabled), Alizarin, wifitab, textCallback);
  mqtt_topic_in_text = ESPUI.addControl(Text, "MQTT topic IN", stored_mqtt_topic_in, Alizarin, wifitab, textCallback);
  mqtt_topic_out_text = ESPUI.addControl(Text, "MQTT topic OUT", stored_mqtt_topic_out, Alizarin, wifitab, textCallback);

  auto wifisave = ESPUI.addControl(Button, "Save", "Save", Peterriver, wifitab, SaveWifiDetailsCallback);
  auto espreset = ESPUI.addControl(Button, "", "Reboot ESP", None, wifisave, ESPReset);

  ESPUI.setEnabled(wifi_ssid_text, true);
  ESPUI.setEnabled(wifi_pass_text, true);
  ESPUI.setEnabled(wifi_ssid_timeout_text, true);
  ESPUI.setEnabled(mqtt_server_text, true);
  ESPUI.setEnabled(mqtt_user_text, true);
  ESPUI.setEnabled(mqtt_pass_text, true);
  ESPUI.setEnabled(mqtt_topic_in_text, true);
  ESPUI.setEnabled(mqtt_topic_out_text, true);
  ESPUI.setEnabled(mqtt_enabled_switch, true);
  ESPUI.setEnabled(wifisave, true);
  ESPUI.setEnabled(espreset, true);
  //WiFi-------------------------------------------------------------------------------------------------------------------

  ESPUI.begin(hostname);
}
//ESPUI=====================================================================================================================
//MQTT RECONNECT==============================================================
void reconnect() {
  if (millis() - last_millis > mqtt_retry_delay) {
    Serial.println("MQTT connection to : " + stored_mqtt_server);
    ESPUI.print(statusLabelId, "MQTT connection to : " + stored_mqtt_server);
    if (client.connect(hostname, stored_mqtt_user.c_str(), stored_mqtt_pass.c_str())) {
      Serial.println("MQTT connected !");
      ESPUI.print(statusLabelId, "MQTT connected !");

      //SUBSCRIBE to Topics--------------------------
      client.subscribe(stored_mqtt_topic_in.c_str());
      //client.subscribe("demo_topic");
      //Other topics HERE !
      //---------------------------------------------

    } else {
      Serial.print("MQTT connection failed : ");
      Serial.println(client.state());
      Serial.println("Retry in 10 sec");
      ESPUI.print(statusLabelId, "MQTT connection failed !");
      last_millis = millis();
      return;
    }
  }
}
//MQTT RECONNECT==============================================================

//MQTT LOOP===================================
void mqtt_loop() {
  if (wificonnected && mqtt_enabled) {
    if (!client.connected()) {
      reconnect();
      return;
    }
    client.loop();
  }
}
//MQTT LOOP===================================




//Custom callback================================
void CustomCallback(Control *sender, int type) {
  //Your code HERE !
}
//Custom callback================================

//MQTT CALLBACK===================================================
void mqtt_callback(String topic, byte *message, unsigned int length) {
  String messageTemp;
  //Read the Payload
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  if (topic == "demo_topic") {
    client.publish("response_topic", "PONG");
    Serial.println("Hello World");
  }

  //Custom action................................................
  //Your code HERE !
  //Custom action................................................
}
//MQTT CALLBACK===================================================







//Serial setup===============================================================
void SerialSetup(String input) {
  if (input.indexOf("ssid") > -1) {
    stored_ssid = splitString(input, ' ', 1);
    preferences.putString("ssid", stored_ssid);
    Serial.println("New SSID : " + stored_ssid);
  }

  else if (input.indexOf("password") > -1) {
    stored_pass = splitString(input, ' ', 1);
    preferences.putString("pass", stored_pass);
    Serial.println("New password : " + stored_pass);
  }

  else if (input.indexOf("ssid_timeout") > -1) {
    stored_ssid_timeout = splitString(input, ' ', 1).toInt();
    preferences.putInt("ssid_timeout", stored_ssid_timeout);
    Serial.println("New ssid timeout : " + stored_ssid_timeout);
  }

  else if (input.indexOf("mqtten") > -1) {
    mqtt_enabled = splitString(input, ' ', 1).toInt() ? true : false;
    preferences.putBool("mqtt_enabled", mqtt_enabled);
    Serial.println("MQTT enabled : " + String(mqtt_enabled));
  }

  else if (input.indexOf("mqttserver") > -1) {
    stored_mqtt_server = splitString(input, ' ', 1);
    preferences.putString("mqtt_server", stored_mqtt_server);
    Serial.println("New MQTT server : " + stored_mqtt_server);
  }

  else if (input.indexOf("mqttuser") > -1) {
    stored_mqtt_user = splitString(input, ' ', 1);
    preferences.putString("mqtt_user", stored_mqtt_user);
    Serial.println("New MQTT user : " + stored_mqtt_user);
  }

  else if (input.indexOf("mqttpass") > -1) {
    stored_mqtt_pass = splitString(input, ' ', 1);
    preferences.putString("mqtt_pass", stored_mqtt_pass);
    Serial.println("New MQTT pass : " + stored_mqtt_pass);
  }

  else if (input.indexOf("topicin") > -1) {
    stored_mqtt_topic_in = splitString(input, ' ', 1);
    preferences.putString("mqtt_topic_in", stored_mqtt_topic_in);
    Serial.println("New Topic IN : " + stored_mqtt_topic_in);
  }

  else if (input.indexOf("topicout") > -1) {
    stored_mqtt_topic_out = splitString(input, ' ', 1);
    preferences.putString("mqtt_topic_out", stored_mqtt_topic_out);
    Serial.println("New Topic OUT : " + stored_mqtt_topic_out);
  }

  else if (input.indexOf("restart") > -1) {
    ESP.restart();
  }

  else if (input.indexOf("info") > -1) {
    Serial.println("SSID " + stored_ssid);
    Serial.println("SSID timeout" + stored_ssid_timeout);
    Serial.println("MQTT server " + stored_mqtt_server);
    Serial.println("MQTT user " + stored_mqtt_user);
    Serial.println("MQTT enabled " + String(mqtt_enabled));
    Serial.println("Topic IN " + stored_mqtt_topic_in);
    Serial.println("Topic OUT " + stored_mqtt_topic_out);
    Serial.print("IP :");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  }

  // else if (input.indexOf("custom_cmd") > -1) {
  //   String demo_cmd = splitString(input, ' ', 1);
  //   Serial.println(demo_cmd);
  //   preferences.putString("demo", "demo_cmd");
  // }
}
//Serial setup===================================================================



//WiFi================================================================================
void wifi_init() {
  stored_ssid = preferences.getString("ssid", "SSID");
  stored_ssid_timeout = preferences.getInt("ssid_timeout", 30);
  stored_pass = preferences.getString("pass", "PASSWORD");
  stored_mqtt_server = preferences.getString("mqtt_server", "192.168.1.10");
  stored_mqtt_user = preferences.getString("mqtt_user", "");
  stored_mqtt_pass = preferences.getString("mqtt_pass", "");
  stored_mqtt_topic_in = preferences.getString("mqtt_topic_in", "demo/in");
  stored_mqtt_topic_out = preferences.getString("mqtt_topic_out", "demo/out");
  mqtt_enabled = preferences.getBool("mqtt_enabled", false);

  //Custom preferences............................................
  //Your code HERE !
  //int demo_last_reading = preferences.getInt("last_reading", 0);
  //Custom preferences............................................

  Serial.println("Connecting to : " + stored_ssid);
  WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
  uint8_t timeout = stored_ssid_timeout;
  while (timeout && WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout--;
  }
  if (WiFi.status() != WL_CONNECTED) {
    wificonnected = false;
    Serial.print("\n\nCreating Hotspot");
    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(hostname);
  } else {
    wificonnected = true;
    client.setServer(stored_mqtt_server.c_str(), 1883);
    client.setCallback(mqtt_callback);
  }
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.print("\nIP address : ");
  Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
}
//WiFi================================================================================
//Temperature================================================================================
void temp_init() {
  // Serial.println(9600);
  // DS18B20.begin();    // initialize the DS18B20 sensor
  //  Serial.begin(9600);
  Serial.print("Devices: ");
  Serial.println(ds.getNumberOfDevices());
  Serial.println();
}
void temp_loop() {
  // DS18B20.requestTemperatures();       // send the command to get temperatures
  // tempC = DS18B20.getTempCByIndex(0);  // read temperature in °C
  // tempF = tempC * 9 / 5 + 32; // convert °C to °F

  // Serial.print("Temperature: ");
  // Serial.print(tempC);    // print the temperature in °C
  // Serial.print("°C");
  // Serial.print("  ~  ");  // separator between °C and °F
  // Serial.print(tempF);    // print the temperature in °F
  // Serial.println("°F");

  // delay(500);
  
  //while (ds.selectNext()) {
    // switch (ds.getFamilyCode()) {
    //   case MODEL_DS18S20:
    //     Serial.println("Model: DS18S20/DS1820");
    //     break;
    //   case MODEL_DS1822:
    //     Serial.println("Model: DS1822");
    //     break;
    //   case MODEL_DS18B20:
    //     Serial.println("Model: DS18B20");
    //     break;
    //   default:
    //     Serial.println("Unrecognized Device");
    //     break;
    // }

    // uint8_t address[8];
    // ds.getAddress(address);

    // Serial.print("Address:");
    // for (uint8_t i = 0; i < 8; i++) {
    //   Serial.print(" ");
    //   Serial.print(address[i]);
    // }
    // Serial.println();

    // Serial.print("Resolution: ");
    // Serial.println(ds.getResolution());

    // Serial.print("Power Mode: ");
    // if (ds.getPowerMode()) {
    //   Serial.println("External");
    // } else {
    //   Serial.println("Parasite");
    // }

    if (millis() - last_temp_millis >= temp_delay) {
      Serial.print("Temperature: ");
      Serial.print(ds.getTempC());
      Serial.print(" C");
      // Serial.print(ds.getTempF());
      // Serial.println(" F");
      Serial.println();
      ESPUI.print(mesure_temp_text, String(ds.getTempC()) + " C");
      last_temp_millis = millis();
    }
    
  // }

  // delay(2000);
}
//Temperature================================================================================

//Pression================================================================================
void pression_init() {
    Serial.println(F("BMP280 Forced Mode Test."));

  //if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
  
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor at 0x76, check wiring or "
                      "try a different address!"));
    // while (1) delay(10);
  } else {

    /* Default settings from datasheet. */
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  }
}
void pression_loop() {
  if (millis() - last_pres_millis >= pres_delay) {
    // must call this to wake sensor up and get new measurement data
    // it blocks until measurement is complete
    if (bmp.takeForcedMeasurement()) {
      // can now print out the new measurements
      Serial.print(F("Temperature = "));
      Serial.print(bmp.readTemperature());
      Serial.println(" *C");

      Serial.print(F("Pressure = "));
      float pres = bmp.readPressure();
      Serial.print(pres);
      Serial.println(" Pa");
      ESPUI.print(mesure_pres_text, String(pres) + " Pa");

      Serial.print(F("Approx altitude = "));
      Serial.print(bmp.readAltitude(0.0)); /* Adjusted to local forecast! */
      Serial.println(" m");

      Serial.println();
      //delay(2000);
    } else {
      Serial.println("Forced measurement failed!");
    }
    last_pres_millis = millis();
  }

}
//Pression================================================================================
//Custom libraries..............

//SETUP=========================
void setup() {

  //Custom setup...............
  //Your code HERE !
  //Custom setup...............

  Serial.begin(115200);
  Serial.println();
  //pinMode(LED_BUILTIN, OUTPUT);
  preferences.begin("Settings");
  wifi_init();
  espui_init();
  temp_init();
  pression_init();
}
//SETUP=========================

//LOOP==========================================
void loop() {
  dnsServer.processNextRequest();
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    ESPUI.print(serialLabelId, input);
    SerialSetup(input);
  }
  mqtt_loop();
  temp_loop();
  pression_loop();
  //Custom loop.................................
  //Your code HERE !
  //Custom loop.................................
}
//LOOP==========================================


