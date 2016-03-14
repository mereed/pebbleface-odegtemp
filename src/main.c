/*

Copyright (C) 2016 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

-------------------------------------------------------------------

*/

#include "pebble.h"
#include "effect_layer.h"
	
EffectLayer* effect_layer;

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_CLEAR_DAY,
  RESOURCE_ID_CLEAR_NIGHT,
  RESOURCE_ID_WINDY,
  RESOURCE_ID_COLD,
  RESOURCE_ID_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_HAZE,
  RESOURCE_ID_CLOUD,
  RESOURCE_ID_RAIN,
  RESOURCE_ID_SNOW,
  RESOURCE_ID_HAIL,
  RESOURCE_ID_CLOUDY,
  RESOURCE_ID_STORM,
  RESOURCE_ID_FOG,
  RESOURCE_ID_NA,
};

static int invert;
static int bluetoothvibe;
static int hourlyvibe;
static int hide_bt;
static int hide_batt;
static int hide_date;
static int hidetemp;
static int hidewicon;
static int min;
static int hidesep;
static int blink;

static bool appStarted = false;

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,
  WEATHER_TEMPERATURE_KEY = 0x1,
  INVERT_COLOR_KEY = 0x2,	  
  BLUETOOTHVIBE_KEY = 0x3,
  HOURLYVIBE_KEY = 0x4,
  HIDE_BT_KEY = 0x5,
  HIDE_BATT_KEY = 0x6,
  HIDE_DATE_KEY = 0x7,
  HIDE_WTEMP_KEY = 0x8,
  HIDE_WICON_KEY = 0x9,
  MIN_KEY = 0xB,
  HIDE_SEP_KEY = 0xC,
  BLINK_KEY = 0xD
};

static Window *window;
static Layer *window_layer;

BitmapLayer *layer_conn_img;
GBitmap *img_bt_connect;
GBitmap *img_bt_disconnect;
BitmapLayer *icon_layer;
GBitmap *icon_bitmap = NULL;
TextLayer *temp_layer;

BitmapLayer *layer_batt_img;
GBitmap *img_battery_100;
GBitmap *img_battery_90;
GBitmap *img_battery_80;
GBitmap *img_battery_70;
GBitmap *img_battery_60;
GBitmap *img_battery_50;
GBitmap *img_battery_40;
GBitmap *img_battery_30;
GBitmap *img_battery_20;
GBitmap *img_battery_10;
GBitmap *img_battery_charge;
int charge_percent = 0;

#ifdef PBL_COLOR
static GBitmap *shoe_image;
static BitmapLayer *shoe_layer;
#endif

static GBitmap *separator_image;
static BitmapLayer *separator_layer;

static GBitmap *hide_separator_image;
static BitmapLayer *hide_separator_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

static GBitmap *time_digits_images2[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers2[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

const int BIG_DIGIT2_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0b,
  RESOURCE_ID_IMAGE_NUM_1b,
  RESOURCE_ID_IMAGE_NUM_2b,
  RESOURCE_ID_IMAGE_NUM_3b,
  RESOURCE_ID_IMAGE_NUM_4b,
  RESOURCE_ID_IMAGE_NUM_5b,
  RESOURCE_ID_IMAGE_NUM_6b,
  RESOURCE_ID_IMAGE_NUM_7b,
  RESOURCE_ID_IMAGE_NUM_8b,
  RESOURCE_ID_IMAGE_NUM_9b
};

const int TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9
};

static GFont custom_font;
static GFont custom_font2;

static AppSync sync;
static uint8_t sync_buffer[256];

#ifdef PBL_COLOR

static TextLayer *steps_label;
static bool status_showing = false;
static AppTimer *display_timer;


static void health_handler(HealthEventType event, void *context) {
  static char s_value_buffer[20];
  if (event == HealthEventMovementUpdate) {
    // display the step count
    snprintf(s_value_buffer, sizeof(s_value_buffer), "%d", (int)health_service_sum_today(HealthMetricStepCount));
	  
    text_layer_set_text(steps_label, s_value_buffer);
  }
}
#endif

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void set_invert_color(bool invert) {
  if (invert && effect_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

#ifdef PBL_PLATFORM_CHALK
    effect_layer = effect_layer_create(GRect(0, 82, 180, 58));
#else
	  effect_layer = effect_layer_create(GRect(0, 83, 144, 58));
#endif
    layer_add_child(window_layer, effect_layer_get_layer(effect_layer));
	      effect_layer_add_effect(effect_layer, effect_invert, NULL);

  } else if (!invert && effect_layer != NULL) {
    // Remove Inverter layer
    layer_remove_from_parent(effect_layer_get_layer(effect_layer));
    effect_layer_destroy(effect_layer);
    effect_layer = NULL;
  }
  // No action required
}

void hide_batt_now(bool hide_batt) {
	
	if (hide_batt) {
		layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), true);		
	} else {
		layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), false);
	}
}

void hide_bt_now(bool hide_bt) {
	
	if (hide_bt) {
		layer_set_hidden(bitmap_layer_get_layer(layer_conn_img), true);
		
	} else {
		layer_set_hidden(bitmap_layer_get_layer(layer_conn_img), false);
	}
}

void hide_temp(bool hidetemp) {
	
	if (hidetemp) {
		layer_set_hidden(text_layer_get_layer(temp_layer), true);
	} else {
		layer_set_hidden(text_layer_get_layer(temp_layer), false);	
	}
}

void hide_wicon(bool hidewicon) {
	
	if (hidewicon) {
		layer_set_hidden(bitmap_layer_get_layer(icon_layer), true);
	} else {
		layer_set_hidden(bitmap_layer_get_layer(icon_layer), false);	
	}
}

void hide_date_now(bool hide_date) {
	
	if (hide_date) {
		layer_set_hidden(bitmap_layer_get_layer(day_name_layer), true);
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), true);
			}
	} else {
		layer_set_hidden(bitmap_layer_get_layer(day_name_layer), false);
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), false);
			}
	}
}

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {	

  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
    case WEATHER_ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      icon_bitmap = gbitmap_create_with_resource(
          WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case WEATHER_TEMPERATURE_KEY:
      text_layer_set_text(temp_layer, new_tuple->value->cstring);
      break;

	case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
	  persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
      break;
	  
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;	  

	case HIDE_BATT_KEY:
	  hide_batt = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_BATT_KEY, hide_batt);
	  hide_batt_now(hide_batt);
	  break;
	  
	case HIDE_BT_KEY:
	  hide_bt = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_BT_KEY, hide_bt);
	  hide_bt_now(hide_bt);
	  break;
	  
	case HIDE_DATE_KEY:
      hide_date = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_DATE_KEY, hide_date);	  
	  hide_date_now(hide_date);
	  break;
	  
   case HIDE_WTEMP_KEY:
      hidetemp = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_WTEMP_KEY, hidetemp);	  
	  hide_temp(hidetemp);
	  break;
	  
	case HIDE_WICON_KEY:
      hidewicon = new_tuple->value->uint8 !=0;
	  persist_write_bool(HIDE_WICON_KEY, hidewicon);
	  hide_wicon(hidewicon);
     break;

	case HIDE_SEP_KEY:
      hidesep = new_tuple->value->uint8 !=0;
	  persist_write_bool(HIDE_SEP_KEY, hidesep);

      if(hidesep) {
		layer_set_hidden(bitmap_layer_get_layer(hide_separator_layer), false);
	} else {
		layer_set_hidden(bitmap_layer_get_layer(hide_separator_layer), true);	
	}
     break;
	  
    case BLINK_KEY:
      blink = new_tuple->value->uint8 !=0;
      persist_write_bool(BLINK_KEY, blink);	  

      tick_timer_service_unsubscribe();
      if(blink) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else {
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
      }
    break;
	  
	case MIN_KEY:
      min = new_tuple->value->uint8 != 0;
	  persist_write_bool(MIN_KEY, min);	
	  
	    if (min) {
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[2]), false);
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[3]), false);	
		} else {
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[2]), true);
		layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[3]), true);				  
		}
	  break;
  }

  // Refresh display

  // Get current time
  time_t now2 = time( NULL );
  struct tm *tick_time = localtime( &now2 );

  // Force update to Refresh display
  handle_tick( tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);
		
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
	
  GRect bounds = gbitmap_get_bounds(*bmp_image);

  GRect main_frame = GRect(origin.x, origin.y, bounds.size.w, bounds.size.h);
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), main_frame);

  if (old_image != NULL) {
  	gbitmap_destroy(old_image);
  }
}

void handle_battery(BatteryChargeState charge_state) {

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, img_battery_charge);
    } else {
        if (charge_state.charge_percent <= 10) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_10);
        } else if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_20);
        } else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_30);
		} else if (charge_state.charge_percent <= 40) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_40);
		} else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_50);
    	} else if (charge_state.charge_percent <= 60) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_60);	
        } else if (charge_state.charge_percent <= 70) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_70);
		} else if (charge_state.charge_percent <= 80) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_80);
		} else if (charge_state.charge_percent <= 90) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_90);
		} else if (charge_state.charge_percent <= 100) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);			
			   
        if (charge_state.charge_percent < charge_percent) {
            if (charge_state.charge_percent==20){
                vibes_double_pulse();
            } else if(charge_state.charge_percent==10){
                vibes_long_pulse();
            }
        }
    }
    charge_percent = charge_state.charge_percent;   
  }
}

void handle_bluetooth(bool connected) {
    if (connected) {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    } else {
        bitmap_layer_set_bitmap(layer_conn_img, img_bt_disconnect);
    }

    if (appStarted && bluetoothvibe) {
      
        vibes_long_pulse();
	}
}
void force_update(void) {
    handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}
	
static void update_days(struct tm *tick_time) {
#ifdef PBL_PLATFORM_CHALK
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint( 20, 56));
#else
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint( 43, 57));
#endif

#ifdef PBL_PLATFORM_CHALK
  set_container_image(&date_digits_images[0], date_digits_layers[0], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(110, 56));
  set_container_image(&date_digits_images[1], date_digits_layers[1], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(135, 56));
#else	
  set_container_image(&date_digits_images[0], date_digits_layers[0], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(95, 57));
  set_container_image(&date_digits_images[1], date_digits_layers[1], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(120, 57));
#endif

}

static void update_hours(struct tm *tick_time) {
  
  if (appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  }
	
   unsigned short display_hour = get_display_hour(tick_time->tm_hour);

#ifdef PBL_PLATFORM_CHALK
  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(18, 85));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(54, 85));	
#else
  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(1, 84));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(36, 84));	
#endif
	
  
  if (!clock_is_24h_style()) {
    if (display_hour/10 == 0) {
    	layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    } else {
    	layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}

static void update_minutes(struct tm *tick_time) {
	

#ifdef PBL_PLATFORM_CHALK
  set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(95, 85));
  set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(128, 85));		
	
  set_container_image(&time_digits_images2[2], time_digits_layers2[2], BIG_DIGIT2_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(95, 85));
  set_container_image(&time_digits_images2[3], time_digits_layers2[3], BIG_DIGIT2_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(128, 85));
  
#else
  set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(76, 84));
  set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(111, 84));
	
  set_container_image(&time_digits_images2[2], time_digits_layers2[2], BIG_DIGIT2_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(76, 84));
  set_container_image(&time_digits_images2[3], time_digits_layers2[3], BIG_DIGIT2_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(111, 84));
#endif

}

static void update_seconds(struct tm *tick_time) {
  if(blink) {
    layer_set_hidden(bitmap_layer_get_layer(separator_layer), tick_time->tm_sec%2);
  }
  else {
    if(layer_get_hidden(bitmap_layer_get_layer(separator_layer))) {
      layer_set_hidden(bitmap_layer_get_layer(separator_layer), false);
    }
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
    update_minutes(tick_time);
  }	 
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }	
}

#ifdef PBL_COLOR

// Hides seconds
void hide_status() {
	status_showing = false;

    health_service_events_unsubscribe();
				
    layer_set_hidden(text_layer_get_layer(steps_label), true); 
    layer_set_hidden(bitmap_layer_get_layer(shoe_layer), true); 
#ifdef PBL_PLATFORM_CHALK
    layer_set_hidden(bitmap_layer_get_layer(layer_conn_img), false); 
#endif
    layer_set_hidden(bitmap_layer_get_layer(day_name_layer), false); 
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), false);
	}
}

// Shows seconds
void show_status() {
	status_showing = true;	
    health_service_events_subscribe(health_handler, NULL);
    health_handler(HealthEventMovementUpdate, NULL);

	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), true);
			} 
    layer_set_hidden(bitmap_layer_get_layer(day_name_layer), true); 
#ifdef PBL_PLATFORM_CHALK
    layer_set_hidden(bitmap_layer_get_layer(layer_conn_img), true); 
#endif
	layer_set_hidden(text_layer_get_layer(steps_label), false); 
    layer_set_hidden(bitmap_layer_get_layer(shoe_layer), false); 
	
	// 10 Sec timer then call "hide_status". Cancels previous timer if another show_status is called within the 10000ms
	app_timer_cancel(display_timer);
	display_timer = app_timer_register(10000, hide_status, NULL);
}

// Shake/Tap Handler. On shake/tap... call "show_status"
void tap_handler(AccelAxisType axis, int32_t direction) {
	show_status();	
}

#endif

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
  memset(&time_digits_layers2, 0, sizeof(time_digits_layers2));
  memset(&time_digits_images2, 0, sizeof(time_digits_images2));
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));

 // Setup messaging
  const int inbound_size = 256;
  const int outbound_size = 256;
  app_message_open(inbound_size, outbound_size);	
	
  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }

  window_set_background_color(window, GColorBlack);

  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);

	
	// resources
	img_bt_connect     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHON);
    img_bt_disconnect  = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTHOFF);
	
    img_battery_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_090_100);
    img_battery_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_080_090);
    img_battery_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_070_080);
    img_battery_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_060_070);
    img_battery_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_050_060);
    img_battery_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_040_050);
    img_battery_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_030_040);
    img_battery_30    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_020_030);
    img_battery_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_010_020);
    img_battery_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_000_010);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);

  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITALIX_14));
  custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITALIX_16));
	
	// layers

#ifdef PBL_PLATFORM_CHALK
    layer_batt_img  = bitmap_layer_create(GRect(50, 156, 79, 17));
    bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
	layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
    layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), true);			
#else
    layer_batt_img  = bitmap_layer_create(GRect(3, 144, 35, 11));
    bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
	layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
    layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), true);			
#endif

#ifdef PBL_PLATFORM_CHALK
	layer_conn_img  = bitmap_layer_create(GRect(78, 61, 22, 16));
    bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img)); 
    layer_set_hidden(bitmap_layer_get_layer(layer_conn_img), true);			
#else
	layer_conn_img  = bitmap_layer_create(GRect(50, 142, 22, 16));
    bitmap_layer_set_bitmap(layer_conn_img, img_bt_connect);
    layer_add_child(window_layer, bitmap_layer_get_layer(layer_conn_img)); 
#endif	

 Layer *weather_holder = layer_create(GRect(0, 0, 144, 168 ));
  layer_add_child(window_layer, weather_holder);
#ifdef PBL_PLATFORM_CHALK
  icon_layer = bitmap_layer_create(GRect(0, 0, 180, 55));
  layer_add_child(weather_holder, bitmap_layer_get_layer(icon_layer));
#else
  icon_layer = bitmap_layer_create(GRect(0, 0, 144, 55));
  layer_add_child(weather_holder, bitmap_layer_get_layer(icon_layer));
#endif
	
#ifdef PBL_PLATFORM_CHALK
  temp_layer = text_layer_create(GRect(0, 139, 184, 30));
  text_layer_set_text_color(temp_layer, GColorWhite);
  text_layer_set_background_color(temp_layer, GColorClear);
  text_layer_set_font(temp_layer, custom_font);	 
  text_layer_set_text_alignment(temp_layer, GTextAlignmentCenter);
  layer_add_child(weather_holder, text_layer_get_layer(temp_layer));
#else
  temp_layer = text_layer_create(GRect(0, 140, 144, 30));
  text_layer_set_text_color(temp_layer, GColorWhite);
  text_layer_set_background_color(temp_layer, GColorClear);
  text_layer_set_font(temp_layer, custom_font);	 
  text_layer_set_text_alignment(temp_layer, GTextAlignmentRight);
  layer_add_child(weather_holder, text_layer_get_layer(temp_layer));
#endif


  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
   day_name_layer = bitmap_layer_create(dummy_frame);
   layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));	
	
    for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	
    time_digits_layers2[0] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers2[0]));
    layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[0]), true);			

    time_digits_layers2[1] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers2[1]));
	layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[1]), true);			

    time_digits_layers2[2] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers2[2]));
	layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[2]), true);			
	
    time_digits_layers2[3] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers2[3]));
    layer_set_hidden(bitmap_layer_get_layer(time_digits_layers2[3]), true);			

		
    for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
	

#ifdef PBL_PLATFORM_CHALK
  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEPARATOR);
  GRect bitmap_bounds = gbitmap_get_bounds(separator_image);
  GRect frame2 = GRect(87, 95, bitmap_bounds.size.w, bitmap_bounds.size.h);
  separator_layer = bitmap_layer_create(frame2);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer)); 		
#else
  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEPARATOR);
  GRect bitmap_bounds = gbitmap_get_bounds(separator_image);
  GRect frame = GRect(69, 95, bitmap_bounds.size.w, bitmap_bounds.size.h);
  separator_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer)); 
#endif	

		
#ifdef PBL_PLATFORM_CHALK
  hide_separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HIDE);
  GRect bitmap_bounds_hidesep = gbitmap_get_bounds(hide_separator_image);
  GRect frame_hide = GRect(87, 95, bitmap_bounds_hidesep.size.w, bitmap_bounds_hidesep.size.h);
  hide_separator_layer = bitmap_layer_create(frame_hide);
  bitmap_layer_set_bitmap(hide_separator_layer, hide_separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(hide_separator_layer)); 		
#else
  hide_separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HIDE);
  GRect bitmap_bounds_hidesep = gbitmap_get_bounds(hide_separator_image);
  GRect frame_hide2 = GRect(69, 95, bitmap_bounds_hidesep.size.w, bitmap_bounds_hidesep.size.h);
  hide_separator_layer = bitmap_layer_create(frame_hide2);
  bitmap_layer_set_bitmap(hide_separator_layer, hide_separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(hide_separator_layer)); 
#endif	
	
#ifdef PBL_COLOR
	
	  // draw shoe icon
  shoe_image = gbitmap_create_with_resource( RESOURCE_ID_IMAGE_SHOE );
#ifdef PBL_PLATFORM_CHALK
  shoe_layer = bitmap_layer_create( GRect(77,49,26,15) );
  bitmap_layer_set_bitmap( shoe_layer, shoe_image );
  GCompOp compositing_shoe = GCompOpSet;	
  bitmap_layer_set_compositing_mode(shoe_layer, compositing_shoe);	
#else
  shoe_layer = bitmap_layer_create(GRect(117, 65, 26, 15));
#endif
  bitmap_layer_set_bitmap( shoe_layer, shoe_image );
  layer_add_child( window_layer, bitmap_layer_get_layer( shoe_layer ) );
  layer_set_hidden(bitmap_layer_get_layer(shoe_layer), true);		
	
	
  steps_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(20, 65, 140, 21),
    GRect(0, 64, 112, 18)));
  text_layer_set_background_color(steps_label, GColorClear);
  text_layer_set_text_color(steps_label, GColorWhite);
	#ifdef PBL_PLATFORM_CHALK
  text_layer_set_text_alignment(steps_label, GTextAlignmentCenter);
#else
  text_layer_set_text_alignment(steps_label, GTextAlignmentRight);
#endif
  text_layer_set_font(steps_label, custom_font);
//  text_layer_set_font(steps_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(steps_label));
  layer_set_hidden(text_layer_get_layer(steps_label), true);
#endif
	
	
Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 14),
    TupletCString(WEATHER_TEMPERATURE_KEY, ""),
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
	TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(HIDE_BT_KEY, persist_read_bool(HIDE_BT_KEY)),
    TupletInteger(HIDE_BATT_KEY, persist_read_bool(HIDE_BATT_KEY)),
	TupletInteger(HIDE_DATE_KEY, persist_read_bool(HIDE_DATE_KEY)),
	TupletInteger(HIDE_WTEMP_KEY, persist_read_bool(HIDE_WTEMP_KEY)),
	TupletInteger(HIDE_WICON_KEY, persist_read_bool(HIDE_WICON_KEY)),
	TupletInteger(MIN_KEY, persist_read_bool(MIN_KEY)),
	TupletInteger(HIDE_SEP_KEY, persist_read_bool(HIDE_SEP_KEY)),
	TupletInteger(BLINK_KEY, persist_read_bool(BLINK_KEY)),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);

  appStarted = true;
	
  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);	

  // handlers
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
#ifdef PBL_COLOR
  accel_tap_service_subscribe(tap_handler);
  //hide_status();
#endif
	
  // draw first frame
  force_update();
	
}

static void deinit(void) {
  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
	
#ifdef PBL_COLOR
  accel_tap_service_unsubscribe();
	
  layer_remove_from_parent(text_layer_get_layer(steps_label));
  text_layer_destroy(steps_label);	
	
  layer_remove_from_parent(bitmap_layer_get_layer(shoe_layer));
  bitmap_layer_destroy(shoe_layer);
  gbitmap_destroy(shoe_image);	
#endif
	
  layer_remove_from_parent(text_layer_get_layer(temp_layer));
  text_layer_destroy(temp_layer);
	
  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(hide_separator_layer));
  bitmap_layer_destroy(hide_separator_layer);
  gbitmap_destroy(hide_separator_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);
  gbitmap_destroy(img_battery_100);
  gbitmap_destroy(img_battery_90);
  gbitmap_destroy(img_battery_80);
  gbitmap_destroy(img_battery_70);
  gbitmap_destroy(img_battery_60);
  gbitmap_destroy(img_battery_50);
  gbitmap_destroy(img_battery_40);
  gbitmap_destroy(img_battery_30);
  gbitmap_destroy(img_battery_20);
  gbitmap_destroy(img_battery_10);
  gbitmap_destroy(img_battery_charge);	
	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(icon_layer));
  bitmap_layer_destroy(icon_layer);
  gbitmap_destroy(icon_bitmap);
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_conn_img));
  bitmap_layer_destroy(layer_conn_img);
  gbitmap_destroy(img_bt_connect);
  gbitmap_destroy(img_bt_disconnect);
	
	for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    bitmap_layer_destroy(date_digits_layers[i]);
  }
	
   for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
  } 
	
   for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers2[i]));
    gbitmap_destroy(time_digits_images2[i]);
    bitmap_layer_destroy(time_digits_layers2[i]);
  } 
	
  fonts_unload_custom_font(custom_font);
  fonts_unload_custom_font(custom_font2);

	 if (effect_layer != NULL) {
	  effect_layer_destroy(effect_layer);
  }
  // Destroy window
  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
	
	//window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}