#include "stdio.h"
#include "jolt_gui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct jolt_group_t {
    lv_group_t *main; // Parent group for user input
    lv_group_t *back; // Group used to handle back button
    lv_group_t *enter;
} jolt_group_t;

jolt_group_t group;

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

lv_res_t jolt_gui_scr_del() {
    lv_res_t res = LV_RES_OK;
    JOLT_GUI_CTX{
        lv_obj_t *scrn = BREAK_IF_NULL(lv_group_get_focused(group.main));
        lv_obj_t *parent = scrn;
        lv_obj_t *tmp = scrn;
        while( (tmp = lv_obj_get_parent(tmp)) ) {
            if( tmp != lv_scr_act() ) {
                parent = tmp;
            }
        }
        jolt_gui_obj_del(parent);
        res = LV_RES_INV;
    }
    return res;
}

void jolt_gui_obj_id_set( lv_obj_t *obj, jolt_gui_obj_id_t id) {
    lv_obj_get_user_data( obj )->id = id;
    lv_obj_get_user_data( obj )->is_scr = 0;
} 

jolt_gui_obj_id_t jolt_gui_obj_id_get( const lv_obj_t *obj ) {
    if( lv_obj_get_user_data((lv_obj_t *)obj)->is_scr ){
        return JOLT_GUI_OBJ_ID_INVALID;
    }
    return lv_obj_get_user_data((lv_obj_t *)obj)->id;
}

void jolt_gui_scr_id_set( lv_obj_t *obj, jolt_gui_scr_id_t id) {
    lv_obj_get_user_data( obj )->id = id;
    lv_obj_get_user_data( obj )->is_scr = 1;
}

jolt_gui_scr_id_t jolt_gui_scr_id_get( const lv_obj_t *obj ) {
    if( !lv_obj_get_user_data((lv_obj_t *)obj)->is_scr ){
        return JOLT_GUI_SCR_ID_INVALID;
    }
    return lv_obj_get_user_data((lv_obj_t *)obj)->id;
}

/**************************************
 * STANDARD SCREEN CREATION FUNCTIONS *
 **************************************/

/* Creates a dummy invisible object to anchor lvgl objects on the screen */
lv_obj_t *jolt_gui_obj_parent_create() {
    lv_obj_t *parent = NULL;
    JOLT_GUI_CTX{
        parent = BREAK_IF_NULL(lv_obj_create(lv_scr_act(), NULL));
        lv_obj_set_size(parent, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_pos(parent,0,0);
        lv_obj_set_style(parent, &lv_style_transp);
    }
    return parent;
}

/* Creates the statusbar title label for a screen. Returns the 
 * label object. */
lv_obj_t *jolt_gui_obj_title_create(lv_obj_t *parent, const char *title) {
    if( NULL == parent ) {
        return NULL;
    }

    lv_obj_t *label = NULL;
    JOLT_GUI_CTX{
        lv_obj_t *statusbar_label = statusbar_get_label();
        /* Create a non-transparent background to block out old titles */
        lv_obj_t *title_cont = BREAK_IF_NULL(lv_cont_create(parent, NULL));
        jolt_gui_obj_id_set(title_cont, JOLT_GUI_OBJ_ID_CONT_TITLE);
        lv_obj_align(title_cont, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
        lv_obj_set_size( title_cont,
                lv_obj_get_x(statusbar_label) - 1, // todo: use style padding
                CONFIG_JOLT_GUI_STATUSBAR_H-1);

        label = BREAK_IF_NULL(lv_label_create(title_cont, NULL));
        jolt_gui_obj_id_set(label, JOLT_GUI_OBJ_ID_LABEL_0);
        lv_style_t *label_style = lv_obj_get_style(label);
        lv_label_set_long_mode(label, LV_LABEL_LONG_ROLL);
        lv_label_set_body_draw(label, false); // dont draw background
        lv_obj_align(label, NULL, LV_ALIGN_IN_LEFT_MID, 0, 0);
        if( NULL == title ){
            lv_label_set_text(label, "");
        }
        else{
            lv_label_set_text(label, title);
        }
        lv_obj_set_size(label, lv_obj_get_width(title_cont), label_style->text.font->h_px);
    }
    return label;
}

/* Creates the body container */ 
lv_obj_t *jolt_gui_obj_cont_body_create( lv_obj_t *parent ) {
    if( NULL == parent ) {
        return NULL;
    }

    lv_obj_t *cont = NULL;
    JOLT_GUI_CTX{
        cont = BREAK_IF_NULL(lv_cont_create(parent, NULL));
        jolt_gui_obj_id_set(cont, JOLT_GUI_OBJ_ID_CONT_BODY);
        lv_obj_set_size(cont, LV_HOR_RES, 
                LV_VER_RES - CONFIG_JOLT_GUI_STATUSBAR_H);
        lv_obj_align(cont, NULL, LV_ALIGN_IN_TOP_LEFT,
                0, CONFIG_JOLT_GUI_STATUSBAR_H);
    }
    return cont;
}

void jolt_gui_obj_del(lv_obj_t *obj){
    if( NULL == obj ) {
        return;
    }

    JOLT_GUI_CTX{
        /* Some objects may require special actions before deleting */
        jolt_gui_scr_id_t id = jolt_gui_scr_id_get( obj );
        switch( id ) {
            case JOLT_GUI_SCR_ID_LOADINGBAR:
                /* Check and Delete the auto updater task */
                jolt_gui_scr_loadingbar_autoupdate_del( obj );
                break;
            default:
                break;
        }
        lv_obj_del(obj);
    }
}

/***************
 * Group Stuff *
 ***************/
void jolt_gui_group_create() {
    /* Create Groups for user input */
    bool success = false;
    JOLT_GUI_CTX{
        group.main = BREAK_IF_NULL(lv_group_create());
        lv_group_set_refocus_policy(group.main, LV_GROUP_REFOCUS_POLICY_PREV);
        group.back = BREAK_IF_NULL(lv_group_create());
        lv_group_set_refocus_policy(group.back, LV_GROUP_REFOCUS_POLICY_PREV);
        group.enter = BREAK_IF_NULL(lv_group_create());
        lv_group_set_refocus_policy(group.enter, LV_GROUP_REFOCUS_POLICY_PREV);
        success = true;
    }
    if( !success ){
        esp_restart();
    }
}

void jolt_gui_group_add( lv_obj_t *obj ){
    JOLT_GUI_CTX{
        lv_group_add_obj(group.main, obj);
        lv_group_focus_obj( obj );
    }
}

lv_group_t *jolt_gui_group_main_get() {
    return group.main;
}

lv_group_t *jolt_gui_group_back_get() {
    return group.back;
}

lv_group_t *jolt_gui_group_enter_get() {
    return group.enter;
}


/**********
 * Action *
 **********/

/* todo; change the lv_action_t return type to void */
static void event_action_wrapper_cb( lv_obj_t *obj, lv_event_t event) {
    ESP_LOGD(TAG, "Event %d", event);
    switch(event){
        case LV_EVENT_PRESSED:
            break;
        case LV_EVENT_SHORT_CLICKED:
            lv_obj_get_user_data(obj)->cb.short_clicked( obj );
            break;
        default:
            break;
    }
}

lv_obj_t *_jolt_gui_scr_set_action(lv_obj_t *parent, lv_action_t cb, lv_group_t *g) {
    lv_obj_t *btn = NULL;
    JOLT_GUI_CTX{
        /* Remove any children buttons already in group g */
        lv_obj_t *child = NULL;
        lv_obj_type_t obj_type;
        /* todo; change this find logic to our macro */
        while( NULL != (child = lv_obj_get_child(parent, child)) ) {
            lv_obj_get_type(child, &obj_type);
            if( 0==strcmp("lv_btn", obj_type.type[0]) && g==lv_obj_get_group(child) ) {
                lv_obj_del(child);
                child = NULL;
            }
        }

        btn = BREAK_IF_NULL(lv_btn_create(parent, NULL));
        lv_obj_set_event_cb(btn, event_action_wrapper_cb);
        lv_obj_get_user_data(parent)->cb.short_clicked = cb;
        lv_obj_set_size(btn, 0, 0);
        lv_group_remove_obj(btn);
        lv_group_add_obj(g, btn);
        lv_group_focus_obj(btn);

        if( g == group.enter ) {
            jolt_gui_obj_id_set(btn, JOLT_GUI_OBJ_ID_ENTER);
        }
        else if ( g == group.back) {
            jolt_gui_obj_id_set(btn, JOLT_GUI_OBJ_ID_BACK);
        }
    }
    return btn;
}

lv_obj_t *jolt_gui_scr_set_back_action(lv_obj_t *parent, lv_action_t cb) {
    lv_obj_t *btn = NULL;
    JOLT_GUI_CTX{
        jolt_gui_scr_id_t type = jolt_gui_scr_id_get( parent );
        switch( type ) {
            /* Some screens need to wrap callbacks. Wrapping them here 
             * enables a unified interface for seting back actions. */
            case JOLT_GUI_SCR_ID_INVALID:{
                ESP_LOGE(TAG, "Not a screen");
                abort();
                break;
            }
            case JOLT_GUI_SCR_ID_DIGIT_ENTRY:{
                jolt_gui_scr_digit_entry_set_back_action(parent, cb);
                break;
            }
            default:{
                /* Usually, just create a button in the back group as a child of the screen */
                btn = _jolt_gui_scr_set_action(parent, cb, group.back);
                break;
            }
        }
    }
    return btn;
}

lv_obj_t *jolt_gui_scr_set_enter_action(lv_obj_t *parent, lv_action_t cb) {
    lv_obj_t *btn = NULL;
    JOLT_GUI_CTX{
        jolt_gui_scr_id_t type = jolt_gui_scr_id_get( parent );
        switch( type ) {
            /* Some screens need to wrap callbacks. Wrapping them here 
             * enables a unified interface for seting back actions. */
            case JOLT_GUI_SCR_ID_INVALID:{
                ESP_LOGE(TAG, "Not a screen");
                abort();
                break;
            }
            case JOLT_GUI_SCR_ID_DIGIT_ENTRY:{
                jolt_gui_scr_digit_entry_set_enter_action(parent, cb);
                break;
            }
            default:{
                /* Usually, just create a button in the back group as a child of the screen */
                btn = _jolt_gui_scr_set_action(parent, cb, group.enter);
                break;
            }
        }
    }
    return btn;
}

void jolt_gui_scr_set_back_param(lv_obj_t *parent, void *param) {
    lv_obj_t *btn = NULL;
    JOLT_GUI_CTX{
        btn = JOLT_GUI_FIND_AND_CHECK(parent, JOLT_GUI_OBJ_ID_BACK);
        lv_obj_get_user_data( btn )->param = param;
    }
}

void jolt_gui_scr_set_enter_param(lv_obj_t *parent, void *param) {
    lv_obj_t *btn = NULL;
    JOLT_GUI_CTX{
        btn = JOLT_GUI_FIND_AND_CHECK(parent, JOLT_GUI_OBJ_ID_ENTER);
        lv_obj_get_user_data( btn )->param = param;
    }
}

lv_res_t jolt_gui_send_enter_main(lv_obj_t *dummy) {
    return lv_group_send_data(group.main, LV_GROUP_KEY_ENTER);
}

lv_res_t jolt_gui_send_left_main(lv_obj_t *dummy) {
    return lv_group_send_data(group.main, LV_GROUP_KEY_LEFT);
}

lv_res_t jolt_gui_send_enter_back(lv_obj_t *dummy) {
    return lv_group_send_data(group.back, LV_GROUP_KEY_ENTER);

}
lv_res_t jolt_gui_send_enter_enter(lv_obj_t *dummy) {
    return lv_group_send_data(group.enter, LV_GROUP_KEY_ENTER);
}

/********
 * MISC *
 ********/
static SemaphoreHandle_t jolt_gui_mutex = NULL;
void jolt_gui_sem_take() {
    if( NULL == jolt_gui_mutex ){
        /* Create the jolt_gui_mutex; avoids need to explicitly initialize */
        jolt_gui_mutex = xSemaphoreCreateRecursiveMutex();
        if( NULL == jolt_gui_mutex ){
            esp_restart();
        }
    }
    xSemaphoreTakeRecursive( jolt_gui_mutex, portMAX_DELAY );
}

void jolt_gui_sem_give() {
    xSemaphoreGiveRecursive( jolt_gui_mutex );
}

/* Finds the first child object with the object identifier.
 * Returns NULL if child not found. */
lv_obj_t *jolt_gui_find(lv_obj_t *parent, jolt_gui_obj_id_t id) {
    lv_obj_t *child = NULL;
    JOLT_GUI_CTX{
        ESP_LOGD(TAG, "Searchng the %d children of %p", lv_obj_count_children(parent), parent);
        while( NULL != (child = lv_obj_get_child(parent, child)) ) {
            jolt_gui_obj_id_t child_id;
            child_id = jolt_gui_obj_id_get( child );
            ESP_LOGD(TAG, "Child: %p %s", child, jolt_gui_obj_id_str(child_id));
            if( child_id == id) {
                break;
            }
        }
    }
    return child;
}

/* Convert the enumerated value to a constant string */
const char *jolt_gui_obj_id_str(jolt_gui_obj_id_t val) {
    const char *names[] = { FOREACH_JOLT_GUI_OBJ_ID(GENERATE_STRING) };
    if( val >= JOLT_GUI_OBJ_ID_MAX ) {
        return "<unknown>";
    }
    return names[val];
}

/* Convert the enumerated value to a constant string */
const char *jolt_gui_scr_id_str(jolt_gui_scr_id_t val) {
    const char *names[] = { FOREACH_JOLT_GUI_SCR_ID(GENERATE_STRING) };
    if( val >= JOLT_GUI_SCR_ID_MAX ) {
        return "<unknown>";
    }
    return names[val];
}

