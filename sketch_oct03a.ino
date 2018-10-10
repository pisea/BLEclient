/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
 
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8
 
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
 
   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

std::string MS01 = "MS01";
std::string MS02 = "MS02";
std::string MS03 = "MS03";
std::string ES01 = "ES01";
std::string ES02 = "ES02";
std::string pass;
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;
bool isMessageUpdate = false;
uint8_t value = 0;
int buttonState = 0;
int passwordWrongcout = 0;
int currentState;
 
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
 
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

 
void setConnectionState(int state) {

  switch(state) {
    
    case 0:
      digitalWrite(12, HIGH);
      digitalWrite(13, LOW);
      digitalWrite(14, LOW);
      break;
    
    case 1:
      digitalWrite(12, LOW);
      digitalWrite(13, HIGH);
      digitalWrite(14, LOW);
      break;
    
    case 2:
      digitalWrite(12, LOW);
      digitalWrite(13, LOW);
      digitalWrite(14, HIGH);
      break;
  }
  
}
 
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        setConnectionState(1); 
    };
 
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        setConnectionState(0);   
    }
};
 
 class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      
      if (rxValue.length() > 0) {

        if(rxValue.find("ES01") != std::string::npos) {
          digitalWrite(23,HIGH);  //鳴らす
          delay(100);
          digitalWrite(23,LOW);
          delay(1);
          digitalWrite(23,HIGH);  //鳴らす
          delay(100);
          digitalWrite(23,LOW);
          delay(1);
          digitalWrite(23,HIGH);  //鳴らす
          delay(100);
          digitalWrite(23,LOW);
          pass = "";
        }
       
        if(rxValue.find("MS02@") != std::string::npos) {
           pass = rxValue.substr(5,rxValue.size() - 1);
           currentState = 2;
           isMessageUpdate = true;
        }
        
        if(rxValue.find("MS03@") != std::string::npos) {
          rxValue = rxValue.substr(5,rxValue.size() - 1);
          if(pass == rxValue) {
            currentState = 3;
            isMessageUpdate = true;
          } else {
            currentState = 5;
            isMessageUpdate = true; 
          }
        } 
      }
    }
};
 
void setup() {
  
  pinMode(12,OUTPUT);
  pinMode(13,OUTPUT);
  pinMode(14,OUTPUT);
  pinMode(23,OUTPUT);
//  pinMode(15, INPUT);
  
  Serial.begin(115200);

  pass = "";
  
  // Create the BLE Device
  BLEDevice::init("Native_BLE_Test");
 
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
 
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
 
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
 
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  setConnectionState(false);  
}
 
void loop() {

//  buttonState = digitalRead(15);
//
//      if(buttonState == HIGH) {
//        Serial.println("press");
//      }
      
  if (deviceConnected) {
      
      if(pass.length() == 0){
        pCharacteristic->setValue(ES01);    
      } else {
        pCharacteristic->setValue(MS01);
      }
          
      if(isMessageUpdate) {
          switch(currentState) {
            case 1:
              pCharacteristic->setValue(MS01);
              break;   
            case 2:
              pCharacteristic->setValue(MS02);
              break;
            case 3:
              passwordWrongcout = 0;
              pCharacteristic->setValue(MS03);
              setConnectionState(2);
              digitalWrite(23,HIGH);  //鳴らす
              delay(100);
              digitalWrite(23,LOW);
              delay(1);
              digitalWrite(23,HIGH);  //鳴らす
              delay(100);
              digitalWrite(23,LOW);
              delay(1);
              digitalWrite(23,HIGH);  //鳴らす
              delay(100);
              digitalWrite(23,LOW);
              break;
            case 4:
              pCharacteristic->setValue(ES01);
              break;
            case 5:
              passwordWrongcout++;
              if(passwordWrongcout > 2) {
                digitalWrite(23,HIGH);  //鳴らす
              }
              pCharacteristic->setValue(ES02);
              break;   
          }
        isMessageUpdate = false;
      }
    
      pCharacteristic->notify();    
      delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    } 

   // disconnecting
   if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
   }
    
   // connecting
   if (deviceConnected && !oldDeviceConnected) {
   // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
   }
}


