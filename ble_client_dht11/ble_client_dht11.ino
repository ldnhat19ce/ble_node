

#include "BLEDevice.h"
#include <DHT.h>

// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID_2("1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e");
static BLEUUID    charUUID_3("f639b61e-f796-11ed-b67e-0242ac120002");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteChar_2;
BLERemoteCharacteristic* pRemoteChar_3;

#define DHTPIN 4
#define DHTTYPE DHT11
#define LED 2

DHT dht(DHTPIN, DHTTYPE);

// Callback function for Notify function
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                           uint8_t* pData,
                           size_t length,
                           bool isNotify) {
  if (pBLERemoteCharacteristic->getUUID().toString() == charUUID_1.toString()) {

    // convert received bytes to integer
    uint32_t counter = pData[0];
    for (int i = 1; i < length; i++) {
      counter = counter | (pData[i] << i * 8);
    }
  }
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.println("onConnect");
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("onDisconnect");
    }
};

// Function that is run whenever the server is connected
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  //pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  connected = true;
  pRemoteChar_2 = pRemoteService->getCharacteristic(charUUID_2);
  pRemoteChar_3 = pRemoteService->getCharacteristic(charUUID_3);

  if (connectCharacteristic(pRemoteService, pRemoteChar_2) == false)
    connected = false;
  else if (connectCharacteristic(pRemoteService, pRemoteChar_3) == false)
    connected = false;

  if (connected == false) {
    pClient-> disconnect();
    Serial.println("At least one characteristic UUID not found");
    return false;
  }
  return true;
}

// Function to chech Characteristic
bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  if (l_BLERemoteChar == nullptr) {
    Serial.print("Failed to find one of the characteristics");
    Serial.print(l_BLERemoteChar->getUUID().toString().c_str());
    return false;
  }
  Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));

  if (l_BLERemoteChar->canNotify())
    l_BLERemoteChar->registerForNotify(notifyCallback);

  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    //Called for each advertising BLE server.
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  pinMode(LED, OUTPUT);

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  if (connected) {
    digitalWrite(LED, HIGH);
    float temperature = dht.readTemperature();
    char strTemperature[5];
    dtostrf(temperature, 4, 2, strTemperature);
    String stringTemperature(strTemperature);
    pRemoteChar_2->writeValue(stringTemperature.c_str(), stringTemperature.length());
    Serial.println("Send value temp: " + String(temperature));

    float humidity = dht.readHumidity();
    char strHumidity[5];
    dtostrf(humidity, 4, 2, strHumidity);
    String stringHumidity(strHumidity);
    pRemoteChar_3->writeValue(stringHumidity.c_str(), stringHumidity.length());
    Serial.println("Send value humidity: " + String(humidity));
    delay(49000);
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  } else {
    digitalWrite(LED, LOW);
  }

  delay(1000);
}
