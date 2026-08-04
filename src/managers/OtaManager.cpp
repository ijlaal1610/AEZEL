#include "OtaManager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

static WebServer otaServer(80);

static const char* otaHtmlIndex = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>AEZEL VCU — Wireless Firmware Update</title>
    <style>
        body { background: #07090e; color: #00d4ff; font-family: sans-serif; text-align: center; padding: 40px 20px; }
        h1 { font-size: 2.2rem; letter-spacing: 3px; }
        .card { background: #0f141c; max-width: 450px; margin: 0 auto; padding: 30px; border-radius: 20px; border: 1px solid #1e293b; }
        input[type=file] { margin: 20px 0; color: #94a3b8; }
        input[type=submit] { background: linear-gradient(135deg, #00d4ff, #0055ff); border: none; color: #fff; padding: 14px 28px; border-radius: 12px; font-weight: bold; cursor: pointer; }
    </style>
</head>
<body>
    <div class="card">
        <h1>AEZEL OTA</h1>
        <p style="color:#94a3b8;">Select firmware binary (.bin) to upload:</p>
        <form method="POST" action="/update" enctype="multipart/form-data">
            <input type="file" name="update"><br>
            <input type="submit" value="⚡ Flash Firmware">
        </form>
    </div>
</body>
</html>
)rawliteral";

void OtaManager::begin() {
    // Initialized on demand when Wi-Fi AP is activated
}

void OtaManager::enableWifiAp() {
    if (_apActive) return;
    WiFi.softAP("AEZEL-VCU-AP", "aezel1610");
    
    otaServer.on("/", HTTP_GET, []() {
        otaServer.send(200, "text/html", otaHtmlIndex);
    });

    otaServer.on("/update", HTTP_POST, []() {
        otaServer.send(200, "text/plain", (Update.hasError()) ? "UPDATE FAILED" : "UPDATE SUCCESSFUL — REBOOTING...");
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = otaServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Update.begin(UPDATE_SIZE_UNKNOWN);
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            Update.write(upload.buf, upload.currentLength);
        } else if (upload.status == UPLOAD_FILE_END) {
            Update.end(true);
        }
    });

    otaServer.begin();
    _apActive = true;
    SharedState::instance().update([](VehicleState& s) { s.wifiConnected = true; });
}

void OtaManager::disableWifiAp() {
    if (!_apActive) return;
    otaServer.stop();
    WiFi.softAPdisconnect(true);
    _apActive = false;
    SharedState::instance().update([](VehicleState& s) { s.wifiConnected = false; });
}

void OtaManager::taskEntry(void* pv) {
    OtaManager& self = instance();
    for (;;) {
        self.tick();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void OtaManager::tick() {
    if (_apActive) {
        otaServer.handleClient();
    }
}
