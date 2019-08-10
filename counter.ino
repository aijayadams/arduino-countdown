// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128

// You can use any (4 or) 5 pins
#define MOSI_PIN 2
#define SCLK_PIN 3
#define DC_PIN 4
#define CS_PIN 5
#define RST_PIN 6
#define OLEDCS 7

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define DS3231_I2C_ADDRESS 0x68

#include "Wire.h"
#include "DS3231.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <SPI.h>

RTClib rtc;

Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

struct time_remain_t
{
    uint16_t days;
    uint8_t hours;
};

time_remain_t tr = {.days = 286, .hours = 13};
uint32_t unitsToReboot = 1444432466;

uint16_t unitsInPeriod = 3600 / 24;
uint16_t cntCurrentPeriod = 3600 / 24;

uint8_t doPaint = 1;

void timer_config()
{
    // TIMER 1 for interrupt frequency 0.5 Hz:
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    // set compare match register for 0.5 Hz increments
    OCR1A = 31249; // = 16000000 / (1024 * 0.5) - 1
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12, CS11 and CS10 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (0 << CS11) | (1 << CS10);
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
    sei(); 
}

ISR(TIMER1_COMPA_vect)
{
    if (--cntCurrentPeriod == 0)
    {
        // Time to Paint!
        cntCurrentPeriod = unitsInPeriod;
        doPaint = 1;
    }
}

void setup(void)
{
    Serial.begin(9600);
    // Initialise I2C Bus
    Wire.begin();
    // Initialise Screen
    tft.begin();
    tft.setRotation(2);
    // Setup ISR
    timer_config();
}

void loop()
{
    if (doPaint == 1)
    {
        doPaint = 0;
        time_remaining();
        paint_screen();
    }
}

void time_remaining()
{
    DateTime now = rtc.now();
    uint32_t rc = 1589216400;
    uint32_t rem = rc - now.unixtime();
    tr.days = uint16_t(rem / (3600L * 24L));
    tr.hours = (rem % (3600L * 24L)) / 3600L;
    Serial.println(tr.days);
}

void paint_screen()
{
    tft.fillScreen(BLACK);

    tft.setFont(&FreeMono9pt7b);
    tft.setCursor(0, 20);
    tft.setTextColor(GREEN);

    tft.print("Reboot in");
    delay(250);
    tft.print(".");
    delay(250);
    tft.print(".");
    delay(250);
    tft.print(".");
    print_days(tr.days);
    tft.setFont(&FreeMono9pt7b);
    tft.setCursor(40, 90);
    char follower[] = "days";
    for (int i = 0; i < sizeof(follower); i++)
    {
        tft.print(follower[i]);
        delay(200);
    }
    tft.println("");
    tft.setCursor(20, tft.getCursorY());
    String hours_str = String(tr.hours);
    String hours_prt = String(hours_str + " hours");
    for (int i = 0; i < hours_prt.length(); i++)
    {
        tft.print(hours_prt[i]);
        delay(200);
    }
}

void print_days(uint16_t days)
{
    tft.setFont(&FreeMonoBold18pt7b);
    uint8_t duration = 5;
    String days_str = String(days, DEC);
    Serial.println(days);
    Serial.println(days_str);
    for (int i = 0; i < days_str.length(); i++)
    {
        // First place?
        if (i != 0)
        {
            duration = 1;
        }
        rolldig(days_str[i], i, duration);
    }
}

void rolldig(char digit, int pos, int blinks)
{
    uint8_t top_offset = 60;
    uint8_t left_offset = 33;
    uint8_t kerning = 20;
    blink_cur(left_offset + kerning * pos, top_offset - 24, blinks);
    tft.drawChar(left_offset + kerning * pos, top_offset, digit, GREEN, BLACK, 1);
}

void blink_cur(int x, int y, int blinks)
{
    int h = 18;
    int w = 25;
    for (int i = 0; i < blinks; i++)
    {
        delay(300);
        tft.fillRect(x, y, h, w, GREEN);
        delay(300);
        tft.fillRect(x, y, h, w, BLACK);
    }
}
