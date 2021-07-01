
// Unique UUID for each board. Any better way to achieve this easily?
#define DEVICE_UUID "a669c4da-5eeb-11eb-ae93-0242ac130002"
//#define DEVICE_UUID "8c7448de-3d36-4e64-bf01-efa58b44d45b"

#define BUS_PIN 25
#define ADR_SIZE 8 //byte

//vytvoreni instanci
OneWire Bus(BUS_PIN);
DallasTemperature Sensors(&Bus);
DeviceAddress DevAdr;

uint8_t nSensors = 0;


#define PLOVAK 12
#define LED_G 0
#define LED_R 0
#define LED_B 0
#define RELE_01 17
#define RELE_02 13
#define RELE_03 15
#define RELE_04 2

#define pinDHT 26
#define typDHT11 DHT11     // or DHT22

#define BAUD 19200

// Available sensors/controls
#define A_TEMP "temp"
#define A_HUM "hum"
#define W_TEMP "water_temp"
#define W_LEVEL "water_level"
#define SWITCH1 "switch_01"
#define SWITCH2 "switch_02"
#define SWITCH3 "switch_03"
#define SWITCH4 "switch_04"
#define BLINK "blink"