#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>

const char *ssid = "ROUTERCLUB";                                                                      // Thay bằng SSID WiFi của bạn
const char *password = "1234567890";                                                                  // Thay bằng mật khẩu WiFi của bạn
const char *fileURL = "https://raw.githubusercontent.com/LeeChunn/Firmware_OTA/main/.pio/build/esp32-s3-devkitm-1/firmware.bin"; // URL của file cần tải
const char *filePath = "/CN/firmware1.bin";                                                               // Đường dẫn lưu file trong SPIFFS

unsigned long downloadStartTime = 0;

void listSPIFFSFiles()
{
    Serial.println("\n📂 Danh sách file trong SPIFFS:");
    File root = SPIFFS.open("/");
    if (!root)
    {
        Serial.println("⚠ Lỗi mở SPIFFS!");
        return;
    }
    File file = root.openNextFile();
    while (file)
    {
        Serial.printf("📄 File: %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
}

bool downloadFile(const char *url, const char *path)
{
    Serial.printf("📥 Đang tải file từ: %s\n", url);

    WiFiClientSecure client;
    client.setInsecure(); // Bỏ kiểm tra chứng chỉ SSL

    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        File file = SPIFFS.open(path, FILE_WRITE);
        if (!file)
        {
            Serial.println("⚠ Lỗi mở file để ghi!");
            http.end();
            return false;
        }

        WiFiClient *stream = http.getStreamPtr();
        uint8_t buffer[512];
        int totalBytes = 0;
        int len = http.getSize();
        downloadStartTime = millis();

        while (http.connected() && (totalBytes < len || len == -1))
        {
            int bytesRead = stream->read(buffer, sizeof(buffer));
            if (bytesRead > 0)
            {
                file.write(buffer, bytesRead);
                totalBytes += bytesRead;
                float progress = (float)totalBytes / len * 100;
                unsigned long duration = (millis() - downloadStartTime) / 1000;
                Serial.printf("Tiến độ: %.1f%% (%d/%d bytes) - Thời gian: %lu giây\r", 
                              progress, totalBytes, len, duration);
            }
        }
        file.close();
        http.end();
        unsigned long duration = (millis() - downloadStartTime) / 1000;
        Serial.printf("\n✅ Tải file thành công, đã lưu %d bytes vào %s sau %lu giây\n", totalBytes, path, duration);
        return true;
    }
    else
    {
        unsigned long duration = (millis() - downloadStartTime) / 1000;
        Serial.printf("\n❌ Lỗi tải file! HTTP Code: %d sau %lu giây\n", httpCode, duration);
        http.end();
        return false;
    }
}

void setup()
{
    Serial.begin(115200);

    if (!SPIFFS.begin(true))
    {
        Serial.println("⚠ Lỗi mount SPIFFS!");
        return;
    }

    listSPIFFSFiles();
    
    WiFi.begin(ssid, password);
    Serial.print("🔗 Kết nối WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi đã kết nối!");

    if (downloadFile(fileURL, filePath))
    {
        listSPIFFSFiles();
    }
}

void loop()
{
}
