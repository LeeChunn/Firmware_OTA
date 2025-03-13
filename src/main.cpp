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
const char *fileList[] = {
  "0.mp3", "1.mp3", "2.mp3", "3.mp3", "4.mp3", "5.mp3", "6.mp3", "7.mp3", "8.mp3", "9.mp3",
  "Switch_Sound.mp3", "danhan.mp3", "lam.mp3", "linh.mp3", "mot.mp3", "muoi.mp3", "nghin.mp3",
  "tram.mp3", "trieu.mp3", "tu.mp3"
};
const size_t numFiles = sizeof(fileList) / sizeof(fileList[0]);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP32 SPIFFS OTA with HTTPS\n");

  // Kết nối WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");

  // Khởi tạo SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed!");
    return;
  }
}

void downloadFirmware() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }

  // Tạo đối tượng WiFiClientSecure cục bộ và bỏ qua chứng chỉ SSL
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  Serial.printf("Starting firmware download from: %s\n", FIRMWARE_URL);
  http.begin(client, FIRMWARE_URL);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  File file = SPIFFS.open("/firmware.bin", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buff[512];
  int totalSize = http.getSize();
  int downloadedSize = 0;
  unsigned long startTime = millis();

  Serial.println("Downloading firmware...");
  while (http.connected() && (downloadedSize < totalSize || totalSize == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buff, min(size, sizeof(buff)));
      file.write(buff, c);
      downloadedSize += c;

      unsigned long currentTime = millis();
      float elapsedTime = (currentTime - startTime) / 1000.0; // in seconds
      float downloadPercentage = (totalSize > 0) ? (downloadedSize * 100.0 / totalSize) : 0;
      Serial.printf("Time: %.2f s, Downloaded: %d/%d bytes (%.2f%%)\n", elapsedTime, downloadedSize, totalSize, downloadPercentage);
    }
    delay(1);
  }

  file.close();
  http.end();
  Serial.println("Firmware download complete.");
}

void downloadAndSaveFile(const char *fileName) {
  String url = String(baseUrl) + fileName;
  String filePath = "/" + String(fileName); // Lưu vào thư mục gốc

  Serial.printf("Downloading %s to %s\n", url.c_str(), filePath.c_str());

  // Tạo đối tượng WiFiClientSecure cục bộ và bỏ qua chứng chỉ SSL
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed for %s, error: %s\n", fileName, http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  File file = SPIFFS.open(filePath, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open file %s for writing\n", filePath.c_str());
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[512];
  int totalSize = http.getSize();
  int downloadedSize = 0;
  unsigned long startTime = millis();

  Serial.println("Downloading file...");
  while (http.connected() && (downloadedSize < totalSize || totalSize == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buffer, min(size, sizeof(buffer)));
      file.write(buffer, c);
      downloadedSize += c;

      unsigned long currentTime = millis();
      float elapsedTime = (currentTime - startTime) / 1000.0;
      float downloadPercentage = (totalSize > 0) ? (downloadedSize * 100.0 / totalSize) : 0;
      Serial.printf("Time: %.2f s, Downloaded: %d/%d bytes (%.2f%%)\n", elapsedTime, downloadedSize, totalSize, downloadPercentage);
    }
    delay(1);
  }

  file.close();
  http.end();
  Serial.println("File download complete.");
}

void downloadAndSaveFileCustom(const String &url, const String &filePath) {
  Serial.printf("Downloading %s to %s\n", url.c_str(), filePath.c_str());

  // Tạo đối tượng WiFiClientSecure cục bộ và bỏ qua chứng chỉ SSL
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed for %s, error: %s\n", url.c_str(), http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  File file = SPIFFS.open(filePath, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open file %s for writing\n", filePath.c_str());
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[512];
  int totalSize = http.getSize();
  int downloadedSize = 0;
  unsigned long startTime = millis();

  Serial.println("Downloading file...");
  while (http.connected() && (downloadedSize < totalSize || totalSize == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buffer, min(size, sizeof(buffer)));
      file.write(buffer, c);
      downloadedSize += c;

      unsigned long currentTime = millis();
      float elapsedTime = (currentTime - startTime) / 1000.0;
      float downloadPercentage = (totalSize > 0) ? (downloadedSize * 100.0 / totalSize) : 0;
      Serial.printf("Time: %.2f s, Downloaded: %d/%d bytes (%.2f%%)\n", elapsedTime, downloadedSize, totalSize, downloadPercentage);
    }
    delay(1);
  }

  file.close();
  http.end();
  Serial.println("File download complete.");
}

void listSPIFFS() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("FILE: %s, SIZE: %d bytes\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

void deleteAllFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    Serial.printf("Deleting: %s\n", fileName.c_str());
    file.close(); // Đảm bảo file đóng trước khi xóa
    delay(10);
    if (SPIFFS.remove(fileName.c_str())) {
      Serial.printf("Deleted: %s\n", fileName.c_str());
    } else {
      Serial.printf("Failed to delete: %s\n", fileName.c_str());
    }
    file = root.openNextFile();
  }
  Serial.println("All files deleted.");
}

void formatSPIFFS() {
  Serial.println("Formatting SPIFFS...");
  if (SPIFFS.format()) {
    Serial.println("SPIFFS formatted successfully.");
  } else {
    Serial.println("SPIFFS format failed.");
  }
}

void loop() {
  if (Serial.available()) {
    // Đọc toàn bộ chuỗi đến khi có ký tự xuống dòng
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;
    
    // Lệnh đơn ký tự
    if (input == "F") {
      formatSPIFFS();
    }
    else if (input == "U") {
      downloadFirmware();
    }
    else if (input == "L") {
      listSPIFFS();
    }
    else if (input == "D") {
      deleteAllFiles();
    }
    else if (input == "C") {
      for (size_t i = 0; i < numFiles; i++) {
        downloadAndSaveFile(fileList[i]);
        delay(3000); // Delay giữa các lần tải để tránh quá tải server
      }
    }
    // Lệnh theo định dạng: MQTT+<folder>+<url>
    else if (input.startsWith("MQTT+")) {
      String command = input.substring(5);
      int plusIndex = command.indexOf('+');
      if (plusIndex != -1) {
        String folder = command.substring(0, plusIndex);
        String url = command.substring(plusIndex + 1);
        folder.trim();
        url.trim();
        
        // Lấy tên file từ url (tìm dấu '/' cuối cùng)
        int lastSlash = url.lastIndexOf('/');
        String fileName = (lastSlash != -1) ? url.substring(lastSlash + 1) : url;
        // Tạo đường dẫn lưu file: /<folder>/<fileName>
        String filePath = "/" + folder + "/" + fileName;
        
        // Tạo folder nếu chưa tồn tại
        String folderPath = "/" + folder;
        if (!SPIFFS.exists(folderPath)) {
          SPIFFS.mkdir(folderPath);
        }
        
        downloadAndSaveFileCustom(url, filePath);
      } else {
        Serial.println("Invalid MQTT command format. Use: MQTT+<folder>+<url>");
      }
    }
    else {
      Serial.println("Unknown command");
    }
  }
  delay(1000);
}
