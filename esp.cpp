#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;
WebServer server(80);

// Kalau pertama kali, bisa pakai default SSID sementara
const char* DEFAULT_SSID = "OEMAH BOETJANG 2.4GHz";
const char* DEFAULT_PASS = "PistoLL96";  // ganti sesuai WiFi kamu

// Timeout connect (ms)
const unsigned long WIFI_CONNECT_TIMEOUT = 15000;  // 15 detik

// Deklarasi handler
void handleWifiScan();
void handleWifiStatus();
void handleWifiConnect();

// Helper untuk kirim JSON + CORS
void sendJson(int code, const String &body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(code, "application/json", body);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  prefs.begin("wifi", false);  // namespace "wifi"

  // Coba ambil SSID & password yang tersimpan
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");

  if (ssid.length() == 0) {
    Serial.println("Belum ada WiFi tersimpan, pakai default sementara...");
    ssid = DEFAULT_SSID;
    pass = DEFAULT_PASS;
  }

  Serial.print("Mencoba connect ke: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ Berhasil terkoneksi ke WiFi");
    Serial.print("IP ESP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ Gagal connect, tapi server HTTP tetap jalan untuk konfigurasi.");
    // Kalau mau, bisa nyalakan SoftAP di sini untuk mode konfigurasi offline.
  }

  // ==== Definisi endpoint API ====

  // Scan jaringan WiFi
  server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);

  // Status koneksi
  server.on("/api/wifi/status", HTTP_GET, handleWifiStatus);

  // Connect ke WiFi baru (POST ssid & password)
  server.on("/api/wifi/connect", HTTP_POST, handleWifiConnect);

  // (Opsional) untuk preflight CORS kalau suatu saat perlu OPTIONS
  server.on("/api/wifi/connect", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
  });

  // Endpoint default
  server.onNotFound([]() {
    sendJson(404, "{\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println("HTTP server berjalan di port 80");
}

void loop() {
  server.handleClient();
}

/* ======================
   Handler: /api/wifi/scan
   ====================== */
void handleWifiScan() {
  Serial.println("Request: /api/wifi/scan");

  int n = WiFi.scanNetworks();
  Serial.printf("Ditemukan %d jaringan\n", n);

  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";

    // RSSI (dBm) → konversi ke 0–100% biar cocok dengan UI progress bar
    int rssi = WiFi.RSSI(i);
    int strength = map(rssi, -90, -30, 0, 100);
    strength = constrain(strength, 0, 100);

    bool secured = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

    json += "{";
    json += "\"ssid\":\"" + String(WiFi.SSID(i)) + "\",";
    json += "\"strength\":" + String(strength) + ","; // 0–100 persen
    json += "\"secured\":" + String(secured ? "true" : "false");
    json += "}";
  }
  json += "]";

  sendJson(200, json);
}

/* =======================
   Handler: /api/wifi/status
   ======================= */
void handleWifiStatus() {
  Serial.println("Request: /api/wifi/status");

  String json = "{";

  if (WiFi.status() == WL_CONNECTED) {
    json += "\"connected\":true,";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI());
  } else {
    json += "\"connected\":false";
  }

  json += "}";

  sendJson(200, json);
}

/* =======================
   Handler: /api/wifi/connect (POST)
   Body/query: ssid, password (x-www-form-urlencoded)
   ======================= */
void handleWifiConnect() {
  Serial.println("Request: /api/wifi/connect (POST)");

  String ssid = "";
  String pass = "";

  // Bisa kirim via form-urlencoded: ssid=...&password=...
  if (server.hasArg("ssid")) {
    ssid = server.arg("ssid");
  }
  if (server.hasArg("password")) {
    pass = server.arg("password");
  }

  if (ssid.length() == 0) {
    sendJson(400, "{\"success\":false,\"message\":\"SSID tidak boleh kosong\"}");
    return;
  }

  Serial.print("Mencoba connect ke SSID baru: ");
  Serial.println(ssid);

  // Putus dulu dari WiFi sekarang
  WiFi.disconnect();
  delay(500);

  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < WIFI_CONNECT_TIMEOUT) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ Berhasil connect ke WiFi baru");

    // Simpan ke Preferences
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);

    String json = "{";
    json += "\"success\":true,";
    json += "\"message\":\"Connected\",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";

    sendJson(200, json);
  } else {
    Serial.println("❌ Gagal connect ke WiFi baru");
    sendJson(200,
             "{\"success\":false,\"message\":\"Failed to connect\"}");
  }
}
