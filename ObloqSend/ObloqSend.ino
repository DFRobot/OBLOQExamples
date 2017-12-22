#include <Arduino.h>
#include <SoftwareSerial.h>


#define WIFISSID "DFRobot-guest" 
#define WIFIPWD  "dfrobot@2017" 
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
		else if (strstr(obloqMessage,"|2|3|") != NULL && strlen(obloqMessage) != 9)
		{
			Serial.println("Wifi ready");
			obloqState = WIFIOK;
		}
		else if (strcmp(obloqMessage, "|4|1|1|1|") == 0)
		{
			Serial.println("Mqtt ready");
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
        case MQTTCONNECTOK : obloqConnectMqtt = true; obloqState = WAIT; break;
        default: break;
    }
}

/********************************************************************************************
Function    : sendMessage(重载)      
Description : 间隔一段时间（interval），向DF-IoT物联网指定设备(topic)发送消息(message)
Params      : topic DF-IoT设备编号；message 发送的消息内容；interval 间隔时间(ms)
Return      : 无 
********************************************************************************************/
void sendMessage(String topic,String message,unsigned long interval = 0)
{
    if(obloqConnectMqtt)
    {
        if (interval == 0)
            publish(topic,message);
        else
        {
            if(millis() - previousSendMessageTime > interval)
            {
                previousSendMessageTime = millis();
                publish(topic,message); 
                Serial.println(message);
            }
        }
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
    float temperature = getTemp();
    sendMessage("Hy6z0Pb1G",(String)temperature,5000);//每隔5s，向Hy6z0Pb1G设备发送消息，消息内容是此时读取的LM35温度数据
    handleUart();
}

