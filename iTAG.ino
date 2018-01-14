#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset= */ 16);


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1        /* Time ESP32 will go to sleep (in seconds) */


static void wifi_power_save(void);

 
 
class iTAG
{
  public:

  void     init( const char* tag_name );

  void     set_btn( uint8_t btn );

  uint8_t  get_btn(){ return _btn;}

  void     set_batt( uint8_t batt );

  uint8_t  get_batt(){ return _batt;}

  bool     is_connected(){ return _cb_server->bConnect;}

  uint8_t  get_alert(){ return _cb_alert->alert;}



  protected:

  enum
  {
      CH_BTN,
      CH_ALERT,
      CH_BATT,
      CH_SZ
  };

  class AlertCallbacks: public BLECharacteristicCallbacks
  {
    public:
    
      void onWrite(BLECharacteristic* pCh )
      {
         std::string rxValue=pCh->getValue();

         if( rxValue.length()>0 )
         {
            alert = rxValue[0];
         }
      }

      uint8_t alert;
  };

  class ServerCallbacks: public BLEServerCallbacks
  {
  public:

      ServerCallbacks():bConnect(false){}
      
      void onConnect(BLEServer* pServer){ bConnect = true;};
      
      void onDisconnect(BLEServer* pServer){ bConnect = false; }

      uint8_t bConnect;
  };

 uint8_t            _btn;
 uint8_t            _batt;
 BLECharacteristic* _bl_char[CH_SZ];
 ServerCallbacks*   _cb_server;
 AlertCallbacks*    _cb_alert;
};

void     
iTAG::init(const char* tag_name )
{
    
    uint8_t new_mac[] = {0xFF, 0xFF, 0xFF, 0x08, 0x08, 0x06};
    //srand(esp_random());for (int i = 0; i < sizeof(new_mac); i++) { new_mac[i] = rand() % 256;}
    esp_base_mac_addr_set(new_mac);

// Create the BLE Device
    BLEDevice::init(tag_name);

// Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();

    _cb_server = new ServerCallbacks();
    pServer->setCallbacks(_cb_server);

// SIMPLE KEY OR BUTTON SERVICE
    BLEService *pService1 = pServer->createService("0000ffe0-0000-1000-8000-00805f9b34fb");
    _bl_char[CH_BTN] = pService1->createCharacteristic(
                           "0000ffe1-0000-1000-8000-00805f9b34fb",
                           BLECharacteristic::PROPERTY_NOTIFY |
                           BLECharacteristic::PROPERTY_READ
                       );
 
 
// IMMEDIATE ALERT SERVICE
    BLEService *pService2 = pServer->createService(BLEUUID((uint16_t)0x1802));
    _bl_char[CH_ALERT] = pService2->createCharacteristic(
                           BLEUUID((uint16_t)0x02a06),
                           BLECharacteristic::PROPERTY_NOTIFY |
                           BLECharacteristic::PROPERTY_WRITE  |
                           BLECharacteristic::PROPERTY_WRITE_NR
                       );

    _cb_alert = new AlertCallbacks();
    _bl_char[CH_ALERT]->setCallbacks( _cb_alert );

// BATTERY_SERVICE
    BLEService *pService3 = pServer->createService(BLEUUID((uint16_t)0x180F));
   _bl_char[CH_BATT]  = pService3->createCharacteristic(
                           BLEUUID((uint16_t)0x2A19),
                           BLECharacteristic::PROPERTY_READ   |
                           BLECharacteristic::PROPERTY_NOTIFY
                       );

    _batt = 20;
    _bl_char[CH_BATT]->setValue(&_batt,1);

// Create a BLE Descriptor - not used in order to mimic iTag
// pCharacteristic1->addDescriptor(new BLE2902());
// pCharacteristic2->addDescriptor(new BLE2902());
// pCharacteristic3->addDescriptor(new BLE2902());


    // Start the service
    pService1->start();
    pService2->start();
    pService3->start();

// Start advertising
   pServer->getAdvertising()->start();
}


 void     
 iTAG::set_btn( uint8_t btn )
 {
     _btn = btn;
     _bl_char[CH_BTN]->setValue(&_btn, 1);
     _bl_char[CH_BTN]->notify();
 }


  void     
 iTAG::set_batt( uint8_t batt )
 {
     _batt = batt;
     _bl_char[CH_BATT]->setValue(&_batt, 1);
     _bl_char[CH_BATT]->notify();
 }



 iTAG g_itag;


void setup()
{
    
    Serial.begin(115200);

// PRGM btn
    pinMode(0, INPUT);

// OLED
    pinMode(16, OUTPUT);
    digitalWrite(16, LOW);  // set GPIO16 low to reset OLED
    delay(50);
    digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

    u8x8.begin();
    u8x8.setPowerSave(0);
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0, 0, "iTAG - Esp32");

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

    g_itag.init("iTAG-EMU");

    delay(3000);

    if (g_itag.is_connected() )
          g_itag.set_batt( 30 );

}



void loop()
{

  static uint8_t alert = -1;

// BLE Connected ?
    if (!g_itag.is_connected() )
    {
        Serial.println("ESP32 go to sleep for " + String(TIME_TO_SLEEP) + " s");
        esp_deep_sleep_start();
    }

   
// Button pressed on iTag ?
   if (!digitalRead(0) )
    {
        g_itag.set_btn( 1 );

        while (!digitalRead(0))
        {
            delay(20);
        }
    }

// Client alert ?
   if( g_itag.get_alert() != alert )
   {
           alert  = g_itag.get_alert();
    
           u8x8.setCursor(0, 2);
            if (alert == 2)
               u8x8.print("ON  ");
           else if (alert == 0)
              u8x8.print("OFF ");
   }

   delay(100);
   
}
