// Note: So far this program pretty unstable, it might take a few restarts.
// WARNING, the free version of the AlphaVantage API gives only 5 requests per minute and 500 max per day
#include <WiFi.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define ENTRIES 64 // Should be a power of 2 for now due to truncation issues?
#define I2C_ADDRESS 0x3C // I2C address for my OLED, use an address scanner to find your OLED's

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

const char* ssid = "your-ssid";
const char* password = "your-password";
const char api_key[] = "your-API-key"; // Sign up for one at https://www.alphavantage.co/support/
const char server[] = "https://www.alphavantage.co"; // Server for JSON data
// HTTPS .pem certificate for alphavantage (obtained on 3/4/20, may become invalid a few years from then)
const char pem[] = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----\n";

char dates[ENTRIES][20];
float DIA[ENTRIES];
float DIA_10[ENTRIES];
float DIA_30[ENTRIES];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFiSetup();

  // Function documentation
  // int acquireSMA(float *sma, const char *ticker, int interval, int count)
  //    Fills the sma array with smas for the number of days given by count.
  //    The interval characterizes the SMA. The stock ticker is ticker string.
  //    Returns ACTUAL number of entries acquired.
  // int acquireDailyClosingPrices(float *closing, const char *ticker, int count)
  //    Fills the closing array with closing prices for the number of days given
  //    by count. The stock ticker is ticker string.
  //    Returns ACTUAL number of entries acquired.
  // int acquireTradingDates(char dates[][20], int count)
  //    Fills dates string array with strings containing the date associated with
  //    the indexes for the array of the above two functions. Returns the ACTUAL
  //    number of entries acquired. Dates are in ISO 8601 format.
  acquireSMA(DIA_10,"DIA",10,ENTRIES);
  acquireSMA(DIA_30,"DIA",30,ENTRIES);
  acquireDailyClosingPrices(DIA,"DIA",ENTRIES);
  acquireTradingDates(dates,ENTRIES);
  dates[0][10] = 0; // Cuts off the time of the first timestamp

  WiFiEnd();
  
  Serial.println("Displaying data...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("7-day DIA Close");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("DIA Close: ");
  for(int i = 0; i < 7; i++){
    display.print(DIA[i]);
    display.print(' ');
    display.println(dates[i]);
  }
  display.display();
  delay(5000);
  Serial.println("7-day DIA 10 SMA");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("DIA 10 SMA: ");
  for(int i = 0; i < 7; i++){
    display.print(DIA_10[i]);
    display.print(' ');
    display.println(dates[i]);
  }
  display.display();
  delay(5000);
  Serial.println("7-day DIA 30 SMA");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("DIA 30 SMA: ");
  for(int i = 0; i < 7; i++){
    display.print(DIA_30[i]);
    display.print(' ');
    display.println(dates[i]);
  }
  display.display();
  delay(5000);

  float maximum = DIA_10[0], minimum = DIA_10[0];
  for(int i = 0; i < ENTRIES; i++){
    if(DIA[i] < minimum) minimum = DIA[i];
    else if(DIA[i] > maximum) maximum = DIA[i];
  }
  for(int i = 0; i < ENTRIES; i++){
    if(DIA_10[i] < minimum) minimum = DIA_10[i];
    else if(DIA_10[i] > maximum) maximum = DIA_10[i];
  }
  for(int i = 0; i < ENTRIES; i++){
    if(DIA_30[i] < minimum) minimum = DIA_30[i];
    else if(DIA_30[i] > maximum) maximum = DIA_30[i];
  }
  maximum += 2;
  minimum -= 2;
  float range = maximum-minimum;
  int pixels_per_entry = SCREEN_WIDTH / (ENTRIES-1); // Truncation causes issues, for now use powers of 2 for ENTRIES

  display.clearDisplay();
  Serial.println("DIA Close Graph");
  display.setCursor(40,0);
  display.println("DIA Close:");
  for(int i = 1; i < ENTRIES; i++){
    int scaled_previous = SCREEN_HEIGHT-SCREEN_HEIGHT*(DIA[i-1]-minimum)/range;
    int scaled_current = SCREEN_HEIGHT-SCREEN_HEIGHT*(DIA[i]-minimum)/range;
    display.writeLine(pixels_per_entry*(ENTRIES-i),scaled_current,pixels_per_entry*(ENTRIES-(i-1)),scaled_previous,WHITE);
  }
  display.display();
  delay(5000);
  display.clearDisplay();
  Serial.println("DIA 10 Graph");
  display.setCursor(40,0);
  display.println("DIA 10 SMA:");
  for(int i = 1; i < ENTRIES; i++){
    int scaled_previous = SCREEN_HEIGHT-SCREEN_HEIGHT*(DIA_10[i-1]-minimum)/range;
    int scaled_current = SCREEN_HEIGHT-SCREEN_HEIGHT*(DIA_10[i]-minimum)/range;
    display.writeLine(pixels_per_entry*(ENTRIES-i),scaled_current,pixels_per_entry*(ENTRIES-(i-1)),scaled_previous,WHITE);
  }
  display.display();
  delay(5000);
  Serial.println("DIA 30 Graph");
  display.clearDisplay();
  display.setCursor(40,0);
  display.println("DIA 30 SMA:");
  for(int i = 1; i < ENTRIES; i++){
    int scaled_previous = SCREEN_HEIGHT-SCREEN_HEIGHT*(DIA_30[i-1]-minimum)/range;
    int scaled_current = SCREEN_HEIGHT-SCREEN_HEIGHT*(DIA_30[i]-minimum)/range;
    display.writeLine(pixels_per_entry*(ENTRIES-i),scaled_current,pixels_per_entry*(ENTRIES-(i-1)),scaled_previous,WHITE);
  }
  display.display();
  delay(5000);
}

void WiFiSetup(){
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println();
  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int connect_attempts = 0;
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    connect_attempts++;
    if(connect_attempts > 30){
      connect_attempts = -1;
      Serial.println("Connection failure");
      break;
    }
  }
  if(connect_attempts != -1) Serial.println("Connected!");
  
  IPAddress remote_ip (1,1,1,1);
  Serial.print("Confirming Internet connection by pinging 1.1.1.1 ... ");
  if(Ping.ping(remote_ip)) Serial.println("Success!");
  else Serial.println("Failure.");
}

void WiFiEnd(){
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

// Accepts pointer to payload string, dates string array, address string, data header string, and maximum number of
//  entries requested. It fills the payload with a smaller modified JSON containing only the object with the data
//  header (ex: "SMA\": {", where the first part was cut off) with only max_entries entries if possible. It also fills
//  the dates array with dates pulled from the JSON to use as an index for ArduinoJson functions
int httpObtainPayload(String *payload_ptr, char dates[][20], const char *address, const char *dataheader, int max_entries){
  int entries;
  *payload_ptr = "{";
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client){
    client -> setCACert(pem);
    {
      HTTPClient http;  //Object of class HTTPClient
      http.useHTTP10(true); // Necessary to use Stream
      http.begin(*client, address);
      int httpCode = http.GET();
      if (httpCode > 0) {
        // Get the request response payload
        Serial.println("JSON obtained.");
        http.getStreamPtr()->setTimeout(10000);
        // Searches for the data header, ignoring everything before it
        if(http.getStreamPtr()->find(dataheader)) Serial.println("Data header identified!");
        else Serial.println("Data header not found.");  
        char entry[300]; bool first_entry = true; int k;
        // Repeats for as many entries as max_entries indicates
        for(k = 0; k < max_entries; k++){
          // Loads an entry in the JSON object into a buffer
          int charsread = http.getStreamPtr()->readBytesUntil('}',entry,300);
          while(http.getStreamPtr()->read() != ',');
          entry[charsread] = '}';
          entry[charsread+1] = 0; // Terminates the buffer as a string
          // Parses for and copies the date
          int j,i;
          for(i = 0; entry[i] != '"'; i++);
          i++;
          for(j = 0; entry[i] != '"'; i++){
            dates[k][j] = entry[i];
            j++;
          }
          dates[k][j] = 0; // Terminates the date string
          // Adds the entry into the payload
          if(first_entry) first_entry = false;
          else *payload_ptr += ',';
          *payload_ptr += entry;
        }
        *payload_ptr += "}";
        entries = k;
      }
      else{
        Serial.print("Failed to obtain JSON, HTTP code error: ");
        Serial.println(httpCode);
      }
      http.end();   //Close connection
    }
    delete client;
  }
  return entries; // TO DO: Consider removing, it's probably not useful
}

// Pulls a JSON containing SMAs from AlphaVantage, parses it, then fills an array with SMA floats. Returns number of entries acquired
// It uses a trimming method involving a string because otherwise the ESP32 runs out of memory. Considering ditching ArduinoJson and
//  parsing manually.
int acquireSMA(float *sma, const char *ticker, int interval, int count){
  // Concatenates the three fields into a full HTTP address
  char resource[128]; // Just needs to be enough to store the entire path
  sprintf(resource,"/query?function=SMA&symbol=%s&interval=daily&time_period=%d&series_type=close&apikey=",ticker,interval);
  char address[sizeof(api_key)+sizeof(server)+sizeof(resource)];
  sprintf(address, "%s%s%s", server, resource, api_key);
  Serial.print("Connecting to ");
  Serial.println(address);
  
  String payload;   // Strings are BAD practice, swap for a char array later
  char dates[count][20];
  int entry_count = httpObtainPayload(&payload, dates, address, "SMA\": {", count);

  Serial.println("Intializing JSON...");
  Serial.print("Heap memory after JSON: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  const size_t capacity = entry_count*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(entry_count) + (entry_count*30);
  DynamicJsonDocument smaJSON(capacity);
  Serial.print(" --> ");
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  
  DeserializationError err;
  Serial.print("Parsing JSON...");
  err = deserializeJson(smaJSON,(const String)payload);
  if(err){
    Serial.print(" Parsing failed with error code ");
    Serial.print(err.c_str());
  }
  Serial.println(" Parsing complete.");
  // The problem seems to begin from here
  for(int i = 0; i < entry_count; i++) sma[i] = atof(smaJSON[dates[i]]["SMA"]);
  smaJSON.clear();
  return entry_count;
}

int acquireDailyClosingPrices(float *closing, const char *ticker, int count){
  // Concatenates the three fields into a full HTTP address
  char resource[128]; // Just needs to be enough to store the entire path
  sprintf(resource,"/query?function=TIME_SERIES_DAILY&symbol=%s&outputsize=full&apikey=",ticker);
  char address[sizeof(api_key)+sizeof(server)+sizeof(resource)];
  sprintf(address, "%s%s%s", server, resource, api_key);
  Serial.print("Connecting to ");
  Serial.println(address);
  
  String payload;   // Strings are BAD practice, swap for a char array later
  char dates[count][20];
  int entry_count = httpObtainPayload(&payload, dates, address, "(Daily)\": {", count);
  
  Serial.println("Intializing JSON...");
  Serial.print("Heap memory after JSON: ");
  Serial.print(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  const size_t capacity = entry_count*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(entry_count) + entry_count*110;
  DynamicJsonDocument pricesJSON(capacity);
  Serial.print(" --> ");
  Serial.println(heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  
  DeserializationError err;
  Serial.print("Parsing JSON...");
  err = deserializeJson(pricesJSON,(const String)payload);
  if(err){
    Serial.print(" Parsing failed with error code ");
    Serial.print(err.c_str());
  }
  Serial.println(" Parsing complete.");
  
  for(int i = 0; i < entry_count; i++) closing[i] = atof(pricesJSON[dates[i]]["4. close"]);
  pricesJSON.clear();
  return entry_count;
}

// Uses a modified AcquireSMA function to fill a dates array. Returns number of entries acquired
int acquireTradingDates(char dates[][20], int count){
  // Concatenates the three fields into a full HTTP address
  char resource[128]; // Just needs to be enough to store the entire path
  sprintf(resource,"/query?function=SMA&symbol=%s&interval=daily&time_period=%d&series_type=close&apikey=","DIA",10);
  char address[sizeof(api_key)+sizeof(server)+sizeof(resource)];
  sprintf(address, "%s%s%s", server, resource, api_key);
  Serial.print("Connecting to ");
  Serial.println(address);
  
  int entries;
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client){
    client -> setCACert(pem);
    {
      HTTPClient http;  //Object of class HTTPClient
      http.useHTTP10(true);
      http.begin(*client, address);
      int httpCode = http.GET();
      if (httpCode > 0) {
        // Get the request response payload
        Serial.println("JSON obtained.");  
        if(http.getStreamPtr()->find("SMA\": {")) Serial.println("Data header identified!");
        else Serial.println("Data header not found.");  
        Serial.println("Obtaining dates");
        char entry[100]; int k;
        for(k = 0; k < count; k++){
          http.getStreamPtr()->readBytesUntil(',',entry,100);
          int j,i;
          for(i = 0; entry[i] != '"'; i++);
          i++;
          for(j = 0; entry[i] != '"'; i++){
            dates[k][j] = entry[i];
            j++;
          }
          dates[k][j] = 0;
        }
        entries = k;
      }
      else{
        Serial.print("Failed to obtain JSON, HTTP code error: ");
        Serial.println(httpCode);
      }
      http.end();   //Close connection
    }
    delete client;
  }
  return entries;
}
