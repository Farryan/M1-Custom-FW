#include "m1_multiprotocol_parsers.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static M1_IrProto_t map_ir_proto_name(const char *str) {
    if (strcmp(str, "NEC") == 0) return M1_IR_PROTO_NEC;
    if (strcmp(str, "NECext") == 0) return M1_IR_PROTO_NEC_EXT;
    if (strcmp(str, "Samsung32") == 0) return M1_IR_PROTO_SAMSUNG32;
    if (strcmp(str, "RC5") == 0 || strcmp(str, "RC5X") == 0) return M1_IR_PROTO_RC5;
    if (strcmp(str, "RC6") == 0) return M1_IR_PROTO_RC6;
    if (strcmp(str, "SIRC") == 0 || strcmp(str, "SIRC15") == 0 || strcmp(str, "SIRC20") == 0) return M1_IR_PROTO_SONY_SIRC;
    if (strcmp(str, "raw") == 0) return M1_IR_PROTO_RAW;
    return M1_IR_PROTO_UNKNOWN;
}

static uint32_t parse_hex_bytes_le(const char *hex_str) {
    uint32_t val = 0;
    unsigned int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
    int n = sscanf(hex_str, "%x %x %x %x", &b0, &b1, &b2, &b3);
    if (n >= 1) val |= (b0 & 0xFFU);
    if (n >= 2) val |= ((b1 & 0xFFU) << 8);
    if (n >= 3) val |= ((b2 & 0xFFU) << 16);
    if (n >= 4) val |= ((b3 & 0xFFU) << 24);
    return val;
}

bool m1_ir_parse_flipper_line_block(const char *block_text, M1_IrSignalRecord_t *out_rec) {
    if (!block_text || !out_rec) return false;
    memset(out_rec, 0, sizeof(*out_rec));
    out_rec->carrier_hz = 38000;
    out_rec->duty_cycle_pct = 33;
    out_rec->protocol = M1_IR_PROTO_UNKNOWN;

    char buf[512];
    strncpy(buf, block_text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *cursor = buf;
    while (*cursor != '\0') {
        while (*cursor == '\r' || *cursor == '\n') {
            *cursor++ = '\0';
        }
        if (*cursor == '\0') break;
        char *line = cursor;
        while (*cursor != '\0' && *cursor != '\r' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor != '\0') {
            *cursor++ = '\0';
        }
        if (strncmp(line, "name: ", 6) == 0) {
            strncpy(out_rec->button_name, line + 6, sizeof(out_rec->button_name) - 1);
        } else if (strncmp(line, "protocol: ", 10) == 0) {
            out_rec->protocol = map_ir_proto_name(line + 10);
        } else if (strncmp(line, "type: raw", 9) == 0) {
            out_rec->protocol = M1_IR_PROTO_RAW;
        } else if (strncmp(line, "address: ", 9) == 0) {
            out_rec->address = parse_hex_bytes_le(line + 9);
        } else if (strncmp(line, "command: ", 9) == 0) {
            out_rec->command = parse_hex_bytes_le(line + 9);
        } else if (strncmp(line, "frequency: ", 11) == 0) {
            out_rec->carrier_hz = (uint32_t)strtoul(line + 11, NULL, 10);
        }
    }
    return (out_rec->button_name[0] != '\0' && out_rec->protocol != M1_IR_PROTO_UNKNOWN);
}

/*
 * Validates Mifare Classic 16-byte sector trailer (bytes 6, 7, 8 contain C1/C2/C3 and their complements)
 */
bool m1_nfc_inspect_mfc_trailer(uint8_t sector_idx, const uint8_t trailer_16b[16], M1_MfcSectorDiag_t *out_diag) {
    if (!trailer_16b || !out_diag) return false;
    memset(out_diag, 0, sizeof(*out_diag));
    out_diag->sector_index = sector_idx;

    uint8_t b6 = trailer_16b[6];
    uint8_t b7 = trailer_16b[7];
    uint8_t b8 = trailer_16b[8];

    uint8_t c1 = (uint8_t)((b7 >> 4) & 0x0FU);
    uint8_t c2 = (uint8_t)(b8 & 0x0FU);
    uint8_t c3 = (uint8_t)((b8 >> 4) & 0x0FU);

    uint8_t c1_inv = (uint8_t)((~b6) & 0x0FU);
    uint8_t c2_inv = (uint8_t)(((~b6) >> 4) & 0x0FU);
    uint8_t c3_inv = (uint8_t)((~b7) & 0x0FU);

    out_diag->access_bits_valid = (c1 == c1_inv) && (c2 == c2_inv) && (c3 == c3_inv);
    if (!out_diag->access_bits_valid) {
        return false;
    }

    for (int blk = 0; blk < 4; ++blk) {
        uint8_t bits = (uint8_t)((((c1 >> blk) & 1U) << 2) |
                                 (((c2 >> blk) & 1U) << 1) |
                                 (((c3 >> blk) & 1U) << 0));
        out_diag->c1_c2_c3[blk] = bits;
    }

    /* Trailer access bits determine if Key B is readable (C1 C2 C3 in {000, 010, 001}) */
    uint8_t tr_bits = out_diag->c1_c2_c3[3];
    out_diag->key_a_readable = false; /* Key A is never readable on Mifare Classic */
    out_diag->key_b_readable = (tr_bits == 0x00 || tr_bits == 0x02 || tr_bits == 0x01);
    return true;
}

void m1_rf_evaluate_field(uint8_t st25r_ad_reg, uint32_t lf_period_ticks_250mhz, M1_RfFieldDiag_t *out_field) {
    if (!out_field) return;
    memset(out_field, 0, sizeof(*out_field));
    out_field->st25r3916_ad_conv_raw = st25r_ad_reg;

    /* ST25R3916 External Field Detector threshold */
    if (st25r_ad_reg >= 24) {
        out_field->active_field = M1_FIELD_HF_13_56MHZ_NFC;
        out_field->measured_freq_hz = 13560000UL;
        out_field->signal_strength_pct = (uint8_t)((st25r_ad_reg * 100U) / 255U);
        return;
    }

    /* STM32H573 Timer Input Capture @ 250 MHz: 125 kHz = 2000 ticks, 134.2 kHz = 1863 ticks */
    if (lf_period_ticks_250mhz >= 1750 && lf_period_ticks_250mhz <= 2150) {
        uint32_t freq = 250000000UL / lf_period_ticks_250mhz;
        out_field->measured_freq_hz = freq;
        out_field->signal_strength_pct = 85;
        if (freq >= 131000UL && freq <= 137500UL) {
            out_field->active_field = M1_FIELD_LF_134KHZ_FDXB;
        } else {
            out_field->active_field = M1_FIELD_LF_125KHZ;
        }
        return;
    }

    out_field->active_field = M1_FIELD_NONE;
}

/*
 * EM4100 64-bit Frame Layout:
 *   Bits 63..55: Nine '1' header bits (0x1FF)
 *   10 rows of (4 data bits + 1 even row parity bit) = 50 bits
 *   4 column parity bits + 1 stop bit ('0')
 */
bool m1_lfrfid_decode_em4100(uint64_t raw, M1_LfEm4100Tag_t *out_tag) {
    if (!out_tag) return false;
    memset(out_tag, 0, sizeof(*out_tag));

    /* Verify 9-bit header of all 1s and stop bit 0 */
    if (((raw >> 55) & 0x1FFULL) != 0x1FFULL) return false;
    if ((raw & 1ULL) != 0ULL) return false;

    uint64_t id40 = 0;
    uint8_t col_parity[4] = {0, 0, 0, 0};

    for (int row = 0; row < 10; ++row) {
        int shift = 50 - (row * 5);
        uint8_t grp5 = (uint8_t)((raw >> shift) & 0x1FU);
        uint8_t nibble = (uint8_t)((grp5 >> 1) & 0x0FU);
        uint8_t p_bit = (uint8_t)(grp5 & 1U);

        uint8_t calc_p = (uint8_t)(((nibble >> 3) & 1U) ^
                                   ((nibble >> 2) & 1U) ^
                                   ((nibble >> 1) & 1U) ^
                                   ((nibble >> 0) & 1U));
        if (calc_p != p_bit) return false;

        for (int c = 0; c < 4; ++c) {
            col_parity[c] ^= (uint8_t)((nibble >> (3 - c)) & 1U);
        }
        id40 = (id40 << 4) | nibble;
    }

    uint8_t rx_col = (uint8_t)((raw >> 1) & 0x0FU);
    uint8_t exp_col = (uint8_t)((col_parity[0] << 3) | (col_parity[1] << 2) |
                                (col_parity[2] << 1) | col_parity[3]);
    if (rx_col != exp_col) return false;

    out_tag->version = (uint8_t)((id40 >> 32) & 0xFFU);
    out_tag->id_code_40bit = id40;
    out_tag->parity_ok = true;
    return true;
}

bool m1_lfrfid_decode_fdxb(uint64_t data_block_64bit, M1_LfFdxbTag_t *out_tag) {
    if (!out_tag) return false;
    memset(out_tag, 0, sizeof(*out_tag));
    out_tag->national_id = data_block_64bit & 0x3FFFFFFFFFULL;          /* Bits 0..37 (38 bits) */
    out_tag->country_code = (uint16_t)((data_block_64bit >> 38) & 0x3FFU); /* Bits 38..47 (10 bits) */
    out_tag->is_animal_tag = ((data_block_64bit >> 63) & 1ULL) != 0ULL;
    return (out_tag->country_code > 0 && out_tag->national_id > 0);
}
