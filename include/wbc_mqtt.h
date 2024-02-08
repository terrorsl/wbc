#ifndef WBC_MQTT_FILE
#define WBC_MQTT_FILE

#include<ArduinoJson.h>
#include <PubSubClient.h>
#include<string>

#define WBC_DEFAULT_KEEP_ALIVE 60000

#define wbc_mqtt_send_interval 10000
#define wbc_mqtt_reconnect_timeout 10000
#define wbc_mqtt_topic_set_prefix "/set"
#define wbc_mqtt_topic_state_prefix "/state"
#define wbc_mqtt_topic_available_prefix "/available"

#define wbc_mqtt_make_topic(discovery,component,object) discovery+"/"+component+"/"+object

class WBCMqttBroker
{
public:
    WBCMqttBroker(const char *_server, unsigned short _port);
    
    void setServer(const char _server){server=server;}
    void setPort(unsigned short _port){port=_port;}

    void publish(Counter *counters, unsigned char count);

    bool connect();
    bool disconnect();
    void update(unsigned long current_time_ms);
    
    virtual void config()=0;
    //virtual void default_config(DynamicJsonDocument &doc)=0;
    virtual void load(DynamicJsonDocument &doc)=0;
    virtual void save(DynamicJsonDocument &doc)=0;
protected:
    void load_common(DynamicJsonDocument &doc);
    void save_common(DynamicJsonDocument &doc);
    void config_common();

    bool is_first_time;
    unsigned long time_ms;

    std::string server, login, password;
    unsigned short port;

    std::string discovery_prefix, component, object_id;

    PubSubClient client;
};

class WBCMqttDealgate:public WBCMqttBroker
{
public:
    WBCMqttDealgate();
    void config();
    //void default_config(DynamicJsonDocument &doc);
    void load(DynamicJsonDocument &doc);
    void save(DynamicJsonDocument &doc);
};
class WBCMqttHomeAssistant:public WBCMqttBroker
{
public:
    WBCMqttHomeAssistant(const char *server, unsigned short port);
    void config();
    //void default_config(DynamicJsonDocument &doc);
    void load(DynamicJsonDocument &doc);
    void save(DynamicJsonDocument &doc);
private:
    std::string discovery_prefix, component, object_id;
};
class WBCMqttCustom:public WBCMqttBroker
{
public:
    WBCMqttCustom(const char *server, unsigned short port);
    void config();
    //void default_config(DynamicJsonDocument &doc){}
    void load(DynamicJsonDocument &doc){}
    void save(DynamicJsonDocument &doc){};
};

WBCMqttBroker *wbc_createBroker(const char *name, const char *server, unsigned short port);
#endif