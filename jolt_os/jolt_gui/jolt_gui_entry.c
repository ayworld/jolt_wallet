#include "jolt_gui/jolt_gui.h"
#include "jolt_gui_entry.h"
#include <stdio.h>
#include "sodium.h"

/* Entry Screen Structures:
 * * SCREEN
 *   +--CONT_TITLE
 *   |   +--LABEL_0 (title)
 *   +--CONT_BODY (extended with metadata)
 *       +--ROLLER[0 ... n]
 *       +--LABEL_0 (decimal point)
 *
 *
 * Typical workflow:
 * lv_obj_t *scr = jolt_gui_scr_digit_entry_create("Title", 10, -1, cb);
 */

typedef struct {
    lv_cont_ext_t cont;       /*The ancestor container structure*/
    uint8_t sel;
    uint8_t num_rollers;
    lv_obj_t *rollers[CONFIG_JOLT_GUI_DIGIT_ENTRY_MAX_LEN];
    lv_obj_t *decimal_point;
    int8_t decimal_point_pos; // Position of decimal point from right. -1 for no decimal
} digit_entry_cont_ext_t;

static const char TAG[] = "digit_entry";
static lv_signal_func_t old_roller_signal = NULL;     /*Store the old signal function*/

static lv_obj_t *digit_create(lv_obj_t *parent);

static void loop_roller( lv_obj_t *rol) {
    /* Loops roller position */
    lv_roller_ext_t *rol_ext = lv_obj_get_ext_attr(rol);
    uint8_t id = rol_ext->ddlist.sel_opt_id;
    if( id < 10 ) { // todo refine loop point
        lv_roller_set_selected(rol, id + 10, false);
    }
    if( id > 25 ) { // todo refine loop point
        lv_roller_set_selected(rol, id - 10, false);
    }
}
#if 0
#endif

static lv_res_t new_roller_signal(lv_obj_t *roller, lv_signal_t sign, void * param) {
    lv_res_t res;
    lv_obj_t *cont_body = lv_obj_get_parent(roller);
    digit_entry_cont_ext_t *ext = lv_obj_get_ext_attr(cont_body);

    res = old_roller_signal(roller, sign, param);
    if(res != LV_RES_OK) return res;

    if(sign == LV_SIGNAL_FOCUS) {
        ESP_LOGE(TAG, "FOCUS");
        lv_roller_set_visible_row_count(roller, 3);
    }
    else if(sign == LV_SIGNAL_DEFOCUS) {
        lv_roller_set_visible_row_count(roller, 1);
    }
    else if(sign == LV_SIGNAL_CONTROLL){
        char c = *((char *)param);
        
        loop_roller(ext->rollers[ext->sel]);
        lv_roller_ext_t *rol_ext = lv_obj_get_ext_attr(ext->rollers[ext->sel]);

        switch(c){
            case LV_GROUP_KEY_LEFT: {
                /* Preserve the currently selected roller value */
                rol_ext->ddlist.sel_opt_id_ori = rol_ext->ddlist.sel_opt_id;

                if( ext->sel > 0 ) {
                    (ext->sel)--;
                    lv_group_focus_obj(ext->rollers[ext->sel]);
                }
                else {
                    // Call back callback
                    lv_group_focus_obj(cont_body);
                    res = lv_group_send_data(jolt_gui_store.group.back, (uint32_t)param);
                }
                break;
            }
            case LV_GROUP_KEY_ENTER: {
                //printf("Pressed enter\n");
                /* Preserve the currently selected roller value */
                rol_ext->ddlist.sel_opt_id_ori = rol_ext->ddlist.sel_opt_id;
                if(ext->sel < ext->num_rollers-1) {
                    (ext->sel)++;
                    lv_group_focus_obj(ext->rollers[ext->sel]);
                }
                else {
                    //perform enter action callback
                    lv_group_focus_obj(cont_body);
                    res = lv_group_send_data(jolt_gui_store.group.enter, (uint32_t)param);
                }
                break;
            }
            default:
                break;
        }
    }

    return res;
}

/* Create and return a lv_roller */
static lv_obj_t *digit_create(lv_obj_t *parent) {
    lv_obj_t *roller;
    roller = lv_roller_create(parent, NULL);
    lv_roller_set_visible_row_count(roller, 1);
    lv_roller_set_anim_time(roller, CONFIG_JOLT_GUI_ANIM_DIGIT_MS);
    lv_roller_set_options(roller, 
            "9\n8\n7\n6\n5\n4\n3\n2\n1\n0"
            "\n9\n8\n7\n6\n5\n4\n3\n2\n1\n0"
            "\n9\n8\n7\n6\n5\n4\n3\n2\n1\n0"
            "\n9\n8\n7\n6\n5\n4\n3\n2\n1\n0"
            );
    lv_roller_set_selected(roller, 19, false); // Set it to the middle 0 entry
    lv_roller_set_align(roller, LV_LABEL_ALIGN_CENTER);
    return roller;
}
static lv_obj_t *create_dp(lv_obj_t *parent){
    lv_obj_t *label = lv_label_create(parent, NULL);
    lv_label_set_text(label, ".");
    lv_obj_set_free_num(label, JOLT_GUI_OBJ_ID_DECIMAL_POINT);
    return label;
}

/* Creates a screen with title, n rollers, decimal place pos from the 
 * right (-1 to disable), and enter_cb. */
lv_obj_t *jolt_gui_scr_digit_entry_create(const char *title,
        int8_t n, int8_t pos){
    JOLT_GUI_SCR_CTX(title){
        lv_obj_allocate_ext_attr(cont_body, sizeof(digit_entry_cont_ext_t));
        digit_entry_cont_ext_t *ext = lv_obj_get_ext_attr(cont_body);
        lv_cont_set_layout(cont_body, LV_LAYOUT_ROW_M);

        ext->sel = 0;

        if( n > sizeof(ext->rollers) || n < 0 ) break;

        ext->decimal_point = NULL;
        if( pos >= 0 && pos <= n){
            /* Decimal will be visible */
            ext->decimal_point_pos = pos;
        }
        /* Create left to right */
        for(int8_t i=0; i < n; i++){
            if(pos + i == n ){
                /* Create decimal place */
                ext->decimal_point = create_dp(cont_body);
            }
            ext->rollers[i] = digit_create(cont_body);
            lv_obj_set_free_num(ext->rollers[i], JOLT_GUI_OBJ_ID_ROLLER);
            jolt_gui_group_add( ext->rollers[i] );
            if( NULL == old_roller_signal) {
                old_roller_signal = lv_obj_get_signal_func(ext->rollers[i]);
            }
            lv_obj_set_signal_func(ext->rollers[i], new_roller_signal);
            //jolt_gui_scr_set_back_action(ext->rollers[i], &jolt_gui_send_left_main);
            //jolt_gui_scr_set_enter_action(ext->rollers[i], &jolt_gui_send_enter_main);
        }
        ext->num_rollers = n;
        if( 0 == pos ) {
            ext->decimal_point = create_dp(cont_body);
        }
        lv_group_focus_obj(ext->rollers[0]);
    }
    return parent;
}

/* Copies over the roller entries.
 * Returns number of rollers. Returns -1 on error. */
int8_t jolt_gui_scr_digit_entry_get_arr(lv_obj_t *parent, uint8_t *arr, uint8_t arr_len) {
    int8_t n_entries = -1;
    digit_entry_cont_ext_t *ext = NULL;
    JOLT_GUI_CTX{
        lv_obj_t *cont_body = NULL;
        cont_body = JOLT_GUI_FIND_AND_CHECK(parent, JOLT_GUI_OBJ_ID_CONT_BODY);
        ext = lv_obj_get_ext_attr(cont_body);
        if( arr_len < ext->num_rollers ) {
            /* insufficient output buffer */
            break;
        }
        for(uint8_t i=0; i < ext->num_rollers; i++) {
            arr[i] = 9 - (lv_roller_get_selected(ext->rollers[i]) % 10);
        }

    }
#if ESP_LOG_LEVEL >= ESP_LOG_DEBUG
    if(n_entries > 0){
        printf("Entered PIN: ");
        for(uint8_t i=0; i < ext->num_rollers; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }
#endif
    return n_entries;
}

/* Computes a 256-bit blake2b hash into *hash 
 * Returns 0 on success; 1 on failure*/
uint8_t jolt_gui_scr_digit_entry_get_hash(lv_obj_t *parent, uint8_t *hash) {
    uint8_t res = 1;
    JOLT_GUI_CTX{
        lv_obj_t *cont_body = NULL;
        cont_body = JOLT_GUI_FIND_AND_CHECK(parent, JOLT_GUI_OBJ_ID_CONT_BODY);
        digit_entry_cont_ext_t *ext = lv_obj_get_ext_attr(cont_body);
        uint8_t pin_array[sizeof(ext->rollers)] = { 0 };
        jolt_gui_scr_digit_entry_get_arr(parent, pin_array, sizeof(pin_array));

        /* Convert pin into a 256-bit key */
        crypto_generichash_blake2b_state hs;
        crypto_generichash_init(&hs, NULL, 32, 32);
        crypto_generichash_update(&hs, (unsigned char *) pin_array,
                CONFIG_JOLT_GUI_PIN_LEN);
        crypto_generichash_final(&hs, hash, 32);

        /* Clean up local pin_array variable */
        sodium_memzero(pin_array, sizeof(pin_array));
        res = 0;
    }
    return res;
}

#if 0
void jolt_gui_scr_digit_entry_set_back_action(lv_obj_t *parent, lv_action_t *cb);
    JOLT_GUI_CTX{
        lv_obj_t *cont_body = NULL;
        cont_body = JOLT_GUI_FIND_AND_CHECK(parent, JOLT_GUI_OBJ_ID_CONT_BODY);
        digit_entry_cont_ext_t *ext = lv_obj_get_ext_attr(cont_body);
        ext->back_cb = cb;
    }
}

void jolt_gui_scr_digit_entry_set_enter_action(lv_obj_t *parent, lv_action_t *cb);
    JOLT_GUI_CTX{
        lv_obj_t *cont_body = NULL;
        cont_body = JOLT_GUI_FIND_AND_CHECK(parent, JOLT_GUI_OBJ_ID_CONT_BODY);
        digit_entry_cont_ext_t *ext = lv_obj_get_ext_attr(cont_body);
        ext->enter_cb = cb;
    }
}
#endif


#if 0
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
static lv_res_t jolt_gui_num_signal(lv_obj_t *btn, lv_signal_t sign, void *param);
static lv_signal_func_t ancestor_signal;

/**********************
 *      MACROS
 **********************/
#define MSG(...) printf(__VA_ARGS__)

/**********************
 *   GLOBAL FUNCTIONS
 **********************/



static void jolt_gui_num_align(lv_obj_t *num) {
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);

    /* Compute Roller Spacing */
    {
        uint8_t rol_w = lv_obj_get_width(ext->rollers[0]);
        uint8_t cont_w = lv_obj_get_width(num);
        cont_w = LV_HOR_RES;
        ext->spacing = ( cont_w - ext->len*rol_w ) / ext->len;
        ext->offset = ( cont_w - 
                (ext->len-1) * ext->spacing - ext->len * rol_w ) / 2;
    }

    for(uint8_t i=0; i < ext->len; i++) {
        if(ext->pos == i){
            lv_roller_set_visible_row_count(ext->rollers[i], 3);
        }
        else{
            lv_roller_set_visible_row_count(ext->rollers[i], 1);
        }
        if(0 == i){
            lv_obj_align(ext->rollers[i],
                    NULL, LV_ALIGN_IN_LEFT_MID, 
                    ext->offset, 0);
        }
        else{
            lv_obj_align(ext->rollers[i],
                    ext->rollers[i-1], LV_ALIGN_OUT_RIGHT_MID, 
                    ext->spacing, 0);
        }
    }
}

lv_obj_t *jolt_gui_obj_num_create(lv_obj_t * par, const lv_obj_t * copy) {
    LV_LOG_TRACE("Jolt Numeric create started");

    /*Create the ancestor basic object*/
    lv_obj_t *new_num = lv_cont_create(par, NULL);
    lv_mem_assert(new_num);
    if(new_num == NULL) return NULL;
    lv_obj_set_size(new_num, lv_obj_get_width(par), lv_obj_get_height(par));

    if(ancestor_signal == NULL) ancestor_signal = lv_obj_get_signal_func(new_num);

    /* Declare and initialize ext */
    jolt_gui_num_ext_t *ext = lv_obj_allocate_ext_attr(new_num,
            sizeof(jolt_gui_num_ext_t));
    lv_mem_assert(ext);
    if(ext == NULL) return NULL;
    ext->len = 0;
    ext->pos = 0;
    ext->decimal = -1;
    ext->spacing = 0;
    ext->offset = 0;
    ext->enter_cb = NULL;
    ext->back_cb = NULL;
    memset(ext->rollers, 0, CONFIG_JOLT_GUI_NUMERIC_LEN * sizeof(lv_obj_t *));

    lv_obj_set_signal_func(new_num, jolt_gui_num_signal);

    /* Create decimal point obj so its behind rollers */
    ext->decimal_obj = lv_label_create(new_num, NULL);
    lv_label_set_static_text(ext->decimal_obj, ".");
    lv_obj_set_hidden(ext->decimal_obj, true); // hide the decimal point

    ext->rollers[0] = digit_create(new_num);

    lv_label_set_style( ext->decimal_obj,
            lv_roller_get_style(ext->rollers[0], LV_ROLLER_STYLE_BG) );

    /*Init the new list object*/
    if(copy == NULL) {
    }
    else {
        // todo
    }
    return new_num;
}

void jolt_gui_num_set_len(lv_obj_t *num, uint8_t n) {
    /* todo; garbage collection on previously defined rollers */
    if( 0 == n || n > CONFIG_JOLT_GUI_NUMERIC_LEN) {
        return;
    }
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);
    ext->len = n; // update number of rollers
    for(uint8_t i=1; i < ext->len; i++) {
        if( NULL != ext->rollers[i] ) {
            //lv_obj_del(ext->rollers[i]);
            printf("oh no\n");
        }
        ext->rollers[i] = lv_roller_create(num, ext->rollers[0]);
    }
    jolt_gui_num_align(num);
}

void jolt_gui_num_set_decimal(lv_obj_t *num, int8_t pos) {
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);
    ext->decimal = pos;
    /* Align the decimal point */
    if( pos == 0 ) {
        lv_obj_align(ext->decimal_obj, ext->rollers[0],
                LV_ALIGN_OUT_LEFT_MID, -1, 0);
        lv_obj_set_hidden(ext->decimal_obj, false); // unhide the decimal point
    }
    else if( pos > 0 ) {
        /* compute how much to shift to be centered*/
        // todo: better shifting
        uint8_t shift = ext->spacing;
        if(shift > 5) {
            shift -= 6;
        }
        shift /=2;
        lv_obj_align(ext->decimal_obj, ext->rollers[ext->decimal-1],
                LV_ALIGN_OUT_RIGHT_MID, shift, 0);
        lv_obj_set_hidden(ext->decimal_obj, false); // unhide the decimal point
    }
    else {
        lv_obj_set_hidden(ext->decimal_obj, true); // hide the decimal point
    }
}

uint8_t jolt_gui_num_get_arr(lv_obj_t *num, uint8_t *arr, uint8_t arr_len) {
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);
    if( arr_len < ext->len ) {
        /* insufficient output buffer */
        return 1;
    }
    for(uint8_t i=0; i < ext->len; i++) {
        arr[i] = 9 - (lv_roller_get_selected(ext->rollers[i]) % 10);
    }

#if ESP_LOG_LEVEL >= ESP_LOG_DEBUG
    {
        printf("Entered PIN: ");
        for(uint8_t i=0; i < ext->len; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");
    }
#endif
    return 0;
}

static void loop_roller( lv_obj_t *rol) {
    /* Loops roller position */
    lv_roller_ext_t *rol_ext = lv_obj_get_ext_attr(rol);
    uint8_t id = rol_ext->ddlist.sel_opt_id;
    if( id < 10 ) { // todo refine loop point
        lv_roller_set_selected(rol, id + 10, false);
    }
    if( id > 25 ) { // todo refine loop point
        lv_roller_set_selected(rol, id - 10, false);
    }
}

static lv_res_t jolt_gui_num_signal(lv_obj_t *num, lv_signal_t sign, void *param) {
    lv_res_t res = 0;
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);

#if 0
    /* Include the ancestor signal function */
    res = ancestor_signal(num, sign, param);
    if(res != LV_RES_OK) return res;
#endif

    if(sign == LV_SIGNAL_CORD_CHG) {
        //printf("coord changed\n");
    }
    else if(sign == LV_SIGNAL_STYLE_CHG) {
        //printf("style changed\n");
    }
    else if(sign == LV_SIGNAL_FOCUS) {
        //printf("signal focus\n");
    }
    else if(sign == LV_SIGNAL_DEFOCUS) {
        //printf("signal defocus\n");
    }
#if USE_LV_GROUP
    else if(sign == LV_SIGNAL_CONTROLL){
        char c = *((char *)param);
        
        loop_roller(ext->rollers[ext->pos]);
        lv_roller_ext_t *rol_ext = lv_obj_get_ext_attr(ext->rollers[ext->pos]);

        switch(c){
            case LV_GROUP_KEY_UP: {
                //printf("Pressed up\n");
                lv_obj_t *rol = ext->rollers[ext->pos];
                rol->signal_func(rol, LV_SIGNAL_CONTROLL, param);
                break;
            }

            case LV_GROUP_KEY_DOWN: {
                //printf("Pressed Down\n");
                ext->rollers[ext->pos]->signal_func(
                        ext->rollers[ext->pos],
                        LV_SIGNAL_CONTROLL, param);
                break;
            }
            case LV_GROUP_KEY_LEFT: {
                //printf("Pressed left\n");
                /* Preserve the currently selected roller value */
                rol_ext->ddlist.sel_opt_id_ori = rol_ext->ddlist.sel_opt_id;

                if( ext->pos > 0 ) {
                    (ext->pos)--;
                    jolt_gui_num_align(num);
                }
                else {
                    // Call back callback
                    res = (ext->back_cb)(num);
                }
                break;
            }
            case LV_GROUP_KEY_ENTER: {
                //printf("Pressed enter\n");
                /* Preserve the currently selected roller value */
                rol_ext->ddlist.sel_opt_id_ori = rol_ext->ddlist.sel_opt_id;
                if(ext->pos < ext->len-1) {
                    (ext->pos)++;
                    jolt_gui_num_align(num);
                }
                else {
                    //perform enter action callback
                    (ext->enter_cb)(num);
                }
                break;
            }
            default:
                break;
        }
    }
#endif
    else if(sign == LV_SIGNAL_GET_TYPE) {
        lv_obj_type_t * buf = param;
        uint8_t i;
        for(i = 0; i < LV_MAX_ANCESTOR_NUM - 1; i++) {  /*Find the last set data*/
            if(buf->type[i] == NULL) break;
        }
        buf->type[i] = "jolt_num";
    }

    return res;
}

void jolt_gui_num_set_enter_action(lv_obj_t *num, lv_action_t cb){
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);
    ext->enter_cb = cb;
}

void jolt_gui_num_set_back_action(lv_obj_t *num, lv_action_t cb){
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);
    ext->back_cb = cb;
}

/* Creates title, number entry screen, and handles buttons.
 * Within Jolt, you usually call this function because it creates the typical
 * numerical entry setup.
 */
lv_obj_t *jolt_gui_scr_num_create(const char *title,
        uint8_t len, int8_t dp, lv_action_t cb) {
    lv_obj_t *parent = jolt_gui_parent_create();

    lv_obj_t *numeric = jolt_gui_num_create(parent, NULL);
    jolt_gui_num_set_len(numeric, len);
    lv_obj_align(numeric, jolt_gui_store.statusbar.container, 
            LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    jolt_gui_num_set_decimal(numeric, dp);

    lv_group_add_obj(jolt_gui_store.group.main, numeric);
    lv_group_focus_obj(numeric);

    // so left/right actions get forwarded to this object
    jolt_gui_scr_set_enter_action(parent, &jolt_gui_send_enter_main);
    jolt_gui_scr_set_back_action(parent, &jolt_gui_send_left_main);

    // actions to perform on exiting via enter or back
    jolt_gui_num_set_enter_action(numeric, cb);
    jolt_gui_num_set_back_action(numeric, jolt_gui_scr_del);

    jolt_gui_obj_title_create(parent, title);

    return parent;
}

lv_obj_t *jolt_gui_scr_num_get(lv_obj_t *parent) {
    lv_obj_t *child = NULL;
    lv_obj_type_t obj_type;
    do {
        child = lv_obj_get_child_back(parent, child); //the menu should be the first child
        if ( NULL == child ) {
            // cannot find the child list
            return NULL;
        }
        lv_obj_get_type(child, &obj_type);
    } while(strcmp("jolt_num", obj_type.type[0]));
    return child;
}

/* Computes a 256-bit blake2b hash into *hash */
uint8_t jolt_gui_num_get_hash(lv_obj_t *num, uint8_t *hash) {
    jolt_gui_num_ext_t *ext = lv_obj_get_ext_attr(num);
    uint8_t *pin_array = NULL;
    pin_array = calloc(ext->len, sizeof(uint8_t));
    if( NULL == pin_array ) {
        return 1;
    }

    jolt_gui_num_get_arr( num, pin_array, ext->len );

    /* Convert pin into a 256-bit key */
    crypto_generichash_blake2b_state hs;
    crypto_generichash_init(&hs, NULL, 32, 32);
    crypto_generichash_update(&hs, (unsigned char *) pin_array,
            CONFIG_JOLT_GUI_PIN_LEN);
    crypto_generichash_final(&hs, hash, 32);

    /* Clean up local pin_array variable */
    sodium_memzero(pin_array, ext->len);
    free(pin_array);
    return 0;
}
#endif
