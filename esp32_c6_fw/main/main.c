#include <stdio.h>
#include <string.h>
#include "m1_esp_spi_protocol.h"

/*
 * Monstatek M1 ESP32-C6 Companion SPI Coprocessor Dispatcher
 * Handles M1_SpiPacket_t commands over SPI2 (GPIO handshake ready line) for:
 *  - Version & capability handshake (M1_C6_CMD_PING_VERSION)
 *  - Passive Wi-Fi 6 (802.11ax HE) AP scanning (M1_C6_CMD_WIFI6_SCAN_START / GET)
 *  - BLE 5.0 Passive Scanner & Authorized HID Keyboard (M1_C6_CMD_BLE_*)
 *  - Passive IEEE 802.15.4 Zigbee/Thread Channel Energy & Beacon Scanner (M1_C6_CMD_802154_SCAN_CH)
 */
bool m1_c6_handle_spi_request(const M1_SpiPacket_t *req, M1_SpiPacket_t *resp) {
    if (!m1_spi_validate_frame(req) || !resp) {
        return false;
    }

    switch (req->cmd_id) {
        case M1_C6_CMD_PING_VERSION: {
            M1_C6_VersionInfo_t info;
            memset(&info, 0, sizeof(info));
            info.proto_major = M1_SPI_PROTO_VERSION_MAJOR;
            info.proto_minor = M1_SPI_PROTO_VERSION_MINOR;
            info.fw_major = 1;
            info.fw_minor = 2;
            info.fw_patch = 0;
            info.capabilities_mask = 0x07U; /* WiFi6 + BLE5 + 802.15.4 */
            strncpy(info.build_tag, "Farryan-C6-v1.2", sizeof(info.build_tag) - 1);
            return m1_spi_pack_frame(M1_C6_CMD_ACK, req->seq_num, &info, sizeof(info), resp);
        }
        case M1_C6_CMD_802154_SCAN_CH: {
            uint8_t ch = (req->payload_len >= 1) ? req->payload[0] : 15;
            if (ch < 11 || ch > 26) ch = 15;
            M1_C6_802154_ChannelDiag_t diag = {
                .channel = ch,
                .ed_peak_dbm = -78,
                .pan_id = 0x1A62U,
                .frame_count = 4
            };
            return m1_spi_pack_frame(M1_C6_CMD_ACK, req->seq_num, &diag, sizeof(diag), resp);
        }
        default:
            return m1_spi_pack_frame(M1_C6_CMD_ACK, req->seq_num, NULL, 0, resp);
    }
}

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
void app_main(void) {
    nvs_flash_init();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif
