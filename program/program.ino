#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <FS.h> // 引入文件系统库
#include <Ticker.h>
#include <Servo.h> // 引入舵机库
#define CONFIG_FILENAME "/config.json"
#define CONFIG_SERVO_FILENAME "/servo.json"
#define CONFIG_POWER_LED_FILENAME "/power_led.json"

Ticker timeoutTickerForEspRestart;

JsonDocument config_json;
JsonDocument config_servo_json;
JsonDocument config_power_led_json;

String ssid = "dormitory";
String password = "123456789";

AsyncWebServer server(80); // Create AsyncWebServer object on port 80
AsyncWebSocket ws("/ws");  // Create a WebSocket object

// 设置静态IP
IPAddress local_ip(192, 168, 192, 1); // 设置静态IP地址
IPAddress gateway(192, 168, 1, 1);	  // 设置网关
IPAddress subnet(255, 255, 255, 0);	  // 设置子网掩码

// ===================== HTML ===========================
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>WebSocket Demo</title><style>.container {display: flex;flex-direction: column;align-items: center;font: 20px Arial, sans-serif;}.setting_line {display: flex;flex-direction: row;align-items: center;justify-content: space-between;width: 100%;margin: 10px 0;}.setting_line label {width: 50%;font-size: 16px;font-weight: normal;display: inline-block;text-align: left;margin-right: 10px;line-height: 2;}.setting_line input {width: 50%;padding: 10px;border-radius: 5px;border: 1px solid #ccc;font-size: 16px;color: #333;background-color: #fff;transition: all 0.3s ease;}.setting_line input:hover {border-color: #888;background-color: #f9f9f9;}.setting_line input:focus {outline: none;border-color: #5b9bd5;box-shadow: 0 0 5px rgba(91, 155, 213, 0.5);}.setting_line button {font-size: large;width: 100%;padding: 10px 20px;border-radius: 5px;border: 1px solid #ccc;background-color: #fff;color: #333;cursor: pointer;transition: all 0.3s ease;}#info {color: red;font-weight: 400;font-size: medium;margin-left: 5px;}h2 {border-bottom: 1px solid #000;padding-bottom: 10px;}.nav {display: flex;flex-direction: row;justify-content: space-around;width: 100%;border-bottom: 1px solid hotpink;color: lightpink;padding-bottom: 5px;margin-bottom: 30px;}.nav_active {color: deeppink;font-weight: bold;}.btn-group {display: flex;flex-direction: column;justify-content: space-around;width: 100%;gap: 10px;}.btn-group button {font-size: 26px;width: 100%;font-weight: lighter;padding: 10px 60px;border-radius: 10px;border: 1px solid darkgray;background-color: #fff;color: #333;cursor: pointer;transition: all 0.3s ease;}.btn-group button:hover {background-color: #f0f0f0;border-color: #999;color: #000;}.btn-group button:focus {outline: none;box-shadow: 0 0 8px rgba(0, 0, 0, 0.2);}.btn-group button:active {background-color: #e0e0e0;border-color: #666;color: #222;transform: scale(0.98);}.setting_line button:hover {background-color: #f0f0f0;border-color: #999;color: #000;}.setting_line button:focus {outline: none;box-shadow: 0 0 8px rgba(0, 0, 0, 0.2);}.setting_line button:active {background-color: #e0e0e0;border-color: #666;color: #222;transform: scale(0.98);}.</style></head><body><h2 style="color: gray;">Message: <span id="info">message bar</span></h2><div class="nav"><div class="nav_active" onclick="page1()" id="page1Nav">homing</div><div onclick="page2()" id="page2Nav">settings</div></div><div class="container"><div class="page1" style="display: block;"><!-- <button onclick="sayHello()">say hello</button> --><div class="btn-group" style="margin-top: 150px;"><button onclick="light_on_bulb()">开灯</button><button onclick="light_off_bulb()">关灯</button></div></div><div class="page2" style="display: none;"><div class="setting_line"><label for="ssid">wifi 名字：</label><input type="text" id="ssid"></div><div class="setting_line"><label for="password">wifi 密码：</label><input type="text" id="password"></div><div class="setting_line"><button onclick="save_wifi_config()">保存并设置wifi名称密码</button></div><div style="height: 20px;"></div><div class="setting_line"><label for="servo_on">舵机开灯角度：</label><input type="text" id="servo_on"></div><div class="setting_line"><label for="servo_off">舵机关灯角度：</label><input type="text" id="servo_off"></div><div class="setting_line"><button onclick="test_servo_on()">测试开灯角度</button><div style="width: 10px;"></div><button onclick="test_servo_off()">测试关灯角度</button></div><div class="setting_line"><button onclick="save_servo_config()">保存并设置舵机角度</button></div><div style="height: 10px;"></div><div class="setting_line"><button onclick="power_led_on()">显示灯开启</button><div style="width: 10px;"></div><button onclick="power_led_off()">显示灯关闭</button></div><div class="setting_line"><button onclick="restart_esp()">重启服务器</button></div></div></div><script>var ws = null;var info = msg => document.getElementById("info").innerHTML = msg;function connect_ws() {try {ws = new WebSocket(`ws://${window.location.hostname}/ws`);ws.onopen = function (evt) {info("Connection open...");};ws.onmessage = on_message;ws.onclose = function (evt) {info("Connection closed.");ws = null;};ws.onerror = function (evt) {info("Connection error.");ws = null;};} catch (e) {ws = null;info("Connection error.");}};function send_message(json_data) {if (ws === null || ws.readyState !== WebSocket.OPEN) {connect_ws();} else {ws.send(JSON.stringify(json_data));}};function on_message(evt) {let data = JSON.parse(evt.data);info(data.message);if (data.cmd === "init") {document.getElementById("ssid").value = data.wifi_config.ssid;document.getElementById("password").value = data.wifi_config.password;document.getElementById("servo_on").value = data.servo_config.servo_on;document.getElementById("servo_off").value = data.servo_config.servo_off;};};function validate_angle(string_value) {let value = parseInt(string_value);if (value < 0) {info("角度不能小于0");return false;} else if (value > 180) {info("角度不能大于180");return false;} else {return value;};};function test_servo_on() {let value = validate_angle(document.getElementById("servo_on").value);if (value !== false) {send_message({ "cmd": "set_angle", "angle": value + "" });};};function test_servo_off() {let value = validate_angle(document.getElementById("servo_off").value);if (value !== false) {send_message({ "cmd": "set_angle", "angle": value + "" });};};function save_servo_config() {let servo_on = validate_angle(document.getElementById("servo_on").value);let servo_off = validate_angle(document.getElementById("servo_off").value);if (servo_on !== false && servo_off !== false) {send_message({ "cmd": "save_servo_config", "servo_on": servo_on + "", "servo_off": servo_off + "" });};};function validate_wifi_config(ssid, password) {if (ssid === "") {info("SSID不能为空");return false;} else if (password === "") {info("密码不能为空");return false;} else {return true;};};function save_wifi_config() {let ssid = document.getElementById("ssid").value;let password = document.getElementById("password").value;if (validate_wifi_config(ssid, password)) {ssid = ssid.trim();password = password.trim();send_message({ "cmd": "save_wifi_config", "ssid": ssid, "password": password });};};function restart_esp() {send_message({ "cmd": "restart_esp" });};function power_led_on() {send_message({ "cmd": "power_led_on" });};function power_led_off() {send_message({ "cmd": "power_led_off" });};function light_on_bulb() {send_message({ "cmd": "light_on_bulb" });};function light_off_bulb() {send_message({ "cmd": "light_off_bulb" });};function page1() {document.querySelector(".page1").style.display = "block";document.getElementById("page1Nav").classList.add("nav_active");document.getElementById("page2Nav").classList.remove("nav_active");document.querySelector(".page2").style.display = "none";}function page2() {document.querySelector(".page1").style.display = "none";document.getElementById("page1Nav").classList.remove("nav_active");document.getElementById("page2Nav").classList.add("nav_active");document.querySelector(".page2").style.display = "block";}window.onload = function () {connect_ws();};</script></body></html>
)rawliteral";

// 时间相关的操作
unsigned long timeout_duration = 2000; // 5秒
unsigned long current_call_servo_moving_time = 0;

// ===================== 舵机操作 =========================
Servo servo;
#define ServoPin D7
#define transistor_pin D8

void servo_setup()
{
	servo.attach(ServoPin);

	// 初始化三极管base控制端
	pinMode(transistor_pin, OUTPUT);

	int servo_on = config_servo_json["servo_on"];
	int servo_off = config_servo_json["servo_off"];
	int servo_middle = (servo_on + servo_off) / 2;
	servo_move(servo_middle); // reset servo position

	delay(600); // delay for servo moving complete
}

void servo_move(int angle)
{
	// 每次调用都保存当前的时间
	current_call_servo_moving_time = millis(); // 获取当前时间
	// 将D8口设置为高电平，使三极管导通
	digitalWrite(transistor_pin, HIGH);
	servo.write(angle);
}

void light_on_bulb()
{
	int servo_on = config_servo_json["servo_on"];
	int servo_off = config_servo_json["servo_off"];
	int servo_middle = (servo_on + servo_off) / 2;
	servo_move(servo_on);
	// delay for servo moving complete
	delay(700);
	servo_move(servo_middle); // reset servo position
}

void light_off_bulb()
{
	int servo_on = config_servo_json["servo_on"];
	int servo_off = config_servo_json["servo_off"];
	int servo_middle = (servo_on + servo_off) / 2;
	servo_move(servo_off);
	// delay for servo moving complete
	delay(700);
	servo_move(servo_middle); // reset servo position
}

// =====================电源指示灯========================
void power_led_setup()
{
	pinMode(2, OUTPUT);
	String power_led_status = config_power_led_json["status"];
	if (power_led_status == "1")
	{
		power_led_on();
	}
	else if (power_led_status == "0")
	{
		power_led_off();
	}
}

void power_led_on()
{
	digitalWrite(2, LOW);
}

void power_led_off()
{
	digitalWrite(2, HIGH);
}

// ===================== 文件操作 ===========================

void write_json_file(String filename, JsonDocument json_document)
{
	File file = SPIFFS.open(filename, "w");
	serializeJson(json_document, file);
	file.close();
}

JsonDocument read_json_file(String filename)
{
	File file = SPIFFS.open(filename, "r");
	JsonDocument json_document;
	deserializeJson(json_document, file);
	file.close();
	return json_document;
}

// 配置文件初始化函数，接收回调函数作为参数
void init_json_file(const char *json_filename, void (*config_func)(JsonDocument &))
{
	// 判断文件是否存在
	if (!SPIFFS.exists(json_filename))
	{
		Serial.print("create config file: ");
		Serial.println(json_filename);

		JsonDocument json_document;
		// 调用传入的回调函数来修改 json_document
		config_func(json_document);

		// 写入 JSON 文件
		write_json_file(json_filename, json_document);
	}
	else
	{
		Serial.print("config file: ");
		Serial.print(json_filename);
		Serial.println(" has been exist.");
	}
}

void handle_config_file_init_callback(JsonDocument &json_document)
{
	// 在这里初始化 JSON 文档
	json_document["ssid"] = ssid;
	json_document["password"] = password;
}

void handle_config_servo_init_callback(JsonDocument &json_document)
{
	json_document["servo_on"] = "0";
	json_document["servo_off"] = "180";
}

void handle_config_power_led_init_callback(JsonDocument &json_document)
{
	// 这里只使用0和1，所以不需要处理字符串
	json_document["status"] = "1";
}

void spiff_file_system_setup()
{
	if (!SPIFFS.begin())
	{
		Serial.println("An Error has occurred while mounting SPIFFS");
		Serial.println("try to format SPIFFS partition");
		if (SPIFFS.format())
		{
			Serial.println("SPIFFS partition formatted successfully");
			ESP.restart();
		}
		else
		{
			Serial.println("SPIFFS partition format failed");
		}
		return;
	}
	else
	{
		Serial.println("SPIFFS initalized successfully");
		init_json_file(CONFIG_FILENAME, handle_config_file_init_callback);
		init_json_file(CONFIG_SERVO_FILENAME, handle_config_servo_init_callback);
		init_json_file(CONFIG_POWER_LED_FILENAME, handle_config_power_led_init_callback);
	}
}

// ===================================== 处理 WebSocket 消息 =====================================

void on_request_response_ws_message(JsonDocument &request_json_data, JsonDocument &response_json_data)
{
	String cmd = request_json_data["cmd"];
	if (cmd == "set_angle")
	{
		int angle = request_json_data["angle"];
		if (angle >= 0 && angle <= 180)
		{
			servo_move(angle);
			response_json_data["message"] = "成功设置角度: " + String(angle) + "°";
		}
		else
		{
			response_json_data["message"] = "角度设置失败: " + String(angle) + "°";
		}
	}
	else if (cmd == "save_servo_config")
	{
		response_json_data["message"] = "成功保存舵机配置";
		config_servo_json["servo_on"] = request_json_data["servo_on"];
		config_servo_json["servo_off"] = request_json_data["servo_off"];
		// 保存到文件
		write_json_file(CONFIG_SERVO_FILENAME, config_servo_json);
	}
	else if (cmd == "save_wifi_config")
	{
		response_json_data["message"] = "成功保存wifi配置,重启生效";
		config_json["ssid"] = request_json_data["ssid"];
		config_json["password"] = request_json_data["password"];
		// 保存到文件
		write_json_file(CONFIG_FILENAME, config_json);
	}
	else if (cmd == "restart_esp")
	{
		response_json_data["message"] = "1s后重启服务器";
		timeoutTickerForEspRestart.attach(1, ESP.restart);
	}
	else if (cmd == "power_led_on")
	{
		response_json_data["message"] = "打开电源指示灯";
		config_power_led_json["status"] = "1";
		// 保存到文件
		write_json_file(CONFIG_POWER_LED_FILENAME, config_power_led_json);
		power_led_on();
	}
	else if (cmd == "power_led_off")
	{
		response_json_data["message"] = "关闭电源指示灯";
		config_power_led_json["status"] = "0";
		// 保存到文件
		write_json_file(CONFIG_POWER_LED_FILENAME, config_power_led_json);
		power_led_off();
	}
	else if (cmd == "light_on_bulb")
	{
		response_json_data["message"] = "打开灯泡";
		light_on_bulb();
	}
	else if (cmd == "light_off_bulb")
	{
		response_json_data["message"] = "关闭灯泡";
		light_off_bulb();
	}
}

void handle_websocket_message_callback(void *arg, uint8_t *data, size_t len)
{
	AwsFrameInfo *info = (AwsFrameInfo *)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		JsonDocument request_json_data;
		JsonDocument response_json_data;
		String serialized_json_document;

		data[len] = 0;
		String message = (char *)data;

		DeserializationError error = deserializeJson(request_json_data, message);

		if (error)
		{
			Serial.print("deserializeJson() failed: ");
			Serial.println(error.c_str());
			return;
		}
		else
		{
			on_request_response_ws_message(request_json_data, response_json_data);
			serializeJson(response_json_data, serialized_json_document);
			ws.textAll(serialized_json_document);
		}
	}
}

void handle_websocket_connected_callback(AsyncWebSocketClient *client)
{
	Serial.println("an user connect the websocket");
	// 向当前连接的用户发送消息
	JsonDocument response_json_data_for_connection;

	response_json_data_for_connection["wifi_config"] = config_json;
	response_json_data_for_connection["servo_config"] = config_servo_json;
	response_json_data_for_connection["message"] = "连接ESP成功";
	response_json_data_for_connection["cmd"] = "init";

	String serialized_json_document;
	serializeJson(response_json_data_for_connection, serialized_json_document);
	client->text(serialized_json_document);
}

void on_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
	switch (type)
	{
	case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		handle_websocket_connected_callback(client);
		break;
	case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		break;
	case WS_EVT_DATA:
		handle_websocket_message_callback(arg, data, len);
		break;
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	}
}

void setup_wifi()
{
	Serial.print("Setting AP (Access Point)…");
	const char *ssid_ = config_json["ssid"];
	const char *password_ = config_json["password"];
	WiFi.softAP(ssid_, password_);

	// 设置静态IP
	if (!WiFi.softAPConfig(local_ip, gateway, subnet))
	{
		Serial.println("Failed to configure AP");
	}
	else
	{
		Serial.println("AP Static IP Configured");
	}

	// 输出AP的IP地址
	Serial.print("AP IP Address: ");
	Serial.println(WiFi.softAPIP());

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send_P(200, "text/html", HTML); });
	ws.onEvent(on_event);
	server.addHandler(&ws);
	server.begin();
}

void setup()
{
	Serial.begin(115200);

	spiff_file_system_setup();

	config_json = read_json_file(CONFIG_FILENAME);
	config_servo_json = read_json_file(CONFIG_SERVO_FILENAME);
	config_power_led_json = read_json_file(CONFIG_POWER_LED_FILENAME);

	power_led_setup();
	servo_setup();

	Serial.print("config_json: ");
	Serial.print("ssid: ");
	Serial.print((String)config_json["ssid"]);
	Serial.print("; password:");
	Serial.print((String)config_json["password"]);
	Serial.println();

	setup_wifi();
}

void loop()
{
	// 判断时间是否超出界限
	if (millis() - current_call_servo_moving_time > timeout_duration)
	{
		// 关闭三极管
		digitalWrite(transistor_pin, LOW);
	}
}