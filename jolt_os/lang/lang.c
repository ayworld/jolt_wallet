#include "jolt_gui/jolt_gui.h"

typedef uint64_t jolt_lang_avail_t;

/* Pointer to the list of strings for the current language */
static const char **lang_pack = NULL;

/********************
 * Public Functions *
 ********************/

/* Returns specified text in current language */
const char *gettext( jolt_text_id_t id ) {
    if( NULL == lang_pack ){
        jolt_lang_set(CONFIG_JOLT_LANG_DEFAULT);
    }
    return lang_pack[id];
}

/* Returns true if language successfully set, false if language is not available */
bool jolt_lang_set( jolt_lang_t lang ) {
    if(!jolt_lang_available(lang)){
        return false;
    }
    const lv_font_t *font = NULL;

#if CONFIG_JOLT_LANG_ENGLISH_EN
    if( JOLT_LANG_ENGLISH == lang ){
        lang_pack = jolt_lang_english;
        font = jolt_lang_english_font;
    }
#endif

    /* Set the font */
    lv_theme_t *theme = jolt_gui_theme_init(100, font);
    lv_theme_set_current(theme);  

    return true;
}

/* Returns True if the queried language is available */
bool jolt_lang_available( jolt_lang_t lang ) {
#define SHIFT(x) ( 1 << x ) |
    static jolt_lang_avail_t available_lang = 
#if CONFIG_JOLT_LANG_ENGLISH_EN
        SHIFT( JOLT_LANG_ENGLISH )
#endif
        0;
#undef SHIFT

    return (1<<lang) & available_lang;
}

