#ifndef WBC_FILE
#define WBC_FILE

// device description
#define WBC_NAME "Water Board Counter"
#define WBC_VERSION "1.0"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#define WBC_COUNTER_SIZE 4
#define LED_PIN 2
#define SETUP_BUTTON_PIN 12
#define VDROP_PIN 4

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

#define WIFI_PARAM_COUNT 5+WBC_COUNTER_SIZE*2
#define WIFI_PARAM_DEVICE 0
#define WIFI_PARAM_SERVER 1
#define WIFI_PARAM_PORT 2
#define WIFI_PARAM_USER 3
#define WIFI_PARAM_PASSW 4
#define WIFI_PARAM_COUNTER_SERIAL_0 5
#define WIFI_PARAM_COUNTER_VAL_0 6
#define WIFI_PARAM_COUNTER_SERIAL_1 7
#define WIFI_PARAM_COUNTER_VAL_1 8
#if WBC_COUNTER_SIZE==4
#endif

#define WBC_INI "/wbc.ini"
#define WBC_JSON "/wbc.json"

static const char *mqtt_topic_result PROGMEM = "/result";
static const char *mqtt_topic_set PROGMEM = "/set";
static const char *mqtt_topic_status PROGMEM = "/status";
static const char *mqtt_topic_firmware PROGMEM = "/firmware";

#define INTERRUPT_ALL 0xff
#define WAIT_SETUP_MS 3000
#define WAIT_SLEEP_MS 60000

#define WBC_STATUS_OK 0
#define WBC_STATUS_SETUP 1
#define WBC_STATUS_VDROP 2
#define WBC_STATUS_SLEEP 3

#include<WiFiManager.h>
#include <PubSubClient.h>

#include"wbc_counter.h"
#include"wbc_mqtt.h"

class WaterBoardCounter
{
public:
    bool setup();
    void loop();

    void connect_broker();

    void update_counter(uint8_t index, uint8_t value);
    void setup_button();

    WiFiManager *get_wifi_manager(){return manager;}
private:
    bool setup_wifi(const char *device_name, const char *server, const char *port, const char *mqtt_user, const char *mqtt_password);
    bool setup_wifi();

    bool init_wifi();

    void init_config();
    void read_config();
    void save_config();
    
    void attach_interrupt(uint8_t pin);
    void detach_interrupt();

    void light_sleep();

    bool send_result();

    void configure_mqtt_home_assistant();

    bool is_setup_active(unsigned long current_time_ms);

    WiFiManager *manager;
    WiFiManagerParameter *wifi_manager_params[WIFI_PARAM_COUNT];

    Counter counters[WBC_COUNTER_SIZE];
    bool need_update_value;
    PubSubClient mqtt_client;
    unsigned long before_time;

    unsigned char status;

    bool setup_button_down;
    unsigned long setup_button_time;

    bool send_module_data;

    WBCMqttBroker *broker;
};
#endif