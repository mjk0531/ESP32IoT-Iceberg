#include "eepromCustom.h"
#include "wifi_custom.h"
#include "SerialUI.h"
#include "spiffsCustom.h"
#include "ble_custom.h"
#include "sensorControl.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "buttonUI.h"

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5           /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;
uint8_t txValue = 0;
unsigned long previousTimeSET;
unsigned long previousTimeALERT;
bool PMICState = false;

// const char* OTA_ssid = "jkmin";
// const char* OTA_password = "antsperpet";
// const char* OTA_ssid = "Thabo";
// const char* OTA_password = "thabo135";

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void deep_sleep_perpet() {
  // //Increment boot number and print it every reboot
  // ++bootCount;
  // Serial.println("Boot number: " + String(bootCount));
  // //Print the wakeup reason for ESP32
  // print_wakeup_reason();

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
  // Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  Serial.println("Going to sleep now");
  Serial.flush();
  // WiFi.disconnect(true);
  // WiFi.mode(WIFI_OFF);

  esp_deep_sleep_start();
  Serial.println("WiFi turning on!");
  // WiFi.begin(ssid, pass);
  // Serial.println("WiFi turned on!");
}

void light_sleep_perpet_motionINT() {
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, 1);

  digitalWrite(16, HIGH);
  delay(1000);
  digitalWrite(16, LOW);
  delay(1000);
  digitalWrite(16, HIGH);
  delay(1000);
  digitalWrite(16, LOW);
  delay(1000);
  digitalWrite(17, HIGH);
  delay(1000);
  digitalWrite(17, LOW);
  delay(1000);
  digitalWrite(17, HIGH);
  delay(1000);
  digitalWrite(17, LOW);
  delay(1000);

  Serial.println("Going to light sleep");
  Serial.flush();

  // esp_light_sleep_start();
}

void IRAM_ATTR intSLP() {
  deep_sleep_perpet();
}

void IRAM_ATTR MotionINT() {
  light_sleep_perpet_motionINT();
}

/***********************************************************************
  Adafruit MQTT Library ESP32 Adafruit IO SSL/TLS example

  Use the latest version of the ESP32 Arduino Core:
    https://github.com/espressif/arduino-esp32

  Works great with Adafruit Huzzah32 Feather and Breakout Board:
    https://www.adafruit.com/product/3405
    https://www.adafruit.com/products/4172

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  Modified by Brent Rubell for Adafruit Industries
  MIT license, all text above must be included in any redistribution
 **********************************************************************/
#include <MQTT.h>



// WiFiClient net;
MQTTClient MQTTclient;

unsigned long lastMillis = 0;

#define LEN_ACCSMPL 30
#define LEN_PRSSMPL 20
#define LEN_TRHSMPL 20
int8_t acc[LEN_ACCSMPL * 3 + 6];
int8_t prs[LEN_PRSSMPL * 4 + 6];
int8_t trh[LEN_TRHSMPL * 4 + 6];
int8_t bat[1];

/************************* WiFi Access Point *********************************/
/// MQTT

void connect() {
  Serial.print("checking wifi...");
  digitalWrite(17, HIGH);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  digitalWrite(17, LOW);

  Serial.print("\nconnecting...");
  digitalWrite(17, HIGH);
  while (!MQTTclient.connect("arduino", "public", "public")) {
    Serial.print(".");
    delay(1000);
  }
  digitalWrite(17, LOW);

  Serial.println("\nconnected!");

  // MQTTclient.subscribe(topic_base+"/acc");
  MQTTclient.subscribe((topic_base + "/CMD").c_str());
  // client.unsubscribe("/hello");
}


void messageReceived(String& topic, String& payload) {
  // Serial.println("incoming: " + topic + " - " + payload);
  if (topic == topic_base + "/CMD") {
    //www.ants.com/perpet/SerialNumber/acc
    // const char* rxPacket=payload.c_str();
    char rxPacket[LEN_ACCSMPL + 2];
    payload.toCharArray(rxPacket, (LEN_ACCSMPL + 2), 0);
    Serial.println("******");
    Serial.println("decoded:");
    // for(int i=0;i<(LEN_ACCSMPL+2);i++){
    //   for(int j=0;j<3;j++){
    //     Serial.print((int8_t)rxPacket[i*3+j]);
    //     Serial.print(",");
    //   }
    //   Serial.println();
    // }
    Serial.println("******");
  }
  if (topic == topic_base + "/trh") {
    const char* rxPacket = payload.c_str();
    Serial.println("******");
    Serial.println("decoded:");
    for (int i = 0; i < LEN_TRHSMPL; i++) {
      Serial.print((int8_t)rxPacket[i]);
      Serial.print(",");
    }
    Serial.println("******");
  }

  if (topic == topic_base + "/CMD") {
    if ((payload.toInt()) % 3 == 0) {
      Serial.println("shutdown");
    }
  }
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}


//button 0
//short press:
//long press: deep sleep / wake-up

//on start, if button0&1 pressed)


//  button 0  | button 1
//  pressed(long)   | released   ---> deep sleep, power on/off

//  pressed(long) | pressed(long)   ---> wifi/ble mode change
//  pressed   | released ---> wifi mode
//  pressed   | released ---> ble mode




const char* path = "/acc.txt";

// ref: (13:04) https://www.youtube.com/watch?v=JFDiqPHw3Vc&list=PL4s_3hkDEX_0YpVkHRY3MYHfGxxBNyKkj&index=100&t=590s
// clock | current wifi(mA) | current no wifi(mA) | Serial(baudrate) | i2c clock
//  240  |      157         |            69       |     115200       | 350kHz
//  160  |      131         |            46       |     115200       | 350kHz
//   80  |      119         |            32       |     115200       | 350kHz
//   40  |       82         |            18       |      57600       | 100kHz
//   20  |       77         |            13       |      28800       | 100kHz
//   10  |       74         |            11       |      14400       | 100kHz

#define PIN_BUTTON1 26
#define PIN_BUTTON2 25
#define PIN_LED1 17
#define PIN_LED2 16


Button button1(PIN_BUTTON1);
Button button2(PIN_BUTTON2);

void handleButton1Press() {
  button1.pressStartTime = millis();
  Serial.print("button1 pressed, t[ms]=");
  Serial.println(button1.pressStartTime);
  op_mode = OP_MODE_BLE;
}
void handleButton2Press() {
  button2.pressStartTime = millis();
  Serial.print("button2 pressed, t[ms]=");
  Serial.println(button2.pressStartTime);
  deep_sleep_perpet();
}

void setup() {
  Serial.begin(230400);
  pinMode(PIN_BUTTON1, INPUT_PULLUP);
  pinMode(PIN_BUTTON2, INPUT_PULLUP);
  pinMode(32, INPUT);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(34, INPUT);

  attachInterrupt(digitalPinToInterrupt(button1.pin), handleButton1Press, FALLING);
  attachInterrupt(digitalPinToInterrupt(button2.pin), handleButton2Press, FALLING);
  // EEPROM setup
  init_eeprom();
  eepromSetup_custom();
  // print params via serial port
  print_settings();

  digitalWrite(PIN_LED2, HIGH);
  int waitCount = 0;
  delay(2000);
  while (waitCount < 5) {
    digitalWrite(PIN_LED1, HIGH);
    delay(300);
    digitalWrite(PIN_LED1, LOW);
    delay(1700);
    if (button1.isPressed()) {
      Serial.println("OP_MODE_");
      op_mode = OP_MODE_BLE;
      break;
    } else if (button2.isPressed()) {
      Serial.println("OP_MODE_SLEEP");
      op_mode = OP_MODE_SLEEP;
      break;
    } else {
      op_mode = OP_MODE_WIFI;
      break;
    }
    waitCount++;
  }
  digitalWrite(PIN_LED2, LOW);
  delay(1000);

  if (op_mode == OP_MODE_BLE) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(PIN_LED2, HIGH);
      delay(300);
      digitalWrite(PIN_LED2, LOW);
      delay(700);
    }
    // BLE setup
    ble_setup_custom();
    delay(1000);

  } else if (op_mode == OP_MODE_WIFI) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(PIN_LED1, HIGH);
      delay(300);
      digitalWrite(PIN_LED1, LOW);
      delay(700);
    }
    //sensor setup
    setSensorPRS();
    setSensorIMU();
    setSensorTRH();
    // Define Dataformat
    acc[0] = 0xA0;
    acc[1] = LEN_ACCSMPL;
    prs[0] = 0xA1;
    prs[1] = LEN_PRSSMPL;
    trh[0] = 0xA2;
    trh[1] = LEN_PRSSMPL;

    // SPIFFS setup
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
      Serial.println("SPIFFS Mount Failed");
      return;
    }
    listDir(SPIFFS, "/", 0);
    deleteFile(SPIFFS, "/acc.txt");
    int8_t accdum[1];
    Serial.println("writing data");
    writeFileBytes(SPIFFS, path, (uint8_t*)accdum, 0);

    // WiFi connection
    Serial.println("connecting to AP");
    bool isAPconnected = false;
    // WiFi.begin(ssid, pass);
    for (int i = 0; i < 3; i++) {
      isAPconnected = ConnectToRouter(AP_id.c_str(), AP_pw.c_str());
      if (isAPconnected) {
        break;
      }
    }
    if (isAPconnected) {
      Serial.println("AP connected");
    } else {
      Serial.println("AP not connected. Proceed anyway..");
    }
    // MQTT setup
    MQTTclient.begin(server_addr.c_str(), server_port, net);
    MQTTclient.onMessage(messageReceived);
    connect();
  } else {
    op_mode = OP_MODE_SLEEP;
    for (int i = 0; i < 3; i++) {
      digitalWrite(PIN_LED1, HIGH);
      digitalWrite(PIN_LED2, HIGH);
      delay(200);
      digitalWrite(PIN_LED1, LOW);
      digitalWrite(PIN_LED2, LOW);
      delay(800);
    }
    deep_sleep_perpet();
    Serial.println("DEEP SLEEP MODE");
  }

  // --------------------------OTA setup---------------------------
  // Serial.println("Booting");
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(OTA_ssid, OTA_password);
  // while (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //   Serial.println("Connection Failed! Rebooting...");
  //   delay(5000);
  //   ESP.restart();
  // }
  // digitalWrite(16, HIGH);

  // // Port defaults to 3232
  // // ArduinoOTA.setPort(3232);

  // // Hostname defaults to esp3232-[MAC]
  // // ArduinoOTA.setHostname("myesp32");

  // // No authentication by default
  // // ArduinoOTA.setPassword("admin");

  // // Password can be set with it's md5 value as well
  // // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  // ArduinoOTA
  //   .onStart([]() {
  //     String type;
  //     if (ArduinoOTA.getCommand() == U_FLASH)
  //       type = "sketch";
  //     else  // U_SPIFFS
  //       type = "filesystem";

  //     // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  //     Serial.println("Start updating " + type);
  //   })
  //   .onEnd([]() {
  //     Serial.println("\nEnd");
  //   })
  //   .onProgress([](unsigned int progress, unsigned int total) {
  //     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  //   })
  //   .onError([](ota_error_t error) {
  //     Serial.printf("Error[%u]: ", error);
  //     if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  //     else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  //     else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  //     else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  //     else if (error == OTA_END_ERROR) Serial.println("End Failed");
  //   });

  // ArduinoOTA.begin();
  // digitalWrite(16, LOW);

  // Serial.println("Ready");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
  // -------------------------------------OTA setup-----------------------------------------------

  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);

  // attachInterrupt(digitalPinToInterrupt(25), intSLP, HIGH);
  // attachInterrupt(digitalPinToInterrupt(26), handleButton1Press, FALLING);
  // attachInterrupt(digitalPinToInterrupt(32), MotionINT, FALLING);


  // light_sleep_purpet();
  // deep_sleep_perpet();

  digitalWrite(18, HIGH);
  digitalWrite(19, PMICState);

  previousTimeSET = millis();
  previousTimeALERT = millis();
}

bool bViewerActive = false;
bool bPetActive = true;
bool bTimerSet = false;
bool ledState = false;
bool flag_Recharge = false;
int counter_Recharge = 0;

uint32_t timestamp[3] = { 10000, 10000, 10000 };
uint32_t dt[3] = { 33, 50, 50 };
int8_t idx[3] = { 0, 0, 0 };
uint16_t iter = 0;

void loop() {
  // ArduinoOTA.handle();
  // ESP.restart();
  int analogVolts = 0;
  int analogVolts_temp = 2 * analogReadMilliVolts(34);

  if (BatVoltinitFlag == true) {
    for (int j = 0; j < BatVoltNumReadings; j++) {
      BatVoltReadings[j] = analogVolts_temp;
      BatVolttotal = BatVolttotal + BatVoltReadings[j];
    }
    BatVoltinitFlag = false;
  }

  BatVolttotal = BatVolttotal - BatVoltReadings[BatVoltindex];
  BatVoltReadings[BatVoltindex] = analogVolts_temp;
  BatVolttotal = BatVolttotal + BatVoltReadings[BatVoltindex];
  BatVoltindex = BatVoltindex + 1;
  if (BatVoltindex >= BatVoltNumReadings) {
    BatVoltindex = 0;
  }

  analogVolts = BatVolttotal / BatVoltNumReadings;
  // Serial.printf("ADC millivolts value = %d\n", analogVolts);

  if (analogVolts < 3375) {
    Serial.println("Low Voltage Alert!");
    unsigned long currentTime = millis();        // get the current time
    if (currentTime - previousTimeSET >= 250) {  // check if the interval has elapsed
      ledState = !ledState;
      digitalWrite(17, ledState);                // update the LED 17
      previousTimeSET = currentTime;             // reset the previous time
    }
  } else if (analogVolts < 3300) {
    digitalWrite(17, LOW);  // turn off the LED 17
    deep_sleep_perpet();    // enter deep sleep mode
  } else {
  }

  if (analogVolts < 3350) {
    // Get the current time in milliseconds
    unsigned long currentTime = millis();
    // Check if the interval has elapsed since the previous time and the flag_Recharge is false
    if (currentTime - previousTimeALERT >= 10000 && flag_Recharge == false) {
      // Toggle the LED state
      PMICState = !PMICState;
      // Write the LED state to the pin
      digitalWrite(19, PMICState);
      // Increment the counter_Recharge by 1
      counter_Recharge++;
      // Check if the counter_Recharge is equal to 2
      if (counter_Recharge == 2) {
        // Set the flag_Recharge to true
        flag_Recharge = true;
      }
      // Update the previous time
      previousTimeALERT = currentTime;
    }
  } else {
    // Reset the flag_Recharge to false
    flag_Recharge = false;
    // Reset the counter_Recharge to 0
    counter_Recharge = 0;
  }

  if (op_mode == OP_MODE_WIFI) {
    MQTTclient.loop();
    delay(10);  // <- fixes some issues with WiFi stability
    if (!MQTTclient.connected()) {
      connect();
    }

    bViewerActive = true;
    if (bViewerActive == true) {
      setCpuFrequencyMhz(80);  //No BT/Wifi: 10,20,40 MHz, for BT/Wifi, 80,160,240MHz
      //function - sensing
      if (!bTimerSet) {
        bTimerSet = true;
        timestamp[0] = millis();
        timestamp[1] = millis();
        timestamp[2] = millis();
      }

      // save sensor data
      // IMU: 30Hz, Alt: 10 Hz, RH: 10s, T: 10s
      if (millis() > timestamp[0]) {

        if (idx[0] == 0) {
          Serial.println("timestamp[0]=" + String(timestamp[0]));
          memcpy(&acc[2], &timestamp[0], sizeof(uint32_t));
        }
        timestamp[0] += dt[0];
        getSensorDataIMU(acc + 3 * idx[0] + 6);
        Serial.print("acc:");
        Serial.print(acc[3 * idx[0] + 6]);
        Serial.print("\t");
        Serial.print(acc[3 * idx[0] + 7]);
        Serial.print("\t");
        Serial.print(acc[3 * idx[0] + 8]);
        Serial.println();

        idx[0]++;
        if (idx[0] == LEN_ACCSMPL) {
          Serial.println("acc tx!");
          uint32_t dum = 0;
          memcpy(&dum, &acc[2], sizeof(uint32_t));
          Serial.println("dum:" + String(dum));
          MQTTclient.publish((topic_base + "/acc").c_str(), (const char*)acc, LEN_ACCSMPL * 3 + 6);
          idx[0] = 0;
        }
      }

      if (millis() > timestamp[1]) {

        if (idx[1] == 0) {
          Serial.println("timestamp[1]=" + String(timestamp[1]));
          memcpy(&prs[2], &timestamp[1], sizeof(uint32_t));
        }
        timestamp[1] += dt[1];
        getSensorPressure(prs + 4 * idx[1] + 6);
        idx[1]++;
        if (idx[1] == LEN_PRSSMPL) {
          idx[1] = 0;
          Serial.println("prs tx!");
          MQTTclient.publish((topic_base + "/prs").c_str(), (const char*)prs, LEN_PRSSMPL * 4 + 6);
          for (int i = 0; i < LEN_PRSSMPL; i++) {
            Serial.print(i);
            Serial.print(":");
            float rxbuf;
            memcpy(&rxbuf, prs + 4 * i + 6, sizeof(float));
            Serial.println(rxbuf);
          }
        }
      }


      if (millis() > timestamp[2]) {

        if (idx[2] == 0) {
          Serial.println("timestamp[2]=" + String(timestamp[2]));
          memcpy(&trh[2], &timestamp[2], sizeof(uint32_t));
        }
        timestamp[2] += dt[2];
        getSensorDataTRH(trh + 4 * idx[2] + 6);
        // int16_t t_data, rh_data;
        // memcpy(&t_data,trh+4*idx[1]+6,2);
        // memcpy(&rh_data,trh+4*idx[1]+8,2);
        // Serial.print("trh["+String(idx[1])+"]:");
        // Serial.println(String(t_data)+","+String(rh_data));
        idx[2]++;
        if (idx[2] == LEN_TRHSMPL) {
          idx[2] = 0;
          Serial.println("trh tx!");
          MQTTclient.publish((topic_base + "/trh").c_str(), (const char*)trh, LEN_TRHSMPL * 4 + 6);
        }

        BAT_LEV.writeString(0, String(batteryLevEEP()));
        BAT_LEV.commit();
        BAT_LEV.get(0, buf_eeprom);
        bat[0] = static_cast<int8_t>(atoi(String(buf_eeprom).c_str()));
        Serial.print("Battery: ");
        Serial.println(bat[0]);
        MQTTclient.publish((topic_base + "/bat").c_str(), (const char*) bat, 1);
      }

    } else {
      Serial.println("owner:non-active");
      if (bPetActive == true) {
        Serial.println("pet:active");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        delay(10);
        // setCpuFrequencyMhz(20);  //No BT/Wifi: 10,20,40 MHz, for BT/Wifi, 80,160,240MHz
        // delay(10);
        Serial.println("appending data");

        uint32_t timestamp_dive = millis();
        idx[0] = 0;
        // timestamp[0] = millis();  //check it later
        while (true) {
          //logging sensor data
          // IMU: 30Hz, Alt: 10 Hz, RH: 10s, T: 10s
          // if (millis() > timestamp[0]) {
          if (idx[0] == 0) {
            timestamp[0] = millis();
            Serial.println(timestamp[0]);
            memcpy(&acc[2], &timestamp[0], sizeof(uint32_t));
          }
          timestamp[0] += dt[0];
          getSensorDataIMU(&acc[3 * idx[0] + 6]);

          idx[0]++;
          if (idx[0] == LEN_ACCSMPL) {
            uint32_t dum = 0;
            memcpy(&dum, &acc[2], sizeof(uint32_t));
            Serial.println("dum:" + String(dum));
            appendFileBytes(SPIFFS, path, (uint8_t*)acc, LEN_ACCSMPL * 3 + 6);
            idx[0] = 0;
            iter++;
          }
          // }

          if (millis() - timestamp_dive > 120 * 1000) {
            break;
          }
        }

        setCpuFrequencyMhz(80);  //No BT/Wifi: 10,20,40 MHz, for BT/Wifi, 80,160,240MHz

        Serial.println("connecting to AP");
        bool isAPconnected = false;
        // WiFi.begin(ssid, pass);
        for (int i = 0; i < 3; i++) {
          isAPconnected = ConnectToRouter(AP_id.c_str(), AP_pw.c_str());
          if (isAPconnected) {
            break;
          }
        }
        if (isAPconnected) {
          Serial.println("AP connected");
        } else {
          Serial.println("AP not connected. Proceed anyway..");
        }

        //transmitting the log
        if (!MQTTclient.connected()) {
          connect();
        }
        Serial.printf("Reading file: %s\r\n", path);
        File file = SPIFFS.open(path);
        if (!file || file.isDirectory()) {
          Serial.println("- failed to open file for reading");
          return;
        }
        Serial.println("- read from file:");
        uint8_t buffer[LEN_ACCSMPL * 3 + 6];
        // while(file.available()){
        if (file.available()) {
          for (int i = 0; i < iter; i++) {
            file.seek(i * (LEN_ACCSMPL * 3 + 6));
            file.read(buffer, LEN_ACCSMPL * 3 + 6);
            // for (int i = 0; i < LEN_ACCSMPL * 3; i++) {
            for (int j = 2; j < 6; j++) {
              Serial.println(buffer[j]);
              // if (i % 3 == 2) {
              //   Serial.println();
              // } else {
              //   Serial.print(",");
              // }
            }
            Serial.println("-----------------------");
            // Serial.println(i);
            // MQTTclient.publish("perpet/SerialNumber/acc", (const char*)buffer,LEN_ACCSMPL*3+6);
            MQTTclient.publish((topic_base + "/acc").c_str(), (const char*)buffer, LEN_ACCSMPL * 3 + 6);
          }
        } else {
          Serial.println("File not available!");
          delay(5000);
        }
        iter = 0;
        file.close();

        if (SPIFFS.remove("/acc.txt")) {
          Serial.println("File '/acc.txt' deleted.");
        } else {
          Serial.println("Error deleting file '/acc.txt'.");
        }

      } else {
        setCpuFrequencyMhz(20);  //No BT/Wifi: 10,20,40 MHz, for BT/Wifi, 80,160,240MHz
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
        Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
        Serial.println("Going to sleep now");
        Serial.flush();
        // WiFi.disconnect(true);
        // WiFi.mode(WIFI_OFF);
        esp_light_sleep_start();
        Serial.println("WiFi turning on!");
      }
    }


  } else if (op_mode == OP_MODE_BLE) {  // BLE Control
    if (deviceConnected) {
      bViewerActive = true;
      pTxCharacteristic->setValue(&txValue, 1);
      pTxCharacteristic->notify();
      txValue++;
      delay(50);  // bluetooth stack will go into congestion, if too many packets are sent
    } else {
      bViewerActive = false;
    }

    // if BLE is disconnected
    if (!deviceConnected && oldDeviceConnected) {
      delay(500);                   // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising();  // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  }
}
