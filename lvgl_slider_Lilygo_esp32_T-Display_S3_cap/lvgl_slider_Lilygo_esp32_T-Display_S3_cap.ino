/* usage of lvgl library on Lilygo T-Display S3 touch version
   for touch processing the TouchLib library is used

   author: Rudi Ackermann - Mai, 2023
*/

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "pin_config.h"
#include <string.h>

static lv_style_t style_btn;
static lv_style_t style_btn_pressed;
static lv_style_t style_btn_red;
static lv_obj_t * label;

#define darkgray 0x7BEF
static bool is_initialized_lvgl = false;
/* Please make sure your touch IC model. */
//#define TOUCH_MODULES_CST_MUTUAL
#define TOUCH_MODULES_CST_SELF
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
    Serial.begin( 115200 ); /* prepare for possible serial debug */
      while(!Serial);
    //Serial.println("free heap: "); Serial.println(ESP.getFreeHeap());

    String LVGL_Arduino = "Hello Liliygo ESP32 T-Display S3 !\n";
    LVGL_Arduino += "LVGL " +String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    #if LV_USE_LOG != 0
        lv_log_register_print_cb( my_print ); /* register print function for debugging */
    #endif

    tft.begin();          /* TFT init */
    tft.setRotation(screen_rotation); /* Landscape orientation, flipped */
    tft.setTextSize(1); // set text size
    tft.setTextColor(TFT_BLACK,darkgray);

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
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(127, 127,127) , LV_PART_MAIN);

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

    // draw widgets, here: draw button
        lv_example_get_started_1();
    //draw slider
        lv_example_get_started_3();
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

// Declare the button event handler function
//static void button_event_handler(lv_obj_t * obj, lv_event_t event);

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

static lv_color_t darken(const lv_color_filter_dsc_t * dsc, lv_color_t color, lv_opa_t opa)
{
    LV_UNUSED(dsc);
    return lv_color_darken(color, opa);
}
static void style_init(void)
{
    /*Create a simple button style*/
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 10);
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_bg_color(&style_btn_red, lv_palette_main(LV_PALETTE_RED));

    lv_style_set_border_color(&style_btn, lv_color_black());
    lv_style_set_border_opa(&style_btn, LV_OPA_20);
    lv_style_set_border_width(&style_btn, 4);

    lv_style_set_text_color(&style_btn, lv_color_white());

    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_palette_lighten(LV_PALETTE_GREY, 3));

}
/**
 * Create a button with a label and react on click event.
 */
void lv_example_get_started_1(void)
  {
      style_init();
      lv_obj_t * btn = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/
      lv_obj_set_pos(btn, 4, 110);                    /*Set its position*/
      lv_obj_set_size(btn, 100, 50);                    /*Set its size*/
      lv_obj_add_style(btn, &style_btn, 0);
      
      lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);  /*Assign a callback to the button*/

      lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
      lv_label_set_text(label, "Button");               /*Set the labels text*/
      lv_obj_center(label);
  }

static void slider_event_cb(lv_event_t * e)
  {
      lv_obj_t * slider = lv_event_get_target(e);

      /*Refresh the text*/
      lv_label_set_text_fmt(label, "%"LV_PRId32, lv_slider_get_value(slider));
      lv_obj_align_to(label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);    /*Align top of the slider*/
  }

/**
 * Create a slider and write its value on a label.
 */
void lv_example_get_started_3(void)
{
    /*Create a slider in the center of the display*/
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);                          /*Set the width*/
    //lv_obj_set_pos(slider, 70, 75);
    lv_obj_center(slider);
    // Set the range of the slider
    lv_slider_set_range(slider, 20, 100);                                  /*Align to the center of the parent (screen)*/
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);     /*Assign an event function*/

    /*Create a label above the slider*/    
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "20");
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);    /*Align label of the slider*/
    // Set the initial value of the slider
    lv_slider_set_value(slider, 20, LV_ANIM_OFF);

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

/*void transform_coordinates(int option)
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
            data->point.x = t.y;
            data->point.y = 169 - t.x;
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
            data->point.x = 319-t.y;
            data->point.y = t.x;
            break;
    }
}
*/
