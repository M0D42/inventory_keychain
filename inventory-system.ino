#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <SPI.h>
#include <DNSServer.h>

#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>

// ----------------------------
// WiFi & Captive Portal
// ----------------------------
const char* ssid = "ᛒᚨᚷ'ᛟᚠ'ᚺᛟᛚᛞᛁX";
const char* password = "12345678";

const byte DNS_PORT = 53;
DNSServer dnsServer;

WebSocketsServer websocket = WebSocketsServer(81);
AsyncWebServer server(80);
Preferences prefs;

// ------- PN532 SPI Wiring --------
// ESP32  PN532
// 23  -> MOSI
// 19  -> MISO
// 18  -> SCK
// 5   -> SS (SSEL)
// ----------------------------------

PN532_SPI pn532_spi(SPI, 5);
NfcAdapter nfc(pn532_spi);

// Inventory stored in RAM
std::vector<String> items;

// -------------------------------------------------------
//  Save items to ESP32 memory
// -------------------------------------------------------
void saveItems() {
    String joined = "";
    for (String &i : items) {
        joined += i + "\n";
    }

    prefs.begin("inv", false);
    prefs.putString("list", joined);
    prefs.end();
}

// -------------------------------------------------------
//  Load items from ESP32 memory
// -------------------------------------------------------
void loadItems() {
    prefs.begin("inv", false);
    String data = prefs.getString("list", "");
    prefs.end();

    items.clear();

    int start = 0;
    while (true) {
        int end = data.indexOf('\n', start);
        if (end == -1) break;
        items.push_back(data.substring(start, end));
        start = end + 1;
    }

    Serial.println("Loaded items:");
    for (String &i : items) Serial.println(" - " + i);
}

// -------------------------------------------------------
//  Add/remove item (toggle behavior)
// -------------------------------------------------------
void processItem(String item) {
    for (int i = 0; i < items.size(); i++) {
        if (items[i] == item) {
            items.erase(items.begin() + i);
            saveItems();
            return;
        }
    }

    items.push_back(item);
    saveItems();
}

// -------------------------------------------------------
//  Send full list to one client
// -------------------------------------------------------
void sendFullList(uint8_t client) {
    websocket.sendTXT(client, "FULL_LIST_BEGIN");
    for (String &i : items) websocket.sendTXT(client, i);
    websocket.sendTXT(client, "FULL_LIST_END");
}

// -------------------------------------------------------
//  WebSocket Events
// -------------------------------------------------------
void onWebSocketEvent(uint8_t client, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.printf("Client %u connected\n", client);
            sendFullList(client);
            break;

        case WStype_TEXT: {
            String msg = String((char*)payload);
            Serial.printf("Message: %s\n", msg.c_str());

            processItem(msg);

            for (String &i : items) websocket.broadcastTXT(i);
            break;
        }
    }
}

// -------------------------------------------------------
//   ONLY EXTRACT TEXT RECORD (ignore URL, MIME, etc.)
// -------------------------------------------------------
String getTextIfTextRecord(NDEFMessage& msg) {
    if (msg.getRecordCount() == 0) return "";

    NDEFRecord record = msg.getRecord(0);
    if (record.getTnf() != TNF_WELL_KNOWN) return "";
    if (record.getType() != "T") return "";

    uint8_t* payload = record.getPayload();
    int len = record.getPayloadLength();

    uint8_t langLen = payload[0] & 0x3F;

    return String((char*)(payload + 1 + langLen), len - 1 - langLen);
}

// -------------------------------------------------------
//  HTML webpage
// -------------------------------------------------------
const char* webpage = R"====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8" />
<title>My inventory</title>
</head>
<style>
#myitems.empty::before {
    content: "No items yet...";
    color: #525151;
    font-style: italic;
}
</style>

<body style="font-family:'Franklin Gothic Medium','Arial Narrow',Arial,sans-serif; background:#897869; color:#000; text-align:center;">
<h1>Inventory</h1>
<p id="myname">Property of Rafael</p>

<h2>Log</h2>
<ul id="myitems" class="empty"
    style="background:#D6D1BD; padding:20px; border-radius:5px; list-style:square;">
</ul>

<script>
const list = document.getElementById("myitems");

function addItem(text) {
    if (text === "FULL_LIST_BEGIN" || text === "FULL_LIST_END") return;

    const items = list.getElementsByTagName("li");
    for (let i = 0; i < items.length; i++) {
        if (items[i].textContent === text) {
            items[i].remove();
            if (list.children.length === 0) list.classList.add("empty");
            return;
        }
    }

    const li = document.createElement("li");
    li.textContent = text;
    list.appendChild(li);
    list.classList.remove("empty");
}

let ws = new WebSocket("ws://" + location.hostname + ":81");
ws.onmessage = (evt) => addItem(evt.data);
</script>

</body>
</html>
)====";

// -------------------------------------------------------
//  SETUP
// -------------------------------------------------------
void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    IPAddress myIP = WiFi.softAPIP();
    Serial.println(myIP);

    // Captive Portal DNS Hijack
    dnsServer.start(DNS_PORT, "*", myIP);

    loadItems();
    nfc.begin();

    websocket.begin();
    websocket.onEvent(onWebSocketEvent);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/html", webpage);
    });

    // Captive portal fallback — ANY URL → "/"
    server.onNotFound([](AsyncWebServerRequest *req) {
        req->redirect("/");
    });

    server.begin();
}

// -------------------------------------------------------
//  LOOP — Scan NFC Text Tags + DNS server
// -------------------------------------------------------
unsigned long lastNfcCheck = 0;
const unsigned long NFC_INTERVAL = 1000; // 1 second

void loop() {
    dnsServer.processNextRequest();
    websocket.loop();

    unsigned long now = millis();
    if (now - lastNfcCheck >= NFC_INTERVAL) {
        lastNfcCheck = now;

        if (nfc.tagPresent()) {
            NfcTag tag = nfc.read();

            if (tag.hasNDEFMessage()) {
                NDEFMessage msg = tag.getNDEFMessage();
                String text = getTextIfTextRecord(msg);

                if (text.length() > 0) {
                    Serial.println("TEXT TAG: " + text);
                    processItem(text);
                    websocket.broadcastTXT(text);
                } else {
                    Serial.println("Not a text tag. Ignored.");
                }
            }
        }
    }
}
