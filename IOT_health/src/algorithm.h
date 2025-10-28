// ============================================================
// algorithm.h
// Khai báo các biến và hàm liên quan đến thuật toán BPM và SpO2
// ============================================================
#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <Arduino.h>
#include "arduinoFFT.h" // Cần khai báo để dùng ArduinoFFT

// ================= constants (giữ nguyên) ======================
#define IR_THRESHOLD 30000
#define PULSE_SAMPLES 256
#define SAMPLE_FREQ 50 // Đã sửa
const int Num = 100;
// ============================================================

// Khai báo biến toàn cục (extern)
extern double redArray[PULSE_SAMPLES];
extern double vReal[PULSE_SAMPLES];
extern double vImag[PULSE_SAMPLES];
extern double beatsPerMinute;
extern int idx_fft;

extern double avered, aveir;
extern double sumirrms, sumredrms;
extern int idx_spo2;
extern float ESpO2;
extern bool spo2_valid;

// Physiological thresholds (extern)
extern const double BPM_MIN;
extern const double BPM_MAX;
extern const float SPO2_MIN;
extern const float SPO2_MAX;

// Khai báo instance của thư viện
extern arduinoFFT FFT;

// Khai báo hàm
void processSpO2(double fred, double fir);
void processBPM(double fred);
void resetSignalBuffers();

#endif // ALGORITHM_H