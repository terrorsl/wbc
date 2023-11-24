#ifndef WBC_FILE
#define WBC_FILE

#ifdef ESP8266
#include <ESP8266WiFi.h>
#define WBC_COUNTER_SIZE 2
#define LED_PIN 2
#define SETUP_BUTTON_PIN 12

#define WBC_0_PIN 13
#define WBC_1_PIN 14
#if WBC_COUNTER_SIZE == 4
#define WBC_2_PIN 5
#define WBC_3_PIN 17
#endif
#elif defined(ESP32)
#include<WiFi.h>
#define LED_PIN 2
#define SETUP_BUTTON_PIN 4
#define WBC_COUNTER_SIZE 4
#define WBC_0_PIN 15
#define WBC_1_PIN 12
#define WBC_2_PIN 13
#define WBC_3_PIN 14
#endif

#define WBC_INI "/wbc.ini"

#define INTERRUPT_ALL 0xff
#define WAIT_SETUP_MS 3000
#define WAIT_SLEEP_MS 60000

#include<WiFiManager.h>
#include <PubSubClient.h>

enum CounterValueType
{
    LITER,
    CUBIC_METER
};
struct Counter
{
    unsigned char value_type;
    // value per count, example 1 count = 10 liter
    unsigned char value_per_count;
    // full value
    unsigned long value;
    unsigned long timestamp;

    char serial[32];
};
class WaterBoardCounter
{
public:
    bool setup();
    void loop();

    void update_counter(uint8_t index, uint8_t value);
    void setup_button();

    WiFiManager *get_wifi_manager(){return manager;}
private:
    bool setup_wifi(const char *device_name, const char *server, const char *port, const char *mqtt_user, const char *mqtt_password);
    bool init_wifi(const char *server, uint16_t port, const char *mqtt_user, const char *mqtt_password);
    void attach_interrupt(uint8_t pin);
    void detach_interrupt();

    void light_sleep();

    void send_result();

    WiFiManager *manager;

    Counter counters[WBC_COUNTER_SIZE];
    PubSubClient mqtt_client;
    unsigned long before_time;

    bool setup_button_down;
    unsigned long setup_button_time;
};
#endif