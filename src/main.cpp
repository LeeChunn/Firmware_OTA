// OTA https wifi:

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

// WiFi credentials
const char* ssid = "ROUTERCLUB";
const char* password = "1234567890";

// Github firmware URL
const char* firmware_url = "https://raw.githubusercontent.com/son-dohoang/ota_firmware/main/firmware.bin";
// const char* firmware_url = "https://raw.githubusercontent.com/LeeChunn/Firmware_OTA/main/.pio/build/esp32-s3-devkitm-1/firmware.bin";

// Thêm biến toàn cục để theo dõi thời gian
unsigned long updateStartTime = 0;

// Sửa lại các hàm callback
void update_started() {
    updateStartTime = millis();  // Lưu thời điểm bắt đầu
    Serial.println("\nBắt đầu quá trình cập nhật OTA");
}

void update_finished() {
     unsigned long duration = (millis() - updateStartTime) / 1000;  // Tính thời gian (giây)
    Serial.printf("\nHoàn thành cập nhật OTA sau %lu giây\n", duration);
}

void update_progress(int cur, int total) {
    float progress = (float)cur / total * 100;
    unsigned long duration = (millis() - updateStartTime) / 1000;
    Serial.printf("Tiến độ: %.1f%% (%d/%d bytes) - Thời gian: %lu giây\r", 
                 progress, cur, total, duration);
}

void update_error(int err) {
    unsigned long duration = (millis() - updateStartTime) / 1000;
    Serial.printf("\nLỗi cập nhật OTA (Mã lỗi %d) sau %lu giây\n", err, duration);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting OTA Update Example");
    while(1);
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Wait for connection
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure();   // Skip verification
        
        // Configure update callbacks
        httpUpdate.onStart(update_started);
        httpUpdate.onEnd(update_finished);
        httpUpdate.onProgress(update_progress);
        httpUpdate.onError(update_error);

        Serial.println("\nKiểm tra bản cập nhật firmware...");
        t_httpUpdate_return ret = httpUpdate.update(client, firmware_url);

        switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("Cập nhật thất bại (Mã lỗi %d): %s\n", 
                       httpUpdate.getLastError(), 
                       httpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("Không có bản cập nhật mới");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("Cập nhật thành công");
            break;
        }
    } else {
        Serial.println("Mất kết nối WiFi");
        // Thử kết nối lại
        WiFi.begin(ssid, password);
    }
    delay(30000); // Đợi 30 giây trước khi kiểm tra lại
}