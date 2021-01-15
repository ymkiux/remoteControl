#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "baseString.c"
#include <HttpClient.h>
#include <ESP8266HTTPClient.h>

#ifndef STASSID
#define STASSID "First"
#define STAPSK  "12345678"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
ESP8266WebServer server(80);
bool LED_Flag = false;
int tag;

/**
 * 其分别表示D3 D2 D4
 */
int Led_Red = 3;
int Led_Green = 2;
int Led_Blue = 4;

String str = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\"content=\"width=device-width, initial-scale=1.0\"><meta http-equiv=\"X-UA-Compatible\"content=\"ie=edge\"><title>不知名up的ESP8266网页配网</title></head><body><form name=\"my\">WiFi名称：<input type=\"text\"name=\"s\"placeholder=\"请输入您WiFi的名称\"id=\"aa\"><br>WiFi密码：<input type=\"text\"name=\"p\"placeholder=\"请输入您WiFi的密码\"id=\"bb\"><br><input type=\"button\"value=\"连接\"onclick=\"wifi()\"></form><script language=\"javascript\">function wifi(){var ssid=my.s.value;var password=bb.value;var xmlhttp=new XMLHttpRequest();xmlhttp.open(\"GET\",\"/HandleVal?ssid=\"+ssid+\"&password=\"+password,true);xmlhttp.send()}</script></body></html>";
/**
   客户端请求回调函数
*/
void handleRoot() {
  server.send(200, "text/html", str);
}
/**
   对客户端请求返回值处理
*/
void HandleVal() {
  String wifis = server.arg("ssid"); //从JavaScript发送的数据中找ssid的值
  String wifip = server.arg("password"); //从JavaScript发送的数据中找password的值
  Serial.println(wifis); Serial.println(wifip);
  WiFi.begin(wifis, wifip);
}
/**
   响应失败函数
*/
void handleNotFound() {
  digitalWrite(LED_BUILTIN, 0);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, 1);
}
/**
   自动连接WiFi函数
*/
bool autoConfig() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.print("AutoConfig Waiting......");
  for (int i = 0; i < 20; i++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("AutoConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      WiFi.printDiag(Serial);
      lightsOnAndOff(true);
      return true;
      //break;
    }
    else
    {
      Serial.print(".");
      LED_Flag = !LED_Flag;
      if (LED_Flag)
        lightsOnAndOff(false);
      else
        lightsOnAndOff(false);
      delay(500);
    }
  }
  Serial.println("AutoConfig Faild!" );
  return false;
}


/**
   web配置WiFi函数
*/
void htmlConfig() {
  WiFi.mode(WIFI_AP_STA);//设置模式为AP+STA
  digitalWrite(LED_BUILTIN, LOW);
  WiFi.softAP(ssid, password);
  Serial.println(ap_setting_success);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/HandleVal", HTTP_GET, HandleVal);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  while (1)
  {
    server.handleClient();
    MDNS.update();
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("HtmlConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      Serial.println(html_success);
      break;
    }
  }
}

/**
   与服务器保持伪连接
   该方法内容仅作思路参考
*/
String ip = "192.168.0.101";
HTTPClient http;
void getHttp() {
  String  str = (String)"http://" + ip + ":8080/TCP/SendWifiModular?strPrm=" + deviceId;
  http.setTimeout(4000);
  http.begin(str);
  int httpResponseCode = http.GET();
  Serial.println(httpResponseCode);
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    if (payload == "1") {
      tag = 1;
      Serial.println(succer);
    } else {
      Serial.println(error);
    }
    delay(1000);
  }
  http.end();
}


/**
   封装指示灯操作
*/
void lightsOnAndOff(boolean state) {
  int max_size = 1;
  if (state) {
    max_size = 2;
  }
  for (int i = 0; i < max_size; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

/**
   当接收到指令时执行的相应操作
*/
void getSwitch() {
  for (int val = 255; val > 0; val--) {
    setColor(val, 255 - val, 128 - val);
    delay (15);
  }
}

/**
   封装设置颜色
*/
void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(Led_Red, redValue);
  analogWrite(Led_Green, greenValue);
  analogWrite(Led_Blue, blueValue);
}


void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  
  pinMode(Led_Red, OUTPUT);
  pinMode(Led_Green, OUTPUT);
  pinMode(Led_Blue, OUTPUT);

  Serial.begin(115200);
  bool wifiConfig = autoConfig();
  if (wifiConfig == false)
    htmlConfig();
}

void loop() {
  if (tag == 1) {
    getSwitch();
    --tag;
  }
  getHttp();
}
