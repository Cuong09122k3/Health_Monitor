// ============================================================
// algorithm.cpp
// Định nghĩa các biến và hàm thuật toán BPM và SpO2
// ============================================================

#include "algorithm.h"
#include <math.h> // Cần cho sqrt()

// Khởi tạo các biến toàn cục
double redArray[PULSE_SAMPLES];
double vReal[PULSE_SAMPLES];
double vImag[PULSE_SAMPLES];
double beatsPerMinute = 0;
double lastBPM = 75.0; 
int idx_fft = 0;

double avered = 0.0, aveir = 0.0;
double sumirrms = 0.0, sumredrms = 0.0;
int idx_spo2 = 0;
float ESpO2 = 97.0;
float lastSpO2 = 97.0; 
bool spo2_valid = false;
double frate = 0.95;
double FSpO2 = 0.7;

// Physiological thresholds
const double BPM_MIN = 40.0;
const double BPM_MAX = 180.0;
const float SPO2_MIN = 80.0;
const float SPO2_MAX = 100.0;

// Khởi tạo instance của thư viện
arduinoFFT FFT;

// ============================================================
// SpO2 (giữ nguyên thuật toán)
// ============================================================
void processSpO2(double fred, double fir) {
    if (fir < IR_THRESHOLD) {
        idx_spo2 = 0;
        sumredrms = 0.0;
        sumirrms = 0.0;
        spo2_valid = false;
        avered = 0.0;
        aveir = 0.0;
        return;
    }

    if (avered <= 0.0) avered = fred;
    if (aveir <= 0.0) aveir = fir; // Đã sửa lỗi ký tự lỗi

    avered = (frate * avered) + ((1.0 - frate) * fred);
    aveir = (frate * aveir) + ((1.0 - frate) * fir); // Đã sửa lỗi ký tự lỗi

    sumredrms += (fred - avered) * (fred - avered);
    sumirrms += (fir - aveir) * (fir - aveir);

    idx_spo2++;
    if (idx_spo2 >= Num) {
        if (sumredrms > 0 && sumirrms > 0 && avered > 0 && aveir > 0) {
            double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
            double SpO2_calc = 110.0 - 25.0 * R;
            SpO2_calc = constrain(SpO2_calc, 50.0, 100.0);
            ESpO2 = (FSpO2 * ESpO2) + (1.0 - FSpO2) * SpO2_calc;
            spo2_valid = true;
            lastSpO2 = ESpO2;
        }
        idx_spo2 = 0;
        sumredrms = 0.0;
        sumirrms = 0.0;
    }
}

// ============================================================
// BPM (FFT) (giữ nguyên thuật toán)
// ============================================================
void processBPM(double fred) {
    redArray[idx_fft] = fred;
    idx_fft++;

    // Chỉ in fred ra Serial (dành cho plot)
    Serial.println(fred);

    if (idx_fft >= PULSE_SAMPLES) {
        idx_fft = 0;
        for (int i = 0; i < PULSE_SAMPLES; i++) {
            vReal[i] = redArray[i];
            vImag[i] = 0.0;
        }
        FFT = arduinoFFT(vReal, vImag, PULSE_SAMPLES, SAMPLE_FREQ);
        FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.Compute(FFT_FORWARD);
        FFT.ComplexToMagnitude();
        double peak = FFT.MajorPeak();
        if (peak > 0.0) {
            beatsPerMinute = peak * 60.0;
        }
    }
}

// ============================================================
// reset FFT & SpO2 accumulators
// ============================================================
void resetSignalBuffers() {
    idx_fft = 0;
    for (int i = 0; i < PULSE_SAMPLES; i++) {
        redArray[i] = 0.0;
        vReal[i] = 0.0;
        vImag[i] = 0.0;
    }
    idx_spo2 = 0;
    sumredrms = 0.0;
    sumirrms = 0.0;
    avered = 0.0;
    aveir = 0.0;
}