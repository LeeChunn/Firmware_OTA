#include "ota.h"

// Biến RTC để đếm số lần reset
RTC_DATA_ATTR int bootCount = 0;
// Callback OTA
void update_started() {
    Serial.println("\nBắt đầu cập nhật OTA...");
}
void update_finished() {
    Serial.println("\nHoàn thành cập nhật OTA.");
}
void update_progress(int cur, int total) {
    Serial.printf("Tiến độ: %d/%d bytes (%.1f%%)\r", cur, total, ((float)cur / total) * 100);
}
void update_error(int err) {
    Serial.printf("\nLỗi cập nhật OTA (Mã lỗi %d)\n", err);
    ota_rollback();
}

// Khởi tạo WiFi
void ota_init_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Đang kết nối WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi đã kết nối!");
}

// Khởi tạo modem PPP
void ota_init_ppp() {
    pinMode(PPP_MODEM_RST, OUTPUT);
    digitalWrite(PPP_MODEM_RST, HIGH);

    Network.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
        if (event == ARDUINO_EVENT_PPP_CONNECTED) {
            Serial.println("PPP Modem đã kết nối!");
        }
    });

    PPP.setApn(PPP_MODEM_APN);
    PPP.setPins(PPP_MODEM_TX, PPP_MODEM_RX);

    if (!PPP.begin(PPP_MODEM_MODEL)) {
        Serial.println("Lỗi khởi động modem PPP!");
    }
}

// Kiểm tra lý do reset & rollback nếu cần
void ota_check_reset_reason() {
    bootCount++;
    Serial.printf("ESP32 đã reset %d lần\n", bootCount);

    if (bootCount > 3) {
        Serial.println("ESP32 reset quá nhiều lần, rollback firmware...");
        ota_rollback();
    }
}

// Hàm OTA chung (WiFi hoặc 4G)
void ota_perform_update(bool useWiFi) {
    if (useWiFi && WiFi.status() == WL_CONNECTED) {
        Serial.println("Dùng WiFi để cập nhật OTA...");
        WiFiClientSecure client;
        client.setInsecure();
        httpUpdate.onStart(update_started);
        httpUpdate.onEnd(update_finished);
        httpUpdate.onProgress(update_progress);
        httpUpdate.onError(update_error);
        httpUpdate.update(client, FIRMWARE_URL);
    } 
    else if (!useWiFi && PPP.connected()) {
        Serial.println("Dùng 4G để cập nhật OTA...");
        NetworkClientSecure client;
        client.setInsecure();
        httpUpdate.onStart(update_started);
        httpUpdate.onEnd(update_finished);
        httpUpdate.onProgress(update_progress);
        httpUpdate.onError(update_error);
        httpUpdate.update(client, FIRMWARE_URL);
    } 
    else {
        Serial.println("Không có kết nối mạng!");
    }
}

// Rollback firmware
void ota_rollback() {
    Update.rollBack();
    ESP.restart();
}
