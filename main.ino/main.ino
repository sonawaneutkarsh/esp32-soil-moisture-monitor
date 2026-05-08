// Succulent moisture monitor — ESP32-C3 Mini + Gmail alert
// --------------------------------------------------------
// Wiring:
//   Sensor VCC  → 3.3V rail (MB102)
//   Sensor GND  → GND rail
//   Sensor AOUT → GPIO1
//   ESP32 3V3   → 3.3V rail
//   ESP32 GND   → GND rail
#include "secrets.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h> 

const char* WIFI_SSID       = WIFI_SSID; // replace with your wifi name
const char* WIFI_PASSWORD   = WIFI_PASSWORD; // replace with your wifi password
const char* SENDER_EMAIL    = SENDER_EMAIL; // replace with your senders email
const char* SENDER_APP_PASS = SENDER_APP_PASS; // replace with your app password, more info in README.md
const char* FRIEND_EMAIL    = FRIEND_EMAIL; // replace with receivers email

// ── Sensor ───────────────────────────────────────────────
const int MOISTURE_PIN = 1;
const int DRY_VALUE = 3890;  // calibrate: raw ADC in dry air
const int WET_VALUE = 1410;  // calibrate: raw ADC submerged

// ── Succulent thresholds ─────────────────────────────────
const int  ALERT_BELOW_PCT   = 15;   // send alert when soil drops below 15%
const unsigned long CHECK_EVERY_MS = 6UL * 60 * 60 * 1000; // check every 6 hours 5000 for testing

// ── State ────────────────────────────────────────────────
bool  alertSentThisCycle = false;
unsigned long lastCheck  = 0;

SMTPSession smtp;

// ── helpers ──────────────────────────────────────────────
int readMoisturePct() {
  long sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(MOISTURE_PIN); delay(10); }
  int raw = sum / 10;
  return constrain(map(raw, DRY_VALUE, WET_VALUE, 0, 100), 0, 100);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("ESP32");
  WiFi.disconnect(true);
  delay(1000);
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 60) {
    delay(500); Serial.print("."); tries++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected!" : " FAILED");
}

void sendAlert(int pct) {
  Serial.println("Sending email alert...");

  ESP_Mail_Session session;
  session.server.host_name = "smtp.gmail.com";
  session.server.port      = 465;
  session.login.email      = SENDER_EMAIL;
  session.login.password   = SENDER_APP_PASS;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name  = "plant";
  message.sender.email = SENDER_EMAIL;
  message.subject      = "bro pls water plant pls";
  message.addRecipient("Friend", FRIEND_EMAIL);

  String body = "yo soil moisture dropped to ";
  body += pct;
  body += "%.\n\nThat's below the 15% threshold for succulents give it some water.\n\n not a lot just enough for a week or two.\n\n";
  message.text.content = body;

  if (!smtp.connect(&session)) {
    Serial.println("SMTP connect failed: " + smtp.errorReason());
    return;
  }
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Send failed: " + smtp.errorReason());
  } else {
    Serial.println("Alert sent!");
    alertSentThisCycle = true;
  }
  smtp.closeSession();
}

// ── setup ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  connectWiFi();
  Serial.println("success!!.");
}


// ── loop ──────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();
  if (now - lastCheck >= CHECK_EVERY_MS) {
    lastCheck = now;
    int pct = readMoisturePct();
    Serial.printf("Moisture: %d%%\n", pct);

    if (pct < ALERT_BELOW_PCT && !alertSentThisCycle) {
      if (WiFi.status() != WL_CONNECTED) connectWiFi();
      sendAlert(pct);
    }

    // Reset alert flag once soil recovers (friend watered it)
    if (pct >= ALERT_BELOW_PCT + 10) {
      alertSentThisCycle = false;
      Serial.println("Soil recovered — alert flag reset.");
    }
  }
}