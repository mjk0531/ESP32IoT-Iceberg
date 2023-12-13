/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#include "eepromCustom.h"

/*************************** Sketch Code ************************************/

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// #define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
// #define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
// #define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define SERVICE_UUID "167d0001-d1ea-11ed-afa1-0242ac120002"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "167d0002-d1ea-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID_TX "167d0003-d1ea-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID_APID "167d0011-d1ea-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID_APPW "167d0012-d1ea-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID_MQTT "167d0013-d1ea-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID_SN "167d0014-d1ea-11ed-afa1-0242ac120002"
#define CHARACTERISTIC_UUID_BAT_VOLT "167d0004-d1ea-11ed-afa1-0242ac120002"
#define HEADERSIZE 0

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i], HEX);

      Serial.println();
      Serial.println("*********");
    }
  }
};

class MyCallbacks_APID : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("APID_Received: ");
      String str = String(rxValue.substr(HEADERSIZE).c_str());
      // String str = String(rxValue.c_str());
      AP_NAME.writeString(0, str);
      AP_NAME.commit();
      AP_NAME.get(0, buf_eeprom);
      Serial.println(String(buf_eeprom));

      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i], HEX);
      Serial.println();
      Serial.println("*********");
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE]APID_Read: ");
    AP_NAME.get(0, buf_eeprom);
    Serial.println(String(buf_eeprom));
    pCharacteristic->setValue((uint8_t *)buf_eeprom, strlen(buf_eeprom));
  }
};

class MyCallbacks_APPW : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("APPW_Received: ");
      String str = String(rxValue.substr(HEADERSIZE).c_str());
      AP_PASSWORD.writeString(0, str);
      AP_PASSWORD.commit();
      AP_PASSWORD.get(0, buf_eeprom);
      Serial.println(String(buf_eeprom));

      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i], HEX);
      Serial.println();
      Serial.println("*********");
    }
  }
};

class MyCallbacks_MQTT : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("[BLE]MQTT_length: ");
      uint16_t len = rxValue[0];
      len = (len << 8) | rxValue[1];
      Serial.println(len);
      Serial.print("[BLE]MQTT_Received: ");
      String str = String(rxValue.substr(HEADERSIZE).c_str());
      MQTT_TOKEN.writeString(0, str);
      MQTT_TOKEN.commit();
      MQTT_TOKEN.get(0, buf_eeprom);
      Serial.println(String(buf_eeprom));


      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i], HEX);
      Serial.println();
      Serial.println("*********");
    }
  }
};

class MyCallbacks_SN : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("[BLE]SN_length: ");
      uint16_t len = rxValue[0];
      len = (len << 8) | rxValue[1];
      Serial.println(len);
      Serial.print("[BLE]SN_Received: ");
      String str = String(rxValue.substr(HEADERSIZE).c_str());
      SERIAL_NUMBER.writeString(0, str);
      SERIAL_NUMBER.commit();
      SERIAL_NUMBER.get(0, buf_eeprom);
      Serial.println(String(buf_eeprom));


      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i], HEX);
      Serial.println();
      Serial.println("*********");
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("[BLE]SN_Read: ");
    SERIAL_NUMBER.get(0, buf_eeprom);
    Serial.println(String(buf_eeprom));
    pCharacteristic->setValue((uint8_t *)buf_eeprom, strlen(buf_eeprom));
  }
};

class MyCallbacks_BAT_VOLT : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    BAT_VOLT.writeString(0, String(batteryVoltEEP()));
    BAT_VOLT.commit();
    BAT_VOLT.get(0, buf_eeprom);
    Serial.println(String(buf_eeprom));
    pCharacteristic->setValue((uint8_t *)buf_eeprom, strlen(buf_eeprom));
  }

  void onNotify(BLECharacteristic *pCharacteristic) {
    BAT_VOLT.writeString(0, String(batteryVoltEEP()));
    BAT_VOLT.commit();
    BAT_VOLT.get(0, buf_eeprom);
    Serial.println(String(buf_eeprom));
    pCharacteristic->setValue((uint8_t *)buf_eeprom, strlen(buf_eeprom));
  }
};

class MyCallBacks_BAT_LEV : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    BAT_LEV.writeString(0, String(batteryLevEEP()));
    BAT_LEV.commit();
    BAT_LEV.get(0, buf_eeprom);
    Serial.println(String(buf_eeprom));
    uint8_t sendValue = String(buf_eeprom).toInt();
    pCharacteristic->setValue(&sendValue, sizeof(sendValue));
  }

  void onNotify(BLECharacteristic *pCharacteristic) {
    BAT_LEV.writeString(0, String(batteryLevEEP()));
    BAT_LEV.commit();
    BAT_LEV.get(0, buf_eeprom);
    Serial.println(String(buf_eeprom));
    pCharacteristic->setValue((uint8_t *)buf_eeprom, strlen(buf_eeprom));
  }
};

void ble_setup_custom() {
  // Create the BLE Device
  BLEDevice::init("Perpet_DEV");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  BLEDescriptor *RxCharacteristic = new BLEDescriptor((uint16_t)0x2904);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // BLECharacteristic *pCharacteristicBAT_VOLT = pService->createCharacteristic(
  //   CHARACTERISTIC_UUID_BAT_VOLT,
  //   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  // pCharacteristicBAT_VOLT->setCallbacks(new MyCallbacks_BAT_VOLT());

  BLECharacteristic *pCharacteristicAPID = pService->createCharacteristic(
    CHARACTERISTIC_UUID_APID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicAPID->setCallbacks(new MyCallbacks_APID());

  BLECharacteristic *pCharacteristicAPPW = pService->createCharacteristic(
    CHARACTERISTIC_UUID_APPW,
    BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicAPPW->setCallbacks(new MyCallbacks_APPW());

  BLECharacteristic *pCharacteristicMQTT = pService->createCharacteristic(
    CHARACTERISTIC_UUID_MQTT,
    BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicMQTT->setCallbacks(new MyCallbacks_MQTT());

  BLECharacteristic *pCharacteristicSN = pService->createCharacteristic(
    CHARACTERISTIC_UUID_SN,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicSN->setCallbacks(new MyCallbacks_SN());

  // Start the service
  pService->start();

  // Create the Battery Service
  BLEService *pBatteryService = pServer->createService("180F");

  // Create the Battery Level Characteristic
  BLECharacteristic *pBatteryLevelCharacteristic = pBatteryService->createCharacteristic(
    "2A19",
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pBatteryLevelCharacteristic->setCallbacks(new MyCallBacks_BAT_LEV());

  // Set the permissions for the Battery Level Characteristic
  pBatteryLevelCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ);

  // Add a BLE2902 descriptor to the Battery Level Characteristic
  pBatteryLevelCharacteristic->addDescriptor(new BLE2902());

  // Start the Battery Service
  pBatteryService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("[BLE]Waiting a client connection to notify...");
}