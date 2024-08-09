/*
 An example digital clock using a TFT LCD screen to show the time.
 Demonstrates use of the font printing routines. (Time updates but date does not.)

 It uses the time of compile/upload to set the time
 For a more accurate clock, it would be better to use the RTClib library.
 But this is just a demo...

 Make sure all the display driver and pin connections are correct by
 editing the User_Setup.h file in the TFT_eSPI library folder.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################

 Based on clock sketch by Gilchrist 6/2/2014 1.0

A few colour codes:

code	color
0x0000	Black
0xFFFF	White
0xBDF7	Light Gray
0x7BEF	Dark Gray
0xF800	Red
0xFFE0	Yellow
0xFBE0	Orange
0x79E0	Brown
0x7E0	Green
0x7FF	Cyan
0x1F	Blue
0xF81F	Pink

 */
// Pin definitions
#define PWR_EN_PIN  (10)
#define PWR_ON_PIN  (14)
#define BAT_ADC_PIN (5)
#define BUTTON1_PIN (0)
#define BUTTON2_PIN (21)

// LCD
#define LCD_DATA0_PIN (48)
#define LCD_DATA1_PIN (47)
#define LCD_DATA2_PIN (39)
#define LCD_DATA3_PIN (40)
#define LCD_DATA4_PIN (41)
#define LCD_DATA5_PIN (42)
#define LCD_DATA6_PIN (45)
#define LCD_DATA7_PIN (46)
#define PCLK_PIN      (8)
#define CS_PIN        (6)
#define DC_PIN        (7)
#define RST_PIN       (-1)
#define BK_LIGHT_PIN  (38)

// Touchscreen
#define TOUCHSCREEN_SCLK_PIN (1)
#define TOUCHSCREEN_MISO_PIN (4)
#define TOUCHSCREEN_MOSI_PIN (3)
#define TOUCHSCREEN_CS_PIN   (2)
#define TOUCHSCREEN_IRQ_PIN  (9)

// SD card
#define SD_MISO_PIN (13)
#define SD_MOSI_PIN (11)
#define SD_SCLK_PIN (12)

#define SDIO_DATA0_PIN (13)
#define SDIO_CMD_PIN   (11)
#define SDIO_SCLK_PIN  (12)


#define LCD_PIXEL_CLOCK_HZ (5 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL (1)
#define LCD_BK_LIGHT_OFF_LEVEL (!LCD_BK_LIGHT_ON_LEVEL)
#define LCD_PIN_NUM_DATA0 (LCD_DATA0_PIN)
#define LCD_PIN_NUM_DATA1 (LCD_DATA1_PIN)
#define LCD_PIN_NUM_DATA2 (LCD_DATA2_PIN)
#define LCD_PIN_NUM_DATA3 (LCD_DATA3_PIN)
#define LCD_PIN_NUM_DATA4 (LCD_DATA4_PIN)
#define LCD_PIN_NUM_DATA5 (LCD_DATA5_PIN)
#define LCD_PIN_NUM_DATA6 (LCD_DATA6_PIN)
#define LCD_PIN_NUM_DATA7 (LCD_DATA7_PIN)
#define LCD_PIN_NUM_PCLK (PCLK_PIN)
#define LCD_PIN_NUM_CS (CS_PIN)
#define LCD_PIN_NUM_DC (DC_PIN)
#define LCD_PIN_NUM_RST (RST_PIN)
#define LCD_PIN_NUM_BK_LIGHT (BK_LIGHT_PIN)
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

#define TFT_GREY 0x5AEB

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library


void setup(void) {
  //Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  
}

void loop() {
  
}


// Function to extract numbers from compile time string




