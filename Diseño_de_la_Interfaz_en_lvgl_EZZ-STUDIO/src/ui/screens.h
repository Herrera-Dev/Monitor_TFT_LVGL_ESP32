#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *monitor;
    lv_obj_t *grafico;
    lv_obj_t *perfiles;
    lv_obj_t *config;
    lv_obj_t *obj0;
    lv_obj_t *icon_location_11p;
    lv_obj_t *label_city;
    lv_obj_t *label_weather_main;
    lv_obj_t *label_time;
    lv_obj_t *image_icon_weather;
    lv_obj_t *label_date;
    lv_obj_t *label_weather_description;
    lv_obj_t *obj1;
    lv_obj_t *label_temperature;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *label_feels_like;
    lv_obj_t *label_temp_max;
    lv_obj_t *label_temp_min;
    lv_obj_t *obj5;
    lv_obj_t *label_humidity;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *label_wind;
    lv_obj_t *label_pressure;
    lv_obj_t *label_visibility;
    lv_obj_t *obj9;
    lv_obj_t *btn_view_config;
    lv_obj_t *obj10;
    lv_obj_t *btn_view_config_per;
    lv_obj_t *obj11;
    lv_obj_t *label_info;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *btn_00;
    lv_obj_t *btn_01;
    lv_obj_t *btn_02;
    lv_obj_t *btn_03;
    lv_obj_t *btn_04;
    lv_obj_t *btn_05;
    lv_obj_t *label_00;
    lv_obj_t *label_01;
    lv_obj_t *label_02;
    lv_obj_t *label_03;
    lv_obj_t *label_04;
    lv_obj_t *label_05;
    lv_obj_t *grap_texto;
    lv_obj_t *label_y;
    lv_obj_t *grap_cont;
    lv_obj_t *grap_line;
    lv_obj_t *grap_alert;
    lv_obj_t *grap_scale_x;
    lv_obj_t *grap_scale_y;
    lv_obj_t *label_x;
    lv_obj_t *label_texto;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *menu_perfiles;
    lv_obj_t *menu_opciones;
    lv_obj_t *label_estado;
    lv_obj_t *text_view;
    lv_obj_t *btn_volver_conf;
    lv_obj_t *obj23;
    lv_obj_t *txt_ymin;
    lv_obj_t *obj24;
    lv_obj_t *txt_ymax;
    lv_obj_t *obj25;
    lv_obj_t *txt_warn;
    lv_obj_t *teclado2;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
    lv_obj_t *obj28;
    lv_obj_t *obj29;
    lv_obj_t *obj30;
    lv_obj_t *txt_ssid;
    lv_obj_t *txt_pass;
    lv_obj_t *obj31;
    lv_obj_t *obj32;
    lv_obj_t *txt_api_coin;
    lv_obj_t *txt_crypto1;
    lv_obj_t *txt_crypto2;
    lv_obj_t *txt_divisa;
    lv_obj_t *obj33;
    lv_obj_t *obj34;
    lv_obj_t *txt_api_weather;
    lv_obj_t *txt_ciudad;
    lv_obj_t *txt_pais;
    lv_obj_t *txt_hora;
    lv_obj_t *label_est_conf;
    lv_obj_t *obj35;
    lv_obj_t *obj36;
    lv_obj_t *my_label_slider;
    lv_obj_t *brillo;
    lv_obj_t *btn_gurdar_cren;
    lv_obj_t *btn_salir_cren;
    lv_obj_t *teclado;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_MONITOR = 2,
    SCREEN_ID_GRAFICO = 3,
    SCREEN_ID_PERFILES = 4,
    SCREEN_ID_CONFIG = 5,
};

void create_screen_main();
void tick_screen_main();

void create_screen_monitor();
void tick_screen_monitor();

void create_screen_grafico();
void tick_screen_grafico();

void create_screen_perfiles();
void tick_screen_perfiles();

void create_screen_config();
void tick_screen_config();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/