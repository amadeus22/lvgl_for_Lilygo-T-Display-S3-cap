/* usage of lvgl library on Lilygo T-Display S3 touch version
   for touch processing the TouchLib library is used
   here simple slider used to adjust the screen brightness
   currently usable with 90 and 270dgr screen rotation

   author: Rudi Ackermann - June, 2023
*/

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "pin_config.h"
#include <string.h>

static lv_obj_t * slider;
static lv_obj_t * slider_label;

// the number of the LED pin
  const int ledPin = 38;  // 16 corresponds to GPIO38
// setting PWM properties
  const int freq = 5000;
  const int ledChannel = 0;
  const int resolution = 8;
  
int initial_slider_value = 50; // initial brightness
String initial_string_value_displayed;

int slider_value;

#define darkgray 0x7BEF

static bool is_initialized_lvgl = false;
/* Please make sure your touch IC model. */
//#define TOUCH_MODULES_CST_MUTUAL
#define TOUCH_MODULES_CST_SELF  // used fornew touch version
#include "TouchLib.h"
#define TOUCH_READ_FROM_INTERRNUPT

/* The product now has two screens, and the initialization code needs a small change in the new version. The LCD_MODULE_CMD_1 is used to define the
 * switch macro. */
//#define LCD_MODULE_CMD_1

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 170;

int screen_rotation = 1; // display rotation angle for TFT_eSPI lib  0 = no rotation, 1 = 90dgr, 2 = 180dgr, 3 = 270dgr 

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * 10 ];

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
        
    #if LV_USE_LOG != 0
        // Serial debugging 
        void my_print(const char * buf)
    {
        Serial.printf(buf);
        Serial.flush();
    }
    #endif

#if defined(TOUCH_MODULES_CST_MUTUAL)
TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS328_SLAVE_ADDRESS, PIN_TOUCH_RES);
#elif defined(TOUCH_MODULES_CST_SELF)
TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);
#endif

bool inited_touch = false;
bool inited_sd = false;
#if defined(TOUCH_READ_FROM_INTERRNUPT)
bool get_int_signal = false;
#endif

void setup() {

    #if LV_USE_LOG != 0
        lv_log_register_print_cb( my_print ); /* register print function for debugging */
    #endif

    tft.begin();          /* TFT init */
    tft.setRotation(screen_rotation); /* Landscape orientation, flipped */
    tft.setTextSize(1); // set text size
    tft.setTextColor(TFT_BLACK,darkgray);

    // configure LED PWM functionalitites
    ledcSetup(ledChannel, freq, resolution);

    // attach the channel to the GPIO to be controlled
    ledcAttachPin(ledPin, ledChannel);
    
    initial_string_value_displayed = String(initial_slider_value);
    initial_string_value_displayed = initial_string_value_displayed + "%";
    ledcWrite(ledChannel, initial_slider_value);
    
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * 10 );
    
    /*Initialize the display*/
    lv_init();
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );     
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register( &disp_drv );
    // change background color of actual screen
    lv_obj_set_style_bg_color(lv_scr_act(), LV_COLOR_MAKE(127,127,127) , LV_PART_MAIN);
    
    // Register touch brush with LVGL 
    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL, 800000);
    inited_touch = touch.init();
    if (inited_touch) {
      static lv_indev_drv_t indev_drv;
      lv_indev_drv_init(&indev_drv);
      indev_drv.type = LV_INDEV_TYPE_POINTER;
      indev_drv.read_cb = lv_touchpad_read;
      lv_indev_drv_register(&indev_drv);
    }
    is_initialized_lvgl = true;
    #if defined(TOUCH_READ_FROM_INTERRNUPT)
      attachInterrupt(
          PIN_TOUCH_INT, [] { get_int_signal = true; }, FALLING);
  #endif

    //draw slider
    lv_example_simple_slider();
 }

// Display flushing
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

static void slider_event_cb(lv_event_t * e)

  {
    lv_obj_t * slider = lv_event_get_target(e);
    char buf[8];
     slider_value = lv_slider_get_value(slider);
    lv_snprintf(buf, sizeof(buf), "%d%%", slider_value);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    delay(5);
    int mappedValue = map(slider_value, 10, 100, 10, 254);
    ledcWrite(ledChannel, mappedValue);
  }

/**
 * A slider with a label displaying the current value
 */
void lv_example_simple_slider(void)
{
    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 250); 
    lv_obj_center(slider);
    // Set the range of the slider
    lv_slider_set_range(slider, 10, 100);
    // Set the initial value of the slider
    lv_slider_set_value(slider, initial_slider_value, LV_ANIM_OFF);  // Set the initial value
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /*Create a label below the slider*/
    slider_label = lv_label_create(lv_scr_act());
    const char *isvd = initial_string_value_displayed.c_str();
    lv_label_set_text(slider_label, isvd);

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

}

static void lv_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) 
{
  #if defined(TOUCH_READ_FROM_INTERRNUPT)
    if (get_int_signal) {
      get_int_signal = false;
        touch.read();
  #else
    if (touch.read()) {
  #endif
      String str_buf;

      TP_Point t = touch.getPoint(0);
      // Format x and y values with leading zeros
        String x_value = String(t.x, DEC);
        String y_value = String(t.y, DEC);
        x_value = String("000" + x_value).substring(x_value.length());
        y_value = String("000" + y_value).substring(y_value.length());
        str_buf += "touch coordinates - t.x: " + x_value + " t.y: " + y_value;
        tft.drawString(str_buf, 2, 10);
      
      switch(screen_rotation)

      { 
        /* rotation done by TFT_eSPI library
          the following has to be done, because Touchlib.h does not 
          rotate with screen rotation, so coordinates have to be transformed.
        */
        
        case 0:
          // no screen rotation
          // touch region of button.
          break;

        case 1:
          // screen rotation by 90 degree clockwise
          // touch region of button. Display with rotation
          data->point.x = t.y;
          data->point.y = 169 - t.x;
          //data->point.x = t.x;
          //data->point.y = t.y;
        break;
        case 2:
          // screen rotation by 180 degree
          // touch region of button. Display with rotation
          break;
        case 3:
          // screen rotation by 270 degree
          // touch region of button. Display with rotation
          data->point.x = 319-t.y;
          data->point.y = t.x;
          break;
      }
      
      data->state = LV_INDEV_STATE_PR;
      //lv_msg_send(MSG_NEW_TOUCH_POINT, str_buf.c_str());
    } else
        data->state = LV_INDEV_STATE_REL;
  }

void loop() {
    lv_timer_handler(); /* let the GUI do its work */
    delay( 5 );
}
