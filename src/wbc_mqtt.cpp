#include"wbc.h"
#include<string>
#include <ArduinoJson.h>

WiFiClient espClient;

WBCMqttBroker *wbc_createBroker(const char *name, const char *server, unsigned short port)
{
    if(strcmp(name,"dealgate")==0)
        return new WBCMqttDealgate();
    if(strcmp(name,"homeassistant")==0)
        return new WBCMqttHomeAssistant(server, port);
    return new WBCMqttCustom(server, port);
};

WBCMqttBroker::WBCMqttBroker(const char *_server, unsigned short _port)
{
    is_first_time=true;
    server=_server;
    port=_port;
    client.setServer(server.c_str(), port);
    client.setClient(espClient);
};
bool WBCMqttBroker::connect()
{
    if(client.connect(object_id.c_str(), login.c_str(), password.c_str()))
    {
        config();
        return true;
    }
    return false;
};
bool WBCMqttBroker::disconnect()
{
    if(client.connected())
    {
        client.disconnect();
        return false;
    }
    return true;
}
void WBCMqttBroker::publish(Counter *counters, unsigned char count)
{
    std::string payload, topic=wbc_mqtt_make_topic(discovery_prefix,component,object_id);
    DynamicJsonDocument doc(256);
    for(unsigned char index=0;index<count;index++)
    {
        char key[3];
        sprintf(key,"c%hhu",index);
        doc[key]=counters[index].value;
    }
    serializeJson(doc, payload);
    client.publish(topic.c_str(), payload.c_str(), true);
};
void WBCMqttBroker::update(unsigned long current_time_ms)
{
    if(client.connected())
    {
        client.loop();
    }
    else
    {
        if(current_time_ms-time_ms>wbc_mqtt_reconnect_timeout)
        {
            connect();
            current_time_ms=time_ms;
        }
    }
};
void WBCMqttBroker::load_common(DynamicJsonDocument &doc)
{
    JsonObject obj = doc["mqtt"];
    discovery_prefix = obj["discovery_prefix"].as<std::string>();
    component = obj["component"].as<std::string>();
    object_id = obj["object_id"].as<std::string>();
};
void WBCMqttBroker::save_common(DynamicJsonDocument &doc)
{
    doc["mqtt"]["discovery_prefix"] = discovery_prefix;
    doc["mqtt"]["component"] = component;
    doc["mqtt"]["object_id"] = object_id;  
};
void WBCMqttBroker::config_common()
{
    std::string topic = wbc_mqtt_make_topic(discovery_prefix, component, object_id);
    std::string topic_set=topic+wbc_mqtt_topic_set_prefix;
    client.subscribe(topic_set.c_str());
};

WBCMqttHomeAssistant::WBCMqttHomeAssistant(const char *server, unsigned short port):WBCMqttBroker(server, port)
{
    discovery_prefix="homeassistant";
    component="sensor";
    object_id="WBC";
};
void WBCMqttHomeAssistant::load(DynamicJsonDocument &doc)
{
    load_common(doc);
};
void WBCMqttHomeAssistant::save(DynamicJsonDocument &doc)
{
    save_common(doc);
};
void WBCMqttHomeAssistant::config()
{
    std::string topic=wbc_mqtt_make_topic(discovery_prefix,component,object_id);
    std::string available_topic=topic+wbc_mqtt_topic_available_prefix;
    if(is_first_time)
    {
        // on first connect send configuration.
        std::string config_topic=topic+"/config";
        DynamicJsonDocument payload(256);
        payload["device_class"]="volume";
        payload["state_topic"]=topic+"/state";
        payload["unit_of_measurement"]="m3";
        payload["value_template"]="{{ value_json.counter0}}";
        payload["unique_id"]="wbc_c0";
        payload["device"]["identifiers"]="Water Counter";
        payload["device"]["name"]=WBC_NAME;
        payload["device"]["manufacturer"]="DIY Board";
        payload["device"]["model"]="WBC";
        payload["device"]["serial_number"]="unknown";
        payload["device"]["hw_version"]="1.0";
        payload["device"]["sw_version"]=WBC_VERSION;

        String payload_str;
        serializeJson(payload, payload_str);
        config_topic = topic + "/c0/config";
        client.publish(config_topic.c_str(), payload_str.c_str(), true);

        config_topic = topic + "/c1/config";
        payload["value_template"]="{{ value_json.counter1}}";
        payload["unique_id"]="wbc_c1";
        serializeJson(payload, payload_str);
        client.publish(config_topic.c_str(), payload_str.c_str(), true);
    }

    client.publish(available_topic.c_str(), "online", true);

    config_common();
}

WBCMqttDealgate::WBCMqttDealgate():WBCMqttBroker("mqtt.dealgate.ru", 1883)
{
};
void WBCMqttDealgate::load(DynamicJsonDocument &doc)
{
    load_common(doc);
}
void WBCMqttDealgate::save(DynamicJsonDocument &doc)
{
    save_common(doc);
};
void WBCMqttDealgate::config()
{
    std::string available_topic=wbc_mqtt_make_topic(discovery_prefix,component,object_id)+wbc_mqtt_topic_available_prefix;
    client.publish(available_topic.c_str(), "online", true);

    config_common();
}

WBCMqttCustom::WBCMqttCustom(const char *server, unsigned short port):WBCMqttBroker(server, port)
{

}
void WBCMqttCustom::config()
{
    config_common();
}