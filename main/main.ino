// esp32-c3 plant monitor
// monitors soil moisture and sends an email alert when
// the normalized moisture value falls below a threshold

#include "secrets.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>

// ── sensor configuration ──────────────────────────────────

const int MOISTURE_PIN = 1;

// calibrate these values using moisture_calibrator.ino
const int DRY_VALUE = 3890;
const int WET_VALUE = 1410;

// ── alert configuration ───────────────────────────────────

const int ALERT_BELOW_PCT = 15;

// check the sensor every 6 hours
const unsigned long CHECK_EVERY_MS = 6UL * 60 * 60 * 1000;

// ── runtime state ──────────────────────────────────────────

bool alertSentThisCycle = false;
unsigned long lastCheck = 0;

SMTPSession smtp;

// ── sensor functions ──────────────────────────────────────

int readMoisturePct()
{
  long sum = 0;

  // average 10 readings to reduce short-term sensor variation
  for (int i = 0; i < 10; i++)
  {
    sum += analogRead(MOISTURE_PIN);
    delay(10);
  }

  int raw = sum / 10;

  // convert the calibrated raw value to a normalized 0–100 scale
  return constrain(
      map(raw, DRY_VALUE, WET_VALUE, 0, 100),
      0,
      100);
}

// ── wi-fi functions ────────────────────────────────────────

void connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("ESP32");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  Serial.print("connecting to wi-fi");

  int tries = 0;

  while (WiFi.status() != WL_CONNECTED && tries < 60)
  {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println(" connected!");
    Serial.print("ip address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println(" failed");
  }
}

// ── email functions ────────────────────────────────────────

void sendAlert(int pct)
{
  Serial.println("sending email alert...");

  ESP_Mail_Session session;

  session.server.host_name = "smtp.gmail.com";
  session.server.port = 465;

  session.login.email = SENDER_EMAIL;
  session.login.password = SENDER_APP_PASS;
  session.login.user_domain = "";

  SMTP_Message message;

  message.sender.name = "plant monitor";
  message.sender.email = SENDER_EMAIL;
  message.subject = "bro pls water plant pls";

  message.addRecipient("plant monitor recipient", RECIPIENT_EMAIL);

  String body = "soil moisture dropped to ";
  body += pct;
  body += "%.\n\n";
  body += "this is below the configured threshold of ";
  body += ALERT_BELOW_PCT;
  body += "%.\n";

  message.text.content = body;

  if (!smtp.connect(&session))
  {
    Serial.println("smtp connection failed: " + smtp.errorReason());
    return;
  }

  if (!MailClient.sendMail(&smtp, &message))
  {
    Serial.println("email failed: " + smtp.errorReason());
  }
  else
  {
    Serial.println("alert sent!");
    alertSentThisCycle = true;
  }

  smtp.closeSession();
}

// ── measurement ────────────────────────────────────────────

void checkMoisture()
{
  int pct = readMoisturePct();

  Serial.printf("moisture: %d%%\n", pct);

  if (pct < ALERT_BELOW_PCT && !alertSentThisCycle)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      connectWiFi();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      sendAlert(pct);
    }
    else
    {
      Serial.println("cannot send alert because wi-fi is unavailable");
    }
  }

  // reset the alert state after the soil recovers
  if (pct >= ALERT_BELOW_PCT + 10)
  {
    if (alertSentThisCycle)
    {
      Serial.println("soil recovered - alert flag reset.");
    }

    alertSentThisCycle = false;
  }
}

// ── setup ──────────────────────────────────────────────────

void setup()
{
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  connectWiFi();

  // take the first measurement immediately after startup
  checkMoisture();

  lastCheck = millis();

  Serial.println("plant monitor ready.");
}

// ── loop ──────────────────────────────────────────────────

void loop()
{
  unsigned long now = millis();

  if (now - lastCheck >= CHECK_EVERY_MS)
  {
    lastCheck = now;

    if (WiFi.status() != WL_CONNECTED)
    {
      connectWiFi();
    }

    checkMoisture();
  }
}