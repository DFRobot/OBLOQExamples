#include <Arduino.h>
#include <SoftwareSerial.h>


#define WIFISSID "DFRobot-guest" 
#define WIFIPWD  "dfrobot@2017" 

//Azure IOT 设备连接字符串,连接不同的设备需要修改这个字符串
const String connectionString = "HostName=iothubDF.azure-devices.net;DeviceId=FactoryDevice;SharedAccessKey=NHR4DHP5lmyrZ750E8BIyLmnFrs5Hq0NKRx3siR1zp8=";
const String separator = "|";
bool pingOn = true;
bool createIoTClientSuccess = false;
static unsigned long pingInterval = 2000;
static unsigned long sendMessageInterval = 10000;
unsigned long previousPingTime = 0;
String receiveStringIndex[10] = {};
bool subceribeIotDevice = false;

SoftwareSerial softSerial(10,11);

enum state{
    WAIT,
    PINGOK,
    WIFIOK,
    AZURECONNECT
}obloqState;

/********************************************************************************************
Function    : sendMessage
Description : 串口发送消息      
Params      : message 设备连接字符串   
Return      : 无 
********************************************************************************************/
void sendMessage(String message)
{
    softSerial.print(message+"\r"); 
}

/********************************************************************************************
Function    : ping      
Description : 检查串口是否正常通信，串口接收到敲门命令会返回：|1|1|\r
Return      : 无 
********************************************************************************************/
void ping()
{
    String pingMessage = F("|1|1|");
    sendMessage(pingMessage);
}

/********************************************************************************************
Function    : connectWifi 
Description : 连接wifi,连接成功串口返回：|2|3|ip|\r 
Params      : ssid  wifi账号 
Params      : pwd   wifi密码 
Return      : 无 
********************************************************************************************/
void connectWifi(String ssid,String pwd)
{
    String wifiMessage = "|2|1|" + ssid + "," + pwd + separator;
    sendMessage(wifiMessage);
} 

/********************************************************************************************
Function    : createIoTClient      
Description : 创建设备凭据，创建成功串口返回：|4|2|1|1|\r
Params      : connecttionString  设备连接字符串
Return      : 无 
********************************************************************************************/
void createIoTClient(String connecttionString)
{
    String azureConnectMessage = "|4|2|1|" + connecttionString + separator;
    sendMessage(azureConnectMessage);
}

/********************************************************************************************
Function    : subscribeDevice      
Description : 监听设备，接收到消息会返回消息内容,
Params      : 无 
Return      : 无 
********************************************************************************************/
void subscribeDevice()
{
    String subscribeDeviceMessage = "|4|2|2|";
    sendMessage(subscribeDeviceMessage);
}

/********************************************************************************************
Function    : unsubscribeDevice      
Description : 取消对设备的监听,发送成功串口返回：|4|2|6|1|\r      
Params      : 无 
Return      : 无 
********************************************************************************************/
void unsubscribeDevice()
{
    String unsubscribeDeviceMessage = "|4|2|6|";
    sendMessage(unsubscribeDeviceMessage);
}


/********************************************************************************************
Function    : publish
Description : 发送消息 ,必须先创建设备凭据
Params      : message 发布的消息内容,发送成功串口返回： |4|2|3|1|\r
Return      : 无 
********************************************************************************************/
void publish(String message)
{
    String publishMessage = "|4|2|3|" + message + separator;
    sendMessage(publishMessage);
}

/********************************************************************************************
Function    : disconnect 
Description : 销毁设备凭据： |4|2|4|1|\r
Params      : 无 
Return      : 无 
********************************************************************************************/
void distoryIotClient()
{
    String distoryIotClientMessage = "|4|2|4|";
    sendMessage(distoryIotClientMessage);
}

/********************************************************************************************
Function    : recreateIoTClient 
Description : 重新穿件设备凭据，创建成功返回： |4|2|1|1|\r
Params      : 无
Return      : 无 
********************************************************************************************/
void recreateIoTClient()
{
    String recreateIoTClientMessage = "|4|2|5|";
    sendMessage(recreateIoTClientMessage);
}

/********************************************************************************************
Function    : splitString 
Description : 切割字符串 
Params      : data 存储切割后的字符数组
Params      : str  被切割的字符串 
Params      : data 切割字符串的标志 
Return      : 无 
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
Function    : handleUart 
Description : 处理串口数据      
Params      : 无 
Return      : 无 
********************************************************************************************/
void handleUart()
{
    while(softSerial.available() > 0)
    {
        String receivedata = softSerial.readStringUntil('\r');
        const char* obloqMessage = receivedata.c_str();
        if(strcmp(obloqMessage, "|1|1|") == 0)
		{
			Serial.println("Pong");
			pingOn = false;
			obloqState = PINGOK;
		}
		else if(strstr(obloqMessage, "|2|3|") != 0 && strlen(obloqMessage) != 9)
		{
			Serial.println("Wifi ready");
			obloqState = WIFIOK;
		}
		else if(strcmp(obloqMessage, "|4|2|1|1|") == 0)
		{
			Serial.println("Azure ready");
            obloqState = AZURECONNECT;
		}
        else if(strstr(obloqMessage, "|4|2|5|") != 0)
        {
            splitString(receiveStringIndex,receivedata,"|");
            receiveMessageCallbak(receiveStringIndex[3]);
        }
    }
}

/********************************************************************************************
Function    : sendPing
Description : 验证串口是否正常通信
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
Description : 根据不同的状态发送不同的命令      
Params      : 无 
Return      : 无 
********************************************************************************************/
void execute()
{
    switch(obloqState)
    {
        case PINGOK: connectWifi(WIFISSID,WIFIPWD); obloqState = WAIT; break;
        case WIFIOK: createIoTClient(connectionString);obloqState = WAIT; break;
        case AZURECONNECT : createIoTClientSuccess = true; obloqState = WAIT; break;
        default: break;
    }
}

/********************************************************************************************
Function    : sendMessage
Description : 创建连接凭据,创建成功返回：      
Params      : connecttionString  设备连接字符串   
Return      : 无 
********************************************************************************************/
void sendMessage(String message,unsigned long interval = 0)
{
    static unsigned long previousSendMessageTime = 0;
    if(createIoTClientSuccess)
    {
        if (interval == 0)
            publish(message);
        else
        {
            if(millis() - previousSendMessageTime > interval)
            {
                previousSendMessageTime = millis();
                publish(message); 
                Serial.println(message);
            }
        }
    } 
}

/********************************************************************************************
Function    : receiveMessageCallbak
Description : 接收消息的回调函数：      
Params      : message  接收到的消息字符串  
Return      : 无 
********************************************************************************************/
void receiveMessageCallbak(String message)
{
    Serial.println("Message content: " + message);
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

    //监听设备
    if(!subceribeIotDevice && createIoTClientSuccess)
    {
        subceribeIotDevice = true;
        subscribeDevice();
    }
    sendMessage("DFRobot IOT",5000);
    handleUart();
}
