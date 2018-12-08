/* Jolt Wallet - Open Source Cryptocurrency Hardware Wallet
 Copyright (C) 2018  Brian Pugh, James Coxon, Michael Smaili
 https://www.joltwallet.com/
 */

#include "sodium.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_console.h"
#include "esp_log.h"
#include "linenoise/linenoise.h"

#include "bipmnemonic.h"

#include "jolt_globals.h"
#include "console.h"
#include "vault.h"
#include "jolt_helpers.h"
#include "hal/radio/wifi.h"
#include "hal/storage/storage.h"
#include "sodium.h"

#include "lvgl.h"
#include "jolt_gui/jolt_gui.h"

#include "../console.h"

/* Communication between jolt/cmd_line inputs and cmd task */
#define BACK 0
#define ENTER 1
#define COMPLETE 2

static const char* TAG = "mnemonic_restore";

static const char title[] = "Restore";
static const char prompt[] = "Enter Mnemonic Word: ";

static QueueHandle_t cmd_q;
static CONFIDENTIAL uint8_t idx[24];
static CONFIDENTIAL char user_words[24][11];

static lv_action_t jolt_cmd_mnemonic_restore_back( lv_obj_t *btn ) {
    const uint8_t val = BACK;
    xQueueSend(cmd_q, (void *) &val, portMAX_DELAY);
    return LV_RES_OK;
}

static lv_action_t jolt_cmd_mnemonic_restore_enter( lv_obj_t *btn ) {
    const uint8_t val = ENTER;
    xQueueSend(cmd_q, (void *) &val, portMAX_DELAY);
    return LV_RES_OK;
}

static void linenoise_task( void *h ) {
    char *line;
    uint8_t val_to_send = BACK;
    for(uint8_t i=0; i < sizeof(idx); i++){
        uint8_t j = idx[i];
        lv_obj_t * scr;

        /* Human-friendly 1-idxing */
        char buf[10];
        snprintf(buf, sizeof(buf), "Word %d", j + 1);

        jolt_gui_sem_take();
        jolt_gui_scr_del();
        scr = jolt_gui_scr_text_create(title, buf);
        jolt_gui_scr_set_back_action(scr,  jolt_cmd_mnemonic_restore_back);
        jolt_gui_scr_set_enter_action(scr,  NULL);
        jolt_gui_sem_give();

        int8_t in_wordlist = 0;
        do {
            if( -1 == in_wordlist ) {
                printf("Invalid word\n");
            }

            line = linenoise(prompt);
            if (line == NULL) { /* Ignore empty lines */
                linenoiseFree(line);
                in_wordlist = -1;
                continue;
            }
            if (strcmp(line, "exit_restore") == 0){
                printf("Aborting mnemonic restore\n");
                linenoiseFree(line);
                goto exit;
            }

            strlcpy(user_words[j], line, sizeof(user_words[j]));
            linenoiseFree(line);
            in_wordlist = bm_search_wordlist(user_words[j], strlen(user_words[j]));
        }while( -1 == in_wordlist );
    }
    val_to_send = COMPLETE;

exit:
    sodium_memzero(idx, sizeof(idx));
    xQueueSend( cmd_q, (void *) &val_to_send, portMAX_DELAY );
    *(TaskHandle_t *)h = NULL;
    vTaskDelete(NULL);
}

int jolt_cmd_mnemonic_restore(int argc, char** argv) {
    int return_code = 0;
    TaskHandle_t linenoise_h = 0;
    memset( idx, 0, sizeof( idx ) );
    memset(user_words, 0, sizeof(user_words));

    cmd_q = xQueueCreate( 3, sizeof( uint8_t  )  );

    /* Create prompt screen */
    jolt_gui_sem_take();
    lv_obj_t *scr;
    scr = jolt_gui_scr_text_create(title, "Begin mnemonic restore?");
    if ( NULL == scr ) {
        return_code = -1;
        goto exit;
    }
    jolt_gui_scr_set_back_action(scr,  jolt_cmd_mnemonic_restore_back);
    jolt_gui_scr_set_enter_action(scr, jolt_cmd_mnemonic_restore_enter);
    jolt_gui_sem_give();

    /* Wait until user made a choice */
    uint8_t response;
    xQueueReceive(cmd_q, &response, portMAX_DELAY);
    if( BACK == response ){
        /* Delete prompt screen and exit */
        jolt_gui_sem_take();
        jolt_gui_scr_del();
        jolt_gui_sem_give();
        return_code = -1;
        goto exit;
    }

    /* Generate Random Order for user to input mnemonic */
    for(uint8_t i=0; i< sizeof(idx); i++){
        idx[i] = i;
    }
    shuffle_arr(idx, sizeof(idx));

    /* Create cmd-line monitoring task */
    xTaskCreate(linenoise_task, "m_restore_cmd", 4096,
            (void*)&linenoise_h, 10, &linenoise_h);

    /* Wait for entry completion or cancellation */
    xQueueReceive(cmd_q, &response, portMAX_DELAY);
    if( BACK == response ) {
        jolt_gui_scr_del();
        return_code = -1;
        goto exit;
    }
    else if ( COMPLETE == response ) {
    }

    // Join Mnemonic into single buffer
    CONFIDENTIAL char mnemonic[BM_MNEMONIC_BUF_LEN];
    size_t offset=0;
    for(uint8_t i=0; i < sizeof(idx); i++){
        strlcpy(mnemonic + offset, user_words[i], sizeof(mnemonic) - offset);
        offset += strlen(user_words[i]);
        mnemonic[offset++] = ' ';
    }
    mnemonic[offset - 1] = '\0'; //null-terminate, remove last space

    CONFIDENTIAL uint256_t bin;
    jolt_err_t err = bm_mnemonic_to_bin(bin, sizeof(bin), mnemonic);
    sodium_memzero(mnemonic, sizeof(mnemonic));

    jolt_gui_sem_take();
    jolt_gui_restore_sequence( bin );
    jolt_gui_sem_give();

exit:
    if( 0 != linenoise_h ) {
        ESP_LOGI(TAG, "Deleting linenoise task");
        vTaskDelete(linenoise_h);
    }
    vQueueDelete(cmd_q);
    sodium_memzero(idx, sizeof(idx));
    sodium_memzero(mnemonic, sizeof(mnemonic));
    return return_code;
}

