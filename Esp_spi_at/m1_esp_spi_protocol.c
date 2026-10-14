#include "m1_esp_spi_protocol.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

uint16_t m1_spi_crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFFU;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000U) {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

bool m1_spi_pack_frame(uint8_t cmd_id, uint8_t seq, const void *payload, uint16_t len, M1_SpiPacket_t *out_pkt) {
    if (!out_pkt || len > M1_SPI_MAX_PAYLOAD) return false;
    memset(out_pkt, 0, sizeof(*out_pkt));
    out_pkt->magic = M1_SPI_SYNC_MAGIC;
    out_pkt->cmd_id = cmd_id;
    out_pkt->seq_num = seq;
    out_pkt->payload_len = len;
    if (payload && len > 0) {
        memcpy(out_pkt->payload, payload, len);
    }
    out_pkt->crc16_ccitt = m1_spi_crc16_ccitt((const uint8_t *)out_pkt, offsetof(M1_SpiPacket_t, crc16_ccitt));
    return true;
}

bool m1_spi_validate_frame(const M1_SpiPacket_t *pkt) {
    if (!pkt) return false;
    if (pkt->magic != M1_SPI_SYNC_MAGIC) return false;
    if (pkt->payload_len > M1_SPI_MAX_PAYLOAD) return false;
    uint16_t expected = m1_spi_crc16_ccitt((const uint8_t *)pkt, offsetof(M1_SpiPacket_t, crc16_ccitt));
    return (expected == pkt->crc16_ccitt);
}

bool m1_spi_check_c6_compatibility(const M1_C6_VersionInfo_t *ver) {
    if (!ver) return false;
    return (ver->proto_major == M1_SPI_PROTO_VERSION_MAJOR) &&
           (ver->proto_minor >= M1_SPI_PROTO_VERSION_MINOR) &&
           ((ver->capabilities_mask & 0x07U) == 0x07U);
}

/* Compact self-contained RFC 1321 MD5 implementation for Esp32_serial_flasher SD card .bin.md5 verification */
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t  buffer[64];
} M1_Md5Ctx_t;

#define MD5_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | (~z)))
#define MD5_ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static const uint32_t K_MD5[64] = {
    0xd76aa478UL, 0xe8c7b756UL, 0x242070dbUL, 0xc1bdceeeUL, 0xf57c0fafUL, 0x4787c62aUL, 0xa8304613UL, 0xfd469501UL,
    0x698098d8UL, 0x8b44f7afUL, 0xffff5bb1UL, 0x895cd7beUL, 0x6b901122UL, 0xfd987193UL, 0xa679438eUL, 0x49b40821UL,
    0xf61e2562UL, 0xc040b340UL, 0x265e5a51UL, 0xe9b6c7aaUL, 0xd62f105dUL, 0x02441453UL, 0xd8a1e681UL, 0xe7d3fbc8UL,
    0x21e1cde6UL, 0xc33707d6UL, 0xf4d50d87UL, 0x455a14edUL, 0xa9e3e905UL, 0xfcefa3f8UL, 0x676f02d9UL, 0x8d2a4c8aUL,
    0xfffa3942UL, 0x8771f681UL, 0x6d9d6122UL, 0xfde5380cUL, 0xa4beea44UL, 0x4bdecfa9UL, 0xf6bb4b60UL, 0xbebfbc70UL,
    0x289b7ec6UL, 0xeaa127faUL, 0xd4ef3085UL, 0x04881d05UL, 0xd9d4d039UL, 0xe6db99e5UL, 0x1fa27cf8UL, 0xc4ac5665UL,
    0xf4292244UL, 0x432aff97UL, 0xab9423a7UL, 0xfc93a039UL, 0x655b59c3UL, 0x8f0ccc92UL, 0xffeff47dUL, 0x85845dd1UL,
    0x6fa87e4fUL, 0xfe2ce6e0UL, 0xa3014314UL, 0x4e0811a1UL, 0xf7537e82UL, 0xbd3af235UL, 0x2ad7d2bbUL, 0xeb86d391UL
};

static const uint8_t S_MD5[64] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t M[16];
    for (int i = 0; i < 16; ++i) {
        M[i] = (uint32_t)block[i * 4] |
               ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) |
               ((uint32_t)block[i * 4 + 3] << 24);
    }
    uint32_t A = state[0], B = state[1], C = state[2], D = state[3];
    for (int i = 0; i < 64; ++i) {
        uint32_t F_val, g;
        if (i < 16) { F_val = MD5_F(B, C, D); g = (uint32_t)i; }
        else if (i < 32) { F_val = MD5_G(B, C, D); g = (uint32_t)((5 * i + 1) % 16); }
        else if (i < 48) { F_val = MD5_H(B, C, D); g = (uint32_t)((3 * i + 5) % 16); }
        else { F_val = MD5_I(B, C, D); g = (uint32_t)((7 * i) % 16); }
        F_val = F_val + A + K_MD5[i] + M[g];
        A = D;
        D = C;
        C = B;
        B = B + MD5_ROTL(F_val, S_MD5[i]);
    }
    state[0] += A; state[1] += B; state[2] += C; state[3] += D;
}

bool m1_esp_flasher_verify_md5_hex(const uint8_t *fw_bytes, size_t fw_len, const char *expected_32char_md5) {
    if (!fw_bytes || !expected_32char_md5 || strlen(expected_32char_md5) < 32) return false;

    uint32_t state[4] = { 0x67452301UL, 0xefcdab89UL, 0x98badcfeUL, 0x10325476UL };
    size_t offset = 0;
    while (offset + 64 <= fw_len) {
        md5_transform(state, fw_bytes + offset);
        offset += 64;
    }

    uint8_t tail[128];
    memset(tail, 0, sizeof(tail));
    size_t rem = fw_len - offset;
    if (rem > 0) {
        memcpy(tail, fw_bytes + offset, rem);
    }
    tail[rem] = 0x80U;
    size_t total_blocks = (rem >= 56) ? 2 : 1;
    uint64_t bit_len = (uint64_t)fw_len * 8ULL;
    for (int i = 0; i < 8; ++i) {
        tail[total_blocks * 64 - 8 + i] = (uint8_t)((bit_len >> (i * 8)) & 0xFFU);
    }
    for (size_t b = 0; b < total_blocks; ++b) {
        md5_transform(state, tail + b * 64);
    }

    uint8_t digest[16];
    for (int i = 0; i < 4; ++i) {
        digest[i * 4 + 0] = (uint8_t)(state[i] & 0xFFU);
        digest[i * 4 + 1] = (uint8_t)((state[i] >> 8) & 0xFFU);
        digest[i * 4 + 2] = (uint8_t)((state[i] >> 16) & 0xFFU);
        digest[i * 4 + 3] = (uint8_t)((state[i] >> 24) & 0xFFU);
    }

    char calc_hex[33];
    for (int i = 0; i < 16; ++i) {
        snprintf(&calc_hex[i * 2], 3, "%02x", digest[i]);
    }
    for (int i = 0; i < 32; ++i) {
        if (tolower((unsigned char)calc_hex[i]) != tolower((unsigned char)expected_32char_md5[i])) {
            return false;
        }
    }
    return true;
}
