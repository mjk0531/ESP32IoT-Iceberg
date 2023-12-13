#ifndef __SETTINGS__
#define __SETTINGS__
#include "EEPROM.h"

#define OP_MODE_SETTING 0
#define OP_MODE_BLE 1
#define OP_MODE_WIFI 2
#define OP_MODE_OTA 3
#define OP_MODE_SLEEP 4

#define N 13

//ESP32 by Espressif Systems Version 2.0.5. Other version may have different EEPROMClass methods

// EEPROMClass  INIT_FLAG("INIT_FLAG", 0x40);
// EEPROMClass  DEV_NAME("dev_name", 0x100);
// EEPROMClass  SERVER_IP("server_ip", 0x200);
// EEPROMClass  SERVER_PORT("server_port", 10);
// EEPROMClass  AP_NAME("ap_id", 0x100);
// EEPROMClass  AP_PASSWORD("ap_pw", 0x100);
EEPROMClass INIT_FLAG("INIT_FLAG");
EEPROMClass DEV_NAME("dev_name");
EEPROMClass SERIAL_NUMBER("serial_number");
EEPROMClass SERVER_IP("server_ip");
EEPROMClass SERVER_PORT("server_port");
EEPROMClass AP_NAME("ap_id");
EEPROMClass AP_PASSWORD("ap_pw");
EEPROMClass MQTT_TOKEN("mqtt_token");
EEPROMClass OP_MODE("op_mode");

EEPROMClass BAT_LEV("bat_lev");
EEPROMClass BAT_VOLT("bat_volt");

EEPROMClass SENSOR_PRS("sensor_prs");
EEPROMClass SENSOR_TEMP("sensor_temp");
EEPROMClass SENSOR_RH("sensor_rh");

EEPROMClass SENSOR_ACC("sensor_acc");

String dev_name = "perpetDevN";
String serial_number = "PET";
String server_addr = "3.35.134.41";
int server_port = 1883;
String AP_id = "";
String AP_pw = "";
String topic_base = "perpet/" + serial_number;
int op_mode = 0;
int analogVolts;
uint8_t batteryLevel;

float table_voltage[] = { 4.2, 4.0, 3.96, 3.93, 3.90, 3.86, 3.83, 3.80, 3.76, 3.73, 3.70, 3.67, 2.75 };
int table_level[] = { 100, 84, 77, 70, 63, 56, 48, 43, 35, 27, 21, 14, 0 };

float table_slope[N - 1];
float y_intercept[N - 1];

const int BatVoltNumReadings = 10;
int BatVoltReadings[BatVoltNumReadings];
int BatVoltindex = 0;
bool BatVoltinitFlag = false;
int BatVolttotal = 0;

struct Sensor {
  String name;
  uint32_t sampling_interval;
  uint16_t buffer_size;
  bool enable;
  bool en_avg_filter;
  //what else..?
};

Sensor sensor[2];

struct DevInfo {
  String id;
  String pw;
  String description;
  bool b_TimeSeriesData;
  bool b_MediaData;
};
DevInfo devInfo;


struct DevParams {
  int32_t condition_1;
  uint32_t condition_2;
  bool condition_3;
};
DevParams devParams;

char buf_eeprom[64];

int batteryVoltEEP() {
  float average = 0.0;
  int analogVolts_temp = 2 * analogReadMilliVolts(34);

  if (BatVoltinitFlag == true) {
    for (int j = 0; j < BatVoltNumReadings; j++) {
      BatVoltReadings[j] = analogVolts_temp;
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

  average = BatVolttotal / BatVoltNumReadings;
  analogVolts_temp = average;
  return analogVolts_temp;
}

int batteryLevEEP() {
  float average = 0.0;
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

  average = BatVolttotal / BatVoltNumReadings;
  int batteryLevel_temp;

  // Convert the average voltage to battery level
  if (average > 4200) {
    batteryLevel_temp = 100;
  } else if (average < 2750) {
    batteryLevel_temp = 0;
  } else {
    float v = average / 1000.0;
    // Find the pair of points that corresponds to the table_voltage range
    int i = 0;
    while (i < N - 1 && v < table_voltage[i + 1]) {
      i++;
    }
    // Calculate the table_level using the equation of the line
    float l = table_slope[i] * v + y_intercept[i];

    // Round the table_level to the nearest integer
    batteryLevel_temp = l;
  }

  return batteryLevel_temp;
}


bool init_eeprom() {
  if (!INIT_FLAG.begin(0x40)) {
    Serial.println("Failed to initialize EEPROM:INIT_FLAG");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!DEV_NAME.begin(0x100)) {
    Serial.println("Failed to initialize EEPROM:DEV_NAME");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!SERIAL_NUMBER.begin(0xFF)) {
    Serial.println("Failed to initialize EEPROM:SERIAL_NUMBER");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!SERVER_IP.begin(0x200)) {
    Serial.println("Failed to initialize EEPROM:SERVER_IP");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!SERVER_PORT.begin(0x0A)) {
    Serial.println("Failed to initialize EEPROM:SERVER_PORT");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  if (!AP_NAME.begin(0x100)) {
    Serial.println("Failed to initialize EEPROM:AP_NAME");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!AP_PASSWORD.begin(0x100)) {
    Serial.println("Failed to initialize EEPROM:AP_PASSWORD");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!MQTT_TOKEN.begin(512)) {
    Serial.println("Failed to initialize EEPROM:MQTT_TOKEN");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!OP_MODE.begin(0x02)) {
    Serial.println("Failed to initialize EEPROM:OP_MODE");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!BAT_LEV.begin(0x40)) {
    Serial.println("Failed to initialize EEPROM:OP_MODE");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  if (!BAT_VOLT.begin(0x40)) {
    Serial.println("Failed to initialize EEPROM:OP_MODE");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  for (int i = 0; i < N - 1; i++) {
    table_slope[i] = (table_level[i + 1] - table_level[i]) / (table_voltage[i + 1] - table_voltage[i]);
    y_intercept[i] = table_level[i] - table_slope[i] * table_voltage[i];
  }
  BatVoltinitFlag = true;

  return true;
}

void eepromSetup_custom() {
  INIT_FLAG.get(0, buf_eeprom);
  String init_flag = String(buf_eeprom);
  Serial.println("--flag--");
  Serial.println(init_flag);
  Serial.println("--------");

  if (init_flag != "EEPROM_INITIATED") {
    // Write: Variables ---> EEPROM stores
    DEV_NAME.writeString(0, dev_name);
    DEV_NAME.commit();
    SERIAL_NUMBER.writeString(0, serial_number);
    SERIAL_NUMBER.commit();
    SERVER_IP.writeString(0, server_addr);
    SERVER_IP.commit();
    SERVER_PORT.put(0, server_port);
    SERVER_PORT.commit();
    AP_NAME.writeString(0, AP_id);
    AP_NAME.commit();
    AP_PASSWORD.writeString(0, AP_pw);
    AP_PASSWORD.commit();
    OP_MODE.put(0, OP_MODE_SETTING);
    OP_MODE.commit();
    BAT_LEV.put(0, String(batteryLevEEP()));
    BAT_LEV.commit();
    BAT_VOLT.put(0, String(batteryVoltEEP()));
    BAT_VOLT.commit();

    INIT_FLAG.writeString(0, "EEPROM_INITIATED");
    INIT_FLAG.commit();
    Serial.print("device name: ");
    Serial.println(dev_name);
    Serial.print("target server addr: ");
    Serial.println(server_addr);
    Serial.print("target server port: ");
    Serial.println(server_port);
    Serial.print("AP name: ");
    Serial.println(AP_id);
    Serial.print("AP password: ");
    Serial.println(AP_pw);
    Serial.println("EEPROM now initiated.");
    Serial.println("");
  } else {
    Serial.println("EEPROM already initiated!");
    Serial.println("");
  }
  // Clear variables
  //  buf_eeprom[0]='\0';

  Serial.println("");
  DEV_NAME.get(0, buf_eeprom);
  dev_name = String(buf_eeprom);
  SERIAL_NUMBER.get(0, buf_eeprom);
  serial_number = String(buf_eeprom);
  SERVER_IP.get(0, buf_eeprom);
  server_addr = String(buf_eeprom);
  SERVER_PORT.get(0, server_port);
  //  Server_port=String(buf_eeprom);
  AP_NAME.get(0, buf_eeprom);
  AP_id = String(buf_eeprom);
  AP_PASSWORD.get(0, buf_eeprom);
  AP_pw = String(buf_eeprom);
  OP_MODE.get(0, op_mode);
  BAT_LEV.put(0, String(batteryLevEEP()));
  BAT_LEV.commit();
  BAT_LEV.get(0, buf_eeprom);
  batteryLevel = String(buf_eeprom).toInt();
  BAT_VOLT.put(0, String(batteryVoltEEP()));
  BAT_VOLT.commit();
  BAT_VOLT.get(0, buf_eeprom);
  analogVolts = String(buf_eeprom).toInt();

  // setDevInfo();
  // setDevParams();
  // setSensorParams();
}
void setOpMode(int _mode) {
  OP_MODE.put(0, _mode);
  OP_MODE.commit();
  return;
}

int getOpMode() {
  int _mode;
  OP_MODE.get(0, _mode);
  return _mode;
}

void setDevInfo() {
  devInfo.id = "PERPET0000";
  devInfo.pw = "0000";
  devInfo.description = "PERPET";
  devInfo.b_TimeSeriesData = true;
  devInfo.b_MediaData = false;
}
void setDevParams() {
  devParams.condition_1 = 1;
  devParams.condition_2 = 2;
  devParams.condition_3 = true;
}
void setSensorParams() {
  sensor[0].name = "potential/V";
  sensor[0].sampling_interval = 10;
  sensor[0].buffer_size = 10;
  sensor[0].enable = true;
  sensor[0].en_avg_filter = false;

  sensor[1].name = "current/mA";
  sensor[1].sampling_interval = 10;
  sensor[1].buffer_size = 10;
  sensor[1].enable = true;
  sensor[1].en_avg_filter = false;

  //what about table-form data?
  //ex)
  //  t: time, x: potential, y: current, ....
}

#endif
