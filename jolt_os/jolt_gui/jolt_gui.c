#include "jolt_gui.h"
#include "stdio.h"
#include "jolt_gui_entry.h"
#include "jolt_gui_symbols.h"
#include "jolt_gui_statusbar.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/
#if PC_SIMULATOR
    #define MSG(...) printf(__VA_ARGS__)
#elif ESP_PLATFORM
    #include "esp_log.h"
    static const char TAG[] = "jolt_gui";
    #define MSG(...) ESP_LOGI(TAG, __VA_ARGS__)
#else
    #define MSG(...) printf(__VA_ARGS__)
#endif

/*********************
 * Screen Management *
 *********************/

lv_action_t jolt_gui_scr_del() {
    lv_obj_t *scrn = lv_group_get_focused(jolt_gui_store.group.main);
    if( NULL == scrn ) {
        MSG("Nothing in focus\n");
        return 0;
    }
    if( scrn == jolt_gui_store.main_menu ) {
        MSG("Can't exit main menu\n");
        return 0;
    }

    // Disable any focus callback
    lv_group_set_focus_cb(jolt_gui_store.group.main, NULL);
    lv_obj_t *parent = scrn;
    lv_obj_t *tmp = scrn;
    while( (tmp = lv_obj_get_parent(tmp)) ) {
        if( tmp != lv_scr_act() ) {
            parent = tmp;
        }
        if( tmp == jolt_gui_store.main_menu ) {
            // todo: refresh main menu
            // don't delete the main menu
            return 0;
        }
    }
    void *free_ptr = lv_obj_get_free_ptr( parent );
    if( NULL != free_ptr ){
        free(free_ptr);
    }
    lv_obj_del(parent);
    return LV_RES_INV;
}

/**************************************
 * STANDARD SCREEN CREATION FUNCTIONS *
 **************************************/

/* Creates a dummy invisible object to anchor lvgl objects on the screen */
lv_obj_t *jolt_gui_parent_create() {
    lv_obj_t *parent = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(parent, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(parent,0,0);
    lv_obj_set_style(parent, &lv_style_transp);
    return parent;
}

/* Creates the statusbar title label for a screen. Returns the 
 * label object. */
lv_obj_t *jolt_gui_obj_title_create(lv_obj_t *parent, const char *title) {
    lv_obj_t *label = NULL;

    if( NULL == parent ) {
        parent = lv_scr_act();
    }

    /* Create a non-transparent background to block out old titles */
    lv_obj_t *title_cont = lv_cont_create(parent, NULL);
    JOLT_GUI_OBJ_CHECK(title_cont);
    lv_obj_set_free_num(title_cont, JOLT_GUI_OBJ_ID_CONT_TITLE);
    lv_obj_align(title_cont, NULL, LV_ALIGN_IN_TOP_LEFT,
            2, 0);
    lv_obj_set_size(title_cont,
            CONFIG_JOLT_GUI_TITLE_W, CONFIG_JOLT_GUI_STATUSBAR_H-1);

    label = lv_label_create(title_cont, NULL);
    JOLT_GUI_OBJ_CHECK(label);
    lv_obj_set_free_num(label, JOLT_GUI_OBJ_ID_LABEL_0);
    lv_style_t *label_style = lv_obj_get_style(label);
    lv_label_set_long_mode(label, LV_LABEL_LONG_ROLL);
    lv_label_set_body_draw(label, false); // dont draw background

    /* The Y-offset may need to be adjusted to the specific font */
    lv_obj_align(label, title_cont, LV_ALIGN_IN_LEFT_MID, 0, 0);
    if( NULL == title ){
        lv_label_set_text(label, "");
    }
    else{
        lv_label_set_text(label, title);
    }
    lv_obj_set_size(label, CONFIG_JOLT_GUI_TITLE_W, label_style->text.font->h_px);

exit:
    return label;
}

/* Creates the body container */ 
lv_obj_t *jolt_gui_obj_cont_body_create( lv_obj_t *parent ) {
    lv_obj_t *cont = lv_cont_create(parent, NULL);
    if( NULL == cont ) {
        goto exit;
    }
    lv_obj_set_free_num(cont, JOLT_GUI_OBJ_ID_CONT_BODY);
    lv_obj_set_size(cont, LV_HOR_RES, 
            LV_VER_RES - CONFIG_JOLT_GUI_STATUSBAR_H);
	lv_obj_align(cont, NULL, LV_ALIGN_IN_TOP_LEFT,
            0, CONFIG_JOLT_GUI_STATUSBAR_H);
exit:
    return cont;
}

/***************
 * Group Stuff *
 ***************/
static void group_mod_cb(lv_style_t *style) {
    style->body.border.part = 0;
}

void jolt_gui_group_create() {
    /* Create Groups for user input */
    jolt_gui_store.group.main = lv_group_create();
    lv_group_set_refocus_policy(jolt_gui_store.group.main, LV_GROUP_REFOCUS_POLICY_PREV);
    jolt_gui_store.group.back = lv_group_create();
    lv_group_set_refocus_policy(jolt_gui_store.group.back, LV_GROUP_REFOCUS_POLICY_PREV);
    jolt_gui_store.group.enter = lv_group_create();
    lv_group_set_refocus_policy(jolt_gui_store.group.enter, LV_GROUP_REFOCUS_POLICY_PREV);
    lv_group_set_style_mod_cb(jolt_gui_store.group.main, group_mod_cb);
}

/* Adds object to main group */
void jolt_gui_group_add( lv_obj_t *obj ){
    lv_group_add_obj(jolt_gui_store.group.main, obj);
    lv_group_focus_obj( obj );
}

/**********
 * Action *
 **********/

static lv_obj_t *jolt_gui_scr_set_action(lv_obj_t *parent, lv_action_t cb, 
        lv_group_t *g) {
    if( NULL == parent ) {
        parent = lv_scr_act();
    }
    else {
        // Remove any children already in group g
        lv_obj_t *child = NULL;
        lv_obj_type_t obj_type;
        while( NULL != (child = lv_obj_get_child(parent, child)) ) {
            lv_obj_get_type(child, &obj_type);
            if(strcmp("lv_btn", obj_type.type[0]) && g==lv_obj_get_group(child) ) {
                lv_obj_del(child);
                child = NULL;
            }
        }
    }
    lv_obj_t *btn = lv_btn_create(parent, NULL);
    lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, cb);
    lv_obj_set_size(btn, 0, 0);
    lv_group_remove_obj(btn);

    lv_group_add_obj(g, btn);
    lv_group_focus_obj(btn);

    return parent;
}

lv_obj_t *jolt_gui_scr_set_back_action(lv_obj_t *parent, lv_action_t cb) {
    return jolt_gui_scr_set_action(parent, cb, jolt_gui_store.group.back);
}

lv_obj_t *jolt_gui_scr_set_enter_action(lv_obj_t *parent, lv_action_t cb) {
    return jolt_gui_scr_set_action(parent, cb, jolt_gui_store.group.enter);
}

lv_action_t jolt_gui_send_enter_main(lv_obj_t *btn) {
    lv_group_send_data(jolt_gui_store.group.main, LV_GROUP_KEY_ENTER);
    return 0;
}

lv_action_t jolt_gui_send_left_main(lv_obj_t *btn) {
    lv_group_send_data(jolt_gui_store.group.main, LV_GROUP_KEY_LEFT);
    return 0;
}

/********
 * MISC *
 ********/
void jolt_gui_sem_take() {
    xSemaphoreTake( jolt_gui_store.mutex, portMAX_DELAY );
}
void jolt_gui_sem_give() {
    xSemaphoreGive( jolt_gui_store.mutex );
}

/* Finds the first child object with the free_num identifier.
 * Returns NULL if child not found. */
lv_obj_t *jolt_gui_find(lv_obj_t *parent, LV_OBJ_FREE_NUM_TYPE id) {
    lv_obj_t *child = NULL;
    ESP_LOGD(TAG, "Search Parent %p has %d children", parent, lv_obj_count_children(parent));
	while( NULL != (child = lv_obj_get_child(parent, child)) ) {
        LV_OBJ_FREE_NUM_TYPE child_id;
        child_id = lv_obj_get_free_num(child);
        ESP_LOGD(TAG, "Child: %p %s", child, jolt_gui_obj_id_str(child_id));
        if( child_id == id) {
            break;
        }
    }
    return child;
}

/* Convert the enumerated value to a constant string */
const char *jolt_gui_obj_id_str(jolt_gui_obj_id_t val) {
#define STRCASE(name) case name: return #name;
    switch(val){
        STRCASE(JOLT_GUI_OBJ_ID_UNINITIALIZED)
        STRCASE(JOLT_GUI_OBJ_ID_CONT_TITLE)   
        STRCASE(JOLT_GUI_OBJ_ID_CONT_BODY)    
        STRCASE(JOLT_GUI_OBJ_ID_PAGE)         
        STRCASE(JOLT_GUI_OBJ_ID_LABEL_0)      
        STRCASE(JOLT_GUI_OBJ_ID_LABEL_1)      
        STRCASE(JOLT_GUI_OBJ_ID_LABEL_2)      
        STRCASE(JOLT_GUI_OBJ_ID_LABEL_3)      
        STRCASE(JOLT_GUI_OBJ_ID_LABEL_4)      
        STRCASE(JOLT_GUI_OBJ_ID_BAR_LOADING)  
        STRCASE(JOLT_GUI_OBJ_ID_PRELOADING)   
        STRCASE(JOLT_GUI_OBJ_ID_IMG_QR)       
        STRCASE(JOLT_GUI_OBJ_ID_SLIDER)
        STRCASE(JOLT_GUI_OBJ_ID_LIST)
        default:
            return "<unknown>";
    }
#undef STRCASE
}

