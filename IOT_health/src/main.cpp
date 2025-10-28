// ============================================================
// main.cpp (Đã cập nhật logic cảnh báo tổng thể)
// ============================================================

#include <Wire.h>
#include "MAX30105.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include "algorithm.h"
#include "web_server.h" 

// -------------------- CONFIG PINS (Giữ nguyên) ----------------
#define BUZZER_PIN 25
#define LED_ALERT_PIN 26
#define BUTTON_PIN 27

// -------------------- GLOBAL MUTEX (ĐỊNH NGHĨA) ----------------
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile bool isPaused = false; 
volatile bool modeMeasure = false; 
double displayBPM = 0;
float displaySpO2 = 0;
float displayTempC = 0;
bool fingerDetected = false;

// ❗️ THÊM CÁC BIẾN TRẠNG THÁI CẢNH BÁO
bool bpmAlert = false;
bool spo2Alert = false;
bool tempAlert = false;
bool overallAlertState = false; // ❗️ BIẾN CẢNH BÁO TỔNG THỂ MỚI

// --- Khởi tạo các đối tượng (Giữ nguyên) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define DS18B20_PIN 18
OneWire oneWire(DS18B20_PIN);
DallasTemperature tempSensor(&oneWire);
MAX30105 particleSensor;

// --- Các hằng số (Giữ nguyên) ---
#define FINGER_ON IR_THRESHOLD
#define TIMETOBOOT 3000

// --- Các biến trạng thái (Giữ nguyên, chỉ còn các biến local) ---
volatile uint32_t latestRed = 0;
volatile uint32_t latestIR = 0;
typedef struct {
    uint32_t red;
    uint32_t ir;
} PPGData;
QueueHandle_t dataQueue = NULL;

// --- Các hằng số điều khiển (Giữ nguyên) ---
const uint32_t BUTTON_DEBOUNCE_MS = 20;
const uint32_t BUTTON_LONGPRESS_MS = 2000;
const float SPO2_ALERT_LOW = 94.0;
const float TEMP_ALERT_LOW = 30.0;
const float TEMP_ALERT_HIGH = 37.8;
const double BPM_ALERT_LOW = 50.0;
const double BPM_ALERT_HIGH = 120.0;
bool ledAlertState = false;
unsigned long ledToggleMs = 0;
const uint32_t LED_BLINK_MS = 500;


// --------------------------- updateOLED (Giữ nguyên) -----------------------
void updateOLED(float bpm, float spo2, float tempC, bool finger, bool paused, bool modeMeasure) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("Mode: ");
    display.print(modeMeasure ? "MAX" : "TEMP");
    if (paused) {
        display.setCursor(90,0);
        display.print("PAUSED");
    }
    if (!modeMeasure) {
        display.setTextSize(2);
        display.setCursor(0, 25);
        display.print("Temp:");
        display.print(tempC, 2);
        display.print(" C");
    } else {
        display.setTextSize(2);
        if (finger) {
            display.setCursor(0, 20);
            display.print("BPM:"); display.print(bpm,0);
            display.setCursor(0,40);
            display.print("SpO2:"); display.print(spo2,0); display.print("%");
        } else {
            display.setTextSize(2);
            display.setCursor(10,25);
            display.print("NO FINGER");
        }
    }
    display.display();
}

// ============================================================
// FreeRTOS task: display (❗️ ĐÃ SỬA: Cập nhật overallAlertState)
// ============================================================
void taskDisplay(void *pvParameters){
    (void) pvParameters;
    const uint32_t displayInterval = 200;
    unsigned long lastPrintMs = 0;

    static bool fingerStateAtPause = false;
    static bool lastPausedState = false;

    for(;;){
        portENTER_CRITICAL(&mux);
        double bpm_local = displayBPM;
        float spo2_local = displaySpO2;
        float tempC = displayTempC;
        bool finger_local = fingerDetected;
        bool paused_local = isPaused;
        bool mode_local = modeMeasure;
        portEXIT_CRITICAL(&mux);

        unsigned long now = millis();
        if(now - lastPrintMs >= displayInterval && now > TIMETOBOOT){
            lastPrintMs = now;
            
            // --- Cập nhật OLED (Logic giữ nguyên) ---
            if(!mode_local){ // CHẾ ĐỘ TEMP
                lastPausedState = false;
                if(!paused_local){
                    tempSensor.requestTemperatures();
                    tempC = tempSensor.getTempCByIndex(0);
                    
                    portENTER_CRITICAL(&mux);
                    displayTempC = tempC;
                    portEXIT_CRITICAL(&mux);
                }
                updateOLED(0,0,tempC,false,paused_local,mode_local);
            }
            else{ // CHẾ ĐỘ MAX (BPM/SpO2)
                if(paused_local){
                    if (!lastPausedState) {
                        fingerStateAtPause = finger_local; 
                    }
                    updateOLED(displayBPM, displaySpO2, 0, fingerStateAtPause, paused_local, mode_local);
                    lastPausedState = true;
                } 
                else { 
                    lastPausedState = false;
                    if(finger_local){
                        updateOLED(bpm_local, spo2_local, 0, true, paused_local, mode_local);
                    } else {
                        updateOLED(0,0,0,false,paused_local,mode_local);
                    }
                }
            }

            // --- GỬI DỮ LIỆU LÊN WEBSOCKET ---
            broadcastStatus(); // Gọi hàm từ module web_server

            
            // --- SỬA LED alert (Cập nhật biến toàn cục) ---
            bool localBpmAlert = false;
            bool localSpo2Alert = false;
            bool localTempAlert = false;
            
            if(mode_local){ // Chế độ MAX
                if(bpm_local > 0 && (bpm_local < BPM_ALERT_LOW || bpm_local > BPM_ALERT_HIGH)) 
                    localBpmAlert = true;
                if(spo2_local > 0 && spo2_local < SPO2_ALERT_LOW) 
                    localSpo2Alert = true;
            } else { // Chế độ TEMP
                if(tempC > 0 && (tempC < TEMP_ALERT_LOW || tempC > TEMP_ALERT_HIGH)) 
                    localTempAlert = true;
            }

            // ❗️ TÍNH TOÁN CẢNH BÁO TỔNG THỂ
            bool anyAlert = localBpmAlert || localSpo2Alert || localTempAlert;

            // Cập nhật biến toàn cục cho web server
            portENTER_CRITICAL(&mux);
            bpmAlert = localBpmAlert;
            spo2Alert = localSpo2Alert;
            tempAlert = localTempAlert;
            overallAlertState = anyAlert; // ❗️ CẬP NHẬT BIẾN MỚI
            portEXIT_CRITICAL(&mux);

            // Logic LED (dựa trên anyAlert)
            if(anyAlert){
                if(now - ledToggleMs >= LED_BLINK_MS){
                    ledToggleMs = now;
                    ledAlertState = !ledAlertState;
                    digitalWrite(LED_ALERT_PIN, ledAlertState ? HIGH : LOW);
                }
            } else {
                digitalWrite(LED_ALERT_PIN,LOW);
                ledAlertState = false;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ============================================================
// taskMAX30102 (Giữ nguyên)
// ============================================================
void taskMAX30102(void *pvParameters) {
    (void) pvParameters;
    PPGData sample;
    bool prevFingerState = false;
    for (;;) {
        particleSensor.check();
        while (particleSensor.available()) {
            uint32_t r = particleSensor.getFIFORed();
            uint32_t i = particleSensor.getFIFOIR();
            if (i < IR_THRESHOLD) {
                if (prevFingerState) {
                    portENTER_CRITICAL(&mux);
                    fingerDetected = false;
                    portEXIT_CRITICAL(&mux);
                    resetSignalBuffers();
                }
                prevFingerState = false;
                particleSensor.nextSample();
                continue;
            }
            prevFingerState = true;
            portENTER_CRITICAL(&mux);
            fingerDetected = true;
            latestRed = r;
            latestIR = i;
            portEXIT_CRITICAL(&mux);
            if (modeMeasure && !isPaused) {
                sample.red = r;
                sample.ir = i;
                xQueueSend(dataQueue, &sample, 0);
            }
            particleSensor.nextSample();
        }
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

// ============================================================
// taskProcessing (Giữ nguyên)
// ============================================================
void taskProcessing(void *pvParameters) {
    (void) pvParameters;
    PPGData sample;
    for (;;) {
        if (xQueueReceive(dataQueue, &sample, portMAX_DELAY)) {
            double fred = (double)sample.red;
            double fir = (double)sample.ir;
            processSpO2(fred, fir);
            processBPM(fred); 
            portENTER_CRITICAL(&mux);
            displayBPM = constrain(beatsPerMinute, BPM_MIN, BPM_MAX);
            displaySpO2 = constrain(ESpO2, SPO2_MIN, SPO2_MAX);
            fingerDetected = true;
            portEXIT_CRITICAL(&mux);
        }
    }
}

// ============================================================
// taskButton (Giữ nguyên)
// ============================================================
void taskButton(void *pvParameters) {
    (void) pvParameters;
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_ALERT_PIN, OUTPUT);
    const int buzzerChannel = 0;
    ledcSetup(buzzerChannel, 2000, 8);
    ledcAttachPin(BUZZER_PIN, buzzerChannel);
    bool lastState = digitalRead(BUTTON_PIN);
    unsigned long lastDebounceMs = 0;
    unsigned long pressStartMs = 0;
    bool pressed = false;
    bool longPressTriggered = false;
    unsigned long lastClickTime = 0;
    const unsigned long DOUBLE_CLICK_MS = 400;
    int clickCount = 0;
    for (;;) {
        bool currentState = digitalRead(BUTTON_PIN);
        if (currentState != lastState) lastDebounceMs = millis();
        if ((millis() - lastDebounceMs) > BUTTON_DEBOUNCE_MS) {
            if (currentState == LOW && !pressed) {
                pressed = true;
                pressStartMs = millis();
                longPressTriggered = false;
            } else if (currentState == HIGH && pressed) {
                pressed = false;
                unsigned long now = millis();
                if (now - lastClickTime < DOUBLE_CLICK_MS) clickCount++;
                else clickCount = 1;
                lastClickTime = now;
                if (clickCount == 2) {
                    portENTER_CRITICAL(&mux);
                    modeMeasure = !modeMeasure;
                    bool modeNow = modeMeasure;
                    portEXIT_CRITICAL(&mux);
                    ledcWriteTone(buzzerChannel, 2500);
                    vTaskDelay(120 / portTICK_PERIOD_MS);
                    ledcWrite(buzzerChannel, 0);
                    vTaskDelay(80 / portTICK_PERIOD_MS);
                    ledcWriteTone(buzzerChannel, 2500);
                    vTaskDelay(120 / portTICK_PERIOD_MS);
                    ledcWrite(buzzerChannel, 0);
                    updateOLED(displayBPM, displaySpO2, displayTempC, fingerDetected, isPaused, modeNow);
                } else if (!longPressTriggered) {
                    portENTER_CRITICAL(&mux);
                    isPaused = !isPaused;
                    bool pausedNow = isPaused;
                    portEXIT_CRITICAL(&mux);
                    ledcWriteTone(buzzerChannel, 2000);
                    vTaskDelay(80 / portTICK_PERIOD_MS);
                    ledcWrite(buzzerChannel, 0);
                    if (!pausedNow) {
                        updateOLED(displayBPM, displaySpO2, displayTempC, fingerDetected, pausedNow, modeMeasure);
                    }
                }
            }
            if (pressed && !longPressTriggered) {
                if ((millis() - pressStartMs) >= BUTTON_LONGPRESS_MS) {
                    longPressTriggered = true;
                    display.clearDisplay();
                    display.setTextSize(2);
                    display.setTextColor(WHITE);
                    display.setCursor(10, 25);
                    display.print("RESTART !");
                    display.display();
                    ledcWriteTone(buzzerChannel, 3000);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                    ledcWrite(buzzerChannel, 0);
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    ledcWriteTone(buzzerChannel, 3000);
                    vTaskDelay(150 / portTICK_PERIOD_MS);
                    ledcWrite(buzzerChannel, 0);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    ESP.restart();
                }
            }
        }
        lastState = currentState;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ============================================================
// setup (Giữ nguyên)
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println(">> Booting...");
    setupWebServer();
    Wire.begin(21, 22);
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("⛔ MAX30102 FAIL!");
        while (1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
    }
    particleSensor.setup(0x7F, 4, 2, 200, 411, 16384);
    particleSensor.enableDIETEMPRDY();
    tempSensor.begin();
    delay(200);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
            Serial.println("⛔ OLED FAIL!");
            while (1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
        }
    }
    dataQueue = xQueueCreate(64, sizeof(PPGData));
    xTaskCreatePinnedToCore(taskMAX30102, "PPGRead", 2*1024, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskProcessing, "Processing", 4*1024, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskDisplay, "Display", 4*1024, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskButton, "Button", 2*1024, NULL, 1, NULL, 0);
}

// ============================================================
void loop() {
    // Hoàn toàn được xử lý bởi FreeRTOS
}