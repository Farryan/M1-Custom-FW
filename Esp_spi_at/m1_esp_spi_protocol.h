#ifndef M1_ESP_SPI_PROTOCOL_H
#define M1_ESP_SPI_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Shared Dual-MCU SPI Frame Contract (STM32H573 <-> ESP32-C6)
 * Used identically by STM32 (`Esp_spi_at/`) and ESP32-C6 (`esp32_c6_fw/main/`).
 */
#define M1_SPI_SYNC_MAGIC           0xA55AU
#define M1_SPI_PROTO_VERSION_MAJOR  1U
#define M1_SPI_PROTO_VERSION_MINOR  2U
#define M1_SPI_MAX_PAYLOAD          240U

typedef enum {
    M1_C6_CMD_PING_VERSION      = 0x01, /* Query ESP32-C6 FW & SPI protocol version */
    M1_C6_CMD_WIFI6_SCAN_START  = 0x10, /* Passive 2.4 GHz 802.11b/g/n/ax AP & HE capability scan */
    M1_C6_CMD_WIFI6_SCAN_GET    = 0x11, /* Fetch discovered AP record */
    M1_C6_CMD_BLE_SCAN_START    = 0x20, /* Passive BLE 5.0 advertisement scanner */
    M1_C6_CMD_BLE_HID_KEYSTROKE = 0x21, /* Authorized BLE HID Keyboard report injection */
    M1_C6_CMD_802154_SCAN_CH    = 0x30, /* Passive IEEE 802.15.4 Zigbee/Thread channel energy & PAN scan */
    M1_C6_CMD_ACK               = 0x80,
    M1_C6_CMD_ERR               = 0xFF
} M1_C6_CommandId_t;

#pragma pack(push, 1)
typedef struct {
    uint16_t magic;             /* M1_SPI_SYNC_MAGIC (0xA55A) */
    uint8_t  cmd_id;            /* M1_C6_CommandId_t */
    uint8_t  seq_num;           /* Rolling sequence number for retry deduplication */
    uint16_t payload_len;       /* 0 .. M1_SPI_MAX_PAYLOAD */
    uint8_t  payload[M1_SPI_MAX_PAYLOAD];
    uint16_t crc16_ccitt;       /* CRC-16/CCITT-FALSE over header + payload */
} M1_SpiPacket_t;

typedef struct {
    uint8_t  proto_major;
    uint8_t  proto_minor;
    uint8_t  fw_major;
    uint8_t  fw_minor;
    uint8_t  fw_patch;
    uint8_t  capabilities_mask; /* Bit0=WiFi6, Bit1=BLE5_HID, Bit2=802154_Zigbee */
    char     build_tag[18];
} M1_C6_VersionInfo_t;

typedef struct {
    char     ssid[33];
    uint8_t  bssid[6];
    int8_t   rssi_dbm;
    uint8_t  channel;
    uint8_t  auth_mode;
    uint8_t  is_wifi6_he;       /* 1 if 802.11ax High Efficiency IE present */
} M1_C6_WifiApRecord_t;

typedef struct {
    uint8_t  channel;           /* IEEE 802.15.4 channels 11..26 */
    int8_t   ed_peak_dbm;       /* Energy Detection peak in dBm */
    uint16_t pan_id;            /* Observed beacon PAN ID (0xFFFF if none) */
    uint16_t frame_count;
} M1_C6_802154_ChannelDiag_t;
#pragma pack(pop)

uint16_t m1_spi_crc16_ccitt(const uint8_t *data, size_t len);
bool m1_spi_pack_frame(uint8_t cmd_id, uint8_t seq, const void *payload, uint16_t len, M1_SpiPacket_t *out_pkt);
bool m1_spi_validate_frame(const M1_SpiPacket_t *pkt);
bool m1_spi_check_c6_compatibility(const M1_C6_VersionInfo_t *ver);

/* Esp32_serial_flasher MD5 verification helper (RFC 1321 digest check before UART flashing) */
bool m1_esp_flasher_verify_md5_hex(const uint8_t *fw_bytes, size_t fw_len, const char *expected_32char_md5);

#ifdef __cplusplus
}
#endif

#endif /* M1_ESP_SPI_PROTOCOL_H */
