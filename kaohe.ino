#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#define pin 2

const char* ssid = "起名困难灯";
const char* password = "11451419";

int statu;
String st;
uint32_t hz = 5;
uint32_t brightness = 255;
int btime = 3000;
String msg;

Preferences pref;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// 网页界面
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta charset="utf-8">
</head>
<body>
  <a href="javascript:connect()">运行 WebSocket</a>
  <br>
  <p id="st"></p>
  <script type="text/javascript">
    var ws
    ws.onmessage = function(event) {
      var msg = event.data;
      document.getElementById("st").innerHTML = msg;
      };
    function connect() {
      ws = new WebSocket("ws://192.168.4.1:80/ws");
      ws.onmessage = function(event) {
      var msg = event.data;
      document.getElementById("st").innerHTML = msg;
      };
    }
  </script>
  <a href="javascript:on()">on</a>
  <script type="text/javascript">
    function on() {
      if(ws && ws.readyState === ws.OPEN) {
        ws.send("on");
        ws.onmessage = function(event) {
        var msg = event.data;
        document.getElementById("st").innerHTML = msg;
        };
      } else {
        console.error('WebSocket未连接');
      }

    }
  </script>
  <br>
  <a href="javascript:off()">off</a>
  <script type="text/javascript">
    function off() {
      if(ws && ws.readyState === ws.OPEN) {
        ws.send("off");
        ws.onmessage = function(event) {
        var msg = event.data;
        document.getElementById("st").innerHTML = msg;
        };
      } else {
        console.error('WebSocket未连接');
      }
    }
  </script>
  <br>
  <a href="javascript:blink()">blink</a>
  <script type="text/javascript">
    function blink() {
      if(ws && ws.readyState === ws.OPEN) {
        ws.send("blink");
        ws.onmessage = function(event) {
        var msg = event.data;
        document.getElementById("st").innerHTML = msg;
        };
      } else {
        console.error('WebSocket未连接');
      }
    }
  </script>
  <br>
  <a href="javascript:breath()">breath</a>
  <script type="text/javascript">
    function breath() {
      if(ws && ws.readyState === ws.OPEN) {
        ws.send("breath");
        ws.onmessage = function(event) {
        var msg = event.data;
        document.getElementById("st").innerHTML = msg;
        };
      } else {
        console.error('WebSocket未连接');
      }
    }
  </script>
</body>
</html>
)rawliteral";

void ledc();
void getStatu();
void APInit();
void onWebsocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) ;
void handleWebsocketMessage(void *arg, uint8_t *data, size_t len);

void setup() {
  Serial.begin(115200);
  getStatu();
  ledcAttach(pin, 115200, 8);
  APInit();

  //网络handler绑定
  ws.onEvent(onWebsocketEvent);
  server.addHandler(&ws);

  //server启动
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  server.begin();
}

void loop() {
  ledc();
}

//灯光控制器
void ledc() {
  switch(statu){
    case 0 :
      // off
      ledcWrite(pin, 0);
      st = "off";
      break;
    case 1 :
      // on
      ledcWrite(pin, 100);
      st = "on";
      break;
    case 2 :
      // blink
      ledcWrite(pin, 100);
      delay(1000/hz);
      ledcWrite(pin, 0);
      delay(1000/hz);
      st = "blink";
      break;
    case 3 :
      // breath
      ledcFade(pin, 0, brightness, btime/2);
      delay(btime/2);
      ledcFade(pin, brightness, 0, btime/2);
      delay(btime/2);
      st = "breath";
      break;
  }
}

//状态储存器初始化
void getStatu() {
  pref.begin("statu");
  if(!pref.isKey("statu")) {
    pref.putInt("statu",0);
    statu = pref.getInt("statu");
  } else {
    statu = pref.getInt("statu");
  }
}

//wifiap初始化
void APInit() {
  Serial.println("AP Initing...");
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("AP Init FAIL");
    while (1);
  }
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
}

//ws事件处理
void onWebsocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  String statuMsg = String("亮度:") + brightness + ",blink频率:"+hz+",breath周期:"+btime+",当前状态:"+st;
  switch(type) {
    case WS_EVT_CONNECT:
      //当连接时
      Serial.println("已连接");

      //上报当前状态
      ws.textAll(String(statuMsg));
      break;
    case WS_EVT_DISCONNECT:
      //当断连时
      Serial.println("已断开连接");

      //statu转为breath
      if(pref.isKey("statu")){
            pref.remove("statu");
          }
          pref.putInt("statu",3);
          statu = pref.getInt("statu");
          st = "breath";
      break;
    case WS_EVT_DATA:
      //ws指令处理
      handleWebsocketMessage(arg, data, len);

      //上报当前状态
      ws.textAll(String(statuMsg));
      break;
    case WS_EVT_ERROR:
      //ws error处理
      Serial.printf("WebSocket错误 #%u\n", client->id());
      break;
    default:
      break;
  }
}

//ws指令处理
void handleWebsocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      String msg = String((char*)data);
      Serial.printf("收到消息: %s\n", msg);

      //ws指令处理（nvs写入，状态变量更改）
      if(msg == "on") {
        if(pref.isKey("statu")){
            pref.remove("statu");
          }
          pref.putInt("statu",1);
          statu = pref.getInt("statu");
      } else if(msg == "off") {
        if(pref.isKey("statu")){
            pref.remove("statu");
          }
          pref.putInt("statu",0);
          statu = pref.getInt("statu");
      } else if(msg == "blink") {
        if(pref.isKey("statu")){
            pref.remove("statu");
          }
          pref.putInt("statu",2);
          statu = pref.getInt("statu");
      } else if(msg == "breath") {
        if(pref.isKey("statu")){
            pref.remove("statu");
          }
          pref.putInt("statu",3);
          statu = pref.getInt("statu");
      }
  }
}
