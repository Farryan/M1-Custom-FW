#ifndef M1_FARRYAN_UI_EXT_H
#define M1_FARRYAN_UI_EXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define M1_LCD_WIDTH   128
#define M1_LCD_HEIGHT   64
#define M1_FARRYAN_BRAND_STRING "M1 Farryan v1.2.0-Unleashed"

typedef enum {
    M1_UI_SCREEN_HOME = 0,
    M1_UI_SCREEN_SUBGHZ_ANALYZER,
    M1_UI_SCREEN_NFC_RFID_FIELD_DIAG,
    M1_UI_SCREEN_WIFI6_BLE_802154,
    M1_UI_SCREEN_ESP32_SD_UPDATER,
    M1_UI_SCREEN_COUNT
} M1_UiScreenId_t;

typedef struct {
    M1_UiScreenId_t active_screen;
    uint8_t         cursor_index;
    uint8_t         esp32_flash_progress_pct;
    bool            esp32_md5_verified;
    char            status_line[32];
    char            lcd_text_rows[4][24]; /* 4 visible text rows on 128x64 ST7586s LCD */
} M1_UiContext_t;

void m1_ui_init(M1_UiContext_t *ctx);
void m1_ui_navigate(M1_UiContext_t *ctx, M1_UiScreenId_t target_screen);
void m1_ui_render_esp32_updater(M1_UiContext_t *ctx, uint8_t progress_pct, bool md5_ok);

#ifdef __cplusplus
}
#endif

#endif /* M1_FARRYAN_UI_EXT_H */
