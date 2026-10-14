#ifndef M1_MULTIPROTOCOL_PARSERS_H
#define M1_MULTIPROTOCOL_PARSERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1. Infrared Extended Protocol Support (NECext, Samsung32, RC5, RC6, SIRC) */
typedef enum {
    M1_IR_PROTO_NEC = 0,
    M1_IR_PROTO_NEC_EXT,
    M1_IR_PROTO_SAMSUNG32,
    M1_IR_PROTO_RC5,
    M1_IR_PROTO_RC6,
    M1_IR_PROTO_SONY_SIRC,
    M1_IR_PROTO_RAW,
    M1_IR_PROTO_UNKNOWN
} M1_IrProto_t;

typedef struct {
    char        button_name[32];
    M1_IrProto_t protocol;
    uint32_t    address;
    uint32_t    command;
    uint32_t    carrier_hz;
    uint8_t     duty_cycle_pct;
} M1_IrSignalRecord_t;

/* 2. ST25R3916 NFC Mifare Classic Sector & Access-Bits Diagnostic Inspector */
typedef struct {
    uint8_t  sector_index;
    bool     key_a_readable;
    bool     key_b_readable;
    bool     access_bits_valid;
    uint8_t  c1_c2_c3[4];         /* Access condition bits for blocks 0, 1, 2, and trailer */
} M1_MfcSectorDiag_t;

/* 3. Dual-Band Field Detector (ST25R3916 13.56 MHz HF + 125/134.2 kHz LF AFE) */
typedef enum {
    M1_FIELD_NONE = 0,
    M1_FIELD_LF_125KHZ,
    M1_FIELD_LF_134KHZ_FDXB,
    M1_FIELD_HF_13_56MHZ_NFC
} M1_RfFieldType_t;

typedef struct {
    M1_RfFieldType_t active_field;
    uint32_t         measured_freq_hz;
    uint8_t          st25r3916_ad_conv_raw; /* Register 0x25 AD_CONV_RESULT */
    uint8_t          signal_strength_pct;
} M1_RfFieldDiag_t;

/* 4. LF RFID (EM4100 64-bit Manchester & ISO 11784/11785 FDX-B 134.2 kHz Animal Tag) */
typedef struct {
    uint8_t  version;
    uint64_t id_code_40bit;
    bool     parity_ok;
} M1_LfEm4100Tag_t;

typedef struct {
    uint16_t country_code;        /* ISO 3166 10-bit country code (e.g., 840 = USA, 900 = Mfg) */
    uint64_t national_id;         /* 38-bit national identification number */
    bool     is_animal_tag;
} M1_LfFdxbTag_t;

bool m1_ir_parse_flipper_line_block(const char *block_text, M1_IrSignalRecord_t *out_rec);
bool m1_nfc_inspect_mfc_trailer(uint8_t sector_idx, const uint8_t trailer_16b[16], M1_MfcSectorDiag_t *out_diag);
void m1_rf_evaluate_field(uint8_t st25r_ad_reg, uint32_t lf_period_ticks_250mhz, M1_RfFieldDiag_t *out_field);
bool m1_lfrfid_decode_em4100(uint64_t raw_64bit_frame, M1_LfEm4100Tag_t *out_tag);
bool m1_lfrfid_decode_fdxb(uint64_t data_block_64bit, M1_LfFdxbTag_t *out_tag);

#ifdef __cplusplus
}
#endif

#endif /* M1_MULTIPROTOCOL_PARSERS_H */
