/* A simple touch button function is shown by using TFT_eSPI and Touchlib library on a Lilygo-T-Display-S3 cap.
* Screen rotation is taken into account.
* 
* author: Rudi Ackermann
* June 2023
*/

#define TOUCH_MODULES_CST_SELF
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TouchLib.h>
#include <Wire.h>
#include "pin_config.h"
#include "LED_gruen.h"
#include "LED_rot.h"

#define gray 0xBDF7
#define lightgray 0xE71C
#define darkgray 0x7BEF
#define blue 0x0AAD
#define orange 0xC260
#define purple 0x604D
#define lightgreen 0x8FF6
#define lightblue 0xc7d9
#define bgcolour lightgreen

#define LED_PIN 4  // pin for the LED 
#define LCD_MODULE_CMD_1

// button parameters for display w/o rotation
int button_x = 220;    // X position of the touch button
int button_y = 120;    // Y position of the touch button
int button_w = 80;     // width of the touch button
int button_h = 40;     // height of the touch button

int x_min;
int x_max;
int y_min;
int y_max;

int screen_width  = 320;
int screen_height = 170;

const char* output = "off";

int screen_rotation = 1; // display rotation angle for TFT_eSPI lib  0 = no rotation, 1 = 90dgr, 2 = 180dgr, 3 = 270dgr 

TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);

TFT_eSPI tft = TFT_eSPI();

bool ledOn = false;

void setup() 
{
  tft.init();
  tft.setRotation(screen_rotation);
  tft.fillScreen(bgcolour); 
  tft.setTextSize(1); // set text size
  tft.setTextColor(TFT_BLUE,lightgreen); // set text color
  tft.setSwapBytes(true);
  Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL); //initialize touch
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,LOW);

  draw_button(-1,-1);
  tft.pushImage(255,5,60,60,LED_rot,TFT_WHITE);
}

void draw_button(int x, int y)
{  
    ledOn=!ledOn; // is used to change led/button color. Can also be used to switch e. g. LED on/off via pin 4
      
    /* when screen rotated by tft.setRotation(screen_rotation) coordinates for touch sensor don´t change, 
    screen coordinates change depending on tft.setRotation. So screen coordinates have to be transformed 
    to touch coordinates */
    transform_coordinates(screen_rotation);

    // Check if the touch point is inside the button bounds
    if (x >= x_min && x < x_max && y >= y_min && y < y_max)
    {       
      // area within button
      tft.fillRoundRect(button_x, button_y, button_w, button_h, 5, ledOn ? gray : darkgray);  // fill button with green or red
      tft.drawRoundRect(button_x-2, button_y-2, button_w+4, button_h+4, 5, ledOn ? darkgray : gray);
      if (ledOn)
      {
        tft.pushImage(255,5,60,60,LED_gruen,TFT_WHITE);
      }
      else
      {
        tft.pushImage(255,5,60,60,LED_rot,TFT_WHITE);
      }
   } else 
    {
      // other screen area
      ledOn=!ledOn;
      tft.fillRoundRect(button_x, button_y, button_w, button_h, 5, ledOn ? gray : darkgray);  // fill button with green or red
      tft.drawRoundRect(button_x-2, button_y-2, button_w+4, button_h+4, 5, ledOn ? darkgray : gray);
    }
 }

void loop() {
    char str_buf[100];
#if TOUCH_GET_FORM_INT
    if (get_int) {
        get_int = 0;
        touch.read();
#else
    if (touch.read()) {
#endif
        uint8_t n = touch.getPointNum();
        for (uint8_t i = 0; i < n; i++) {
            TP_Point t = touch.getPoint(i);
            sprintf(str_buf, "t.x:%04d t.y:%04d ", t.x, t.y);
            tft.drawString(str_buf, 10, 10);
            draw_button(t.x, t.y);
            
        }
    }
    if(ledOn){
      center_text_in_box(button_x, button_y, button_w, button_h, "on", 1, gray);
      tft.setTextSize(1);
      tft.setTextColor(TFT_BLUE, bgcolour);
    }
    else
    {
      center_text_in_box(button_x, button_y, button_w, button_h, "off", 1, darkgray);
      tft.setTextSize(1);
      tft.setTextColor(TFT_BLUE, bgcolour);      
    }
    delay(150);    
}

void transform_coordinates(int option)
{
switch(option) {
               case 0:
            // no screen rotation
            // touch region of button.
            x_min = button_x;
            x_max = button_x + button_w;
            y_min = button_y;
            y_max = button_y + button_h;
            break;
         case 1:
            // screen rotation by 90 degree
            // touch region of button. Display with rotation
            x_min = (screen_height-1) - (button_y + button_h);
            x_max = (screen_height-1) - button_y;
            y_min = button_x;
            y_max = button_x + button_w;
            break;
          case 2:
            // screen rotation by 180 degree
            // touch region of button. Display with rotation
            x_min = (screen_height-1) - (button_x + button_w);
            x_max = (screen_height-1) -  button_x;
            y_min = (screen_width-1) - (button_y + button_h);
            y_max = (screen_width-1) -  button_y;
            break;
          case 3:
            // screen rotation by 270 degree
            // touch region of button. Display with rotation
            x_min = button_y;
            x_max = button_y + button_h;
            y_min = (screen_width-1) - (button_x + button_w);
            y_max = (screen_width-1) -  button_x;
            break;
    }
}

void center_text_in_box(int16_t x, int16_t y, int16_t w,  int16_t h, const char* text, uint8_t textSize, int16_t bgcolour_box)
{
  // Calculate the text coordinates to center it within the rectangle
  int16_t textWidth = tft.textWidth(text) * textSize;
  int16_t textHeight = tft.fontHeight(textSize);
  int16_t textX = x + (w - textWidth) / 2;
  int16_t textY = y + (h - textHeight) / 2;

  // Set the text color and size
  tft.setTextColor(TFT_BLACK,bgcolour_box);
  tft.setTextSize(textSize);

  // Print the centered text
  tft.setCursor(textX, textY);
  tft.println(text);
}
