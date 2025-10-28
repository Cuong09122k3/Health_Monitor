# Health Monitor System with WiFi & Web Interface

Hệ thống giám sát sức khỏe sử dụng ESP32 với MAX30102 (BPM/SpO2), DS18B20 (nhiệt độ), màn hình OLED, và giao diện web điều khiển từ xa.

## Tính năng chính

### Phần cứng
- **ESP32**: Vi điều khiển chính
- **MAX30102**: Cảm biến nhịp tim và SpO2
- **DS18B20**: Cảm biến nhiệt độ
- **OLED 128x64**: Hiển thị dữ liệu
- **Button (GPIO27)**: Điều khiển trên thiết bị
- **Buzzer (GPIO25)**: Cảnh báo âm thanh
- **LED (GPIO26)**: Cảnh báo trực quan

### Chức năng
- ✅ Đo nhịp tim (BPM) và nồng độ oxy máu (SpO2)
- ✅ Đo nhiệt độ cơ thể
- ✅ Phát hiện ngón tay tự động
- ✅ Giao diện web real-time
- ✅ Điều khiển từ xa (Pause/Resume, Đổi mode, Restart)
- ✅ Cảnh báo khi giá trị bất thường
- ✅ Đa tasking với FreeRTOS

## Cấu hình WiFi

Trước khi sử dụng, bạn cần cấu hình thông tin WiFi trong file `src/main.cpp`:

```cpp
const char* ssid = "YOUR_WIFI_SSID";      // Thay đổi SSID WiFi
const char* password = "YOUR_WIFI_PASSWORD";  // Thay đổi mật khẩu
```

## Giao diện Web

### Truy cập
Sau khi ESP32 kết nối WiFi thành công, mở trình duyệt và truy cập:
```
http://<IP_ESP32>
```
IP của ESP32 sẽ được hiển thị trên Serial Monitor.

### Tính năng Web
1. **Hiển thị Real-time**: Cập nhật tự động mỗi 500ms
   - Nhịp tim (BPM)
   - SpO2 (%)
   - Nhiệt độ (°C)
   - Trạng thái hệ thống
   - Phát hiện ngón tay

2. **Điều khiển**:
   - **Pause/Resume**: Tạm dừng/tiếp tục đo
   - **Switch Mode**: Chuyển giữa chế độ PPG (BPM/SpO2) và TEMP
   - **Restart**: Khởi động lại hệ thống

3. **Cảnh báo**: Hiển thị cảnh báo khi:
   - BPM < 50 hoặc > 120
   - SpO2 < 94%
   - Nhiệt độ < 36°C hoặc > 37.8°C

## Điều khiển bằng nút vật lý

| Thao tác | Chức năng |
|----------|-----------|
| **Nháy đơn** | Pause/Resume |
| **Nháy đúp** | Đổi mode (PPG ↔ TEMP) |
| **Giữ 2 giây** | Restart hệ thống |

## API Endpoints

### GET `/api/data`
Lấy dữ liệu hệ thống dưới dạng JSON:
```json
{
  "bpm": 72.5,
  "spo2": 98.0,
  "temp": 36.5,
  "finger": true,
  "paused": false,
  "mode": "MAX",
  "alert": false,
  "alertMsg": ""
}
```

### POST `/api/pause`
Tạm dừng hệ thống

### POST `/api/resume`
Tiếp tục hệ thống

### POST `/api/switchmode`
Đổi chế độ đo

### POST `/api/restart`
Khởi động lại ESP32

## Cài đặt

1. Cài đặt PlatformIO
2. Clone repository này
3. Cấu hình WiFi trong `src/main.cpp`
4. Upload code lên ESP32
5. Mở Serial Monitor để xem IP
6. Truy cập web interface

## Thư viện sử dụng

- SparkFun MAX3010x Library
- arduinoFFT
- Adafruit GFX & SSD1306
- DallasTemperature & OneWire
- AsyncTCP
- ESPAsyncWebServer
- ArduinoJson

## Lưu ý

- Đảm bảo ESP32 và các cảm biến được kết nối đúng chân
- Khi không có ngón tay, hệ thống sẽ không đo và reset buffers
- Các ngưỡng cảnh báo có thể được điều chỉnh trong code
- Web interface tự động làm mới dữ liệu mỗi 500ms

## License

MIT License
