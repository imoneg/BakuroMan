/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

int scanTime = 5; //In seconds
BLEScan* pBLEScan;
const char* uuid = "0000fd6f-0000-1000-8000-00805f9b34fb";
WiFiMulti WiFiMulti;
/*
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice ad) {
      num = 0;
      if(ad.haveServiceUUID() && strncmp(ad.getServiceUUID().toString().c_str(),uuid,36) == 0){
        Serial.printf("uuid : %s, addr : %s, rssi : %d\n",ad.getServiceUUID().toString().c_str(),ad.getAddress().toString().c_str(),ad.getRSSI());
        num ++;
      }
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};
*/

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Arigato_Java","takishima");
  WiFiMulti.addAP("JAISTALL","");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  int deviceNum = foundDevices.getCount();
  int enDevNum = 0;
  Serial.println(deviceNum);
  for(int i = 0; i < deviceNum; i ++){
    BLEAdvertisedDevice ad = foundDevices.getDevice(i);
    if(ad.haveServiceUUID() && strncmp(ad.getServiceUUID().toString().c_str(),uuid,36) == 0){
      Serial.printf("uuid : %s, addr : %s, rssi : %d\n",ad.getServiceUUID().toString().c_str(),ad.getAddress().toString().c_str(),ad.getRSSI());
      enDevNum ++;
    }
  }
  Serial.println("Scan done!");
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.println(" connected");
      HTTPClient http;
      http.begin("150.65.118.96",8086,"/write?db=mydb");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      char writeBuf[512];
      sprintf(writeBuf,"%s %s=%d","phone","value",enDevNum);
      int res = http.POST(writeBuf);
      http.end();
      Serial.println(res);
  }
}
