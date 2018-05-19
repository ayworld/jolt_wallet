#ifndef __NANORAY_HELPERS_H__
#define __NANORAY_HELPERS_H__

#include "u8g2.h"

uint8_t get_center_x(u8g2_t *u8g2, const char *text);
void nvs_log_err(esp_err_t err);

#endif