#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>

#define WIFI_SSID "ROUTERCLUB"
#define WIFI_PASS "1234567890"

// URL tải firmware OTA
#define FIRMWARE_URL "https://raw.githubusercontent.com/son-dohoang/ota_firmware/main/firmware.bin"
const char *baseUrl = "https://raw.githubusercontent.com/LeeChunn/Firmware_OTA/main/data/MN/Number/";

// Danh sách file cần tải
const char *fileList[] = {"0.mp3", "1.mp3", "2.mp3", "3.mp3", "4.mp3", "5.mp3", "6.mp3", "7.mp3", "8.mp3", "9.mp3", "Switch_Sound.mp3", "danhan.mp3", "lam.mp3", "linh.mp3", "mot.mp3", "muoi.mp3", "nghin.mp3", "tram.mp3", "trieu.mp3", "tu.mp3"};
const size_t numFiles = sizeof(fileList) / sizeof(fileList[0]);

WiFiClientSecure client; // Dùng HTTPS

void setup()
{
    Serial.begin(115200);
    Serial.println("\n\nESP32 SPIFFS OTA with HTTPS\n");

    // Kết nối WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected!");

    // Khởi tạo SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    // Bỏ qua kiểm tra chứng chỉ HTTPS
    client.setInsecure();
}

// Tải file firmware OTA
void downloadFirmware()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected!");
        return;
    }

    HTTPClient http;
    http.begin(client, FIRMWARE_URL);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
        return;
    }

    File file = SPIFFS.open("/firmware.bin", FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    uint8_t buff[512];
    int totalSize = http.getSize();
    int downloadedSize = 0;

    Serial.println("Downloading firmware...");
    while (http.connected() && (downloadedSize < totalSize || totalSize == -1))
    {
        size_t size = stream->available();
        if (size)
        {
            int c = stream->readBytes(buff, min(size, sizeof(buff)));
            file.write(buff, c);
            downloadedSize += c;
            Serial.printf("Downloaded: %d%%\n", (downloadedSize * 100) / totalSize);
        }
        delay(1);
    }

    file.close();
    http.end();
    Serial.println("Firmware download complete.");
}

// Tải và lưu file vào SPIFFS
void downloadAndSaveFile(const char *fileName)
{
    String url = String(baseUrl) + fileName;
    String filePath = "/" + String(fileName); // Lưu vào thư mục gốc

    Serial.printf("Downloading %s to %s\n", url.c_str(), filePath.c_str());

    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP GET failed for %s, error: %s\n", fileName, http.errorToString(httpCode).c_str());
        http.end();
        return;
    }

    File file = SPIFFS.open(filePath, FILE_WRITE);
    if (!file)
    {
        Serial.printf("Failed to open file %s for writing\n", filePath.c_str());
        http.end();
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    uint8_t buffer[512];
    int len;
    while (http.connected() && (len = stream->readBytes(buffer, sizeof(buffer))) > 0)
    {
        file.write(buffer, len);
    }
    file.close();
    http.end();

    Serial.printf("Downloaded %s successfully.\n", fileName);
}

// Liệt kê file trong SPIFFS
void listSPIFFS()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    while (file)
    {
        Serial.printf("FILE: %s, SIZE: %d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }
}

// Xóa tất cả file trong SPIFFS
void deleteAllFiles()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    while (file)
    {
        String fileName = file.name();
        Serial.printf("Deleting: %s\n", fileName.c_str());

        file.close(); // Đảm bảo file đóng trước khi xóa
        delay(10);

        if (SPIFFS.remove(fileName.c_str()))
        {
            Serial.printf("Deleted: %s\n", fileName.c_str());
        }
        else
        {
            Serial.printf("Failed to delete: %s\n", fileName.c_str());
        }

        file = root.openNextFile();
    }

    Serial.println("All files deleted.");
}

// Format SPIFFS
void formatSPIFFS()
{
    Serial.println("Formatting SPIFFS...");
    if (SPIFFS.format())
    {
        Serial.println("SPIFFS formatted successfully.");
    }
    else
    {
        Serial.println("SPIFFS format failed.");
    }
}

void loop()
{
    if (Serial.available())
    {
        char input = Serial.read();
        if (input == 'F')
        {
            formatSPIFFS();
        }
        else if (input == 'U')
        {
            downloadFirmware();
        }
        else if (input == 'L')
        {
            listSPIFFS();
        }
        else if (input == 'D')
        {
            deleteAllFiles();
        }
        else if (input == 'C')
        {
            for (size_t i = 0; i < numFiles; i++)
            {
                downloadAndSaveFile(fileList[i]);
            }
        }
    }
    delay(1000);
}
