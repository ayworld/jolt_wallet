#include "jolt_gui/jolt_gui.h"
#include "hal/storage/storage.h"

static lv_obj_t *scr = NULL;
static lv_obj_t *sw_en = NULL;

static void create_list() {
    jolt_gui_scr_menu_add(scr, NULL, "test", NULL);
}

static void destroy_list() {
     jolt_gui_scr_menu_remove(scr, 1, 0);
}

static lv_res_t sw_en_cb(lv_obj_t *btn) {
    uint8_t state;
    state = lv_sw_toggle_anim(sw_en);
    storage_set_u8(state, "user", "bluetooth_en");

    if( state ) {
        create_list();
    }
    else {
        destroy_list();
    }
    return LV_RES_OK;
}

lv_res_t menu_bluetooth_create(lv_obj_t *btn) {
    uint8_t bluetooth_en;
    storage_get_u8(&bluetooth_en, "user", "bluetooth_en", 0 );

    scr = jolt_gui_scr_menu_create(gettext(JOLT_TEXT_SETTINGS));
    lv_obj_t *btn_en = jolt_gui_scr_menu_add(scr, NULL, gettext(JOLT_TEXT_BLUETOOTH_ENABLE), sw_en_cb);
    sw_en = jolt_gui_scr_menu_add_sw( btn_en );

    if( bluetooth_en ) {
        lv_sw_on( sw_en );
        create_list();
    }
    else{
        lv_sw_off( sw_en );
    }

    return LV_RES_OK;
}
