#include "config.hpp"
#include "utils/bme_sample.hpp"
#include "utils/i2c_scan.hpp"
#include "utils/mdelay.hpp"
#include "utils/static_buffer.hpp"
#include <7Semi_BME280.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <Wire.h>
#include <array>
#include <cstdlib>
#include <stdio.h>
#include <string_view>
const int LED2_PIN = D0;

constexpr std::array<std::array<const char[], 7>, 3> LittleSeijiFaces{{
    {" (+_+) ", "\\(^_^)/", " (^_^)/", " (>_<) ", " (;_;) ", " (0_0) ",
     " (@_@) "},
    {" /(`)\\ ", " `(`)'  ~*~ ", " /(')'   u", " /(`)\\ ", " |(')| ",
     " /(')\\ ", " /(`)\\ "},
    {"  / | ", "  / \\   \\|/", "  | |   \\|/", "  / | ", "  | | ", "  / \\ ",
     "  | \\ ~."},
}};

// put function declarations here:
int myFunction(int, int);

utils::StaticBuffer<128> setup_message;

const uint32_t DELAY_MSEC = 1000;

// Set in setup() after detect; passed to BME constructor on first
// AppContext::get()
static uint8_t g_bme_i2c_addr = 0x76;

struct LED {
  int pin;
  bool state;
  bool active_low;

  LED(int pin, bool active_low = false)
      : pin(pin), state(false), active_low(active_low) {
    pinMode(pin, OUTPUT);
    off();
  }

  void on() {
    state = true;
    digitalWrite(pin, active_low ? LOW : HIGH);
  }
  void off() {
    state = false;
    digitalWrite(pin, active_low ? HIGH : LOW);
  }
  void toggle() {
    state = !state;
    digitalWrite(pin, active_low ? (state ? LOW : HIGH) : (state ? HIGH : LOW));
  }
};

class AppContext {
public:
  LED led;
  LED led2;
  utils::MDelay sleep;
  BMI280_7Semi bme;
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
  bool bme_ok = false;

  AppContext(const AppContext &) = delete;
  AppContext &operator=(const AppContext &) = delete;

  static AppContext &get() {
    static AppContext _instance;
    return _instance;
  }

  void init() {
    led.off(); // Built-in off (active low → pin HIGH)
    led2.on(); // External LED on

    bme_ok = bme.begin();
    if (!bme_ok) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      i2cScanAndPrint();
    } else {
      Serial.println("BME280 sensor found");
    }

    Wire.setClock(400000);
    u8g2.setI2CAddress(0x3C *
                       2); // 7-bit 0x3C (use 0x3D * 2 if module is at 0x3D)
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, bme_ok ? "BME280 + OLED" : "BME fail");
    u8g2.sendBuffer();
  }

private:
  AppContext()
      : led(LED_BUILTIN, true), led2(LED2_PIN), sleep(DELAY_MSEC),
        bme(g_bme_i2c_addr, SDA, SCL, 100000, Wire),
        u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE) {}
  ~AppContext() = default;
};

bool init_wifi() {
  WiFi.mode(WIFI_STA);
  WiFiSettings settings;
  WiFi.begin(settings.ssid, settings.password);
  int status = 0;
  for (int i = 0; (status = WiFi.status()) != WL_CONNECTED and i < 6; i++) {
    Serial.printf("Connecting to WiFi... Retry=%d\n", i + 1);
    delay(1000);
    Serial.printf("WiFi status: %d\n", status);
  }
  if (status == WL_CONNECTED)
    Serial.printf("Connected to WiFi: %s\n", WiFi.localIP().toString().c_str());
  return status == WL_CONNECTED;
}

void setup() {
  Serial.begin(115200);
  delay(1500); // Wait for USB CDC before WiFi / I2C init
  setup_message.append("Hello, World!\n");
  // put your setup code here, to run once:
  char result_str[20];
  int result = myFunction(2, 3);
  snprintf(result_str, sizeof(result_str), "result: %d\n", result);

  Wire.begin(SDA, SCL);
  Wire.setClock(100000);
  i2cScanAndPrint();
  const uint8_t detected = detectBme280Address();
  if (detected != 0) {
    g_bme_i2c_addr = detected;
  }
  AppContext::get().init();

  if (!init_wifi()) {
    Serial.println("Failed to connect to WiFi (I2C/BME already initialized)");
  }

  Serial.println(result_str);
  setup_message.append(result_str);
}

// void little_seiji(std::array<std::array<char>> &faces) {

//   int num = rand() % 7;
//   std::copy_n()

//       Serial.printf("face=%d line=%d\n", random_face, random_line);
//   Serial.printf("face=%s line=%s\n", faces[random_face][random_line],
//                 faces[random_face][random_line]);
//   Serial.printf("face=%s line=%s\n", faces[random_face][random_line],
//                 faces[random_face][random_line]);
// }

void loop() {
  auto &ctx = AppContext::get();
  ctx.led.toggle();
  ctx.led2.toggle();
  if (!ctx.bme_ok) {
    ctx.sleep();
    return;
  }
  BmeSample env;
  if (readBmeSample(ctx.bme, env)) {

    char line1[32];
    char line2[32];
    char line3[32];
    snprintf(line1, sizeof(line1), "T:%.1f C H:%.1f %%", env.temperature_c,
             env.humidity_rh);
    snprintf(line2, sizeof(line2), "P:%.0f hPa", env.pressure_hpa);
    // snprintf(line3, sizeof(line3), "%s", little_seiji());

    ctx.u8g2.clearBuffer();
    ctx.u8g2.drawStr(0, 12, line1);
    ctx.u8g2.drawStr(0, 28, line2);
    ctx.u8g2.drawStr(0, 44, line3);
    ctx.u8g2.sendBuffer();
    Serial.printf("temperature=%f pressure=%f humidity=%f\n", env.temperature_c,
                  env.pressure_hpa, env.humidity_rh);
  } else {
    Serial.println("BME280 read failed");
  }
  uint32_t elapsed = ctx.sleep();
  Serial.printf("builtin=%d ext=%d elapsed=%lu\n", (int)ctx.led.state,
                (int)ctx.led2.state, (unsigned long)elapsed);
}

// put function definitions here:
int myFunction(int x, int y) { return x + y; }
