#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <time.h>
#include <LiquidCrystal_I2C.h>

int NTPClientFlag = 0;
// wifi AP
const char *ssid = "TOP";
const char *password = "wifi@2992256";
// esp32 info
// const char *id = "vhl-esp32";   //vehicle height limit esp32

// RESTful API server addr
IPAddress targetIP_1(192, 168, 67, 111);
IPAddress targetIP_2(192, 168, 67, 151); // I heard the management office uses static IP, 151.
IPAddress deviceIP(192, 168, 67, 200);  //// esp32 static IP address define

IPAddress gateway(192, 168, 67, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(168, 95, 1, 1);

const int connectPort = 18989;

bool isConnected_1 = false;
bool isConnected_2 = false;

u_int32_t lastDetectedTime = 0;
u_int32_t lastSendTime_1 = 0;
u_int32_t lastSendTime_2 = 0;

const u_int16_t i_delay = 25;
// const u_int16_t sendInterval = 1000;
const u_int16_t keepDuration = 20000 / i_delay; // 20 seconds
const u_int16_t stopDuration = 5000 / i_delay; // 5 seconds

WiFiClient client_1;
WiFiClient client_2;

int inputPin_36 = 36;
u_int16_t secondCnt = 0;
u_int16_t stopCnt = 0;

// function declare
int tcp_post(int, WiFiClient&, IPAddress);
int tcp_check(int, WiFiClient&, IPAddress, int, bool&);

void setup()
{
    pinMode(inputPin_36, INPUT);
    // pinMode(inputPin_16, INPUT);
    Serial.begin(115200);

    // static ip setting
    if(!WiFi.config(deviceIP, gateway, subnet, dns)){
        Serial.println("Static IP address setting FAIL!");
    }

    // connet to wifi AP
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("WiFi connected! device IP: %s\n", deviceIP);
    
   
}

void loop()
{  
    int vhl_sensor = analogRead(inputPin_36);
    bool detected = vhl_sensor > 500;

    if(detected){
        secondCnt = 1;
        tcp_check(0, client_1, targetIP_1, connectPort, isConnected_1);
        tcp_check(0, client_2, targetIP_2, connectPort, isConnected_2);
        if(isConnected_1) tcp_post(1, client_1, targetIP_1);
        if(isConnected_2) tcp_post(1, client_2, targetIP_2);
    }else{
        if(secondCnt > 0 && secondCnt <= keepDuration){
            if (secondCnt % (1000 / i_delay) == 0){
                tcp_post(1, client_1, targetIP_1);
                tcp_post(1, client_2, targetIP_2);
            } 
            secondCnt++;
        }else if (secondCnt > 0 && stopCnt <= stopDuration){
            if (secondCnt % (1000 / i_delay) == 0){
                tcp_post(0, client_1, targetIP_1);
                tcp_post(0, client_2, targetIP_2);
            }             
            stopCnt++;
        }else{
            secondCnt = 0;
            stopCnt = 0;
        }
    }

    tcp_check(1, client_1, targetIP_1, connectPort, isConnected_1);
    tcp_check(1, client_2, targetIP_2, connectPort, isConnected_2);
//     if(vhl_sensor > 500) {
//         secondCnt = 0;
//         secondCnt ++;
//         int tmp = tcp_post(1);
//     }else{
//         if(secondCnt > 0){
//             if (secondCnt % (1000 / i_delay) == 0) int tmp = tcp_post(1);
//             secondCnt++;
//         }else if (end_limit_cnt > 0){
//             int tmp = tcp_post(0);
//             end_limit_cnt++;
//             Serial.printf("%d, %d\n", end_limit_cnt, secondCnt);
//         }
//         if(end_limit_cnt == end_limit) end_limit_cnt = 0;
//         if(secondCnt > alert_limit){
//             secondCnt = 0;
//             end_limit_cnt = 1;
//         }
//     }
   
//     // printf("Analog value: %d\n", vhl_sensor);

    delay(i_delay);  // 每 0.025 秒發送一次 
}

int tcp_check(int mode, WiFiClient &client, IPAddress targetIP, int port, bool &isConnected){   // mode 0 is to check connected , mode 1 is to check timeout
    if (mode == 0){
        if(!isConnected){
            client.stop();
            if(client.connect(targetIP, port)){
                isConnected = true;
                Serial.printf("%s is connected.", client);
            }
        }
    }else if (mode == 1) {
        if (isConnected && !client.connected()) {
            Serial.print("Timeout: client disconnected unexpectedly: ");
            Serial.println(targetIP);
            client.stop();
            isConnected = false;
        }
    }
    
    return 0;
}

int tcp_post(int state, WiFiClient &client, IPAddress targetIP){

    // time
    time_t curTime;
    struct tm timeinfo;
    time(&curTime);
    localtime_r(&curTime, &timeinfo);

    char message[128];
    sprintf(message, "{\"timestamp\":\"%02d:%02d:%02d\",\"id\":\"%s\",\"state\":%d}", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, "vhl-esp32", state);
    client.println(message);

    Serial.print("Sent to ");
    Serial.print(targetIP);
    Serial.print(": ");
    Serial.println(message);

    return 0;
}
