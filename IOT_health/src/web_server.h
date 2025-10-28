// ============================================================
// web_server.h (Đã cập nhật extern)
// ============================================================

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h> // Cần cho portMUX_TYPE

// --- Khai báo các biến chia sẻ (dùng extern) ---
extern portMUX_TYPE mux;
extern volatile bool isPaused; 
extern volatile bool modeMeasure; 
extern double displayBPM;
extern float displaySpO2;
extern float displayTempC;
extern bool fingerDetected;

// ❗️ THÊM CÁC BIẾN CẢNH BÁO
extern bool bpmAlert;
extern bool spo2Alert;
extern bool tempAlert;
extern bool overallAlertState; // ❗️ BIẾN CẢNH BÁO TỔNG THỂ MỚI

// --- Khai báo các hàm public ---
void setupWebServer();
void broadcastStatus();

#endif