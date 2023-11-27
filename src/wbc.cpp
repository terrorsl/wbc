#include"wbc.h"
#include<LittleFS.h>
#include"SPIFFS_ini.h"
#include <ArduinoJson.h>

WiFiClient espClient;
extern WaterBoardCounter wbc;

void callback(char* topic, byte* payload, unsigned int length)
{

};
void preSaveWifiManagerParam()
{
    WiFiManager *manager=wbc.get_wifi_manager();
    WiFiManagerParameter **params=manager->getParameters();
    for(int index=0;index<manager->getParametersCount();index++)
    {
        //params[index].
    }
}
void saveWifiManagerParam()
{
    Serial.println("saveWifiManagerParam");
    WiFiManager *manager=wbc.get_wifi_manager();
    WiFiManagerParameter **params=manager->getParameters();
    ini_write("mqtt", "device", params[0]->getValue());
    ini_write("mqtt", "server", params[1]->getValue());
    ini_write("mqtt", "port", params[2]->getValue());
    ini_write("mqtt", "user", params[3]->getValue());
    ini_write("mqtt", "passw", params[4]->getValue());

    ini_write("counter_0", "serial", params[5]->getValue());
    ini_write("counter_0", "value", params[6]->getValue());
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
    counters[index].value += v;
    counters[index].timestamp = millis();
};
void WaterBoardCounter::send_result()
{
    Serial.println("try send result");
    if(WiFi.isConnected()==false || mqtt_client.connected()==false)
    {
        String mqtt_server=ini_read("mqtt", "server", "mqtt.dealgate.ru");
        uint16_t mqtt_port=ini_read("mqtt", "port", String(1883)).toInt();
        String mqtt_user=ini_read("mqtt", "user", "terror");
        String mqtt_passw=ini_read("mqtt", "passw", "terror_23011985");
        Serial.printf("param:%s %s %s\n", mqtt_server.c_str(), mqtt_user.c_str(), mqtt_passw.c_str());

        if(init_wifi(mqtt_server.c_str(), mqtt_port, mqtt_user.c_str(), mqtt_passw.c_str())==false)
            return;
    }
    String mqtt_device=ini_read("mqtt", "device", "WBC_test");
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
    String topic=mqtt_device+"/result";
    mqtt_client.publish(topic.c_str(), payload.c_str(), true);
    topic = mqtt_device+"/status";
    payload="ok";
    mqtt_client.publish(topic.c_str(), payload.c_str(), true);
};
bool WaterBoardCounter::setup()
{
    Serial.begin(115200);
    delay(500);

    pinMode(LED_PIN, OUTPUT);
    pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WBC_0_PIN, INPUT_PULLUP);
    pinMode(WBC_1_PIN, INPUT_PULLUP);
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
    if(LittleFS.exists(WBC_INI)==false)
    {
        Serial.println("New config");
#ifdef ESP8266
        fs::File ini = LittleFS.open(WBC_INI,"w");
#else
        fs::File ini = LittleFS.open(WBC_INI,"w", true);
#endif
        ini.close();
        String mqtt_server("mqtt.dealgate.ru"), mqtt_user, mqtt_passw;
        uint16_t mqtt_port=1883;
        if(ini_open(WBC_INI))
        {
    #ifdef ESP8266
            String mqtt_device = "WBC_" + String(ESP.getChipId());
    #else
            String mqtt_device = "WBC_" + String(ESP.getEfuseMac());
    #endif
            ini_write("mqtt", "device", mqtt_device);
            ini_write("mqtt", "server", mqtt_server);
            ini_write("mqtt", "port", String(mqtt_port));
            ini_write("mqtt", "user", mqtt_user);
            ini_write("mqtt", "passw", mqtt_passw);
        }
    }
    else
    {
        ini_open(WBC_INI);
        Serial.println(ini_read("mqtt", "port", "0"));
        counters[0].value = ini_read("counter_0", "value", "0").toInt();
        strcpy(counters[0].serial, ini_read("counter_0", "serial", "").c_str());
    }

    mqtt_client.setClient(espClient);
    mqtt_client.setCallback(callback);
 
    before_time = millis();
    return true;
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
    for(int i=0;i<7;i++)
        manager->addParameter(wifi_manager_params[i]);
    manager->setSaveConfigCallback(saveWifiManagerParam);
    const char *menu[] = {"wifi","wifinoscan","exit"};//, "wifinoscan", "info", "param"};
    manager->setMenu(menu,sizeof(menu)/sizeof(menu[0]));
    manager->setTitle("WBC configuration");
    bool status=manager->startConfigPortal(name);
    delete manager;
    for(int i=0;i<7;i++)
        delete wifi_manager_params[i];
    Serial.printf("status:%d\n", status);
    return status;
    /*WiFiManager manager;

    WiFiManagerParameter mqtt_server_param("mqtt_server", "mqtt server", server, 40);
    WiFiManagerParameter mqtt_server_port_param("mqtt_port", "mqtt port", port, 6);

    WiFiManagerParameter mqtt_server_user_param("mqtt_user", "mqtt user", mqtt_user, 32);
    WiFiManagerParameter mqtt_server_pass_param("mqtt_pass", "mqtt password", mqtt_password, 32);

    WiFiManagerParameter counter_0_serial_param("counter_0_serial", "Counter 0 serial", counters[0].serial, 32);
    WiFiManagerParameter counter_0_value_param("counter_0_value", "Counter 0 value", String(counters[0].value).c_str(), 32);

    manager.addParameter(&mqtt_server_param);
    manager.addParameter(&mqtt_server_port_param);
    manager.addParameter(&mqtt_server_user_param);
    manager.addParameter(&mqtt_server_pass_param);

    manager.addParameter(&counter_0_serial_param);
    manager.addParameter(&counter_0_value_param);

    //manager.setSaveConfigCallback(saveWifiManagerParam);
    //manager.setPreSaveParamsCallback(preSaveWifiManagerParam);
    //manager.setSaveParamsCallback
    const char *menu[] = {"wifi","wifinoscan","exit"};//, "wifinoscan", "info", "param"};
    manager.setMenu(menu,sizeof(menu)/sizeof(menu[0]));
    manager.setTitle("WBC configuration");

    if(manager.startConfigPortal(name)==true)
    {
        Serial.println("save param");
        ini_write("mqtt", "server", mqtt_server_param.getValue());
        ini_write("mqtt", "port", mqtt_server_port_param.getValue());
        ini_write("mqtt", "user", mqtt_server_user_param.getValue());
        ini_write("mqtt", "passw", mqtt_server_pass_param.getValue());

        ini_write("counter_0", "serial", counter_0_serial_param.getValue());
        ini_write("counter_0", "value", counter_0_value_param.getValue());
        return true;
    }*/
    return false;
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
    setup_button_down=!setup_button_down;
    setup_button_time=millis();
}
bool WaterBoardCounter::init_wifi(const char *mqtt_server, uint16_t mqtt_port, const char *mqtt_user, const char *mqtt_password)
{
    WiFi.begin();
    delay(500);
    if(WiFi.isConnected())
    {
        mqtt_client.setServer(mqtt_server, mqtt_port);
        String mqtt_device=ini_read("mqtt", "device", "WBC_test");
        if(mqtt_client.connect(mqtt_device.c_str(), mqtt_user, mqtt_password))
        {
            String topic=mqtt_device+"/set";
            mqtt_client.subscribe(topic.c_str());
            return true;
        }
    }
    return false;
};
void WaterBoardCounter::loop()
{
    unsigned long current = millis();
    if(current - before_time > 5000)
    {
        send_result();
        before_time = current;
    }

    if(mqtt_client.connected())
    {
        mqtt_client.loop();
    }

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
        //light_sleep();
    }
    if(setup_button_down)
    {
        if(current-setup_button_time>WAIT_SETUP_MS)
        {
            Serial.println("button down 3 sec");

#ifdef ESP8266
            String mqtt_device = "WBC_" + String(ESP.getChipId());
#else
            String mqtt_device = "WBC_" + String(ESP.getEfuseMac());
#endif
            mqtt_device=ini_read("mqtt", "device", mqtt_device);
            String mqtt_server=ini_read("mqtt", "server", "mqtt.dealgate.ru");
            uint16_t mqtt_port=ini_read("mqtt", "port", "1883").toInt();
            String mqtt_user=ini_read("mqtt", "user", "");
            String mqtt_passw=ini_read("mqtt", "passw", "");
            Serial.println(mqtt_server);
            Serial.printf("port:\n",mqtt_port);
            Serial.println(mqtt_user);

            digitalWrite(LED_PIN, HIGH);

            if(setup_wifi(mqtt_device.c_str(), mqtt_server.c_str(), String(mqtt_port).c_str(), mqtt_user.c_str(), mqtt_passw.c_str()))
            {
                ini_close();
                Serial.printf("restart esp");
                ESP.restart();
            }
            setup_button_down=false;
        }
    }
};
void WaterBoardCounter::light_sleep()
{
#ifdef ESP8266
    wifi_station_disconnect();
    wifi_set_opmode_current(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); // set sleep type, the above    posters wifi_set_sleep_type() didnt seem to work for me although it did let me compile and upload with no errors
    wifi_fpm_open();// Enables force sleep
    gpio_pin_wakeup_enable(SETUP_BUTTON_PIN, GPIO_PIN_INTR_LOLEVEL);
    wifi_fpm_do_sleep(0xFFFFFFF); // Sleep for longest possible time
#else
    //esp_sleep_enable_ext1_wakeup()
    esp_light_sleep_start();
#endif
}