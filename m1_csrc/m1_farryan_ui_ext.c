#include "m1_farryan_ui_ext.h"
#include <string.h>
#include <stdio.h>

void m1_ui_init(M1_UiContext_t *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->active_screen = M1_UI_SCREEN_HOME;
    snprintf(ctx->lcd_text_rows[0], sizeof(ctx->lcd_text_rows[0]), "Monstatek M1 [Farryan]");
    snprintf(ctx->lcd_text_rows[1], sizeof(ctx->lcd_text_rows[1]), "> Sub-GHz (Si4463)");
    snprintf(ctx->lcd_text_rows[2], sizeof(ctx->lcd_text_rows[2]), "  NFC/RFID Field Diag");
    snprintf(ctx->lcd_text_rows[3], sizeof(ctx->lcd_text_rows[3]), "  ESP32-C6 Radio/OTA");
}

void m1_ui_navigate(M1_UiContext_t *ctx, M1_UiScreenId_t target_screen) {
    if (!ctx || target_screen >= M1_UI_SCREEN_COUNT) return;
    ctx->active_screen = target_screen;
    switch (target_screen) {
        case M1_UI_SCREEN_SUBGHZ_ANALYZER:
            snprintf(ctx->lcd_text_rows[0], sizeof(ctx->lcd_text_rows[0]), "[Sub-GHz Si4463]");
            snprintf(ctx->lcd_text_rows[1], sizeof(ctx->lcd_text_rows[1]), "Preset: AM650 / FM238");
            snprintf(ctx->lcd_text_rows[2], sizeof(ctx->lcd_text_rows[2]), "Codecs: Princ/CAME/Flo");
            snprintf(ctx->lcd_text_rows[3], sizeof(ctx->lcd_text_rows[3]), "Status: Ready");
            break;
        case M1_UI_SCREEN_NFC_RFID_FIELD_DIAG:
            snprintf(ctx->lcd_text_rows[0], sizeof(ctx->lcd_text_rows[0]), "[HF/LF Field Detector]");
            snprintf(ctx->lcd_text_rows[1], sizeof(ctx->lcd_text_rows[1]), "ST25R3916 + 125/134kHz");
            snprintf(ctx->lcd_text_rows[2], sizeof(ctx->lcd_text_rows[2]), "MFC Sector Trailer OK");
            snprintf(ctx->lcd_text_rows[3], sizeof(ctx->lcd_text_rows[3]), "EM4100 / FDX-B Ready");
            break;
        case M1_UI_SCREEN_WIFI6_BLE_802154:
            snprintf(ctx->lcd_text_rows[0], sizeof(ctx->lcd_text_rows[0]), "[ESP32-C6 Coprocessor]");
            snprintf(ctx->lcd_text_rows[1], sizeof(ctx->lcd_text_rows[1]), "1. Wi-Fi 6 HE Scanner");
            snprintf(ctx->lcd_text_rows[2], sizeof(ctx->lcd_text_rows[2]), "2. BLE 5.0 Scan & HID");
            snprintf(ctx->lcd_text_rows[3], sizeof(ctx->lcd_text_rows[3]), "3. 802.15.4 Zigbee ED");
            break;
        case M1_UI_SCREEN_ESP32_SD_UPDATER:
            m1_ui_render_esp32_updater(ctx, ctx->esp32_flash_progress_pct, ctx->esp32_md5_verified);
            break;
        default:
            m1_ui_init(ctx);
            break;
    }
}

void m1_ui_render_esp32_updater(M1_UiContext_t *ctx, uint8_t progress_pct, bool md5_ok) {
    if (!ctx) return;
    if (progress_pct > 100) progress_pct = 100;
    ctx->active_screen = M1_UI_SCREEN_ESP32_SD_UPDATER;
    ctx->esp32_flash_progress_pct = progress_pct;
    ctx->esp32_md5_verified = md5_ok;

    snprintf(ctx->lcd_text_rows[0], sizeof(ctx->lcd_text_rows[0]), "[ESP32-C6 SD Updater]");
    snprintf(ctx->lcd_text_rows[1], sizeof(ctx->lcd_text_rows[1]), "File: esp32_c6_fw.bin");
    snprintf(ctx->lcd_text_rows[2], sizeof(ctx->lcd_text_rows[2]), "MD5 : %s", md5_ok ? "VERIFIED" : "CHECKING");
    snprintf(ctx->lcd_text_rows[3], sizeof(ctx->lcd_text_rows[3]), "Prog: %3u%% [%s]",
             progress_pct, (progress_pct == 100) ? "COMPLETE" : "FLASHING");
}
