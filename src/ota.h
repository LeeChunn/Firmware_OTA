#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <esp_system.h>
#include <PPP.h>

// Định nghĩa thông tin WiFi
#define WIFI_SSID "ROUTERCLUB"
#define WIFI_PASSWORD "1234567890"

// Định nghĩa thông tin modem PPP
#define PPP_MODEM_APN "v-internet"
#define PPP_MODEM_RST 33
#define PPP_MODEM_TX 18
#define PPP_MODEM_RX 17
#define PPP_MODEM_FC ESP_MODEM_FLOW_CONTROL_NONE
#define PPP_MODEM_MODEL PPP_MODEM_GENERIC

// Địa chỉ firmware OTA
#define FIRMWARE_URL "https://raw.githubusercontent.com/LeeChunn/Firmware_OTA/main/.pio/build/esp32-s3-devkitm-1/firmware.bin"

// Biến RTC Memory để đếm số lần reset
extern RTC_DATA_ATTR int bootCount;

// Hàm khởi động WiFi
void ota_init_wifi(void);

// Hàm khởi động modem PPP
void ota_init_ppp(void);

// Kiểm tra lý do reset và xử lý rollback
void ota_check_reset_reason(void);

// Cập nhật OTA (chung cho WiFi & 4G)
void ota_perform_update(bool useWiFi);

// Rollback firmware nếu cần
void ota_rollback(void);

#endif
