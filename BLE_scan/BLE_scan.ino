/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/
#include "NimBLEDevice.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <String>
#include <time.h>

int scanTime = 5; //In seconds
NimBLEScan* pBLEScan;
const char* uuid = "0xfd6f";
WiFiMulti WiFiMulti;

static std::string addrSwitchBot = "fd:8d:3f:e8:3d:d3";
static NimBLEUUID serviceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
static NimBLEUUID    charUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");
static uint8_t cmdPress[3] = {0x57, 0x01, 0x00};

static NimBLEAddress *pGattServerAddress;
static NimBLERemoteCharacteristic* pRemoteCharacteristic;
static NimBLEClient*  pClient = NULL;
static boolean doSendCommand = false;
boolean firstTime = true;
int lastTimeInHour = -1;
int lastTimeInDay = -1;

static bool connectAndSendCommand(BLEAddress pAddress) {
  Serial.printf("start connectAndSendCommand\n");
  pClient  = NimBLEDevice::createClient();

  pClient->connect(pAddress);
  Serial.printf("connected\n");

  // 対象サービスを得る
  NimBLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.printf("target service not found\n");
    return false;
  }
  Serial.printf("found target service\n");

  // 対象キャラクタリスティックを得る
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.printf("target characteristic not found\n");
    return false;
  }
  Serial.printf("found target characteristic\n");

  // キャラクタリスティックに Press コマンドを書き込む
  pRemoteCharacteristic->writeValue(cmdPress, sizeof(cmdPress), false);

  delay(3000);
  if (pClient) {
    pClient->disconnect();
    pClient = NULL;
  }
  return true;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Arigato_Java", "takishima");
  WiFiMulti.addAP("JAISTALL", "");
  NimBLEDevice::init("");
  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void sendCommandToSwitchBotIfNeeded(NimBLEAdvertisedDevice ad) {
  Serial.println("send comamnd if needed");
  struct tm timeInfo;
  getLocalTime(&timeInfo);
  if (lastTimeInHour != timeInfo.tm_hour) {
    if (lastTimeInHour == -1) {
      Serial.println("first_time");
      lastTimeInHour = timeInfo.tm_hour;
      return;
    }
    lastTimeInHour = timeInfo.tm_hour;
    if(lastTimeInHour >= 18 || lastTimeInHour <= 8){
    NimBLEAddress adress = NimBLEAddress(ad.getAddress());
    Serial.printf("comamnd send Time : %d\n",lastTimeInHour);
    connectAndSendCommand(adress); 
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  NimBLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  int deviceNum = foundDevices.getCount();
  int enDevNum = 0;
  Serial.println(deviceNum);
  bool foundSwitchBot = false;
  NimBLEAdvertisedDevice switchBotAd;
  for (int i = 0; i < deviceNum; i ++) {
    NimBLEAdvertisedDevice ad = foundDevices.getDevice(i);
    Serial.printf("uuid : %s\n",ad.getServiceUUID().toString().c_str());
    if (ad.haveServiceUUID() && strncmp(ad.getServiceUUID().toString().c_str(), uuid, 36) == 0) {
      Serial.printf("uuid : %s, addr : %s, rssi : %d\n", ad.getServiceUUID().toString().c_str(), ad.getAddress().toString().c_str(), ad.getRSSI());
      enDevNum ++;
    } else if (ad.getAddress().toString() == addrSwitchBot) {
      Serial.printf("found switchBot\n");
      foundSwitchBot = true;
      switchBotAd = ad;
    }
  }
  if(foundSwitchBot && enDevNum >= 1){
    Serial.println("");
    sendCommandToSwitchBotIfNeeded(switchBotAd);
  }
  Serial.println("Scan done!");
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    //Serial.println(" connected");
    if (firstTime) {
      configTzTime("JST-9", "ntp.nict.jp");
      Serial.println("configure time");
      firstTime = false;
    }
    HTTPClient http;
    http.begin("150.65.118.96", 8086, "/write?db=mydb");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    char writeBuf[512];
    sprintf(writeBuf, "%s %s=%d", "phone", "value", enDevNum);
    int res = http.POST(writeBuf);
    http.end();
    Serial.println(res);
  }
}
