// OTA HTTPS WiFi với rollback nếu reset nhiều lần

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <esp_system.h>
#include <PPP.h>

// WiFi credentials
const char* ssid = "ROUTERCLUB";
const char* password = "1234567890";

// Link OTA firmware
const char* firmware_url = "https://raw.githubusercontent.com/LeeChunn/Firmware_OTA/main/.pio/build/esp32-s3-devkitm-1/firmware.bin";

// PPP Modem Configuration
#define PPP_MODEM_APN "v-internet"
#define PPP_MODEM_RST 33
#define PPP_MODEM_TX 18
#define PPP_MODEM_RX 17
#define PPP_MODEM_FC ESP_MODEM_FLOW_CONTROL_NONE
#define PPP_MODEM_MODEL PPP_MODEM_GENERIC

// Chân 46
#define PIN_46 46

// Biến theo dõi thời gian cập nhật
unsigned long updateStartTime = 0;

// Biến RTC Memory để đếm số lần reset
RTC_DATA_ATTR int bootCount = 0;

void check_reset_reason();
void update_started();
void update_finished();
void update_progress(int cur, int total);
void update_error(int err);
void handle_uart_command();
void perform_ota_update();
void perform_ota_update_ppp();
void onPPPEvent(arduino_event_id_t event, arduino_event_info_t info);

void check_reset_reason() {
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.print("ESP32 Reset Reason: ");
    switch (reason) {
        case ESP_RST_POWERON:
            Serial.println("Power-On Reset");
            break;
        case ESP_RST_EXT:
            Serial.println("External Reset (nút reset)");
            break;
        case ESP_RST_SW:
            Serial.println("Software Reset (ESP.restart())");
            break;
        case ESP_RST_PANIC:
            Serial.println("Panic Reset (Lỗi CPU, watchdog timeout)");
            break;
        case ESP_RST_INT_WDT:
            Serial.println("Interrupt Watchdog Reset");
            break;
        case ESP_RST_TASK_WDT:
            Serial.println("Task Watchdog Reset");
            break;
        case ESP_RST_WDT:
            Serial.println("Other Watchdog Reset");
            break;
        case ESP_RST_DEEPSLEEP:
            Serial.println("Wake-up từ Deep Sleep");
            break;
        case ESP_RST_BROWNOUT:
            Serial.println("Brownout Reset (nguồn điện yếu)");
            break;
        case ESP_RST_SDIO:
            Serial.println("Reset từ SDIO");
            break;
        default:
            Serial.println("Không xác định");
            break;
    }
}

// Callback OTA
void update_started() {
    updateStartTime = millis();
    Serial.println("\nBắt đầu quá trình cập nhật OTA");
}

void update_finished() {
    unsigned long duration = (millis() - updateStartTime) / 1000;
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
    Serial.println("Khôi phục firmware cũ...");
    Update.rollBack();
    ESP.restart();
}

void onPPPEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_PPP_START:
            Serial.println("PPP Started");
            break;
        case ARDUINO_EVENT_PPP_CONNECTED:
            Serial.println("PPP Connected");
            break;
        case ARDUINO_EVENT_PPP_GOT_IP:
            Serial.println("PPP Got IP");
            break;
        case ARDUINO_EVENT_PPP_LOST_IP:
            Serial.println("PPP Lost IP");
            break;
        case ARDUINO_EVENT_PPP_DISCONNECTED:
            Serial.println("PPP Disconnected");
            break;
        case ARDUINO_EVENT_PPP_STOP:
            Serial.println("PPP Stopped");
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Bắt đầu chương trình ESP32 OTA");
    Serial.println("======================V2.0======================");

    // Kiểm tra lý do reset
    check_reset_reason();

    // Đếm số lần khởi động lại
    bootCount++;
    Serial.printf("ESP32 đã khởi động lại %d lần\n", bootCount);

    // Nếu thiết bị reset liên tục >3 lần sau khi cập nhật → rollback firmware
    if (bootCount > 3) {
        Serial.println("ESP32 reset quá nhiều lần, rollback firmware...");
        Update.rollBack();
        ESP.restart();
    }

    // Kết nối WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Đang kết nối WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nKết nối WiFi thành công!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());

    // Initialize PPP modem
    pinMode(PPP_MODEM_RST, OUTPUT);
    digitalWrite(PPP_MODEM_RST, HIGH);
    pinMode(PIN_46, OUTPUT);
    digitalWrite(PIN_46, HIGH);

    Network.onEvent(onPPPEvent);
    PPP.setApn(PPP_MODEM_APN);
    PPP.setPins(PPP_MODEM_TX, PPP_MODEM_RX);

    Serial.println("Starting modem...");
    if (!PPP.begin(PPP_MODEM_MODEL)) {
        Serial.println("Failed to start modem!");
        return;
    }

    // Wait for network attachment
    bool attached = PPP.attached();
    if (!attached) {
        Serial.print("Waiting for network");
        int attempts = 0;
        while (!attached && attempts < 60) {
            Serial.print(".");
            delay(1000);
            attached = PPP.attached();
            attempts++;
        }
        Serial.println();
    }

    if (attached) {
        Serial.println("Network attached! Switching to data mode...");
        PPP.mode(ESP_MODEM_MODE_DATA);
        if (PPP.waitStatusBits(ESP_NETIF_CONNECTED_BIT, 10000)) {
            Serial.println("Connected to internet!");
        } else {
            Serial.println("Failed to connect to internet!");
        }
    } else {
        Serial.println("Failed to attach to network!");
    }
}

void handle_uart_command() {
    if (Serial.available() > 0) {
        char command = Serial.read();
        if (command == 'U') {
            Serial.println("Nhận lệnh cập nhật OTA từ UART");
            perform_ota_update();
        } else if (command == 'R') {
            Serial.println("Nhận lệnh rollback firmware từ UART");
            Update.rollBack();
            ESP.restart();
        }
    }
}

void perform_ota_update() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure();  // Bỏ qua xác minh chứng chỉ SSL

        // Cấu hình callback OTA
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
                bootCount = 0; // Reset bộ đếm nếu update thành công
                break;
        }
    } else {
        Serial.println("Mất kết nối WiFi, thử kết nối lại...");
        WiFi.begin(ssid, password);
    }
}

void perform_ota_update_ppp() {
    if (PPP.connected()) {
        NetworkClientSecure client;
        client.setInsecure(); // Skip SSL verification

        httpUpdate.onStart(update_started);
        httpUpdate.onEnd(update_finished);
        httpUpdate.onProgress(update_progress);
        httpUpdate.onError(update_error);
        Serial.println("Checking for firmware updates...");
        t_httpUpdate_return ret = httpUpdate.update(client, "https://raw.githubusercontent.com/son-dohoang/ota_firmware/main/firmware.bin");

        switch (ret) {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n",
                              httpUpdate.getLastError(),
                              httpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                Serial.println("HTTP_UPDATE_OK");
                break;
        }
    } else {
        Serial.println("Not connected to internet");
    }
}

void loop() {
    handle_uart_command();
    delay(100); // Đợi một chút trước khi kiểm tra lệnh UART tiếp theo

    // Check for updates via PPP every 30 seconds
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 30000) {
        lastCheck = millis();
        perform_ota_update_ppp();
    }
}
