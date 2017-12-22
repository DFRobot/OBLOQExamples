#include <Arduino.h>
#include <SoftwareSerial.h>

#define WIFISSID "DFRobot-guest" 
#define WIFIPWD  "dfrobot@2017" 
#define SERVER   "iot.dfrobot.com.cn"
#define IOTID    "r1qHJFJ4Z"
#define IOTPWD   "SylqH1Y1VZ"
#define PORT     1883


const String separator = "|";
bool pingOn = true;
bool obloqConnectMqtt = false;
bool subscribeMqttTopic = false;
static unsigned long pingInterval = 2000;
unsigned long previousPingTime = 0;
unsigned long previousSendMessageTime = 0;
bool subscribeSuccess = false;
String receiveStringIndex[10] = {};

SoftwareSerial softSerial(10,11);

//存放监听设备的topic数组，将需要监听的topic放到数组里，最多5个Topic
String topicStore[] = {"Hy6z0Pb1G","S1pMPGNRb","rydv8Kv-G"};

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
Function    : subscribe      
Description : 订阅DF-IoT物联网设备     
Params      : topic DF-IoT物联网设备编号
Return      : 无 
********************************************************************************************/
void subscribe(String topic)
{
    String subscribeMessage = "|4|1|2|" + topic + separator;
    sendMessage(subscribeMessage);
    Serial.println(subscribeMessage);
}

/********************************************************************************************
Function    : splitString      
Description : 剔除分隔符，逐一提取字符串     
Params      : data[] 提取的字符串的目标储存地址；str 源字符串；delimiters 分隔符
Return      : 共提取的字符串的个数 
********************************************************************************************/
int splitString(String data[],String str,const char* delimiters)
{
  char *s = (char *)(str.c_str());
  int count = 0;
  data[count] = strtok(s, delimiters);
  while(data[count]){
    data[++count] = strtok(NULL, delimiters);
  }
  return count;
}

/********************************************************************************************
Function    : receiveMessageCallbak      
Description : 接收消息的回调函数  
Params      : topic 发出消息的DF-IoT物联网设备编号；message 收到的消息内容
Return      : 无 
********************************************************************************************/
void receiveMessageCallbak(String topic,String message)
{
    Serial.println("Message from: " + topic);
    Serial.println("Message content: " + message);
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
        Serial.print("receivedata = ");
        Serial.println(receivedata);
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
        else if (strcmp(obloqMessage, "|4|1|2|1|") == 0)
		{
			subscribeMqttTopic = false;
        }
        else if (strstr(obloqMessage, "|4|1|5|") != nullptr)
        {
            splitString(receiveStringIndex,receivedata,"|");
            receiveMessageCallbak(receiveStringIndex[3],receiveStringIndex[4]);
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
Function    : subscribeTopic      
Description : 需要监听多个DF-IoT物联网设备时，运行一次注册一个设备 
Params      : 无
Return      : 无 
********************************************************************************************/
void subscribeTopic()
{
    static int num = 1 ;
    if(!subscribeSuccess && !subscribeMqttTopic && obloqConnectMqtt)
    {
        subscribeMqttTopic = true;
        switch(num)
        {
            //subscribe()内是监听的设备的topic
            case 1:subscribe("Hy6z0Pb1G");num++;break;
            case 2:subscribe("S1pMPGNRb");num++;break;
            case 3:subscribe("rydv8Kv-G");num++;break;
            case 4:subscribe("SycPItDZG");num++;break;
            case 5:subscribe("H1xdUtPZG");num++; subscribeSuccess = true;break;
            default:break;
        }
    }    
}


/********************************************************************************************
Function    : subscribeMultipleTopic      
Description : 监听多个DF-IoT物联网设备，本例中注册了5个设备 
Params      : 无
Return      : 无 
********************************************************************************************/
void subscribeMultipleTopic()
{
    static int num = 0 ;
    //如果设备没有全部完成注册，subscribeSuccess == false
    if(!subscribeSuccess && !subscribeMqttTopic && obloqConnectMqtt)
    {
        subscribeMqttTopic = true;
        int length = sizeof(topicStore)/sizeof(String);//获得topicStore数组的大小
        if(num < length)
        {
            subscribe(topicStore[num]);
            num++;
        }
        //当完成了全部设备注册后，subscribeSuccess置为true
        else
        {
            subscribeSuccess = true;
        }
    }    
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
    //监听多个Iot设备，需要将监听的多个topic放入数组topicStore[]中，一个OBLOQ最多监听5个Topic
    subscribeMultipleTopic();
    handleUart();
}
