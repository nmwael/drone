/*
 * TS Muxer - MPEG-TS Packetization with PAT/PMT/PES
 *
 * Creates proper MPEG-TS packets for STANAG 4609 compliance:
 * - PAT (PID 0x0000) - Program Association Table
 * - PMT (PID 0x1000) - Program Map Table
 * - Video PES (PID 0x0100) - H.264 video stream
 * - KLV PES (PID 0x0101) - MISB ST 0601 metadata
 * - PCR embedded in video packets
 */

#include "stanag_types.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static const char *TAG = "TS_MUXER";

/* PIDs */
static const uint16_t PAT_PID       = 0x0000;
static const uint16_t PMT_PID       = 0x1000;
static const uint16_t VIDEO_PID     = 0x0100;
static const uint16_t KLV_PID       = 0x0101;
static const uint16_t NULL_PID      = 0x1FFF;

/* Stream types (used in PMT table) */
/* static const uint8_t STREAM_TYPE_H264 = 0x1B; */
/* static const uint8_t STREAM_TYPE_KLV  = 0x06; */

/* Continuity counters */
static uint8_t s_cc_pat = 0;
static uint8_t s_cc_pmt = 0;
static uint8_t s_cc_video = 0;
static uint8_t s_cc_klv = 0;

/* MPEG-TS packet buffer (up to 10 packets = 1880 bytes) */
static uint8_t s_ts_buffer[188 * 10];
static ts_packet_t s_ts_packet;

/* PAT table (static for single program) */
static const uint8_t s_pat_data[] = {
    /* table_id = 0x00 (PAT) */
    0x00,
    /* section_syntax_indicator=1, private=0, reserved=3, section_length=13 (0x00D) */
    0xB0, 0x0D,
    /* transport_stream_id = 1 */
    0x00, 0x01,
    /* reserved=3, version_number=0, current_next_indicator=1 */
    0xC1,
    /* section_number = 0 */
    0x00,
    /* last_section_number = 0 */
    0x00,
    /* Program 1 -> PMT PID 0x1000 */
    0x00, 0x01,  /* program_number = 1 */
    0xE0, 0x00,  /* reserved=7, network_PID=0x0000 (NIT) */
    /* Program 0 -> NIT PID (optional) */
    /* CRC32 placeholder (4 bytes) - computed at runtime */
    0x00, 0x00, 0x00, 0x00
};

/* PMT table */
static const uint8_t s_pmt_data_base[] = {
    /* table_id = 0x02 (PMT) */
    0x02,
    /* section_syntax_indicator=1, private=0, reserved=3, section_length=0x0017 (23 bytes) */
    0xB0, 0x17,
    /* program_number = 1 */
    0x00, 0x01,
    /* reserved=3, version_number=0, current_next_indicator=1 */
    0xC1,
    /* section_number = 0 */
    0x00,
    /* last_section_number = 0 */
    0x00,
    /* reserved=7, PCR_PID = 0x0100 (video PID carries PCR) */
    0xE1, 0x00,
    /* reserved=4, program_info_length = 0 */
    0xF0, 0x00,
    /* Stream 1: H.264 video */
    0x1B,        /* stream_type = 0x1B (H.264) */
    0xE1, 0x00,  /* reserved=3, elementary_PID = 0x0100 */
    0xF0, 0x00,  /* reserved=4, ES_info_length = 0 */
    /* Stream 2: KLV metadata (private) */
    0x06,        /* stream_type = 0x06 (private) */
    0xE1, 0x01,  /* reserved=3, elementary_PID = 0x0101 */
    0xF0, 0x00,  /* reserved=4, ES_info_length = 0 */
    /* CRC32 placeholder (4 bytes) */
    0x00, 0x00, 0x00, 0x00
};

/* CRC32 table for MPEG-TS */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
    0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
    0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
    0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
    0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
    0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
    0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCE753D95,
    0xF63A8028, 0xF2FB9D9F, 0xFFB8BB46, 0xFB79A6F1, 0xE53EF6F4, 0xE1FFFB43, 0xECBCDD9A, 0xE87DC02D,
    0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003D15, 0x2AC120B2,
    0x128E7D0F, 0x164F60B8, 0x1B0C4661, 0x1FCD5BD6, 0x018A0BD1, 0x054B1666, 0x080830BF, 0x0CC92D08,
    0x7897AB85, 0x7C56B632, 0x711590EB, 0x75D48D5C, 0x6B93DD59, 0x6F52C0EE, 0x6211E637, 0x66D0FBA0,
    0x5A9FA61D, 0x5E5EBBAA, 0x531D9D73, 0x57DC80C4, 0x499BD0C1, 0x4D5ACD76, 0x4019EB4F, 0x44D8F6F8,
    0x6A536D55, 0x6E9270E2, 0x63D1563B, 0x67104B8C, 0x79571B89, 0x7D96063E, 0x70D520E7, 0x74143D50,
    0x485B40ED, 0x4C9A5D5A, 0x41D97B83, 0x45186634, 0x5B5F3631, 0x5F9E2B86, 0x52DD0D5F, 0x561C10E8,
    0x22429C25, 0x26838192, 0x2BC0A74B, 0x2F01BABE, 0x3146EA5B, 0x3587F7EC, 0x38C4D135, 0x3C05CC82,
    0x044A513F, 0x008B4C88, 0x0DC86A51, 0x090977E6, 0x174E27E3, 0x138F3A54, 0x1ECC1C8D, 0x1A0D013A,
    0xE854E0E7, 0xEC95FD50, 0xE1D6DB89, 0xE517C63E, 0xFB50963B, 0xFF918B8C, 0xF2D2AD55, 0xF613B0E2,
    0xCDDC8D5F, 0xC91D90E8, 0xC45EB631, 0xC09FAB86, 0xDED8FBC3, 0xDA19E674, 0xD75AC0AD, 0xD39BCD1A,
    0xB9C531A7, 0xBD042C10, 0xB0470AC9, 0xB486177E, 0xAAC1477B, 0xAE005ACA, 0xA3437C13, 0xA78261A4,
    0xD3DDE7C9, 0xD71CFB7E, 0xDA5FDAA7, 0xDE9EC010, 0xC0D99015, 0xC4188DA2, 0xC95BAB7B, 0xCF9AB6CC,
    0xF3D5CB71, 0xF714D6C6, 0xFA57F01F, 0xFE96EDA8, 0xE0D1BDA3, 0xE410A014, 0xE95386CD, 0xED929B7A,
    0x99CD17B7, 0x9D0C0A00, 0x904F2CD9, 0x948E316E, 0x8AC7616B, 0x8E067CDA, 0x83455A03, 0x878447B4,
    0xBFCB3A09, 0xBB0A27BE, 0xB6490167, 0xB2881CDA, 0xACCF4CDF, 0xA80E5168, 0xA54D77B1, 0xA18C6A06,
    0xE9C6C0AC, 0xED0DDB1B, 0xE04EFDC2, 0xE48FE075, 0xFAC9B070, 0xFE08ADA7, 0xFD4BAF3E, 0xF98AB289,
    0xC5C5CF34, 0xC104D283, 0xCC47F45A, 0xC886E9ED, 0xD6C1B9E8, 0xD200A45F, 0xDF438286, 0xDB829F31,
    0xA75A0A5E, 0xA39B17E9, 0xAED83130, 0xAE192C87, 0xB05E7C82, 0xB49F6135, 0xB9DC47EC, 0xBD1D5A5B,
    0x815227E6, 0x85933A51, 0x88D01C88, 0x8C11013F, 0x9256513A, 0x96974C8D, 0x9BD46A54, 0x9F1577E3,
    0x6D4CC7BB, 0x698DDAD0, 0x64CEFC09, 0x600FEF5E, 0x7E48BF5B, 0x7A89A2EC, 0x77CA8435, 0x730B9982,
    0x4F44E43F, 0x4B85F988, 0x46C6DF51, 0x4207C2E6, 0x5C4092E3, 0x58818F54, 0x55C2A98D, 0x5103B43A,
    0x255D38F7, 0x219C2540, 0x2CDF0399, 0x281E1E2A, 0x36594E2D, 0x3238539A, 0x3F7B7543, 0x3BBA68F4,
    0x07F51549, 0x033408FE, 0x0E772E27, 0x0AB63390, 0x14F16395, 0x10307E22, 0x1D7358FB, 0x19B2454C
};

static uint32_t crc32_mpeg2(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/* Create a single MPEG-TS packet (188 bytes) */
static void ts_create_packet(uint8_t *buf, uint16_t pid, uint8_t *cc,
                              int payload_unit_start, int adaptation_field,
                              const uint8_t *payload, size_t payload_len,
                              int pcr_flag, uint64_t pcr_value)
{
    /* Clear buffer with 0xFF (stuffing) */
    memset(buf, 0xFF, 188);

    /* TS Header (4 bytes) */
    buf[0] = 0x47;  /* Sync byte */
    buf[1] = (payload_unit_start ? 0x40 : 0x00) | ((pid >> 8) & 0x1F);
    buf[2] = pid & 0xFF;

    uint8_t adaptation_control = 0x10;  /* Payload only, no adaptation */
    if (adaptation_field) {
        adaptation_control |= 0x20;  /* Adaptation field present */
    }
    if (pcr_flag) {
        adaptation_control |= 0x20;  /* Adaptation field required for PCR */
    }
    buf[3] = adaptation_control | ((*cc) & 0x0F);
    (*cc) = ((*cc) + 1) & 0x0F;

    size_t offset = 4;

    if (adaptation_field || pcr_flag) {
        /* Adaptation field */
        size_t af_start = offset;
        buf[offset++] = 0;  /* adaptation_field_length (will update) */

        if (pcr_flag) {
            buf[af_start] = 7;  /* adaptation_field_length = 7 bytes (PCR only) */
            buf[offset++] = 0x10;  /* PCR_flag=1, others=0 */

            /* PCR: 33-bit base + 6 reserved + 9-bit extension */
            /* PCR base = 90kHz clock, PCR ext = 27MHz clock */
            uint64_t pcr_base = pcr_value / 300;  /* Convert us to 90kHz ticks */
            uint64_t pcr_ext = (pcr_value % 300) * 90;  /* Remainder in 27MHz ticks */

            buf[offset++] = (pcr_base >> 25) & 0xFF;
            buf[offset++] = (pcr_base >> 17) & 0xFF;
            buf[offset++] = (pcr_base >> 9) & 0xFF;
            buf[offset++] = (pcr_base >> 1) & 0xFF;
            buf[offset++] = ((pcr_base & 1) << 7) | 0x7E | ((pcr_ext >> 8) & 1);
            buf[offset++] = pcr_ext & 0xFF;
        } else {
            buf[af_start] = 0;  /* Empty adaptation field */
        }
    }

    /* Copy payload */
    size_t payload_remaining = 188 - offset;
    size_t copy_len = (payload_len < payload_remaining) ? payload_len : payload_remaining;
    if (payload && copy_len > 0) {
        memcpy(&buf[offset], payload, copy_len);
    }
    /* Rest is stuffing (0xFF) */
}

/* Build PAT packet */
static void build_pat_packet(uint8_t *buf, uint64_t pcr)
{
    uint8_t pat_with_crc[sizeof(s_pat_data)];
    memcpy(pat_with_crc, s_pat_data, sizeof(s_pat_data));

    /* Compute CRC32 over PAT data (excluding first 3 bytes: table_id, section_syntax, section_length) */
    uint32_t crc = crc32_mpeg2(&pat_with_crc[3], sizeof(s_pat_data) - 3 - 4);
    pat_with_crc[sizeof(s_pat_data) - 4] = (crc >> 24) & 0xFF;
    pat_with_crc[sizeof(s_pat_data) - 3] = (crc >> 16) & 0xFF;
    pat_with_crc[sizeof(s_pat_data) - 2] = (crc >> 8) & 0xFF;
    pat_with_crc[sizeof(s_pat_data) - 1] = crc & 0xFF;

    ts_create_packet(buf, PAT_PID, &s_cc_pat, 1, 1, pat_with_crc, sizeof(pat_with_crc), 0, 0);
}

/* Build PMT packet */
static void build_pmt_packet(uint8_t *buf, uint64_t pcr)
{
    uint8_t pmt_with_crc[sizeof(s_pmt_data_base)];
    memcpy(pmt_with_crc, s_pmt_data_base, sizeof(s_pmt_data_base));

    uint32_t crc = crc32_mpeg2(&pmt_with_crc[3], sizeof(s_pmt_data_base) - 3 - 4);
    pmt_with_crc[sizeof(s_pmt_data_base) - 4] = (crc >> 24) & 0xFF;
    pmt_with_crc[sizeof(s_pmt_data_base) - 3] = (crc >> 16) & 0xFF;
    pmt_with_crc[sizeof(s_pmt_data_base) - 2] = (crc >> 8) & 0xFF;
    pmt_with_crc[sizeof(s_pmt_data_base) - 1] = crc & 0xFF;

    ts_create_packet(buf, PMT_PID, &s_cc_pmt, 1, 1, pmt_with_crc, sizeof(pmt_with_crc), 0, 0);
}

/* Build PES packet for H.264 video */
static size_t build_video_pes(uint8_t *buf, const uint8_t *nalu_data, size_t nalu_len, uint64_t pts, uint64_t dts)
{
    /* PES header (19 bytes with PTS/DTS) */
    size_t pes_header_len = 19;
    size_t pes_packet_len = 6 + pes_header_len + nalu_len;  /* PES length field doesn't include first 6 bytes */

    if (pes_packet_len > 184) {
        /* Too large for single TS packet - would need fragmentation */
        /* For now, truncate */
        nalu_len = 184 - 6 - pes_header_len;
        pes_packet_len = 6 + pes_header_len + nalu_len;
    }

    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x01;
    buf[3] = 0xE0;  /* stream_id = video (0xE0) */
    buf[4] = (pes_packet_len - 6) >> 8;  /* PES_packet_length (excludes first 6 bytes) */
    buf[5] = (pes_packet_len - 6) & 0xFF;

    /* PES header flags */
    buf[6] = 0x80;  /* '10' + scrambling=0, priority=0, data_alignment=0, copyright=0, original=0 */
    buf[7] = 0xC0;  /* PTS_DTS_flags=10 (both), ESCR=0, ES_rate=0, DSM_trick=0, add_copy_info=0, PES_CRC=0, PES_extension=0 */
    buf[8] = pes_header_len - 3;  /* PES_header_data_length */

    /* PTS (33 bits) */
    uint64_t pts_90khz = pts / 300;  /* Convert us to 90kHz */
    buf[9]  = 0x21 | ((pts_90khz >> 29) & 0x0E) | 1;
    buf[10] = (pts_90khz >> 22) & 0xFF;
    buf[11] = ((pts_90khz >> 14) & 0xFE) | 1;
    buf[12] = (pts_90khz >> 7) & 0xFF;
    buf[13] = ((pts_90khz << 1) & 0xFE) | 1;

    /* DTS (33 bits) */
    uint64_t dts_90khz = dts / 300;
    buf[14] = 0x11 | ((dts_90khz >> 29) & 0x0E) | 1;
    buf[15] = (dts_90khz >> 22) & 0xFF;
    buf[16] = ((dts_90khz >> 14) & 0xFE) | 1;
    buf[17] = (dts_90khz >> 7) & 0xFF;
    buf[18] = ((dts_90khz << 1) & 0xFE) | 1;

    /* Copy NALU data */
    memcpy(&buf[19], nalu_data, nalu_len);

    return 19 + nalu_len;
}

/* Build PES packet for KLV data */
static size_t build_klv_pes(uint8_t *buf, const uint8_t *klv_data, size_t klv_len, uint64_t pts)
{
    size_t pes_header_len = 10;  /* PTS only */
    size_t pes_packet_len = 6 + pes_header_len + klv_len;

    if (pes_packet_len > 184) {
        klv_len = 184 - 6 - pes_header_len;
        pes_packet_len = 6 + pes_header_len + klv_len;
    }

    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x01;
    buf[3] = 0xBD;  /* stream_id = private_stream_1 (0xBD) */
    buf[4] = (pes_packet_len - 6) >> 8;
    buf[5] = (pes_packet_len - 6) & 0xFF;

    buf[6] = 0x80;  /* '10' + flags */
    buf[7] = 0x80;  /* PTS_DTS_flags=10 (PTS only), others=0 */
    buf[8] = pes_header_len - 3;

    /* PTS */
    uint64_t pts_90khz = pts / 300;
    buf[9]  = 0x21 | ((pts_90khz >> 29) & 0x0E) | 1;
    buf[10] = (pts_90khz >> 22) & 0xFF;
    buf[11] = ((pts_90khz >> 14) & 0xFE) | 1;
    buf[12] = (pts_90khz >> 7) & 0xFF;
    buf[13] = ((pts_90khz << 1) & 0xFE) | 1;

    memcpy(&buf[14], klv_data, klv_len);

    return 14 + klv_len;
}

/* Output TS packet as hex dump to serial for capture and analysis */
static void ts_packet_hexdump(const uint8_t *data, size_t len, const char *prefix)
{
    char hexbuf[2048];
    size_t max_bytes = (sizeof(hexbuf) - 1) / 3;
    size_t dump_len = (len < max_bytes) ? len : max_bytes;

    size_t pos = 0;
    for (size_t i = 0; i < dump_len; i++) {
        pos += snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "%02X ", data[i]);
    }
    if (len > max_bytes) {
        snprintf(&hexbuf[pos], sizeof(hexbuf) - pos, "...");
    }
    printf("%s: %s\n", prefix, hexbuf);
}

esp_err_t ts_muxer_init(void)
{
    ESP_LOGI(TAG, "MPEG-TS muxer initialized (PAT/PMT/PES, PID 0x%04X video, PID 0x%04X KLV)", VIDEO_PID, KLV_PID);
    s_cc_pat = 0;
    s_cc_pmt = 0;
    s_cc_video = 0;
    s_cc_klv = 0;
    memset(&s_ts_packet, 0, sizeof(s_ts_packet));
    return ESP_OK;
}

ts_packet_t* ts_muxer_mux(h264_nalu_t *nalu, klv_packet_t *klv)
{
    if (!nalu) {
        ESP_LOGW(TAG, "NULL nalu passed to muxer");
        return NULL;
    }

    memset(&s_ts_packet, 0, sizeof(s_ts_packet));
    memset(s_ts_buffer, 0xFF, sizeof(s_ts_buffer));

    size_t packet_count = 0;
    size_t total_len = 0;
    uint64_t pcr = nalu->timestamp_us;

    /* Packet 0: PAT (every ~100ms, for now every frame) */
    build_pat_packet(&s_ts_buffer[packet_count * 188], pcr);
    packet_count++;
    total_len += 188;

    /* Packet 1: PMT */
    build_pmt_packet(&s_ts_buffer[packet_count * 188], pcr);
    packet_count++;
    total_len += 188;

    /* Packet 2+: Video PES (may need multiple TS packets for large NALU) */
    uint8_t pes_buffer[188];
    size_t pes_len = build_video_pes(pes_buffer, nalu->data, nalu->len, nalu->timestamp_us, nalu->timestamp_us);

    /* Fragment PES into TS packets */
    size_t pes_offset = 0;
    bool first_video_packet = true;
    while (pes_offset < pes_len) {
        size_t chunk = (pes_len - pes_offset > 184) ? 184 : (pes_len - pes_offset);
        ts_create_packet(&s_ts_buffer[packet_count * 188], VIDEO_PID, &s_cc_video,
                         first_video_packet, 0, &pes_buffer[pes_offset], chunk,
                         first_video_packet, pcr);
        packet_count++;
        total_len += 188;
        pes_offset += chunk;
        first_video_packet = false;
    }

    /* KLV PES (if available) */
    if (klv && klv->data && klv->len > 0) {
        uint8_t klv_pes_buffer[188];
        size_t klv_pes_len = build_klv_pes(klv_pes_buffer, klv->data, klv->len, klv->timestamp_us);

        size_t klv_offset = 0;
        bool first_klv_packet = true;
        while (klv_offset < klv_pes_len) {
            size_t chunk = (klv_pes_len - klv_offset > 184) ? 184 : (klv_pes_len - klv_offset);
            ts_create_packet(&s_ts_buffer[packet_count * 188], KLV_PID, &s_cc_klv,
                             first_klv_packet, 0, &klv_pes_buffer[klv_offset], chunk,
                             false, 0);
            packet_count++;
            total_len += 188;
            klv_offset += chunk;
            first_klv_packet = false;
        }

        ESP_LOGI(TAG, "Muxed KLV: %zu bytes on PID 0x%04X", klv->len, KLV_PID);
    }

    /* Null packet for bitrate padding (optional) */
    if (packet_count < 8) {
        ts_create_packet(&s_ts_buffer[packet_count * 188], NULL_PID, &s_cc_video, 0, 0, NULL, 0, false, 0);
        packet_count++;
        total_len += 188;
    }

    s_ts_packet.data = s_ts_buffer;
    s_ts_packet.count = packet_count;
    s_ts_packet.total_len = total_len;
    s_ts_packet.timestamp_us = nalu->timestamp_us;

    /* Hex dump for analysis */
    ts_packet_hexdump(s_ts_buffer, total_len, "TS_OUT");

    ESP_LOGI(TAG, "Muxed %zu TS packets (%zu bytes total)", packet_count, total_len);
    return &s_ts_packet;
}

void ts_packet_free(ts_packet_t *ts)
{
    /* Static buffer, nothing to free */
    (void)ts;
}

esp_err_t ts_muxer_deinit(void)
{
    ESP_LOGI(TAG, "TS muxer deinitialized");
    s_cc_pat = 0;
    s_cc_pmt = 0;
    s_cc_video = 0;
    s_cc_klv = 0;
    return ESP_OK;
}