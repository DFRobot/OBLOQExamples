#include <Arduino.h>
#include <SoftwareSerial.h>


// #define WIFISSID "DFRobot-guest" 
// #define WIFIPWD  "dfrobot@2017" 

#define WIFISSID "Smartisan" 
#define WIFIPWD  "123456789"

#define SERVER   "iot.dfrobot.com.cn"
#define PORT     1883
#define IOTID    "r1qHJFJ4Z"
#define IOTPWD   "SylqH1Y1VZ"


const String separator = "|";
bool pingOn = true;
bool obloqConnectMqtt = false;
static unsigned long pingInterval = 2000;
static unsigned long sendMessageInterval = 10000;
unsigned long previousPingTime = 0;
unsigned long previousSendMessageTime = 0;

//wifi异常断开检测变量
bool wifiConnect = false;
bool wifiAbnormalDisconnect = false;

//mqtt因为网络断开后重新连接标志
bool mqttReconnectFlag = false;

SoftwareSerial softSerial(10,11);

enum state{
    WAIT,
    PINGOK,
    WIFIOK,
    MQTTCONNECTOK
}obloqState;

/********************************************************************************************
Function    : sendMessage      
Description : 通过串口向OBLOQ发送一次消息
Params      : 无
Return      : 无 
********************************************************************************************/
void sendMessage(String message)
{
    softSerial.print(message+"\r"); 
}

/********************************************************************************************
Function    : ping      
Description : 通过串口向OBLOQ发送一次字符串"|1|1|"，尝试与OBLOQ取得连接
Params      : 无
Return      : 无 
********************************************************************************************/
void ping()
{
    String pingMessage = "|1|1|";
    sendMessage(pingMessage);
}

/********************************************************************************************
Function    : connectWifi      
Description : 连接wifi   
Params      : ssid 连接wifi的ssid；pwd 连接wifi的password
Return      : 无 
********************************************************************************************/
void connectWifi(String ssid,String pwd)
{
    String wifiMessage = "|2|1|" + ssid + "," + pwd + separator;
    sendMessage(wifiMessage);
} 

/********************************************************************************************
Function    : connectMqtt      
Description : 连接DF-IoT    
Params      : server 物联网网址；port 端口；iotid 物联网登录后分配的iotid；iotpwd 物联网登录后分配的iotpwd
Return      : 无 
********************************************************************************************/
void connectMqtt(String server, String port, String iotid, String iotpwd)
{
    String mqttConnectMessage = "|4|1|1|" + server + separator + port + separator + iotid + separator + iotpwd + separator;
    sendMessage(mqttConnectMessage);
}


/********************************************************************************************
Function    : reconnectMqtt      
Description : 重新连接DF-IoT    
Params      : 无 
Return      : 无 
********************************************************************************************/
void reconnectMqtt()
{
    String mqttReconnectMessage = "|4|1|5|"; 
    sendMessage(mqttReconnectMessage);
}


/********************************************************************************************
Function    : publish      
Description : 向DF-IoT物联网设备发送信息    
Params      : topic DF-IoT物联网设备编号；message 发送的消息内容
Return      : 无 
********************************************************************************************/
void publish(String topic,String message)
{
    String publishMessage = "|4|1|3|" + topic + separator + message + separator;
    sendMessage(publishMessage);
}

/********************************************************************************************
Function    : handleUart      
Description : 处理串口传回的数据      
Params      : 无   
Return      : 无 
********************************************************************************************/
void handleUart()
{
    while(softSerial.available() > 0)
    {
        String receivedata = softSerial.readStringUntil('\r');
        const char* obloqMessage = receivedata.c_str();
        if (strcmp(obloqMessage, "|1|1|") == 0)
		{
			Serial.println("Pong");
			pingOn = false;
			obloqState = PINGOK;
		}
        if(strcmp(obloqMessage, "|2|1|") == 0)
        {
            if(wifiConnect)
            {
                wifiConnect = false;
                wifiAbnormalDisconnect = true;
            }
        }
		else if (strstr(obloqMessage,"|2|3|") != NULL && strlen(obloqMessage) != 9)
		{
			Serial.println("Wifi ready");
            wifiConnect = true;
            if(wifiAbnormalDisconnect)
            {
                wifiAbnormalDisconnect = false;
                return;
            }
			obloqState = WIFIOK;
		}
		else if (strcmp(obloqMessage, "|4|1|1|1|") == 0)
		{
			Serial.println("Mqtt ready");
            obloqConnectMqtt = true;
            if(mqttReconnectFlag)
            {
                mqttReconnectFlag = false;
                return;
            }
            obloqState = MQTTCONNECTOK;
		}
    }
}

/********************************************************************************************
Function    : sendPing      
Description : 每隔pingInterval一段时间，通过串口向OBLOQ ping一次      
Params      : 无   
Return      : 无 
********************************************************************************************/
void sendPing()
{
    if(pingOn && millis() - previousPingTime > pingInterval)
    {
        previousPingTime = millis();
        ping();
    }
}

/********************************************************************************************
Function    : execute      
Description : 根据OBLOQ的状态，进行下一步相应操作。      
Params      : 无
Return      : 无 
********************************************************************************************/
void execute()
{
    switch(obloqState)
    {
        case PINGOK: connectWifi(WIFISSID,WIFIPWD); obloqState = WAIT; break;
        case WIFIOK: connectMqtt(SERVER,String(PORT),IOTID,IOTPWD);obloqState = WAIT; break;
        case MQTTCONNECTOK : obloqState = WAIT; break;
        default: break;
    }
}

/********************************************************************************************
Function    : checkWifiState      
Description : 检查wifi状态,如果检测到wifi热点断开会间隔1分钟去重新连接wifi
Params      : 无
Return      : 无
********************************************************************************************/
void checkWifiState()
{
    static unsigned long previousTime = 0;
    static bool reconnectWifi = false;
    if(wifiAbnormalDisconnect && millis() - previousTime > 60000)
    {
        previousTime = millis();
        Serial.println("Wifi abnormal disconnect");
        reconnectWifi = true;
        obloqConnectMqtt = false;
        connectWifi(WIFISSID,WIFIPWD);
    }
    if(!wifiAbnormalDisconnect && reconnectWifi)
    {
        reconnectWifi = false;
        mqttReconnectFlag = true;
        Serial.println("Reconnect mqtt");
        reconnectMqtt();
    }

}

/********************************************************************************************
Function    : getTemp      
Description : 获得LM35测得的温度
Params      : 无
Return      : 无 
********************************************************************************************/
float getTemp()
{
    uint16_t val;
    float dat;
    val=analogRead(A0);//Connect LM35 on Analog 0
    dat = (float) val * (5/10.24);
    return dat;
}

void setup()
{
    Serial.begin(9600);
    softSerial.begin(9600);    
}

void loop()
{
    sendPing();
    execute();
    handleUart();
    checkWifiState();

    if(obloqConnectMqtt && millis() - previousSendMessageTime > 5000) //每隔5s，向Hy6z0Pb1G设备发送消息，消息内容是此时读取的LM35温度数据
    {
        previousSendMessageTime = millis();
        //获取温度数据
        float temperature = getTemp();
        Serial.println(temperature);
        publish("Hy6z0Pb1G",(String)temperature); 
        
    }
}
