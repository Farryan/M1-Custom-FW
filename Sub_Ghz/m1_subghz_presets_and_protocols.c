#include "m1_subghz_DoubleCheck_presets.h"
#include <string.h>
#include <stdlib.h>

/* Si4463 SET_PROPERTY (0x11) command blobs for Monstatek M1 30 MHz TCXO */
static const uint8_t SI4463_PROP_OOK_270KHZ[] = {
    /* Group 0x20 (MODEM), 4 bytes from 0x00: Direct Async OOK + GPIO1/2 mapping */
    0x06, 0x11, 0x20, 0x04, 0x00, 0x89, 0x00, 0x07, 0x20,
    /* Group 0x20, BCR/AFC/AGC OOK 270 kHz decimation filter */
    0x07, 0x11, 0x20, 0x03, 0x1E, 0x01, 0xB0, 0x02,
    0x00
};

static const uint8_t SI4463_PROP_OOK_650KHZ[] = {
    0x06, 0x11, 0x20, 0x04, 0x00, 0x89, 0x00, 0x07, 0x20,
    0x07, 0x11, 0x20, 0x03, 0x1E, 0x00, 0x70, 0x01,
    0x00
};

static const uint8_t SI4463_PROP_2FSK_DEV238[] = {
    /* Direct/FIFO 2-FSK, 4.8 kbps, 2.38 kHz deviation */
    0x06, 0x11, 0x20, 0x04, 0x00, 0x02, 0x00, 0x07, 0x20,
    0x07, 0x11, 0x20, 0x03, 0x0A, 0x00, 0x00, 0xA4,
    0x00
};

static const uint8_t SI4463_PROP_2FSK_DEV476[] = {
    /* 2-FSK, 38.4 kbps, 47.6 kHz deviation */
    0x06, 0x11, 0x20, 0x04, 0x00, 0x02, 0x00, 0x07, 0x20,
    0x07, 0x11, 0x20, 0x03, 0x0A, 0x00, 0x0C, 0xE5,
    0x00
};

static const uint8_t SI4463_PROP_GFSK_9K6[] = {
    /* Packet GFSK, 9.6 kbps, Gaussian BT=0.5 */
    0x06, 0x11, 0x20, 0x04, 0x00, 0x03, 0x00, 0x07, 0x20,
    0x07, 0x11, 0x20, 0x03, 0x0A, 0x00, 0x02, 0x8F,
    0x00
};

static const M1_Si4463_PresetDesc_t s_presets[M1_SI4463_MOD_COUNT] = {
    {
        M1_SI4463_MOD_OOK_270KHZ,
        "FuriHalSubGhzPresetOok270Async",
        "AM270",
        0x89, 4800, 0, 270000,
        SI4463_PROP_OOK_270KHZ, sizeof(SI4463_PROP_OOK_270KHZ)
    },
    {
        M1_SI4463_MOD_OOK_650KHZ,
        "FuriHalSubGhzPresetOok650Async",
        "AM650",
        0x89, 10000, 0, 650000,
        SI4463_PROP_OOK_650KHZ, sizeof(SI4463_PROP_OOK_650KHZ)
    },
    {
        M1_SI4463_MOD_2FSK_DEV238,
        "FuriHalSubGhzPreset2FSKDev238Async",
        "FM238",
        0x02, 4800, 2380, 50000,
        SI4463_PROP_2FSK_DEV238, sizeof(SI4463_PROP_2FSK_DEV238)
    },
    {
        M1_SI4463_MOD_2FSK_DEV476,
        "FuriHalSubGhzPreset2FSKDev476Async",
        "FM476",
        0x02, 38400, 47600, 150000,
        SI4463_PROP_2FSK_DEV476, sizeof(SI4463_PROP_2FSK_DEV476)
    },
    {
        M1_SI4463_MOD_GFSK_9K6,
        "FuriHalSubGhzPresetGFSK9_99KbAsync",
        "GFSK9K6",
        0x03, 9600, 9600, 50000,
        SI4463_PROP_GFSK_9K6, sizeof(SI4463_PROP_GFSK_9K6)
    }
};

const M1_Si4463_PresetDesc_t *m1_subghz_get_preset(M1_Si4463_ModPreset_t preset) {
    if (preset >= M1_SI4463_MOD_COUNT) {
        return NULL;
    }
    return &s_presets[preset];
}

const M1_Si4463_PresetDesc_t *m1_subghz_find_preset_by_name(const char *name) {
    if (!name) return NULL;
    for (size_t i = 0; i < M1_SI4463_MOD_COUNT; ++i) {
        if (strcmp(s_presets[i].flipper_name, name) == 0 ||
            strcmp(s_presets[i].short_tag, name) == 0) {
            return &s_presets[i];
        }
    }
    return NULL;
}

bool m1_subghz_is_tx_frequency_authorized(M1_RfRegion_t region, uint32_t freq_hz, bool lab_bench_confirmed) {
    if (freq_hz < 300000000UL || freq_hz > 928000000UL) {
        return false;
    }
    switch (region) {
        case M1_RF_REGION_US_FCC:
            return (freq_hz >= 310000000UL && freq_hz <= 320000000UL) ||
                   (freq_hz >= 433050000UL && freq_hz <= 434790000UL) ||
                   (freq_hz >= 902000000UL && freq_hz <= 928000000UL);
        case M1_RF_REGION_EU_ETSI:
            return (freq_hz >= 433050000UL && freq_hz <= 434790000UL) ||
                   (freq_hz >= 868000000UL && freq_hz <= 868600000UL);
        case M1_RF_REGION_JP_ARIB:
            return (freq_hz >= 312000000UL && freq_hz <= 315250000UL) ||
                   (freq_hz >= 920500000UL && freq_hz <= 923500000UL);
        case M1_RF_REGION_LAB_TEST_BENCH:
            return lab_bench_confirmed;
        default:
            return false;
    }
}

/*
 * Computes Si4463 FREQ_CONTROL_INTE (0x4000) and FREQ_CONTROL_FRAC (0x4001..0x4003)
 * for a 30 MHz crystal oscillator and NPRESC = 2:
 *   f_rf = (f_xo / outdiv) * (INTE + FRAC / 2^19)
 */
bool m1_subghz_calc_si4463_freq_props(uint32_t freq_hz, uint8_t *out_inte, uint32_t *out_frac, uint8_t *out_outdiv) {
    if (!out_inte || !out_frac || !out_outdiv) return false;
    if (freq_hz < 300000000UL || freq_hz > 928000000UL) return false;

    uint8_t outdiv = 4;
    if (freq_hz < 420000000UL) {
        outdiv = 12;
    } else if (freq_hz < 525000000UL) {
        outdiv = 8;
    } else {
        outdiv = 4;
    }

    const uint64_t f_xo_div = 60000000ULL / outdiv; /* 2 * 30 MHz / outdiv */
    uint64_t total_scaled = ((uint64_t)freq_hz << 19) / f_xo_div;
    uint8_t inte = (uint8_t)((total_scaled >> 19) - 1);
    uint32_t frac = (uint32_t)(total_scaled & 0xFFFFFULL) | (1UL << 19);

    *out_inte = inte;
    *out_frac = frac;
    *out_outdiv = outdiv;
    return true;
}

static bool timing_near(int32_t measured, int32_t target, int32_t tol) {
    int32_t diff = measured - target;
    if (diff < 0) diff = -diff;
    return diff <= tol;
}

/*
 * Princeton (PT2262 / EV1527) 24-bit OOK Decoder:
 *   Bit 0: +1*TE, -3*TE
 *   Bit 1: +3*TE, -1*TE
 *   Sync : +1*TE, -31*TE (TE typically 350us)
 */
bool m1_subghz_decode_princeton(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out) {
    if (!timings || !out || count < 50) return false;
    const int32_t te = 350;
    const int32_t tol = 120;

    for (size_t i = 0; i + 49 < count; i += 2) {
        uint64_t key = 0;
        bool valid = true;
        for (size_t b = 0; b < 24; ++b) {
            int32_t mark = timings[i + b * 2];
            int32_t space = -timings[i + b * 2 + 1];
            if (mark <= 0 || space <= 0) { valid = false; break; }

            if (timing_near(mark, te, tol) && timing_near(space, 3 * te, tol * 2)) {
                key = (key << 1) | 0ULL;
            } else if (timing_near(mark, 3 * te, tol * 2) && timing_near(space, te, tol)) {
                key = (key << 1) | 1ULL;
            } else {
                valid = false;
                break;
            }
        }
        if (valid) {
            int32_t sync_mark = timings[i + 48];
            int32_t sync_space = -timings[i + 49];
            if (timing_near(sync_mark, te, tol) && sync_space >= 25 * te) {
                memset(out, 0, sizeof(*out));
                out->proto_id = M1_SUBGHZ_PROTO_PRINCETON;
                out->protocol_name = "Princeton";
                out->bit_count = 24;
                out->raw_key = key & 0xFFFFFFULL;
                out->serial = (uint32_t)((key >> 4) & 0xFFFFFULL);
                out->button = (uint8_t)(key & 0x0FULL);
                out->te_us = (uint32_t)te;
                out->is_rolling_code = false;
                return true;
            }
        }
    }
    return false;
}

size_t m1_subghz_encode_princeton(uint32_t key24, uint32_t te_us, int32_t *out_timings, size_t max_len) {
    if (!out_timings || max_len < 50) return 0;
    if (te_us == 0) te_us = 350;
    size_t idx = 0;
    for (int bit = 23; bit >= 0; --bit) {
        if ((key24 >> bit) & 1U) {
            out_timings[idx++] = (int32_t)(3 * te_us);
            out_timings[idx++] = -(int32_t)(1 * te_us);
        } else {
            out_timings[idx++] = (int32_t)(1 * te_us);
            out_timings[idx++] = -(int32_t)(3 * te_us);
        }
    }
    out_timings[idx++] = (int32_t)(1 * te_us);
    out_timings[idx++] = -(int32_t)(31 * te_us);
    return idx;
}

/*
 * CAME 12/24-bit OOK Decoder & Encoder (TE = 320us):
 *   Pilot: -11520us (36*TE), Start bit: +320us
 *   Bit 0: -320us, +640us
 *   Bit 1: -640us, +320us
 */
bool m1_subghz_decode_came(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out) {
    if (!timings || !out || count < 26) return false;
    const int32_t te = 320;
    const int32_t tol = 110;

    for (size_t i = 0; i + 25 < count; ++i) {
        if (timings[i] <= -(30 * te) && timing_near(timings[i + 1], te, tol)) {
            uint64_t key = 0;
            uint8_t bits = 0;
            size_t pos = i + 2;
            while (pos + 1 < count && bits < 24) {
                int32_t low_dur = -timings[pos];
                int32_t high_dur = timings[pos + 1];
                if (timing_near(low_dur, te, tol) && timing_near(high_dur, 2 * te, tol)) {
                    key = (key << 1) | 0ULL;
                    bits++;
                    pos += 2;
                } else if (timing_near(low_dur, 2 * te, tol) && timing_near(high_dur, te, tol)) {
                    key = (key << 1) | 1ULL;
                    bits++;
                    pos += 2;
                } else {
                    break;
                }
            }
            if (bits == 12 || bits == 24) {
                memset(out, 0, sizeof(*out));
                out->proto_id = M1_SUBGHZ_PROTO_CAME_12_24;
                out->protocol_name = "CAME";
                out->bit_count = bits;
                out->raw_key = key;
                out->serial = (uint32_t)(key >> 4);
                out->button = (uint8_t)(key & 0x0F);
                out->te_us = (uint32_t)te;
                out->is_rolling_code = false;
                return true;
            }
        }
    }
    return false;
}

size_t m1_subghz_encode_came(uint32_t key, uint8_t bits, int32_t *out_timings, size_t max_len) {
    if (!out_timings || (bits != 12 && bits != 24) || max_len < (size_t)(bits * 2 + 2)) {
        return 0;
    }
    const int32_t te = 320;
    size_t idx = 0;
    out_timings[idx++] = -(36 * te);
    out_timings[idx++] = te;
    for (int b = bits - 1; b >= 0; --b) {
        if ((key >> b) & 1U) {
            out_timings[idx++] = -(2 * te);
            out_timings[idx++] = te;
        } else {
            out_timings[idx++] = -te;
            out_timings[idx++] = 2 * te;
        }
    }
    return idx;
}

/* Nice FLO 12/24-bit OOK Decoder (TE = 700us) */
bool m1_subghz_decode_nice_flo(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out) {
    if (!timings || !out || count < 26) return false;
    const int32_t te = 700;
    const int32_t tol = 200;

    for (size_t i = 0; i + 25 < count; ++i) {
        if (timings[i] <= -(30 * te) && timing_near(timings[i + 1], te, tol)) {
            uint64_t key = 0;
            uint8_t bits = 0;
            size_t pos = i + 2;
            while (pos + 1 < count && bits < 24) {
                int32_t low_dur = -timings[pos];
                int32_t high_dur = timings[pos + 1];
                if (timing_near(low_dur, te, tol) && timing_near(high_dur, 2 * te, tol)) {
                    key = (key << 1) | 0ULL;
                    bits++;
                    pos += 2;
                } else if (timing_near(low_dur, 2 * te, tol) && timing_near(high_dur, te, tol)) {
                    key = (key << 1) | 1ULL;
                    bits++;
                    pos += 2;
                } else {
                    break;
                }
            }
            if (bits == 12 || bits == 24) {
                memset(out, 0, sizeof(*out));
                out->proto_id = M1_SUBGHZ_PROTO_NICE_FLO;
                out->protocol_name = "Nice FLO";
                out->bit_count = bits;
                out->raw_key = key;
                out->serial = (uint32_t)(key >> 4);
                out->button = (uint8_t)(key & 0x0F);
                out->te_us = (uint32_t)te;
                out->is_rolling_code = false;
                return true;
            }
        }
    }
    return false;
}

/* Linear 10-bit DIP-switch Decoder (TE = 500us) */
bool m1_subghz_decode_linear(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out) {
    if (!timings || !out || count < 20) return false;
    const int32_t te = 500;
    const int32_t tol = 150;

    uint64_t key = 0;
    for (size_t b = 0; b < 10; ++b) {
        int32_t mark = timings[b * 2];
        int32_t space = -timings[b * 2 + 1];
        if (timing_near(mark, te, tol) && timing_near(space, 3 * te, tol)) {
            key = (key << 1) | 0ULL;
        } else if (timing_near(mark, 3 * te, tol) && timing_near(space, te, tol)) {
            key = (key << 1) | 1ULL;
        } else {
            return false;
        }
    }
    memset(out, 0, sizeof(*out));
    out->proto_id = M1_SUBGHZ_PROTO_LINEAR_10BIT;
    out->protocol_name = "Linear";
    out->bit_count = 10;
    out->raw_key = key;
    out->serial = (uint32_t)key;
    out->te_us = (uint32_t)te;
    out->is_rolling_code = false;
    return true;
}

/*
 * KeeLoq 66-bit Diagnostic Frame Inspector (Read-only signal analysis):
 * Extracts the unencrypted 28-bit Serial Number, 4-bit Button code, and 16-bit
 * Hop-block diagnostic checksum so authorized engineers can verify transmitter health.
 */
bool m1_subghz_inspect_keeloq_frame(const int32_t *timings, size_t count, M1_SubGhz_DecodedFrame_t *out) {
    if (!timings || !out || count < 132) return false;
    const int32_t te = 400;
    const int32_t tol = 140;

    uint64_t first64 = 0;
    uint8_t tail2 = 0;
    for (size_t b = 0; b < 66; ++b) {
        int32_t mark = timings[b * 2];
        int32_t space = -timings[b * 2 + 1];
        uint8_t bit_val = 0;
        /* KeeLoq PWM: '0' = 2*TE High + 1*TE Low; '1' = 1*TE High + 2*TE Low (LSB first) */
        if (timing_near(mark, 2 * te, tol) && timing_near(space, te, tol)) {
            bit_val = 0;
        } else if (timing_near(mark, te, tol) && timing_near(space, 2 * te, tol)) {
            bit_val = 1;
        } else {
            return false;
        }
        if (b < 64) {
            first64 |= ((uint64_t)bit_val << b);
        } else {
            tail2 |= (uint8_t)(bit_val << (b - 64));
        }
    }

    uint32_t hop_part = (uint32_t)(first64 & 0xFFFFFFFFULL);
    uint32_t fix_part = (uint32_t)(first64 >> 32);

    memset(out, 0, sizeof(*out));
    out->proto_id = M1_SUBGHZ_PROTO_KEELOQ_DIAG;
    out->protocol_name = "KeeLoq (Diag)";
    out->bit_count = 66;
    out->raw_key = first64;
    out->serial = fix_part & 0x0FFFFFFFUL;
    out->button = (uint8_t)((fix_part >> 28) & 0x0FU);
    out->counter_or_hop_diag = (uint16_t)((hop_part >> 16) ^ (hop_part & 0xFFFFU) ^ tail2);
    out->te_us = (uint32_t)te;
    out->is_rolling_code = true;
    return true;
}
