#ifndef M1_SUBGHZ_PRESETS_H
#define M1_SUBGHZ_PRESETS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Monstatek M1 Sub-GHz Radio Architecture:
 * Translates Flipper Zero CC1101 modulation presets to Silicon Labs Si4463 (EZRadioPRO)
 * property arrays (300.000 MHz - 928.000 MHz across Si4463 high/mid/low bands).
 */
typedef enum {
    M1_SI4463_MOD_OOK_270KHZ = 0,   /* AM270 / OOK 270 kHz RX BW (Princeton, CAME, Nice FLO, Linear) */
    M1_SI4463_MOD_OOK_650KHZ,       /* AM650 / OOK 650 kHz RX BW (Wideband OOK remotes, KeeLoq) */
    M1_SI4463_MOD_2FSK_DEV238,      /* FM238 / 2-FSK 2.38 kHz dev, 4.8 kbps Manchester */
    M1_SI4463_MOD_2FSK_DEV476,      /* FM476 / 2-FSK 47.6 kHz dev, 38.4 kbps */
    M1_SI4463_MOD_GFSK_9K6,         /* GFSK 9.6 kbps, BT=0.5 (Sub-GHz packet data link) */
    M1_SI4463_MOD_COUNT
} M1_Si4463_ModPreset_t;

typedef enum {
    M1_RF_REGION_US_FCC = 0,        /* 315 MHz, 433.92 MHz, 902-928 MHz ISM */
    M1_RF_REGION_EU_ETSI,           /* 433.05-434.79 MHz, 868.0-868.6 MHz SRD */
    M1_RF_REGION_JP_ARIB,           /* 315 MHz, 426/429 MHz, 920 MHz */
    M1_RF_REGION_LAB_TEST_BENCH     /* Authorized shielded RF test bench / dummy load (300-928 MHz) */
} M1_RfRegion_t;

typedef struct {
    uint32_t start_hz;
    uint32_t end_hz;
    uint8_t  max_tx_power_ddm;      /* Si4463 PA_PWR_LVL (0x00 .. 0x7F) */
} M1_FreqBandRule_t;

typedef struct {
    M1_Si4463_ModPreset_t preset_id;
    const char           *flipper_name;     /* e.g. "FuriHalSubGhzPresetOok650Async" */
    const char           *short_tag;        /* e.g. "AM650" */
    uint8_t               mod_type_prop;    /* Si4463 PKT/MODEM_MOD_TYPE (0x2000) */
    uint32_t              data_rate_bps;
    uint32_t              freq_dev_hz;
    uint32_t              rx_bw_hz;
    const uint8_t        *si4463_prop_blob; /* Length-prefixed EZRadioPRO SET_PROPERTY commands */
    size_t                prop_blob_len;
} M1_Si4463_PresetDesc_t;

typedef enum {
    M1_SUBGHZ_PROTO_PRINCETON = 0,  /* 24-bit OOK PT2262 / EV1527 */
    M1_SUBGHZ_PROTO_CAME_12_24,     /* 12-bit / 24-bit OOK 320us TE */
    M1_SUBGHZ_PROTO_NICE_FLO,       /* 12-bit / 24-bit OOK 700us TE */
    M1_SUBGHZ_PROTO_LINEAR_10BIT,   /* 10-bit DIP switch 500us TE */
    M1_SUBGHZ_PROTO_CHAMBERLAIN_9,  /* 9-bit / 390 MHz OOK pulse-width */
    M1_SUBGHZ_PROTO_KEELOQ_DIAG,    /* 66-bit KeeLoq diagnostic frame parser (fix + hop inspection) */
    M1_SUBGHZ_PROTO_COUNT
} M1_SubGhz_ProtoId_t;

typedef struct {
    M1_SubGhz_ProtoId_t proto_id;
    const char         *protocol_name;
    uint8_t             bit_count;
    uint64_t            raw_key;
    uint32_t            serial;
    uint8_t             button;
    uint16_t            counter_or_hop_diag; /* Diagnostic read-only counter/hop checksum */
    uint32_t            te_us;
    bool                is_rolling_code;
} M1_SubGhz_DecodedFrame_t;

const M1_Si4463_PresetDesc_t *m1_subghz_get_preset(M1_Si4463_ModPreset_t preset);
const M1_Si4463_PresetDesc_t *m1_subghz_find_preset_by_name(const char *flipper_or_short_name);
bool m1_subghz_is_tx_frequency_authorized(M1_RfRegion_t region, uint32_t freq_hz, bool lab_bench_confirmed);
bool m1_subghz_calc_si4463_freq_props(uint32_t freq_hz, uint8_t *out_inte, uint32_t *out_frac, uint8_t *out_outdiv);

/* Pure pulse-duration decoders & encoders (in microseconds: positive = HIGH/mark, negative = LOW/space) */
bool m1_subghz_decode_princeton(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out);
size_t m1_subghz_encode_princeton(uint32_t key24, uint32_t te_us, int32_t *out_timings, size_t max_len);

bool m1_subghz_decode_came(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out);
size_t m1_subghz_encode_came(uint32_t key, uint8_t bits, int32_t *out_timings, size_t max_len);

bool m1_subghz_decode_nice_flo(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out);
bool m1_subghz_decode_linear(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out);
bool m1_subghz_inspect_keeloq_frame(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out);

#ifdef __cplusplus
}
#endif

#endif /* M1_SUBGHZ_PRESETS_H */
