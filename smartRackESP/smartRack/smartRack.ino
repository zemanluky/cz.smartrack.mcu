#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <secrets.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

WiFiClient espClient;
PubSubClient client(espClient);

String device_id = String((uint32_t)ESP.getEfuseMac(), HEX);


// Define the e-paper display class and parameters
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

#define LINE_SPACING 5 // Define custom line spacing in pixels

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Convert payload to string
  String jsonStr;
  for (unsigned int i = 0; i < length; i++) {
    jsonStr += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(jsonStr);

  // Parse JSON
  StaticJsonDocument<200> doc;  // Adjust size if needed
  DeserializationError error = deserializeJson(doc, jsonStr);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Access data
  String Item1Name = doc["Item1Name"];
  Serial.print("Parsed name1: ");
  Serial.println(Item1Name);
  float Item1Price = doc["Item1Price"];
  Serial.print("Parsed price1: ");
  Serial.println(Item1Price);
  String Item2Name = doc["Item2Name"];
  Serial.print("Parsed name2: ");
  Serial.println(Item2Name);
  float Item2Price = doc["Item2Price"];
  Serial.print("Parsed price2: ");
  Serial.println(Item2Price);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client_device123")) {
      Serial.println("connected");
      client.subscribe("shelf/device123/price");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void drawTextWithWrap(int16_t x, int16_t y, const char* text, float price, int16_t maxWidth, int16_t maxHeight) {
  display.setFont(&FreeMonoBold9pt7b);

  int16_t cursorX = x;
  int16_t cursorY = y;
  char buffer[128];
  strncpy(buffer, text, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';
  char* word = strtok(buffer, " ");
  int16_t spaceWidth;
  
  // Measure the width of a space character
  int16_t x1, y1;
  uint16_t w, h, C_h;
  display.getTextBounds("I", cursorX, cursorY, &x1, &y1, &w, &C_h);
  spaceWidth = w / 1.2;

  bool firstWord;
  firstWord = true;

  int lineCount = 1;

  while (word != NULL) {
    display.getTextBounds(word, cursorX, cursorY, &x1, &y1, &w, &h);

    if (cursorX + w > x + maxWidth) {
      lineCount += 1;
      if (lineCount > 3) break;
      cursorX = x;
      cursorY += C_h + LINE_SPACING; // Move to next line with custom spacing
      firstWord = true;
    } else {
      firstWord = false;
    }
    
    display.setCursor(cursorX, cursorY);
    display.print(word);

    cursorX += w + spaceWidth;

    word = strtok(NULL, " ");
  }
    int16_t underlineY = cursorY + 5; // Small padding
    display.drawLine(x, underlineY, x + maxWidth, underlineY, GxEPD_BLACK);

  // Split the price
  int mainPart = (int)price;
  int decimalPart = (int)((price - mainPart) * 100 + 0.5);

  char mainStr[8];
  char decimalStr[4];
  sprintf(mainStr, "%d", mainPart);
  sprintf(decimalStr, "%02d", decimalPart);  // e.g., "99"

  // Measure both parts with their fonts
  display.setFont(&FreeMonoBold18pt7b); // big font
  display.getTextBounds(mainStr, 0, 0, &x1, &y1, &w, &h);
  int16_t mainWidth = w;
  display.getTextBounds("0", 0, 0, &x1, &y1, &w, &h);
  int16_t mainHeight = h;

  display.setFont(&FreeMonoBold12pt7b); // small built-in font
  display.getTextBounds(decimalStr, 0, 0, &x1, &y1, &w, &h);
  int16_t decimalWidth = w;
  display.getTextBounds("0", 0, 0, &x1, &y1, &w, &h);
  int16_t decimalHeight = h;

  // Calculate positions
  int16_t totalWidth = mainWidth + decimalWidth;
  int16_t baseY = (maxHeight - underlineY)/2 + underlineY + mainHeight / 2;
  int16_t priceX = x + (maxWidth - totalWidth) / 2;

  // Draw main part
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(priceX, baseY);
  display.print(mainStr);

  // Draw decimal part
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(priceX + mainWidth + 5, baseY - mainHeight + (decimalHeight - 2)); // align bottoms
  display.print(decimalStr);
}

void setup() {
  Serial.begin(115200);

  Serial.print(String((uint32_t)ESP.getEfuseMac(), HEX));

  display.cp437(true);

  // Initialize the display
  display.init();
  display.setRotation(1); // Set display to landscape mode
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  // Power the e-paper display by setting GPIO 2 to HIGH
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  setup_wifi();

  client.setServer("172.20.10.8", 1883);
  client.setCallback(callback);

  if (!client.connected()) {
    reconnect();
  }

  // Clear the display
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.cp437(true);

    // Example product names
    const char* product1 = "Cini Minis";
    const char* product2 = "Corn Flakes Mini";

    // Define the area for each product
    int16_t halfWidth = display.width() / 2;
    int16_t startX1 = 0;
    int16_t startX2 = halfWidth + 5;
    int16_t startY = 10;
    int16_t textAreaWidth = halfWidth;

    // Draw product names with word wrap
    drawTextWithWrap(startX1, startY, product1, 99.90, textAreaWidth, display.height());
    drawTextWithWrap(startX2, startY, product2, 49.90, textAreaWidth, display.height());

    // Draw a line to separate the two product areas
    display.drawLine(halfWidth, 0, halfWidth, display.height(), GxEPD_BLACK);

  } while (display.nextPage());

  pinMode(13, OUTPUT);
  pinMode(12, INPUT);
  pinMode(27, OUTPUT);
  pinMode(26, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Send ultrasonic pulse
  digitalWrite(13, LOW);
  delayMicroseconds(2);
  digitalWrite(13, HIGH);
  delayMicroseconds(10);
  digitalWrite(13, LOW);

  // Read the echo time
  long duration1 = pulseIn(12, HIGH);
  float distance1 = min(duration1 * 0.0343 / 2, 100.00);

  float distance1Persentage = 100.0 - ((distance1 / 80) * 100);

  delay(100); // Add 100ms between readings

  digitalWrite(27, LOW);
  delayMicroseconds(2);
  digitalWrite(27, HIGH);
  delayMicroseconds(10);
  digitalWrite(27, LOW);

  // Read the echo time
  long duration2 = pulseIn(26, HIGH);
  float distance2 = min(duration2 * 0.0343 / 2, 100.00);
  float distance2Persentage = 100.0 - ((distance2 / 80) * 100);

  float volt = 0.0;
  volt = (analogReadMilliVolts(34) * 1.769 / 1000);
  float battPercentage = (volt - 3.0) / (4.2 - 3.0) * 100.0;

  StaticJsonDocument<200> doc;
  
  doc["distance1"] = distance1Persentage;
  doc["distance2"] = distance2Persentage;
  doc["battery"] = battPercentage;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  const char* mqtt_topic = "shelf/device123/stock"; // Unique topic for this ESP
  client.publish(mqtt_topic, jsonBuffer);

  delay(10000);
}