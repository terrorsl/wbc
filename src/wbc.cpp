#include"wbc.h"
#include<LittleFS.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <CronAlarms.h>

//WiFiClient espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
DynamicJsonDocument config(256);
extern WaterBoardCounter wbc;

void onStationWifiConnected(const WiFiEventStationModeConnected &)
{
    wbc.connect_broker();
}
void onStationWifiDisconnected(const WiFiEventStationModeDisconnected&)
{
    //wbc.connect_wifi();
}

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.println(topic);
};
void saveWifiManagerParam()
{
    Serial.println("saveWifiManagerParam");
    WiFiManager *manager=wbc.get_wifi_manager();
    WiFiManagerParameter **params=manager->getParameters();

    String value = params[WIFI_PARAM_DEVICE]->getValue();
    config["mqtt"]["device"]=value;
    value=params[WIFI_PARAM_SERVER]->getValue();
    config["mqtt"]["server"]=value;
    config["mqtt"]["port"]=atol(params[WIFI_PARAM_PORT]->getValue());
    value=params[WIFI_PARAM_USER]->getValue();
    config["mqtt"]["user"]=value;
    value = params[WIFI_PARAM_PASSW]->getValue();
    config["mqtt"]["passw"]=value;

    value=params[WIFI_PARAM_COUNTER_SERIAL_0]->getValue();
    config["counter0"]["serial"]=value;
    config["counter0"]["value"]=atol(params[WIFI_PARAM_COUNTER_VAL_0]->getValue());
    //config("counter_1", "serial", params[WIFI_PARAM_COUNTER_SERIAL_1]->getValue());
    //config("counter_1", "value", params[WIFI_PARAM_COUNTER_VAL_1]->getValue());
}
void WeeklyAlarm()
{
}

IRAM_ATTR void wbc_0_callback()
{
    wbc.update_counter(0, 1);
}
IRAM_ATTR void wbc_1_callback()
{
    wbc.update_counter(1, 1);
}
IRAM_ATTR void wbc_2_callback()
{
    wbc.update_counter(2, 1);
}
IRAM_ATTR void wbc_3_callback()
{
    wbc.update_counter(3, 1);
}
IRAM_ATTR void setup_callback()
{
    wbc.setup_button();
}
void WaterBoardCounter::update_counter(uint8_t index, uint8_t v)
{
    unsigned long time = millis();
    if(time-counters[index].timestamp<600)
        return;
    //Serial.println("update_counter");
    counters[index].value += v;
    counters[index].timestamp = time;
    need_update_value=true;
};
bool WaterBoardCounter::send_result()
{
    Serial.println("try send result");
    if(WiFi.isConnected()==false || mqtt_client.connected()==false)
    {
        if(init_wifi()==false)
            return false;
    }
    String mqtt_device=config["mqtt"]["device"];
    DynamicJsonDocument root(256);
    for(int index=0;index<WBC_COUNTER_SIZE;index++)
    {
        String name("counter");
        name+=String(index);
        root[name]["value"]=counters[index].value * counters[index].value_per_count;
        root[name]["serial"]=counters[index].serial;
        root[name]["type"]=counters[index].value_type;
    }
    String payload;
    serializeJson(root, payload);
    String topic=mqtt_device+mqtt_topic_result;//"/result";
    mqtt_client.publish(topic.c_str(), payload.c_str(), true);
    topic = mqtt_device+mqtt_topic_status;//"/status";
    switch(status)
    {
    case WBC_STATUS_OK:
        payload="ok";
        break;
    case WBC_STATUS_VDROP:
        payload="low battery";
        break;
    }
    return mqtt_client.publish(topic.c_str(), payload.c_str(), true);
};
bool WaterBoardCounter::setup()
{
    Serial.begin(115200);
    delay(500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WBC_0_PIN, INPUT_PULLUP);
    pinMode(WBC_1_PIN, INPUT_PULLUP);
    pinMode(VDROP_PIN, INPUT);
#if WBC_COUNTER_SIZE == 4
    pinMode(WBC_2_PIN, INPUT_PULLUP);
    pinMode(WBC_3_PIN, INPUT_PULLUP);
#endif

    setup_button_down=false;

    attach_interrupt(INTERRUPT_ALL);

    // init file system
#ifdef ESP8266
    LittleFS.begin();
#else
    LittleFS.begin(true);
#endif
    if(LittleFS.exists(WBC_JSON)==false)
    {
        init_config();
        save_config();
    }
    else
    {
        read_config();
        counters[0].value=config["counter0"]["value"];
    }

    //mqtt_client.setClient(espClient);
    mqtt_client.setCallback(callback);

    if(config["mqtt"].containsKey("default"))
    {
        broker = wbc_createBroker(config["mqtt"]["default"], config["mqtt"]["server"], config["mqtt"]["port"]);
        broker->load(config);
    }

    String mqtt_server=config["mqtt"]["server"];
    uint16_t mqtt_port=config["mqtt"]["port"];
    mqtt_client.setServer(mqtt_server.c_str(), mqtt_port);
 
    //Cron.create("0 00 24 * * 6", WeeklyAlarm);

    before_time = millis();
    status = WBC_STATUS_OK;

    send_module_data=true;

    WiFi.onStationModeConnected(onStationWifiConnected);
    WiFi.onStationModeDisconnected(onStationWifiDisconnected);

    WiFi.begin();
    return true;
};
bool WaterBoardCounter::setup_wifi()
{
    manager = new WiFiManager();

    wifi_manager_params[0]=new WiFiManagerParameter("mqtt_select", "Select broker","", 40, "<select></select>");

    manager->setSaveConfigCallback(saveWifiManagerParam);

    const char *name = config["mqtt"]["device"];
    bool status=manager->startConfigPortal(name);

    delete manager;
    return status;
};
bool WaterBoardCounter::setup_wifi(const char *name, const char *server, const char *port, const char *mqtt_user, const char *mqtt_password)
{
    manager=new WiFiManager();

    wifi_manager_params[0]=new WiFiManagerParameter("mqtt_device", "mqtt device", name, 40);
    wifi_manager_params[1]=new WiFiManagerParameter("mqtt_server", "mqtt server", server, 40);
    wifi_manager_params[2]=new WiFiManagerParameter("mqtt_port", "mqtt port", port, 6);
    wifi_manager_params[3]=new WiFiManagerParameter("mqtt_user", "mqtt user", mqtt_user, 32);
    wifi_manager_params[4]=new WiFiManagerParameter("mqtt_pass", "mqtt password", mqtt_password, 32);
    wifi_manager_params[5]=new WiFiManagerParameter("counter_0_serial", "Counter 0 serial", counters[0].serial, 32);
    wifi_manager_params[6]=new WiFiManagerParameter("counter_0_value", "Counter 0 value", String(counters[0].value).c_str(), 32);
    wifi_manager_params[7]=new WiFiManagerParameter("counter_1_serial", "Counter 1 serial", counters[1].serial, 32);
    wifi_manager_params[8]=new WiFiManagerParameter("counter_1_value", "Counter 1 value", String(counters[1].value).c_str(), 32);
    for(int i=0;i<WIFI_PARAM_COUNT;i++)
        manager->addParameter(wifi_manager_params[i]);
    manager->setSaveConfigCallback(saveWifiManagerParam);
    const char *menu[] = {"wifi","wifinoscan","exit"};//, "wifinoscan", "info", "param"};
    manager->setMenu(menu,sizeof(menu)/sizeof(menu[0]));
    manager->setTitle("WBC configuration");
    bool status=manager->startConfigPortal(name);
    delete manager;
    for(int i=0;i<WIFI_PARAM_COUNT;i++)
        delete wifi_manager_params[i];
    Serial.printf("status:%d\n", status);
    return status;
};
void WaterBoardCounter::attach_interrupt(uint8_t pin)
{
    switch(pin)
    {
    case WBC_0_PIN:
        attachInterrupt(digitalPinToInterrupt(WBC_0_PIN), wbc_0_callback, FALLING);
        break;
    case WBC_1_PIN:
        attachInterrupt(digitalPinToInterrupt(WBC_1_PIN), wbc_1_callback, FALLING);
        break;
#if WBC_COUNTER_SIZE == 4
    case WBC_2_PIN:
        attachInterrupt(digitalPinToInterrupt(WBC_2_PIN), wbc_2_callback, FALLING);
        break;
    case WBC_3_PIN:
        attachInterrupt(digitalPinToInterrupt(WBC_2_PIN), wbc_3_callback, FALLING);
        break;
#endif
    case SETUP_BUTTON_PIN:
        attachInterrupt(digitalPinToInterrupt(SETUP_BUTTON_PIN), setup_callback, CHANGE);
        break;
    default:
        attachInterrupt(digitalPinToInterrupt(WBC_0_PIN), wbc_0_callback, FALLING);
        attachInterrupt(digitalPinToInterrupt(WBC_1_PIN), wbc_1_callback, FALLING);
#if WBC_COUNTER_SIZE == 4
        attachInterrupt(digitalPinToInterrupt(WBC_2_PIN), wbc_2_callback, FALLING);
        attachInterrupt(digitalPinToInterrupt(WBC_2_PIN), wbc_3_callback, FALLING);
#endif
        attachInterrupt(digitalPinToInterrupt(SETUP_BUTTON_PIN), setup_callback, CHANGE);
    }
};
void WaterBoardCounter::detach_interrupt()
{
    //detachInterrupt()
};
void WaterBoardCounter::setup_button()
{
    unsigned long time = millis();
    if(time-setup_button_time<100)
        return;
    //Serial.println("setup_button");
    setup_button_down=!setup_button_down;
    setup_button_time=time;
}
void WaterBoardCounter::connect_broker()
{
    broker->connect();
};
bool WaterBoardCounter::init_wifi()
{
    if(WiFi.isConnected())
    {
        broker->connect();

        String mqtt_device=config["mqtt"]["device"];
        String mqtt_user=config["mqtt"]["user"];
        String mqtt_passw=config["mqtt"]["passw"];
        Serial.printf("param:%s %s\n", mqtt_user.c_str(), mqtt_passw.c_str());

        timeClient.begin();
        if(mqtt_client.connect(mqtt_device.c_str(), mqtt_user.c_str(), mqtt_passw.c_str()))
        {
            Serial.println("mqtt connected");
            String topic=mqtt_device+mqtt_topic_set;//"/set";
            mqtt_client.subscribe(topic.c_str());

            if(send_module_data)
            {
                String topic=mqtt_device+mqtt_topic_firmware;
                mqtt_client.publish(topic.c_str(),WBC_VERSION,true);
                send_module_data=false;
            }
            return true;
        }
    }
    else
        WiFi.begin();
    return false;
};
bool WaterBoardCounter::is_setup_active(unsigned long current_time_ms)
{
    if(setup_button_down==false)
        return false;
    if(current_time_ms-setup_button_time<WAIT_SETUP_MS)
        return false;
    setup_button_down = false;
    return true;
};
void WaterBoardCounter::loop()
{
    unsigned long current = millis();

    broker->update(current);

    if(need_update_value && (current - before_time > 10000))
    {
        broker->publish(counters, WBC_COUNTER_SIZE);
        need_update_value = false;
        before_time = current;
    }

    if(mqtt_client.connected())
    {
        mqtt_client.loop();
    }
    if(WiFi.isConnected())
    {
        if(timeClient.update())
        {
            timeval tv = {timeClient.getEpochTime(), 0};
            settimeofday(&tv,0);
        }
    }
#if 0
    if(setup_button_down)
    {
        if(current-setup_button_time>WAIT_SETUP_MS)
        {
            Serial.println("button down 3 sec");

            String mqtt_device=config["mqtt"]["device"];
            String mqtt_server=config["mqtt"]["server"];
            uint16_t mqtt_port=config["mqtt"]["port"];
            String mqtt_user=config["mqtt"]["user"];
            String mqtt_passw=config["mqtt"]["passw"];
            Serial.println(mqtt_server);
            Serial.printf("port:%d\n",mqtt_port);
            Serial.println(mqtt_user);

            digitalWrite(LED_PIN, LOW);

            if(setup_wifi(mqtt_device.c_str(), mqtt_server.c_str(), String(mqtt_port).c_str(), mqtt_user.c_str(), mqtt_passw.c_str()))
            {
                save_config();
                Serial.printf("restart esp");
                ESP.restart();
            }
            setup_button_down=false;
        }
    }
#else
    if(is_setup_active(current))
    {
        digitalWrite(LED_PIN, LOW);

        if(setup_wifi())
        {
            save_config();
        }

        digitalWrite(LED_PIN, HIGH);
    }
#endif
    else
    {
        bool need_sleep=true;
        for(int i=0;i<WBC_COUNTER_SIZE;i++)
        {
            if(current-counters[i].timestamp<WAIT_SLEEP_MS)
            {
                need_sleep=false;
                break;
            }
        }
        if(need_sleep)
        {
#if 0
            if(mqtt_client.connected())
            {
                mqtt_client.disconnect();
            }
            else
#else
            if(broker->disconnect())
#endif
            {
                if(WiFi.isConnected())
                {
                    //WiFi.disconnect(true, false);
                    WiFi.mode(WIFI_OFF);
                }
                else
                {
                    save_config();
                    digitalWrite(LED_PIN, HIGH);  // turn the LED off so they know the CPU isn't running
                    light_sleep();
                    before_time=millis();
                }
            }
        }
    }
};
void WaterBoardCounter::light_sleep()
{
    Serial.println("Light sleep enter");
#ifdef ESP8266
    //wifi_station_disconnect();
    wifi_set_opmode_current(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); // set sleep type, the above    posters wifi_set_sleep_type() didnt seem to work for me although it did let me compile and upload with no errors
    wifi_fpm_open();// Enables force sleep
    gpio_pin_wakeup_enable(SETUP_BUTTON_PIN, GPIO_PIN_INTR_LOLEVEL);
    gpio_pin_wakeup_enable(WBC_0_PIN, GPIO_PIN_INTR_LOLEVEL);
    gpio_pin_wakeup_enable(WBC_1_PIN, GPIO_PIN_INTR_LOLEVEL);
    wifi_fpm_do_sleep(0xFFFFFFF); // Sleep for longest possible time
    delay(10);
#else
    //esp_sleep_enable_ext1_wakeup()
    esp_light_sleep_start();
#endif
    Serial.println("Light sleep leave");
}
void WaterBoardCounter::init_config()
{
#ifdef ESP8266
    String mqtt_device = "WBC_" + String(ESP.getChipId());
#else
    String mqtt_device = "WBC_" + String(ESP.getEfuseMac());
#endif
    config["mqtt"]["enable"] = false;
    config["mqtt"]["default"] = "custom";

    config["mqtt"]["device"]=mqtt_device;
    config["mqtt"]["server"]="mqtt.dealgate.ru";
    config["mqtt"]["port"]=1883;
    config["mqtt"]["user"]="";
    config["mqtt"]["passw"]="";

    for(int i=0;i<WBC_COUNTER_SIZE;i++)
    {
        char name[16];
        sprintf(name,"counter%d",i);
        config[name]["value"]=counters[i].value;
        config[name]["value_per_count"]=counters[i].value_per_count;
    }
};
void WaterBoardCounter::read_config()
{
    fs::File file = LittleFS.open(WBC_JSON,"r");
    deserializeJson(config, file);
    file.close();
    serializeJsonPretty(config, Serial);
};
void WaterBoardCounter::save_config()
{
    fs::File file = LittleFS.open(WBC_JSON,"w");
    serializeJson(config, file);
    file.close();
};
void WaterBoardCounter::configure_mqtt_home_assistant()
{
    std::string discovery_prefix("homeassistant");
    std::string component("sensor");
    std::string object_id("WBC");

    std::string topic=discovery_prefix+"/"+component+"/"+object_id;
    std::string config_topic=discovery_prefix+"/"+component+"/"+object_id+"/config";
    std::string available_topic=discovery_prefix+"/"+component+"/"+object_id+"/available";
    DynamicJsonDocument payload(256);
    payload["device_class"]="volume";
    payload["state_topic"]=topic+"/state";
    payload["unit_of_measurement"]="m3";
    payload["value_template"]="{{ value_json.counter0}}";
    payload["unique_id"]="wbc_c0";
    payload["device"]["identifiers"]="Water Counter";
    payload["device"]["name"]="Water Counter";
    payload["device"]["manufacturer"]="DIY Board";
    payload["device"]["model"]="WBC";
    payload["device"]["serial_number"]="unknown";
    payload["device"]["hw_version"]="1.0";
    payload["device"]["sw_version"]="1.0";

    String payload_str;
    serializeJson(payload, payload_str);
    config_topic = topic + "/c0/config";
    mqtt_client.publish(config_topic.c_str(), payload_str.c_str(), true);

    config_topic = topic + "/c1/config";
    payload["value_template"]="{{ value_json.counter1}}";
    payload["unique_id"]="wbc_c1";
    serializeJson(payload, payload_str);
    mqtt_client.publish(config_topic.c_str(), payload_str.c_str(), true);


    mqtt_client.publish(available_topic.c_str(), "online");
};