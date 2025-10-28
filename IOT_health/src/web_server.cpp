// ============================================================
// web_server.cpp (Giao diện web mới, bắt mắt hơn)
// ============================================================

#include "web_server.h" 
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ❗️ 1. THÊM THƯ VIỆN MDNS
#include <ESPmDNS.h>

// -------------------- CẤU HÌNH WIFI ------------------------
char ssid[] = "Cuongg";
char pass[] = "00000000";
const char* ap_ssid = "ESP32_SPO2_Monitor";
const char* ap_pass = "12345678";

// -------------------- KHỞI TẠO WEBSERVER & WEBSOCKET --------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); 


// ============================================================
// ❗️ TRANG WEB (HTML, CSS, JAVASCRIPT) - GIAO DIỆN MỚI
// ============================================================
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Health Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap" rel="stylesheet">

    <style>
        /* 2. CSS Tinh Chỉnh Lại Toàn Bộ */
        :root {
            --bg-color: #f4f7f6;
            --card-bg: #ffffff;
            --shadow: 0 10px 25px rgba(0, 0, 0, 0.08);
            --border-radius: 12px;
            --text-color: #333;
            --accent-blue: #007aff;
            --accent-green: #34c759;
            --accent-red: #ff3b30;
            --accent-orange: #ff9500;
        }

        body { 
            font-family: 'Roboto', Arial, sans-serif; 
            background: var(--bg-color); 
            text-align: center; 
            padding-top: 20px; 
            color: var(--text-color);
            margin: 0;
        }

        .container { 
            max-width: 450px; 
            margin: auto; 
            padding: 20px; 
        }

        h1 {
            font-weight: 700;
            color: #222;
            font-size: 2em;
            margin-bottom: 20px;
        }
        
        /* Cấu trúc card chung */
        .card { 
            padding: 20px; 
            margin-bottom: 20px; 
            border-radius: var(--border-radius);
            background: var(--card-bg);
            box-shadow: var(--shadow);
            border: 1px solid #eee;
            text-align: left;
        }
        
        .card h2 { 
            margin-top: 0;
            margin-bottom: 15px;
            font-size: 1.2em;
            font-weight: 500;
            color: #555;
            border-bottom: 1px solid #f0f0f0;
            padding-bottom: 10px;
        }
        
        /* Card BPM & SpO2 */
        .card-max {
            /* Gradient nền nhẹ nhàng */
            background-image: linear-gradient(135deg, #f8ffff 0%, #e6f7ff 100%);
            border-color: #d0ebf9;
        }
        
        .value-group {
            display: flex; /* Sắp xếp ngang */
            justify-content: space-around; /* Căn đều */
            text-align: center;
        }
        
        .value-item h2 {
            border: none;
            font-size: 1.1em;
            margin-bottom: 5px;
        }
        
        .value { 
            font-size: 3.5em; /* Làm số to hơn */
            font-weight: 700; 
            color: #000;
            line-height: 1.1;
        }
        .unit { 
            font-size: 1.1em; 
            color: #555;
            margin-left: 2px;
        }

        /* Card Nhiệt độ */
        .card-temp {
            background-image: linear-gradient(135deg, #fafff8 0%, #f6ffed 100%);
            border-color: #dff0d0;
            /* Căn giữa vì chỉ có 1 giá trị */
            text-align: center; 
        }
        .card-temp h2 {
            text-align: left;
        }
        
        /* Trạng thái */
        #status { 
            font-size: 1.1em; 
            font-weight: 500; 
            padding: 12px; 
            border-radius: 8px;
            margin-bottom: 20px;
            transition: all 0.3s ease;
        }
        .status-running { color: #fff; background: var(--accent-green); }
        .status-paused { color: #fff; background: var(--accent-orange); }
        .status-no-finger { color: #fff; background: var(--accent-red); }
        .status-connecting... { color: #555; background: #eee; } /* Trạng thái ban đầu */

        /* Hộp cảnh báo (Giữ nguyên logic) */
        .alert-box {
            padding: 15px;
            margin-bottom: 20px;
            border-radius: var(--border-radius);
            font-size: 1.1em;
            font-weight: 500;
            background-color: #fff1f0;
            border: 1px solid var(--accent-red);
            color: var(--accent-red);
            animation: blinker 1s linear infinite; 
            text-align: left; 
        }
        .alert-hidden {
            display: none;
        }
        /* Giá trị nhấp nháy (Giữ nguyên logic) */
        .alert-value {
            color: var(--accent-red);
            animation: blinker 1s linear infinite;
        }
        @keyframes blinker {
            50% { opacity: 0.3; }
        }
        
        /* Nút bấm */
        .buttons {
            display: grid;
            grid-template-columns: 1fr 1fr; /* Chia 2 cột */
            gap: 12px; /* Khoảng cách giữa các nút */
        }
        
        .buttons button { 
            font-size: 1em; 
            font-weight: 500;
            padding: 15px; 
            border: none; 
            border-radius: 8px; 
            color: white; 
            cursor: pointer; 
            transition: all 0.2s ease;
            box-shadow: 0 4px 15px rgba(0,0,0,0.1);
        }
        /* Hiệu ứng khi di chuột */
        .buttons button:hover {
            transform: translateY(-2px); /* Nâng nút lên */
            box-shadow: 0 6px 20px rgba(0,0,0,0.15); /* Bóng đổ to hơn */
            filter: brightness(1.1); /* Sáng hơn */
        }
        
        #btn-pause { background: var(--accent-orange); }
        #btn-mode { background: var(--accent-blue); }
        
        /* Nút Restart chiếm 2 cột */
        #btn-restart { 
            background: var(--accent-red);
            grid-column: 1 / -1; /* Mở rộng ra toàn bộ */
        }

    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Health Monitor</h1>
        
        <div id="status" class="status-connecting...">CONNECTING...</div>

        <div id="alert-box" class="alert-box alert-hidden">
            </div>

        <div id="max-card" class="card card-max">
            <h2>❤️ Vital Signs</h2>
            <div class="value-group">
                <div class="value-item">
                    <h2>BPM</h2>
                    <span id="bpm" class="value">--</span>
                    <span class="unit"> bpm</span>
                </div>
                <div class="value-item">
                    <h2>SpO2</h2>
                    <span id="spo2" class="value">--</span>
                    <span class="unit"> %</span>
                </div>
            </div>
        </div>

        <div id="temp-card" class="card card-temp">
            <h2>🌡️ Temperature</h2>
            <div class="value-item">
                 <span id="temp" class="value">--</span>
                 <span class="unit"> &deg;C</span>
            </div>
        </div>

        <div class="buttons">
            <button id="btn-pause">Pause / Resume</button>
            <button id="btn-mode">Switch Mode</button>
            <button id="btn-restart">Restart</button>
        </div>
    </div>

    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;

        function initWebSocket() {
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onmessage = onMessage;
        }

        function onOpen(event) {
            console.log('Connection opened');
            document.getElementById('status').innerHTML = "CONNECTED";
            document.getElementById('status').className = 'status-running'; // Đổi màu khi kết nối
        }

        function onClose(event) {
            console.log('Connection closed');
            document.getElementById('status').innerHTML = "DISCONNECTED";
            document.getElementById('status').className = 'status-no-finger'; // Dùng màu đỏ khi mất kết nối
            setTimeout(initWebSocket, 2000); // Reconnect after 2s
        }

        function onMessage(event) {
            try {
                var data = JSON.parse(event.data);
                
                var bpmEl = document.getElementById('bpm');
                var spo2El = document.getElementById('spo2');
                var tempEl = document.getElementById('temp');
                var alertBox = document.getElementById('alert-box'); 
                var statusEl = document.getElementById('status');

                bpmEl.innerText = data.bpm;
                spo2El.innerText = data.spo2;
                tempEl.innerText = data.temp;

                bpmEl.classList.toggle('alert-value', data.bpm_alert);
                spo2El.classList.toggle('alert-value', data.spo2_alert);
                tempEl.classList.toggle('alert-value', data.temp_alert);
                
                if (data.overall_alert) {
                    var warningMessage = "<strong>WARNING:</strong><br>"; 
                    if (data.bpm_alert) {
                        warningMessage += "BPM is abnormal (" + data.bpm + ").<br>";
                    }
                    if (data.spo2_alert) {
                        warningMessage += "SpO2 is abnormal (" + data.spo2 + "%).<br>";
                    }
                    if (data.temp_alert) {
                        warningMessage += "Temperature is abnormal (" + data.temp + " &deg;C).";
                    }
                    alertBox.innerHTML = warningMessage;
                    alertBox.classList.remove('alert-hidden');
                } else {
                    alertBox.classList.add('alert-hidden');
                }
                
                // Cập nhật trạng thái (text và màu)
                statusEl.innerText = data.status;
                statusEl.className = 'status-' + data.status.toLowerCase().replace(' ', '-');

                // Ẩn/hiện card (giữ nguyên)
                if (data.mode === 'MAX') {
                    document.getElementById('max-card').style.display = 'block';
                    document.getElementById('temp-card').style.display = 'none';
                } else {
                    document.getElementById('max-card').style.display = 'none';
                    document.getElementById('temp-card').style.display = 'block';
                }
            } catch (e) {
                console.error("Error parsing JSON: ", e);
            }
        }

        window.onload = function() {
            initWebSocket();
            document.getElementById('btn-pause').onclick = function() { websocket.send("PAUSE"); }
            document.getElementById('btn-mode').onclick = function() { websocket.send("MODE"); }
            document.getElementById('btn-restart').onclick = function() { websocket.send("RESTART"); }
        };
    </script>
</body>
</html>
)rawliteral";


// ============================================================
// HÀM (Private): Xử lý kết nối WiFi (MẠNH MẼ HƠN)
// (Giữ nguyên)
// ============================================================
void connectToWiFiOrAP() {
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA); 
    delay(10); 
    WiFi.begin(ssid, pass);
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttemptTime > 5000) {
            Serial.println("\nWiFi connection timed out after 5s.");
            break; 
        }
        Serial.print(".");
        delay(100); 
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect. Starting AP mode.");
        WiFi.mode(WIFI_AP);
        delay(10); 
        WiFi.softAP(ap_ssid, ap_pass);
        Serial.print("AP IP address (Connect to this): ");
        Serial.println(WiFi.softAPIP());
    }
}


// ============================================================
// HÀM (Private): Xử lý sự kiện WebSocket (nhận lệnh từ web)
// (Giữ nguyên)
// ============================================================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0; // Null-terminate string
            String msg = (char*)data;
            Serial.printf("WS Rcvd: %s\n", msg.c_str());

            if (msg == "PAUSE") {
                portENTER_CRITICAL(&mux);
                isPaused = !isPaused;
                portEXIT_CRITICAL(&mux);
            } else if (msg == "MODE") {
                portENTER_CRITICAL(&mux);
                modeMeasure = !modeMeasure;
                portEXIT_CRITICAL(&mux);
            } else if (msg == "RESTART") {
                ESP.restart();
            }
        }
    }
}

// ============================================================
// HÀM (Public): Gửi dữ liệu JSON qua WebSocket
// (Giữ nguyên)
// ============================================================
void broadcastStatus() {
    if (ws.count() == 0) return;
    portENTER_CRITICAL(&mux);
    double bpm_local = displayBPM;
    float spo2_local = displaySpO2;
    float tempC = displayTempC;
    bool finger_local = fingerDetected;
    bool paused_local = isPaused;
    bool mode_local = modeMeasure;
    bool bpm_alert = bpmAlert;
    bool spo2_alert = spo2Alert;
    bool temp_alert = tempAlert;
    bool overall_alert = overallAlertState; 
    portEXIT_CRITICAL(&mux);

    String statusStr = "NO FINGER";
    if (mode_local) { 
        if (paused_local) statusStr = "PAUSED";
        else if (finger_local) statusStr = "RUNNING";
    } else { 
        if (paused_local) statusStr = "PAUSED";
        else statusStr = "RUNNING";
    }

    String jsonString = "";
    jsonString += "{";
    jsonString += "\"bpm\":" + String(bpm_local, 0);
    jsonString += ", \"spo2\":" + String(spo2_local, 0);
    jsonString += ", \"temp\":" + String(tempC, 2);
    jsonString += ", \"mode\":\"" + String(mode_local ? "MAX" : "TEMP") + "\"";
    jsonString += ", \"status\":\"" + statusStr + "\"";
    jsonString += ", \"bpm_alert\":" + String(bpm_alert ? "true" : "false");
    jsonString += ", \"spo2_alert\":" + String(spo2_alert ? "true" : "false");
    jsonString += ", \"temp_alert\":" + String(temp_alert ? "true" : "false");
    jsonString += ", \"overall_alert\":" + String(overall_alert ? "true" : "false"); 
    jsonString += "}";

    ws.textAll(jsonString);
}

// ============================================================
// HÀM (Public): Khởi tạo WebServer
// (Giữ nguyên)
// ============================================================
void setupWebServer() {
    // 1. Kết nối WiFi
    connectToWiFiOrAP();

    // 2. KHỞI TẠO MDNS
    const char* mdns_hostname = "esp32-monitor"; 
    if (!MDNS.begin(mdns_hostname)) {
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.print("mDNS responder started. Access at: http://");
        Serial.print(mdns_hostname);
        Serial.println(".local");
        MDNS.addService("http", "tcp", 80); 
    }

    // 3. Khởi tạo WebServer và WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", HTML_PAGE);
    });
    server.begin(); 
}