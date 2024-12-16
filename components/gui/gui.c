#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "gui.h"
#include "gui_positions_and_units.h"

#include "esp_log.h"
#include "sdkconfig.h"
#include "common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_freertos_hooks.h"

#include "icons/icons.h"

#include "mqtt_config.h"
#include "lvgl_helpers.h"

#define LV_TICK_PERIOD                1
#define MS                            1000
#define GUI_TASK_DELAY_MS             1

// SemaphoreHandle_t xGuiSemaphore;
SemaphoreHandle_t xLblSemaphore = NULL;

lv_obj_t *imgbtn_ac = NULL;
lv_obj_t *imgbtn_light = NULL;
lv_obj_t *imgbtn_settings = NULL;

lv_obj_t *power_ac_btn = NULL;
lv_obj_t *lbl_power_ac = NULL;

lv_obj_t *decrease_temp_btn = NULL;
lv_obj_t *lbl_decrease_temp = NULL;

lv_obj_t *increase_temp_btn = NULL;
lv_obj_t *lbl_increase_temp = NULL;

lv_obj_t *btnmatrix_lights = NULL;

lv_obj_t *win_ac = NULL;
lv_obj_t *win_light = NULL;
lv_obj_t *win_settings = NULL;
lv_obj_t *win_content = NULL;
lv_obj_t *win_tv_control = NULL;
lv_obj_t *win_heater_temp = NULL;

lv_obj_t *lbl_setTmp = NULL;

lv_obj_t *lbl_room = NULL;
lv_obj_t *lbl_time = NULL;

lv_obj_t *btnmatrix_settings_1 = NULL;
lv_obj_t *btnmatrix_settings_2 = NULL;

lv_obj_t *ta_heater_temp = NULL;
lv_obj_t *kb_heater_temp = NULL;

lv_obj_t *dropdwn_tv_time = NULL;
lv_obj_t *dropdwn_tv_channel = NULL;
lv_obj_t *btn_tv_time_channel = NULL;

lv_style_t ac_style;
lv_style_t light_style;
lv_style_t settings_style;
lv_style_t style_label_room;

bool is_ac_enabled = false;
bool is_heater_enabled = false;
double temperature_value_c = 17.00;
double temperature_value_f = 0.0;
bool is_converted_to_f = false;

gui_mqtt msg = {0};

char strftime_buf[256];

static void ac_control();
static void light_control();
static void settings_control();

static void open_control(lv_obj_t *btn, lv_event_t event);

static void label_time_event(lv_obj_t *label, lv_event_t event);

static void ac_control_style();
static void ac_control_cb();
static void turn_ac(lv_obj_t *btn, lv_event_t event);

static void manage_temp(lv_obj_t *btn, lv_event_t event);
static void blinds_control(lv_obj_t *slider, lv_event_t event);

static void btnmatrix_control(lv_obj_t *btnmatrix, lv_event_t event);
static void hide_ac_event(lv_obj_t *hide_ac, lv_event_t event);
static void hide_light_event(lv_obj_t *hide_light, lv_event_t event);
static void hide_settings_event(lv_obj_t *hide_settings, lv_event_t event);

static void btnmatrix_style(lv_style_t * style, lv_obj_t * btnmatrix);

static void settings1_cb(lv_obj_t *btn_matrix, lv_event_t event);
static void settings2_cb(lv_obj_t *btn_matrix, lv_event_t event);
static void heater_temp();
static void tv_control();

void gui(void *pvParameter);
static void lv_tick_hook(void *pvData);

static void send_data(uint8_t data, char *topic)
{
    memset(&msg, 0, sizeof(msg));
    msg.value = data;
    msg.topic = topic;
    xEventGroupSetBits(get_mqtt_cfg_handle(), 1 << 4);
}

static void c_to_f(double temperature)
{
    temperature_value_f = ((temperature * 9)/5) + 32;
}

void refresh_lbl_time()
{
    BaseType_t refresh_state;
    if (pdTRUE == xSemaphoreTakeFromISR(xLblSemaphore, &refresh_state))
    {
        lv_event_send(lbl_time, LV_EVENT_REFRESH, NULL);
        xSemaphoreGiveFromISR(xLblSemaphore, &refresh_state);
    }
}


static void configure_initial_widgets()
{
    lv_obj_align(lbl_time, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_label_set_text(lbl_time, strftime_buf);
    lv_obj_set_event_cb(lbl_time, label_time_event);

    lv_obj_align(imgbtn_ac, NULL, LV_ALIGN_CENTER, IMGBTN_AC_X_POS, IMGBTN_AC_Y_POS);
    lv_imgbtn_set_checkable(imgbtn_ac, true);
    lv_obj_set_event_cb(imgbtn_ac, open_control);

    lv_obj_align(imgbtn_light, NULL, LV_ALIGN_CENTER, IMGBTN_LIGHT_X_POS, IMGBTN_LIGHT_Y_POS);
    lv_imgbtn_set_checkable(imgbtn_light, true);
    lv_obj_set_event_cb(imgbtn_light, open_control);

    lv_obj_align(imgbtn_settings, NULL, LV_ALIGN_CENTER, IMGBTN_SETTINGS_X_POS, IMGBTN_SETTINGS_Y_POS);
    lv_imgbtn_set_checkable(imgbtn_settings, true);
    lv_obj_set_event_cb(imgbtn_settings, open_control);
}

static void configure_ac_widgets()
{
    lv_obj_set_style_local_bg_color(win_ac, LV_WIN_PART_BG, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_bg_color(win_ac, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_win_set_scrollbar_mode(win_ac, LV_SCROLLBAR_MODE_OFF);

    power_ac_btn = lv_btn_create(win_content, NULL);
    lbl_power_ac = lv_label_create(power_ac_btn, NULL);

    decrease_temp_btn = lv_btn_create(win_content, NULL);
    lbl_decrease_temp = lv_label_create(decrease_temp_btn, NULL);

    increase_temp_btn = lv_btn_create(win_content, NULL);
    lbl_increase_temp = lv_label_create(increase_temp_btn, NULL);

    lbl_setTmp = lv_label_create(win_content, NULL);
    lv_label_set_text(lbl_setTmp, "");

    ac_control_style();
    ac_control_cb();
}

/** @brief Create initial gui */
void gui_init()
{
    lv_style_init(&style_label_room);
    lv_style_set_text_font(&style_label_room, LV_STATE_DEFAULT, &lv_font_montserrat_48);

    LV_THEME_DEFAULT_INIT
    (
            lv_theme_get_color_primary(), 
            lv_theme_get_color_secondary(),
            LV_THEME_MATERIAL_FLAG_DARK,
            lv_theme_get_font_small(), 
            lv_theme_get_font_normal(), 
            lv_theme_get_font_subtitle(), 
            lv_theme_get_font_title()
    );

    lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, \
        LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lbl_time = lv_label_create(lv_scr_act(), NULL);
    

    imgbtn_ac = lv_imgbtn_create(lv_scr_act(), NULL);
    imgbtn_light = lv_imgbtn_create(lv_scr_act(), NULL);
    imgbtn_settings = lv_imgbtn_create(lv_scr_act(), NULL);

    lv_imgbtn_set_src(imgbtn_ac, LV_BTN_STATE_RELEASED, get_ac_unit());
    lv_imgbtn_set_src(imgbtn_light, LV_BTN_STATE_RELEASED, get_lights_unit());
    lv_imgbtn_set_src(imgbtn_settings, LV_BTN_STATE_RELEASED, get_settings_unit());

    strftime(strftime_buf, sizeof(strftime_buf), "%c", get_tm());

    configure_initial_widgets();
}


static void convert_c_to_f(lv_obj_t *lbl, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event && false == is_converted_to_f)
    {
        is_converted_to_f = true;
        ESP_LOGI("", "Clicked to f");
        lv_event_send_refresh(lbl_setTmp);
    }
    else if (LV_EVENT_CLICKED == event && true == is_converted_to_f)
    {
        is_converted_to_f = false;
    }
    lv_event_send_refresh(lbl_setTmp);
}

/** @brief Window for controlling the AC unit
 * 
*/

static void ac_control()
{
    lv_obj_t *lbl_f;
    win_ac = lv_win_create(lv_scr_act(), NULL);

    lv_win_set_title(win_ac, "Air Condition Settings");
    lv_obj_t *win_close = lv_win_add_btn_right(win_ac, LV_SYMBOL_CLOSE);

    win_content = lv_win_get_content(win_ac);
    // lv_obj_set_parent(lbl_time, win_ac);

    lv_obj_set_event_cb(win_close, hide_ac_event);

    lbl_f = lv_label_create(win_content, NULL);
    lv_label_set_text(lbl_f, "Temperature C/F");
    lv_obj_align(lbl_f, NULL, LV_ALIGN_IN_TOP_RIGHT, -100, 10);
    lv_obj_set_click(lbl_f, true);
    lv_obj_set_event_cb(lbl_f, convert_c_to_f);

    configure_ac_widgets();

}

/**
 * @brief ekran za kontrolu svetla
 * 
 */
static void light_control()
{
    win_light = lv_win_create(lv_scr_act(), NULL);

    lv_win_set_title(win_light, "Light Settings");

    lv_obj_t *win_close = lv_win_add_btn_right(win_light, LV_SYMBOL_CLOSE);

    win_content = lv_win_get_content(win_light);

    lv_obj_set_event_cb(win_close, hide_light_event);

    lv_obj_set_style_local_bg_color(win_light, LV_WIN_PART_BG, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_bg_color(win_light, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_win_set_scrollbar_mode(win_light, LV_SCROLLBAR_MODE_OFF);

    static const char *bntmatrix_btns[] =
    {
        "Lights ON",
        "Lights OFF",
        "Work Scene",

        "\n",

        "Lunch Scene",
        "Relax Scene",
        "Night Scene",

        ""
    };

    btnmatrix_lights = lv_btnmatrix_create(win_content, NULL);

    lv_btnmatrix_set_map(btnmatrix_lights, bntmatrix_btns);
    
    lv_obj_align(btnmatrix_lights, NULL, LV_ALIGN_CENTER, BTNMATRIX_LIGHTS_X_POS, BTNMATRIX_LIGHTS_Y_POS);
    btnmatrix_style(&light_style, btnmatrix_lights);
    lv_btnmatrix_set_btn_ctrl_all(btnmatrix_lights, LV_BTNMATRIX_CTRL_CLICK_TRIG);
    lv_obj_set_event_cb(btnmatrix_lights, btnmatrix_control);

    lv_obj_set_size(btnmatrix_lights, BTNMATRIX_LIGHTS_WIDTH, BTNMATRIX_LIGHTS_HEIGHT);
}

/**
 * @brief ekran za kontrolu televizora, bojlera, vrata
 * 
 */

static void settings_control()
{

    static const char *settings_1[] =
    {
        "Open\nDoor",
        "Lock\nDoor",

        "\n",

        "On/Off\nHeater",
        "Set\nHeater\nTemp",

        ""
    };

    static const char *settings_2[] =
    {
        "TV On",
        "Do\nNot\nDisturb",

        "\n",

        "TV Off",
        "Help",

        ""
    };

    win_settings = lv_win_create(lv_scr_act(), NULL);

    lv_win_set_title(win_settings, "Other Settings");

    lv_obj_t *win_close = lv_win_add_btn_right(win_settings, LV_SYMBOL_CLOSE);

    win_content = lv_win_get_content(win_settings);

    lv_obj_set_event_cb(win_close, hide_settings_event);

    lv_obj_set_style_local_bg_color(win_settings, LV_WIN_PART_BG, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_bg_color(win_settings, LV_WIN_PART_HEADER, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_win_set_scrollbar_mode(win_settings, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *slider_blinds = lv_slider_create(win_content, NULL);
    lv_obj_set_size(slider_blinds, SLIDER_BLINDS_WIDTH, SLIDER_BLINDS_HEIGHT);
    lv_obj_align(slider_blinds, NULL, LV_ALIGN_IN_TOP_MID, SLIDER_BLINDS_X_POS, SLIDER_BLINDS_Y_POS);
    lv_slider_set_range(slider_blinds, SLIDER_BLINDS_MIN_VALUE, SLIDER_BLINDS_MAX_VALUE);

    btnmatrix_settings_1 = lv_btnmatrix_create(win_content, NULL);
    lv_btnmatrix_set_map(btnmatrix_settings_1, settings_1);
    lv_btnmatrix_set_align(btnmatrix_settings_1, LV_LABEL_ALIGN_CENTER);
	lv_obj_set_event_cb(btnmatrix_settings_1, settings1_cb);

    lv_obj_set_size(btnmatrix_settings_1, BTNMATRIX_SETTINGS_1_WIDTH, BTNMATRIX_SETTINGS_1_HEIGHT);

    btnmatrix_settings_2 = lv_btnmatrix_create(win_content, NULL);
    lv_btnmatrix_set_map(btnmatrix_settings_2, settings_2);
    lv_btnmatrix_set_align(btnmatrix_settings_2, LV_LABEL_ALIGN_CENTER);

    lv_obj_set_size(btnmatrix_settings_2, BTNMATRIX_SETTINGS_2_WIDTH, BTNMATRIX_SETTINGS_2_HEIGHT);
    lv_obj_align(btnmatrix_settings_2, NULL, LV_ALIGN_IN_TOP_RIGHT, BTNMATRIX_SETTINGS_2_X_POS, BTNMATRIX_SETTINGS_2_Y_POS);

    btnmatrix_style(&settings_style, btnmatrix_settings_1);
    btnmatrix_style(&settings_style, btnmatrix_settings_2);
    lv_obj_set_event_cb(slider_blinds, blinds_control);

    lv_obj_set_event_cb(btnmatrix_settings_2, settings2_cb);
}

static void settings1_cb(lv_obj_t *btn_matrix, lv_event_t event)
{
	if (LV_EVENT_VALUE_CHANGED == event)
	{
		switch (lv_btnmatrix_get_active_btn(btnmatrix_settings_1))
		{
			case 0:
				send_data(0, "/door/open/");
				break;
			case 1:
				send_data(1, "/door/lock/");
				break;
			case 2:
				if (true == is_heater_enabled)
				{
					is_heater_enabled = false;
				}
				else
				{
					is_heater_enabled = true;
				}
				send_data(is_heater_enabled, "/heater/status/");
				break;
			case 3:
				if (NULL == win_heater_temp)
                {
                    heater_temp();
                }
                else
                {
                     lv_obj_set_hidden(win_heater_temp, false);
                }
			default:
				break;
			}
		}
	}

	static void settings2_cb(lv_obj_t *btn_matrix, lv_event_t event)
	{
		if (LV_EVENT_VALUE_CHANGED == event)
		{
			switch (lv_btnmatrix_get_active_btn(btnmatrix_settings_2))
			{
				case 0:
					ESP_LOGI("", "TV On");
					if (NULL == win_tv_control)
					{
						tv_control();
					}
					else
					{
						lv_obj_set_hidden(win_tv_control, false);
					}
                // send_data(0, "/tv/state/");
                break;
            case 1:
                ESP_LOGI("", "Do not Disturb");
                send_data(0, "/dnd/active/");
                break;
            case 2:
                ESP_LOGI("", "TV Off");
                send_data(1, "/tv/state/");
                break;
            case 3:
                ESP_LOGI("", "Help");
                send_data(1, "/help/manual/");
                break;
        }
    }
}

static void open_control(lv_obj_t *btn, lv_event_t event)
{
    if (LV_EVENT_VALUE_CHANGED == event && btn == imgbtn_ac)
    {
        if (NULL == win_ac)
        {
            ac_control();
        }
        else
        {
            lv_obj_set_hidden(win_ac, false);
        }
    }
    else if (LV_EVENT_VALUE_CHANGED == event && btn == imgbtn_light)
    {
        if (win_light == NULL)
        {
            light_control();
        }
        else
        {
            lv_obj_set_hidden(win_light, false);
        }
    }
    else if (LV_EVENT_VALUE_CHANGED == event && btn == imgbtn_settings)
    {
        if (win_settings == NULL)
        {
            settings_control();
        }
        else
        {
            lv_obj_set_hidden(win_settings, false);
        }
    }
}

static void update_temp(lv_obj_t *btn, lv_event_t event)
{

    if (LV_EVENT_REFRESH != event)
    {
        return;
    }

    if (is_converted_to_f)
    {
        c_to_f(temperature_value_c);
        lv_label_set_text_fmt(lbl_setTmp, "%.2lf", temperature_value_f);
        send_data(temperature_value_f, "/ac/temp/");
    }
    else
    {
        lv_label_set_text_fmt(lbl_setTmp, "%.2lf", temperature_value_c);
        send_data(temperature_value_c, "/ac/temp/");
    }
}

static void ac_control_cb()
{
    lv_obj_set_event_cb(power_ac_btn, turn_ac);
    lv_obj_set_event_cb(decrease_temp_btn, manage_temp);
    lv_obj_set_event_cb(increase_temp_btn, manage_temp);
    lv_obj_set_event_cb(lbl_setTmp, update_temp);
}

static void turn_ac(lv_obj_t *btn, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {
        if (is_ac_enabled == false)
        {
            lv_obj_set_state(decrease_temp_btn, LV_STATE_DEFAULT);
            lv_obj_set_state(increase_temp_btn, LV_STATE_DEFAULT);
            is_ac_enabled = true;
        }
        else
        {
            lv_obj_set_state(decrease_temp_btn, LV_STATE_DISABLED);
            lv_obj_set_state(increase_temp_btn, LV_STATE_DISABLED);
            is_ac_enabled = false;
        }
        send_data(is_ac_enabled, "/ac/unit/");
    }
}

static void blinds_control(lv_obj_t *slider, lv_event_t event)
{
    if (LV_EVENT_VALUE_CHANGED == event)
    {
        send_data((uint8_t)lv_slider_get_value(slider), "/blinds/pos/");
    }
}

static void manage_temp(lv_obj_t *btn, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event && decrease_temp_btn == btn)
    {    
            --temperature_value_c;
    }
    else if (LV_EVENT_CLICKED == event && increase_temp_btn == btn)
    {
            ++temperature_value_c;
    }

    if (TEMPERATURE_MIN_VALUE_C > temperature_value_c)
    {
        temperature_value_c = TEMPERATURE_MIN_VALUE_C;
    }
   
    if (TEMPERATURE_MAX_VALUE_C < temperature_value_c)
    {
        temperature_value_c = TEMPERATURE_MAX_VALUE_C;
    }
    lv_event_send_refresh(lbl_setTmp);
    
}

static void ac_control_style()
{
    lv_style_init(&ac_style);

    lv_state_t LV_STATES_ARR[7] =
        {
            LV_STATE_DEFAULT,
            LV_STATE_EDITED,
            LV_STATE_CHECKED,
            LV_STATE_FOCUSED,
            LV_STATE_HOVERED,
            LV_STATE_PRESSED,
            LV_STATE_DISABLED,
        };

    for (int8_t state = 0; state < sizeof(LV_STATES_ARR); state++)
    {
        lv_style_set_text_font(&ac_style, LV_STATES_ARR[state], &lv_font_montserrat_48);
        lv_style_set_border_color(&ac_style, LV_STATES_ARR[state], LV_COLOR_BLACK);
        lv_style_set_outline_color(&ac_style, LV_STATES_ARR[state], LV_COLOR_BLACK);
        lv_style_set_shadow_color(&ac_style, LV_STATES_ARR[state], LV_COLOR_BLACK);
        lv_style_set_bg_color(&ac_style, LV_STATES_ARR[state], LV_COLOR_BLACK);
    }

    lv_label_set_text(lbl_power_ac, LV_SYMBOL_POWER);
    lv_label_set_text(lbl_decrease_temp, LV_SYMBOL_MINUS);
    lv_label_set_text(lbl_increase_temp, LV_SYMBOL_PLUS);

    lv_obj_align(power_ac_btn, NULL, LV_ALIGN_CENTER, POWER_AC_BTN_X_POS, POWER_AC_BTN_Y_POS);
    lv_obj_align(decrease_temp_btn, NULL, LV_ALIGN_CENTER, DECREASE_TEMP_BTN_X_POS, -DECREASE_TEMP_BTN_Y_POS);
    lv_obj_align(increase_temp_btn, NULL, LV_ALIGN_CENTER, INCREASE_TEMP_BTN_X_POS, INCREASE_TEMP_BTN_Y_POS);
    lv_obj_align(lbl_setTmp, NULL, LV_ALIGN_CENTER, LBL_SET_TEMP_X_POS, LBL_SET_TEMP_Y_POS);

    lv_obj_set_size(power_ac_btn, AC_CONTROL_BNT_SIZE, AC_CONTROL_BNT_SIZE);
    lv_obj_set_size(decrease_temp_btn, AC_CONTROL_BNT_SIZE, AC_CONTROL_BNT_SIZE);
    lv_obj_set_size(increase_temp_btn, AC_CONTROL_BNT_SIZE, AC_CONTROL_BNT_SIZE);

    lv_obj_set_state(decrease_temp_btn, LV_STATE_DISABLED);
    lv_obj_set_state(increase_temp_btn, LV_STATE_DISABLED);

    lv_obj_add_style(lbl_power_ac, LV_LABEL_PART_MAIN, &ac_style);
    lv_obj_add_style(power_ac_btn, LV_LABEL_PART_MAIN, &ac_style);

    lv_obj_add_style(lbl_decrease_temp, LV_LABEL_PART_MAIN, &ac_style);
    lv_obj_add_style(decrease_temp_btn, LV_BTN_PART_MAIN, &ac_style);

    lv_obj_add_style(lbl_increase_temp, LV_LABEL_PART_MAIN, &ac_style);
    lv_obj_add_style(increase_temp_btn, LV_BTN_PART_MAIN, &ac_style);

    lv_obj_add_style(lbl_setTmp, LV_LABEL_PART_MAIN, &ac_style);
}

static void label_time_event(lv_obj_t *label, lv_event_t event)
{
    if (LV_EVENT_REFRESH == event)
    {
        strftime(strftime_buf, sizeof(strftime_buf), "%c",  get_tm());
        lv_label_set_text(lbl_time, strftime_buf);
    }
}

static void btnmatrix_control(lv_obj_t *btnmatrix, lv_event_t event)
{
    if (LV_EVENT_VALUE_CHANGED == event)
    {
        switch (lv_btnmatrix_get_active_btn(btnmatrix_lights))
        {
            case BTNMATRIX_EVENT_LIGHTS_ON:
                ESP_LOGI("", "Turn ON");
                send_data(0, "/lights/setting/");
                break;
            case BTNMATRIX_EVENT_LIGHTS_OFF:
                ESP_LOGI("", "Turn OFF");
                send_data(1, "/lights/setting/");
                break;
            case BTNMATRIX_EVENT_WORK_SCENE:
                ESP_LOGI("", "Work Scene");
                send_data(2, "/lights/setting/");
                break;
            case BTNMATRIX_EVENT_LUNCH_SCENE:
                ESP_LOGI("", "Lunch Scene");
                send_data(3, "/lights/setting/");
                break;
            case BTNMATRIX_EVENT_RELAX_SCENE:
                ESP_LOGI("", "Relax Scene");
                send_data(4, "/lights/setting/");
                break;
            case BTNMATRIX_EVENT_NIGHT_SCENE:
                ESP_LOGI("", "Night Scene");
                send_data(5, "/lights/setting/");
                break;
            default:
                break;
        }
    }
}

static void hide_ac_event(lv_obj_t *hide_ac, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {   
        lv_obj_set_hidden(win_ac, true);
        // lv_obj_set_parent(lbl_time, lv_scr_act());
    }
}

static void hide_light_event(lv_obj_t *hide_light, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {
        lv_obj_set_hidden(win_light, true);
    }
}

static void hide_settings_event(lv_obj_t *hide_settings, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {
        lv_obj_set_hidden(win_settings, true);
    }
}

static void btnmatrix_style(lv_style_t * style, lv_obj_t * btnmatrix)
{
    lv_style_init(style);

    lv_state_t LV_STATES[] = {
        LV_STATE_DEFAULT,
        LV_STATE_EDITED,
        LV_STATE_CHECKED,
        LV_STATE_FOCUSED,
        LV_STATE_HOVERED,
        LV_STATE_PRESSED,
        LV_STATE_DISABLED,
    };

    lv_btnmatrix_part_t BTNMATRIX_PARTS[] = {
        LV_BTNMATRIX_PART_BG,
        LV_BTNMATRIX_PART_BTN,
    };

    
    for (uint8_t i = 0; i < sizeof(LV_STATES) / sizeof(LV_STATES[0]); i++)
    {
        lv_style_set_bg_color(style, LV_STATES[i], LV_COLOR_BLACK);
        lv_style_set_border_color(style, LV_STATES[i], LV_COLOR_BLACK);
        lv_style_set_text_font(style, LV_STATES[i], &lv_font_montserrat_22);
    }

    for (uint8_t i = 0; i < sizeof(BTNMATRIX_PARTS) / sizeof(BTNMATRIX_PARTS[0]); i++)
    {
        lv_obj_add_style(btnmatrix, BTNMATRIX_PARTS[i], style);
    }
}

static void tv_time_channel_cb(lv_obj_t *btn, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {
        send_data(lv_dropdown_get_selected(dropdwn_tv_time), "/tv/time/");
        vTaskDelay(15);
        send_data(lv_dropdown_get_selected(dropdwn_tv_channel), "/tv/channel/");
    }
}

static void hide_tv_time_channel_cb(lv_obj_t *btn, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {
        lv_obj_set_hidden(win_tv_control, true);
    }
}

static void hide_water_heater_cb(lv_obj_t *btn, lv_event_t event)
{
    if (LV_EVENT_CLICKED == event)
    {
        lv_obj_set_hidden(win_heater_temp, true);
    }
}

static void kb_heater_temp_cb(lv_obj_t *kb, lv_event_t event)
{
	lv_keyboard_def_event_cb(kb_heater_temp, event);
	if (LV_EVENT_APPLY == event)
	{
		send_data(atoi(lv_textarea_get_text(ta_heater_temp)), "/heater/temp/");
	}
}

static void ta_heater_temp_cb(lv_obj_t *ta, lv_event_t event)
{
	if (LV_EVENT_CLICKED == event)
	{
		if (kb_heater_temp == NULL)
		{
			kb_heater_temp = lv_keyboard_create(win_heater_temp, NULL);

			lv_obj_set_style_local_bg_color(kb_heater_temp, LV_KEYBOARD_PART_BG, \
				LV_STATE_DEFAULT, LV_COLOR_BLACK);
			lv_obj_set_style_local_bg_color(kb_heater_temp, LV_KEYBOARD_PART_BTN, \
				LV_STATE_DEFAULT, LV_COLOR_BLACK);
			lv_keyboard_set_mode(kb_heater_temp, LV_KEYBOARD_MODE_NUM);
			lv_keyboard_set_textarea(kb_heater_temp, ta_heater_temp);
			lv_obj_set_event_cb(kb_heater_temp, kb_heater_temp_cb);
		}
	}
}

static void heater_temp()
{
	win_heater_temp = lv_win_create(lv_scr_act(), NULL);

	lv_obj_set_style_local_bg_color(win_heater_temp, LV_WIN_PART_HEADER, \
        LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_obj_set_style_local_bg_color(win_heater_temp, LV_WIN_PART_BG, \
        LV_STATE_DEFAULT, LV_COLOR_BLACK);
	
    
    lv_win_set_title(win_heater_temp, "Set water heater temperature");
    lv_obj_t *btn_tv_close = lv_win_add_btn_right(win_heater_temp, LV_SYMBOL_CLOSE);
    lv_obj_set_event_cb(btn_tv_close, hide_water_heater_cb);
    
    lv_win_set_scrollbar_mode(win_heater_temp, LV_SCROLLBAR_MODE_OFF);

	ta_heater_temp = lv_textarea_create(win_heater_temp, NULL);
	lv_obj_set_style_local_bg_color(ta_heater_temp, LV_TEXTAREA_PART_BG, \
		LV_STATE_DEFAULT, LV_COLOR_BLACK);
	lv_textarea_set_one_line(ta_heater_temp, true);
	lv_obj_set_event_cb(ta_heater_temp, ta_heater_temp_cb);

	
}

static void tv_control()
{
    win_tv_control = lv_win_create(lv_scr_act(), NULL);
    
    lv_obj_set_style_local_bg_color(win_tv_control, LV_WIN_PART_HEADER, \
        LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_bg_color(win_tv_control, LV_WIN_PART_BG, \
        LV_STATE_DEFAULT, LV_COLOR_BLACK);

    lv_win_set_title(win_tv_control, "Set TV channel");
    lv_obj_t *btn_tv_close = lv_win_add_btn_right(win_tv_control, LV_SYMBOL_CLOSE);
    lv_obj_set_event_cb(btn_tv_close, hide_tv_time_channel_cb);
    
    lv_win_set_scrollbar_mode(win_tv_control, LV_SCROLLBAR_MODE_OFF);

    // lv_obj_t *win_tv_content = lv_win_get_content(win_tv_control);

    lv_obj_t *lbl_tv_time = lv_label_create(win_tv_control, NULL);
    lv_label_set_text(lbl_tv_time, "Set time:");
    lv_obj_align(lbl_tv_time, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 40);

    lv_obj_t *lbl_tv_channel = lv_label_create(win_tv_control, NULL);
    lv_label_set_text(lbl_tv_channel, "Set channel:");
    lv_obj_align(lbl_tv_channel, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 80);

    dropdwn_tv_time = lv_dropdown_create(win_tv_control, NULL);
    lv_dropdown_set_options(dropdwn_tv_time, 
        "06:00\n"
        "06:15\n"
        "06:30\n"
        "06:45\n"
        "07:00\n"
        "07:15\n"
        "07:30\n");

    lv_dropdown_set_max_height(dropdwn_tv_time, 85);
    lv_obj_align(dropdwn_tv_time, NULL, LV_ALIGN_IN_TOP_LEFT, 150, 30);


    dropdwn_tv_channel = lv_dropdown_create(win_tv_control, NULL);
    lv_dropdown_set_options(dropdwn_tv_channel,
        "0. News\n"
        "1. Business\n"
        "2. Nature\n"
        "3. Movies\n"
        "4. Tv Shows\n"
        "5. Music\n"
        "6. Sport\n");

    lv_obj_align(dropdwn_tv_channel, NULL, LV_ALIGN_IN_TOP_LEFT, 150, 70);
    lv_dropdown_set_max_height(dropdwn_tv_channel, 85);

    btn_tv_time_channel = lv_btn_create(win_tv_control, NULL);
    lv_obj_t *lbl_time_channel = lv_label_create(btn_tv_time_channel, NULL);
    lv_label_set_text_static(lbl_time_channel, "Set channel");

    lv_obj_align(btn_tv_time_channel, NULL, LV_ALIGN_IN_TOP_RIGHT, -30, 40);
    lv_obj_set_event_cb(btn_tv_time_channel, tv_time_channel_cb);

}

static void lv_tick_hook(void *pvData) {
    lv_tick_inc(LV_TICK_PERIOD);
}

void gui(void *pvParameter)
{
    (void) pvParameter;
    SemaphoreHandle_t xGuiSemaphore = xSemaphoreCreateMutex();
    xLblSemaphore = xSemaphoreCreateMutex();

    lv_init();
    lvgl_driver_init();

    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);

    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);

    static lv_disp_buf_t disp_buffer;
    uint32_t size_in_pix = DISP_BUF_SIZE;

    lv_disp_buf_init(&disp_buffer, buf1, buf2, size_in_pix);

    lv_disp_drv_t disp_driver;
    lv_disp_drv_init(&disp_driver);
    disp_driver.buffer = &disp_buffer;
    disp_driver.flush_cb = disp_driver_flush;
    lv_disp_flush_ready(&disp_driver);

    lv_disp_drv_register(&disp_driver);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read;
    lv_indev_drv_register(&indev_drv);
    
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_hook,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD));
    
    gui_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(GUI_TASK_DELAY_MS));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }
    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

void gui_task(const uint32_t usStackDepth,void * const pvParameters, \
    UBaseType_t uxPriority, TaskHandle_t * const pvCreatedTask, const BaseType_t xCoreID)
{
    xTaskCreatePinnedToCore(gui, "gui", usStackDepth, pvParameters, \
            uxPriority, pvCreatedTask, xCoreID);
}