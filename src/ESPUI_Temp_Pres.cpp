/*
 lgarnier11
 v1.1.0 - 07/02/2023
 Adaptation en c++ du "framework" Arduino de Neodyme.
 Utilisation pour gérer une sonde de température (étanche) et
 une sonde de pression.
 Utilisation du circuit dans une cloche à vide.
 Le circuit devient serveur wifi, avec une page qui
 présente trois onglets :
 - Mesures      : Température et pression
 - Debug        : Affichage des messages
 - Paramétrages : Délais des mesures, wifi

 Par soucis de simplicité, on passe de 4 fichiers .ino
 à un seul fichier .cpp
*/

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

unsigned long last_print_millis = 0;
unsigned long print_delay = 1000;

bool bMesure = true;

String myLog1, myLog2, myLog3, myLog4;

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
const char* hostname = "ESPUI-Temp-Pression-1.1.0";
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
uint16_t mesure_temp_text, mesure_pres_text, cmdMesure;
uint16_t param_delay_temp_text, param_delay_pres_text;
uint16_t mqtt_server_text, mqtt_topic_in_text, mqtt_topic_out_text, mqtt_user_text, mqtt_pass_text, mqtt_enabled_switch;
uint16_t statusLabelId, serialLabelId;
uint16_t labelLog1, labelLog2, labelLog3, labelLog4;
String option;
String stored_ssid, stored_pass;
int stored_delay_temp, stored_delay_pres;
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

int maxLog = 0;
void println(String s) {
    Serial.println(s);
    myLog1 = myLog2;
    myLog2 = myLog3;
    myLog3 = myLog4;
    myLog4 = s;
    ESPUI.print(labelLog1, myLog1);
    ESPUI.print(labelLog2, myLog2);
    ESPUI.print(labelLog3, myLog3);
    ESPUI.print(labelLog4, myLog4);
}
void println(int i) {
  println(String(i));
}
void println() {
  println("");
}

void print(String s) {
    myLog4 += s;
    Serial.print(s);
}
void print(int i) {
  print(String(i));
}

//WiFi settings callback=====================================================
void SaveWifiDetailsCallback(Control *sender, int type) {
  if (type == B_UP) {
    println("Saving params");

    stored_delay_temp = ESPUI.getControl(param_delay_temp_text)->value.toInt();
    stored_delay_pres = ESPUI.getControl(param_delay_pres_text)->value.toInt();
    stored_ssid = ESPUI.getControl(wifi_ssid_text)->value;
    stored_pass = ESPUI.getControl(wifi_pass_text)->value;
    stored_ssid_timeout = ESPUI.getControl(wifi_ssid_timeout_text)->value.toInt();
    stored_mqtt_topic_in = ESPUI.getControl(mqtt_topic_in_text)->value;
    stored_mqtt_topic_out = ESPUI.getControl(mqtt_topic_out_text)->value;
    stored_mqtt_server = String(ESPUI.getControl(mqtt_server_text)->value);
    stored_mqtt_user = String(ESPUI.getControl(mqtt_user_text)->value);
    stored_mqtt_pass = String(ESPUI.getControl(mqtt_pass_text)->value);
    mqtt_enabled = ESPUI.getControl(mqtt_enabled_switch)->value.toInt() ? true : false;

    preferences.putInt("delay_temp", stored_delay_temp);
    preferences.putInt("delay_pres", stored_delay_pres);
    preferences.putString("ssid", stored_ssid);
    preferences.putString("pass", stored_pass);
    preferences.putInt("ssid_timeout", stored_ssid_timeout);
    preferences.putString("mqtt_server", stored_mqtt_server);
    preferences.putString("mqtt_user", stored_mqtt_user);
    preferences.putString("mqtt_pass", stored_mqtt_pass);
    preferences.putString("mqtt_topic_in", stored_mqtt_topic_in);
    preferences.putString("mqtt_topic_out", stored_mqtt_topic_out);
    preferences.putBool("mqtt_enabled", mqtt_enabled);

    println(stored_delay_temp);
    println(stored_delay_pres);
    println(stored_ssid);
    println(stored_pass);
    println(stored_ssid_timeout);
    println(stored_mqtt_server);
    println(stored_mqtt_user);
    println(stored_mqtt_pass);
    println(stored_mqtt_topic_in);
    println(stored_mqtt_topic_out);
    println(mqtt_enabled);

    println("Saving settings");
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
    bMesure = !bMesure;
    // setCmdMesure();
  }
  ESPUI.updateButton(cmdMesure, bMesure ? "Pause" : "Reprendre");
    
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
  mesure_temp_text = ESPUI.addControl(Label, "Température", "", Peterriver, mesuretab, textCallback);
  mesure_pres_text = ESPUI.addControl(Label, "Pression", "", Peterriver, mesuretab, textCallback);
  cmdMesure = ESPUI.addControl(Button, "", "Pause", Peterriver, mesuretab, cmdMesurer);
  ESPUI.setEnabled(cmdMesure, true);
  //Mesure-------------------------------------------------------------------------------------------------------------------

  
  //Debug-----------------------------------------------------------------------------------------------
  auto debug = ESPUI.addControl(Tab, "", "Debug");
  labelLog1 = ESPUI.addControl(Text, "", myLog1, Peterriver, debug, textCallback);
  labelLog2 = ESPUI.addControl(Text, "", myLog2, Peterriver, labelLog1, textCallback);
  serialLabelId = ESPUI.addControl(Label, "Serial", "Serial IN", Peterriver, debug, textCallback);
  statusLabelId = ESPUI.addControl(Label, "", "Serial OUT", Peterriver, serialLabelId, textCallback);
  labelLog3 = ESPUI.addControl(Text, "", myLog3, Peterriver, debug, textCallback);
  labelLog4 = ESPUI.addControl(Text, "", myLog4, Peterriver, labelLog3, textCallback);
  //Debug-----------------------------------------------------------------------------------------------

  //Param-----------------------------------------------------------------------------------------------
  auto param = ESPUI.addControl(Tab, "", "Paramétrage");
  
  param_delay_temp_text = ESPUI.addControl(Text, "Température", String(stored_delay_temp), Peterriver, param, textCallback);
  param_delay_pres_text = ESPUI.addControl(Text, "Pression", String(stored_delay_pres), Peterriver, param, textCallback);
  
  wifi_ssid_text = ESPUI.addControl(Text, "SSID", stored_ssid, Alizarin, param, textCallback);
  wifi_pass_text = ESPUI.addControl(Text, "Password", stored_pass, Alizarin, param, textCallback);
  wifi_ssid_timeout_text = ESPUI.addControl(Text, "SSID Timeout", String(stored_ssid_timeout), Alizarin, param, textCallback);
  ESPUI.setInputType(wifi_pass_text, "password");
  ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
  ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
  mqtt_server_text = ESPUI.addControl(Text, "MQTT server", stored_mqtt_server, Alizarin, param, textCallback);
  mqtt_user_text = ESPUI.addControl(Text, "MQTT user", stored_mqtt_user, Alizarin, param, textCallback);
  mqtt_pass_text = ESPUI.addControl(Text, "MQTT password", stored_mqtt_pass, Alizarin, param, textCallback);
  ESPUI.setInputType(mqtt_pass_text, "password");
  mqtt_enabled_switch = ESPUI.addControl(Switcher, "Enable MQTT", String(mqtt_enabled), Alizarin, param, textCallback);
  mqtt_topic_in_text = ESPUI.addControl(Text, "MQTT topic IN", stored_mqtt_topic_in, Alizarin, param, textCallback);
  mqtt_topic_out_text = ESPUI.addControl(Text, "MQTT topic OUT", stored_mqtt_topic_out, Alizarin, param, textCallback);

  auto paramsave = ESPUI.addControl(Button, "Save", "Save", Peterriver, param, SaveWifiDetailsCallback);
  auto espreset = ESPUI.addControl(Button, "", "Reboot ESP", None, paramsave, ESPReset);

  ESPUI.setEnabled(param_delay_temp_text, true);
  ESPUI.setEnabled(param_delay_pres_text, true);
  ESPUI.setEnabled(wifi_ssid_text, true);
  ESPUI.setEnabled(wifi_pass_text, true);
  ESPUI.setEnabled(wifi_ssid_timeout_text, true);
  ESPUI.setEnabled(mqtt_server_text, true);
  ESPUI.setEnabled(mqtt_user_text, true);
  ESPUI.setEnabled(mqtt_pass_text, true);
  ESPUI.setEnabled(mqtt_topic_in_text, true);
  ESPUI.setEnabled(mqtt_topic_out_text, true);
  ESPUI.setEnabled(mqtt_enabled_switch, true);
  ESPUI.setEnabled(paramsave, true);
  ESPUI.setEnabled(espreset, true);
  //Param-----------------------------------------------------------------------------------------------

  ESPUI.begin(hostname);
}
//ESPUI=====================================================================================================================
//MQTT RECONNECT==============================================================
void reconnect() {
  if (millis() - last_millis > mqtt_retry_delay) {
    println("MQTT connection to : " + stored_mqtt_server);
    ESPUI.print(statusLabelId, "MQTT connection to : " + stored_mqtt_server);
    if (client.connect(hostname, stored_mqtt_user.c_str(), stored_mqtt_pass.c_str())) {
      println("MQTT connected !");
      ESPUI.print(statusLabelId, "MQTT connected !");

      //SUBSCRIBE to Topics--------------------------
      client.subscribe(stored_mqtt_topic_in.c_str());
      //client.subscribe("demo_topic");
      //Other topics HERE !
      //---------------------------------------------

    } else {
      print("MQTT connection failed : ");
      println(client.state());
      println("Retry in 10 sec");
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
  println(messageTemp);

  if (topic == "demo_topic") {
    client.publish("response_topic", "PONG");
    println("Hello World");
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
    println("New SSID : " + stored_ssid);
  }
  else if (input.indexOf("delay_temp") > -1) {
    stored_delay_temp = splitString(input, ' ', 1).toInt();
    preferences.putInt("delay_temp", stored_delay_temp);
    println("New delay temp : " + stored_delay_temp);
  }
  else if (input.indexOf("delay_pres") > -1) {
    stored_delay_pres = splitString(input, ' ', 1).toInt();
    preferences.putInt("delay_pres", stored_delay_pres);
    println("New delay pres : " + stored_delay_pres);
  }
  else if (input.indexOf("password") > -1) {
    stored_pass = splitString(input, ' ', 1);
    preferences.putString("pass", stored_pass);
    println("New password : " + stored_pass);
  }
  else if (input.indexOf("password") > -1) {
    stored_pass = splitString(input, ' ', 1);
    preferences.putString("pass", stored_pass);
    println("New password : " + stored_pass);
  }

  else if (input.indexOf("ssid_timeout") > -1) {
    stored_ssid_timeout = splitString(input, ' ', 1).toInt();
    preferences.putInt("ssid_timeout", stored_ssid_timeout);
    println("New ssid timeout : " + stored_ssid_timeout);
  }

  else if (input.indexOf("mqtten") > -1) {
    mqtt_enabled = splitString(input, ' ', 1).toInt() ? true : false;
    preferences.putBool("mqtt_enabled", mqtt_enabled);
    println("MQTT enabled : " + String(mqtt_enabled));
  }

  else if (input.indexOf("mqttserver") > -1) {
    stored_mqtt_server = splitString(input, ' ', 1);
    preferences.putString("mqtt_server", stored_mqtt_server);
    println("New MQTT server : " + stored_mqtt_server);
  }

  else if (input.indexOf("mqttuser") > -1) {
    stored_mqtt_user = splitString(input, ' ', 1);
    preferences.putString("mqtt_user", stored_mqtt_user);
    println("New MQTT user : " + stored_mqtt_user);
  }

  else if (input.indexOf("mqttpass") > -1) {
    stored_mqtt_pass = splitString(input, ' ', 1);
    preferences.putString("mqtt_pass", stored_mqtt_pass);
    println("New MQTT pass : " + stored_mqtt_pass);
  }

  else if (input.indexOf("topicin") > -1) {
    stored_mqtt_topic_in = splitString(input, ' ', 1);
    preferences.putString("mqtt_topic_in", stored_mqtt_topic_in);
    println("New Topic IN : " + stored_mqtt_topic_in);
  }

  else if (input.indexOf("topicout") > -1) {
    stored_mqtt_topic_out = splitString(input, ' ', 1);
    preferences.putString("mqtt_topic_out", stored_mqtt_topic_out);
    println("New Topic OUT : " + stored_mqtt_topic_out);
  }

  else if (input.indexOf("restart") > -1) {
    ESP.restart();
  }

  else if (input.indexOf("info") > -1) {
    println("Temperature delay " + stored_delay_temp);
    println("Pression delay " + stored_delay_pres);
    println("SSID " + stored_ssid);
    println("SSID timeout" + stored_ssid_timeout);
    println("MQTT server " + stored_mqtt_server);
    println("MQTT user " + stored_mqtt_user);
    println("MQTT enabled " + String(mqtt_enabled));
    println("Topic IN " + stored_mqtt_topic_in);
    println("Topic OUT " + stored_mqtt_topic_out);
    print("IP :");
    println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
  }
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

  println("Connecting to : " + stored_ssid);
  WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
  uint8_t timeout = stored_ssid_timeout;
  while (timeout && WiFi.status() != WL_CONNECTED) {
    delay(500);
    print(".");
    timeout--;
  }
  if (WiFi.status() != WL_CONNECTED) {
    wificonnected = false;
    print("\n\nCreating Hotspot");
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
  print("\nIP address : ");
  println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
}
//WiFi================================================================================
//Temperature================================================================================
void temp_init() {
  stored_delay_temp = preferences.getInt("delay_temp", 1000);
  println("Devices: " + String(ds.getNumberOfDevices()));
}
void temp_loop() {
    if (bMesure && millis() - last_temp_millis >= stored_delay_temp) {
      println("Temperature: " + String(ds.getTempC()) + " C");
      ESPUI.print(mesure_temp_text, String(ds.getTempC()) + " C");
      last_temp_millis = millis();
    }
}
//Temperature================================================================================

//Pression================================================================================
void pression_init() {
  stored_delay_pres = preferences.getInt("delay_pres", 1000);
    println(F("BMP280 Forced Mode Test."));

  //if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
  
  if (!bmp.begin(0x76)) {
    println(F("Could not find a valid BMP280 sensor at 0x76, check wiring or "
                      "try a different address!"));
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
  if (bMesure && millis() - last_pres_millis >= stored_delay_pres) {
    // must call this to wake sensor up and get new measurement data
    // it blocks until measurement is complete
    if (bmp.takeForcedMeasurement()) {
      // can now print out the new measurements
      println("Temperature = " + String(bmp.readTemperature()) + " *C");

      float pres = bmp.readPressure();
      println("Pressure = " + String(pres / 100.0F) + " hPa");
      ESPUI.print(mesure_pres_text, String(pres / 100.0F) + " hPa");
    } else {
      println("Forced measurement failed!");
    }
    last_pres_millis = millis();
  }

}
//Pression================================================================================
//Custom libraries..............

//SETUP=========================
void setup() {
  Serial.begin(115200);
  println();
  preferences.begin("Settings");
  wifi_init();
  //Custom setup...............
  temp_init();
  pression_init();
  //Custom setup...............
  espui_init();
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
  //Custom loop.................................
  temp_loop();
  pression_loop();
  //Custom loop.................................
}
//LOOP==========================================


