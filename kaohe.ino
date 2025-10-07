#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#define pin 2

const char *ssid = "起名困难灯";
const char *password = "11451419";

int statu;
uint32_t hz = 5;
uint32_t brightness = 255;
int btime = 3000;
String msg;

Preferences pref;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<body>
  <a href="javascript:connect()">运行 WebSocket</a>
  <script type="text/javascript">
    function connect() {
      var ws = new WebSocket("ws://192.168.4.1:80/ws");
    }
  </script>
  <br>
  <a href="javascript:on()">on</a>
  <script type="text/javascript">
    function on() {
      ws.send("1");
    }
  </script>
  <br>
  <a href="javascript:off()">off</a>
  <script type="text/javascript">
    function off() {
      ws.send("0");
    }
  </script>
  <br>
  <a href="javascript:blink()">blink</a>
  <script type="text/javascript">
    function blink() {
      ws.send("2");
    }
  </script>
  <br>
  <a href="javascript:breath()">breath</a>
  <script type="text/javascript">
    function breath() {
      ws.send("3");
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

  ws.onEvent(onWebsocketEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  server.begin();
}

void loop() {
  ledc();
}

void ledc() {
  switch(statu){
    case 0 :
      // off
      ledcWrite(pin, 0);
      break;
    case 1 :
      // on
      ledcWrite(pin, 100);
      break;
    case 2 :
      // blink
      ledcWrite(pin, 100);
      delay(1000/hz);
      ledcWrite(pin, 0);
      delay(1000/hz);
      break;
    case 3 :
      // breath
      ledcFade(pin, 0, brightness, btime/2);
      delay(btime/2);
      ledcFade(pin, brightness, 0, btime/2);
      delay(btime/2);
      break;
  }
}

void getStatu() {
  pref.begin("statu");
  if(!pref.isKey("statu")) {
    pref.putInt("statu",0);
    statu = pref.getInt("statu");
  } else {
    statu = pref.getInt("statu");
  }
}

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

void onWebsocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch(type) {
    case WS_EVT_CONNECT:
      Serial.println("已连接");
      break;
    case WS_EVT_DISCONNECT:
      Serial.println("已断开连接");
      break;
    case WS_EVT_DATA:
      handleWebsocketMessage(arg, data, len);
      break;
    case WS_EVT_ERROR:
      Serial.printf("WebSocket错误 #%u\n", client->id());
      break;
    default:
      break;
  }
}

void handleWebsocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      if (strcmp((char*)data, "0") == 0) {
        pref.putInt("statu",0);
        statu = 0;
      } else if (strcmp((char*)data, "1") == 0) {
        pref.putInt("statu",1);
        statu = 1;
      } else if (strcmp((char*)data, "2") == 0) {
        pref.putInt("statu",2);
        statu = 2;
      } else if (strcmp((char*)data, "3") == 0) {
        pref.putInt("statu",3);
        statu = 3;
      }
  }
}
