#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset= */ 16);


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1        /* Time ESP32 will go to sleep (in seconds) */


static void wifi_power_save(void);

BLECharacteristic* pCharacteristic1 = 0;
BLECharacteristic* pCharacteristic2 = 0;
BLECharacteristic* pCharacteristic3 = 0;



bool deviceConnected = false;
uint8_t value = 0;

// SIMPLE KEY OR BUTTON SERVICE
#define S1_UUID    "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHAR1_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

// IMMEDIATE ALERT SERVICE
#define S2_UUID    BLEUUID((uint16_t)0x1802)
#define CHAR2_UUID BLEUUID((uint16_t)0x02a06)

// BATTERY_SERVICE
#define S3_UUID    BLEUUID((uint16_t)0x180F)
#define CHAR3_UUID BLEUUID((uint16_t)0x2A19)

class MyCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic2)
    {

        Serial.print("Received Value: ");

        std::string rxValue = pCharacteristic2->getValue();

        u8x8.setCursor(0, 2);
        if (rxValue[0] == 2)
            u8x8.print("ON  ");
        else if (rxValue[0] == 0)
            u8x8.print("OFF ");
    }
};


class MyServerCallbacks: public BLEServerCallbacks
{

    void onConnect(BLEServer* pServer)
    {
        deviceConnected = true;
        Serial.println("Device connected ...");

    };
    void onDisconnect(BLEServer* pServer)
    {
        deviceConnected = false;
        Serial.println("Device disconnected ......");
        Serial.println("ESP32 go to sleep for " + String(TIME_TO_SLEEP) + " s");
        esp_deep_sleep_start();
    }
};

void setup()
{
    Serial.begin(115200);


    pinMode(0, INPUT);
    pinMode(16, OUTPUT);
    digitalWrite(16, LOW);  // set GPIO16 low to reset OLED
    delay(50);
    digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

    u8x8.begin();
    u8x8.setPowerSave(0);
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0, 0, "iTAG - Esp32");

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    uint8_t new_mac[] = {0xFF, 0xFF, 0xFF, 0x08, 0x08, 0x06};
    //srand(esp_random());for (int i = 0; i < sizeof(new_mac); i++) { new_mac[i] = rand() % 256;}
    esp_base_mac_addr_set(new_mac);

    // Create the BLE Device
    BLEDevice::init("iTAG-EMU");

    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service   // Create a BLE Characteristic
    BLEService *pService1 = pServer->createService(S1_UUID);
    pCharacteristic1 = pService1->createCharacteristic(
                           CHAR1_UUID,
                           BLECharacteristic::PROPERTY_NOTIFY |
                           BLECharacteristic::PROPERTY_READ
                       );


//  BLECharacteristic::PROPERTY_NOTIFY |
// Create the BLE Service   // Create a BLE Characteristic


    BLEService *pService2 = pServer->createService(S2_UUID);
    pCharacteristic2 = pService2->createCharacteristic(
                           CHAR2_UUID,
                           BLECharacteristic::PROPERTY_NOTIFY |
                           BLECharacteristic::PROPERTY_WRITE  |
                           BLECharacteristic::PROPERTY_WRITE_NR
                       );

    pCharacteristic2->setCallbacks(new MyCallbacks());


    BLEService *pService3 = pServer->createService(S3_UUID);
    pCharacteristic3 = pService3->createCharacteristic(
                           CHAR3_UUID,
                           BLECharacteristic::PROPERTY_READ   |
                           BLECharacteristic::PROPERTY_NOTIFY
                       );



// Create a BLE Descriptor - not used in order to mimic iTag
// pCharacteristic1->addDescriptor(new BLE2902());
// pCharacteristic2->addDescriptor(new BLE2902());
// pCharacteristic3->addDescriptor(new BLE2902());


    // Start the service
    pService1->start();
    pService2->start();


   value = 20;
   pCharacteristic3->setValue(&value,1);
   pService3->start();

// Start advertising
    pServer->getAdvertising()->start();

    delay(3000);

    if (!deviceConnected)
    {
        Serial.println("ESP32 go to sleep for " + String(TIME_TO_SLEEP) + " s");
        esp_deep_sleep_start();
    }
}



void loop()
{

    if (!digitalRead(0) )
    {
        value = 1;
        Serial.printf("*** NOTIFY: %d ***\n", value);
        pCharacteristic1->setValue(&value, 1);
        pCharacteristic1->notify();

        while (!digitalRead(0))
        {
            delay(20);
        }
    }
    else
    {
        value = 0;
        //Serial.printf("*** NOTIFY: %d ***\n", value);
        // pCharacteristic1->setValue(&value, 1);
        // pCharacteristic1->notify();
        delay(100);
    }
}
