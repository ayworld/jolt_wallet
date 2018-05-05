#ifndef __NANORAY_STATUSBAR_H__
#define __NANORAY_STATUSBAR_H__

void statusbar_enable(menu8g2_t *menu);
void statusbar_disable(menu8g2_t *menu);

void statusbar_update(menu8g2_t *menu);
void statusbar_task(void *menu_in);

#endif
