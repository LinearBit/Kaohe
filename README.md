# 题目一

## 功能一二

![功能一二](https://github.com/LinearBit/Kaohe/blob/main/videos/12.mp4)
ps:如果演示视频不能可以在videos目录下找
### 功能一
- led控制
```c++
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
```
- AP
```c++
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
```
- ws服务器
 - 初始化
```c++
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
void webInit() {
		//网络事件handler绑定
		ws.onEvent(onWebsocketEvent);
		server.addHandler(&ws);

		//server启动
		server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
			request->send(200, "text/html", index_html);
		});
		server.begin();
}
```
 - handler
```c++
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
```
 - ws命令处理
```c++
void handleWebsocketMessage(void *arg, uint8_t *data, size_t len) {
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
```
- 网页ws客户端
 - 建立ws连接
```javascript
function connect() {
		ws = new WebSocket("ws://192.168.4.1:80/ws");
		ws.onmessage = function(event) {
		var msg = event.data;
		document.getElementById("st").innerHTML = msg;
		};
}
```
 - 发送指令 on/off/blink/breath
```javascript
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
```

### 功能二
- 接收状态信息
```javascript
ws.onmessage = function(event) {
		var msg = event.data;
		document.getElementById("st").innerHTML = msg;
};
```

## 功能三

![功能三](https://github.com/LinearBit/Kaohe/blob/main/videos/3.mp4)
ps:如果演示视频不能可以在videos目录下找
```cpp
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
```

### 难度ProMax
![ProMax](https://github.com/LinearBit/Kaohe/blob/main/videos/pm.mp4)
ps:如果演示视频不能可以在videos目录下找

调用Preferences库储存当前状态为一个键值对

储存当前状态：
```c++
//判断是否已有储存的状态信息
if(pref.isKey("statu")){
	pref.remove("statu");
}

//将状态信息储存入flash
pref.putInt("statu",0); //0:off 1:on 2:blink 3:breath

//将状态信息同步至程序
statu = pref.getInt("statu");
st = "breath"; //便于显示
```

初始化时获取flash中的状态信息：
```c++
void getStatu() {
	//初始化空间
	pref.begin("statu");

	//判断空间中是否已储存状态信息
	if(!pref.isKey("statu")) {
		pref.putInt("statu",0);
	statu = pref.getInt("statu");
	} else {
	statu = pref.getInt("statu");
	}
}
```
