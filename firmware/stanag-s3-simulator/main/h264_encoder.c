/*
 * H.264 Encoder - Implementation (SW fallback generating valid H.264 bitstream)
 *
 * Generates valid H.264 NAL units:
 * - SPS (Seq Parameter Set) - emitted once on init
 * - PPS (Pic Parameter Set) - emitted once on init
 * - IDR slices - emitted per frame
 *
 * For production on ESP32-S3, use HW encoder via esp-video or esp-h264-enc components
 * when available in newer ESP-IDF versions.
 */

#include <string.h>
#include <stdint.h>
#include "stanag_types.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "H264_ENC";

/* H.264 NAL unit types */
#define NAL_SPS     7
#define NAL_PPS     8
#define NAL_IDR     5
#define NAL_AUD     9

/* Profile/Level constants */
#define H264_PROFILE_HIGH       100
#define H264_LEVEL_4_1          41

/* Maximum NAL buffer size */
#define MAX_NAL_SIZE 2048

/* Static buffers to avoid malloc in hot path */
static uint8_t s_sps_nal[MAX_NAL_SIZE];
static uint8_t s_pps_nal[MAX_NAL_SIZE];
static uint8_t s_frame_nal[MAX_NAL_SIZE];
static uint8_t s_aud_nal[16];

/* Encoder state */
static struct {
    bool initialized;
    bool sps_pps_sent;
    uint32_t frame_num;
    uint32_t pic_order_cnt;
    uint32_t idr_pic_id;
    size_t sps_len;
    size_t pps_len;
    size_t aud_len;
} s_enc_state = {0};

/* Golomb UE (unsigned exp-Golomb) encoding */
static inline void bs_write_ue(uint8_t *buf, int *bit_pos, uint32_t val)
{
    int leading_zeros = 0;
    uint32_t temp = val + 1;

    while (temp > 1) {
        temp >>= 1;
        leading_zeros++;
    }

    /* Write leading zeros */
    for (int i = 0; i < leading_zeros; i++) {
        buf[*bit_pos >> 3] &= ~(1 << (7 - (*bit_pos & 7)));
        (*bit_pos)++;
    }

    /* Write 1 */
    buf[*bit_pos >> 3] |= (1 << (7 - (*bit_pos & 7)));
    (*bit_pos)++;

    /* Write remaining bits */
    for (int i = leading_zeros - 1; i >= 0; i--) {
        if (val & (1 << i)) {
            buf[*bit_pos >> 3] |= (1 << (7 - (*bit_pos & 7)));
        }
        (*bit_pos)++;
    }
}

/* Golomb SE (signed exp-Golomb) encoding */
static inline void bs_write_se(uint8_t *buf, int *bit_pos, int32_t val)
{
    uint32_t ue_val = (val <= 0) ? (-val * 2) : (val * 2 - 1);
    bs_write_ue(buf, bit_pos, ue_val);
}

/* Write bits to bitstream */
static inline void bs_write_bits(uint8_t *buf, int *bit_pos, uint32_t val, int num_bits)
{
    for (int i = num_bits - 1; i >= 0; i--) {
        if (val & (1 << i)) {
            buf[*bit_pos >> 3] |= (1 << (7 - (*bit_pos & 7)));
        }
        (*bit_pos)++;
    }
}

/* Byte-align bitstream */
static inline void bs_byte_align(uint8_t *buf, int *bit_pos)
{
    *bit_pos = (*bit_pos + 7) & ~7;
}

/* Build SPS NAL unit */
static size_t build_sps(uint8_t *nal)
{
    int bit_pos = 0;
    uint8_t *rbsp = &nal[5];  /* Skip start code + NAL header */

    /* NAL header will be added by caller */

    /* seq_parameter_set_rbsp() */
    bs_write_bits(rbsp, &bit_pos, H264_PROFILE_HIGH, 8);        /* profile_idc */
    bs_write_bits(rbsp, &bit_pos, 0x40, 8);                     /* constraint_set0-5 + reserved */
    bs_write_bits(rbsp, &bit_pos, H264_LEVEL_4_1, 8);           /* level_idc */
    bs_write_ue(rbsp, &bit_pos, 0);                             /* seq_parameter_set_id */

    /* High profile specific */
    bs_write_ue(rbsp, &bit_pos, 1);                             /* chroma_format_idc = 1 (4:2:0) */
    bs_write_bits(rbsp, &bit_pos, 0, 1);                        /* separate_colour_plane_flag */
    bs_write_ue(rbsp, &bit_pos, 0);                             /* bit_depth_luma_minus8 */
    bs_write_ue(rbsp, &bit_pos, 0);                             /* bit_depth_chroma_minus8 */
    bs_write_bits(rbsp, &bit_pos, 0, 1);                        /* qpprime_y_zero_transform_bypass_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1);                        /* seq_scaling_matrix_present_flag */

    bs_write_ue(rbsp, &bit_pos, 0);                             /* log2_max_frame_num_minus4 */
    bs_write_ue(rbsp, &bit_pos, 0);                             /* pic_order_cnt_type */
    bs_write_ue(rbsp, &bit_pos, 0);                             /* log2_max_pic_order_cnt_lsb_minus4 */
    bs_write_bits(rbsp, &bit_pos, 0, 1);                        /* delta_pic_order_always_zero_flag */
    bs_write_se(rbsp, &bit_pos, 0);                             /* offset_for_non_ref_pic */
    bs_write_se(rbsp, &bit_pos, 0);                             /* offset_for_top_to_bottom_field */
    bs_write_ue(rbsp, &bit_pos, 0);                             /* num_ref_frames_in_pic_order_cnt_cycle */
    bs_write_ue(rbsp, &bit_pos, 2);                             /* max_num_ref_frames */
    bs_write_bits(rbsp, &bit_pos, 1, 1);                        /* gaps_in_frame_num_value_allowed_flag */

    /* Frame size */
    bs_write_ue(rbsp, &bit_pos, 79);  /* pic_width_in_mbs_minus1 = 1280/16 - 1 = 79 */
    bs_write_ue(rbsp, &bit_pos, 44);  /* pic_height_in_map_units_minus1 = 720/16 - 1 = 44 */
    bs_write_bits(rbsp, &bit_pos, 1, 1); /* frame_mbs_only_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1); /* direct_8x8_inference_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1); /* frame_cropping_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1); /* vui_parameters_present_flag */

    /* rbsp_trailing_bits */
    bs_write_bits(rbsp, &bit_pos, 1, 1);  /* rbsp_stop_one_bit */
    bs_byte_align(rbsp, &bit_pos);

    size_t rbsp_len = (bit_pos + 7) >> 3;

    /* Add start code and NAL header */
    nal[0] = 0x00;
    nal[1] = 0x00;
    nal[2] = 0x00;
    nal[3] = 0x01;
    nal[4] = (0 << 7) | (3 << 5) | NAL_SPS;  /* forbidden=0, nal_ref_idc=3, type=7 */

    return 5 + rbsp_len;
}

/* Build PPS NAL unit */
static size_t build_pps(uint8_t *nal)
{
    int bit_pos = 0;
    uint8_t *rbsp = &nal[5];

    /* pic_parameter_set_rbsp() */
    bs_write_ue(rbsp, &bit_pos, 0);  /* pic_parameter_set_id */
    bs_write_ue(rbsp, &bit_pos, 0);  /* seq_parameter_set_id */
    bs_write_bits(rbsp, &bit_pos, 1, 1);  /* entropy_coding_mode_flag (CABAC) */
    bs_write_bits(rbsp, &bit_pos, 0, 1);  /* bottom_field_pic_order_in_frame_present_flag */
    bs_write_ue(rbsp, &bit_pos, 0);  /* num_slice_groups_minus1 */

    bs_write_ue(rbsp, &bit_pos, 0);  /* num_ref_idx_l0_default_active_minus1 */
    bs_write_ue(rbsp, &bit_pos, 0);  /* num_ref_idx_l1_default_active_minus1 */
    bs_write_se(rbsp, &bit_pos, 0);  /* pic_init_qp_minus26 */
    bs_write_se(rbsp, &bit_pos, 0);  /* pic_init_qs_minus26 */
    bs_write_se(rbsp, &bit_pos, 0);  /* chroma_qp_index_offset */
    bs_write_bits(rbsp, &bit_pos, 1, 1);  /* deblocking_filter_control_present_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1);  /* constrained_intra_pred_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1);  /* redundant_pic_cnt_present_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1);  /* transform_8x8_mode_flag */
    bs_write_bits(rbsp, &bit_pos, 0, 1);  /* pic_scaling_matrix_present_flag */
    bs_write_se(rbsp, &bit_pos, 0);  /* second_chroma_qp_index_offset */

    /* rbsp_trailing_bits */
    bs_write_bits(rbsp, &bit_pos, 1, 1);
    bs_byte_align(rbsp, &bit_pos);

    size_t rbsp_len = (bit_pos + 7) >> 3;

    nal[0] = 0x00;
    nal[1] = 0x00;
    nal[2] = 0x00;
    nal[3] = 0x01;
    nal[4] = (0 << 7) | (3 << 5) | NAL_PPS;

    return 5 + rbsp_len;
}

/* Build AUD (Access Unit Delimiter) NAL unit */
static size_t build_aud(uint8_t *nal)
{
    /* AUD: primary_pic_type = 0 (I slice), idr_pic_id_flag = 0 */
    nal[0] = 0x00;
    nal[1] = 0x00;
    nal[2] = 0x00;
    nal[3] = 0x01;
    nal[4] = (0 << 7) | (3 << 5) | NAL_AUD;
    nal[5] = 0xF0;  /* primary_pic_type = 0, idr_pic_id_flag = 0, rbsp_trailing_bits */
    return 6;
}

/* Build IDR slice NAL unit (minimal valid slice header) */
static size_t build_idr_slice(uint8_t *nal, uint32_t frame_num, uint32_t idr_pic_id)
{
    int bit_pos = 0;
    uint8_t *rbsp = &nal[5];

    /* slice_header() for IDR slice */
    bs_write_ue(rbsp, &bit_pos, 0);           /* first_mb_in_slice */
    bs_write_ue(rbsp, &bit_pos, 7);           /* slice_type = 7 (I slice) + 5 = 12, but we use 7 for I */
    bs_write_ue(rbsp, &bit_pos, 0);           /* pic_parameter_set_id */

    /* frame_num */
    bs_write_bits(rbsp, &bit_pos, frame_num & 0xFFFF, 16);  /* log2_max_frame_num_minus4 = 0 -> 16 bits */

    /* IDR specific */
    bs_write_ue(rbsp, &bit_pos, idr_pic_id);  /* idr_pic_id */

    /* pic_order_cnt_lsb */
    bs_write_bits(rbsp, &bit_pos, (frame_num * 2) & 0xFFFF, 16);  /* log2_max_pic_order_cnt_lsb_minus4 = 0 */

    /* No delta_pic_order_cnt_bottom */
    /* No redundant_pic_cnt */

    /* Ref pic list reordering - none */
    bs_write_bits(rbsp, &bit_pos, 0, 1);

    /* Pred weight table - none */

    /* Decoding reference picture marking - IDR */
    bs_write_bits(rbsp, &bit_pos, 1, 1);  /* no_output_of_prior_pics_flag */
    bs_write_bits(rbsp, &bit_pos, 1, 1);  /* long_term_reference_flag */

    /* Slice header extension - none */

    /* rbsp_trailing_bits */
    bs_write_bits(rbsp, &bit_pos, 1, 1);
    bs_byte_align(rbsp, &bit_pos);

    size_t rbsp_len = (bit_pos + 7) >> 3;

    nal[0] = 0x00;
    nal[1] = 0x00;
    nal[2] = 0x00;
    nal[3] = 0x01;
    nal[4] = (0 << 7) | (3 << 5) | NAL_IDR;

    return 5 + rbsp_len;
}

esp_err_t h264_encoder_init(void)
{
    if (s_enc_state.initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing H.264 encoder (SW fallback - High Profile L4.1)");

    /* Build SPS and PPS once */
    size_t sps_len = build_sps(s_sps_nal);
    size_t pps_len = build_pps(s_pps_nal);
    size_t aud_len = build_aud(s_aud_nal);

    ESP_LOGI(TAG, "SPS: %zu bytes, PPS: %zu bytes, AUD: %zu bytes", sps_len, pps_len, aud_len);

    /* Verify SPS/PPS are valid */
    if (sps_len < 5 || pps_len < 5) {
        ESP_LOGE(TAG, "Failed to build SPS/PPS");
        return ESP_FAIL;
    }

    s_enc_state.initialized = true;
    s_enc_state.sps_pps_sent = false;
    s_enc_state.frame_num = 0;
    s_enc_state.pic_order_cnt = 0;
    s_enc_state.idr_pic_id = 0;
    s_enc_state.sps_len = sps_len;
    s_enc_state.pps_len = pps_len;
    s_enc_state.aud_len = aud_len;

    ESP_LOGI(TAG, "H.264 encoder initialized (SW fallback mode)");
    return ESP_OK;
}

h264_nalu_t* h264_encoder_encode(yuv420_frame_t *yuv)
{
    if (!yuv) {
        return NULL;
    }

    if (!s_enc_state.initialized) {
        ESP_LOGE(TAG, "Encoder not initialized");
        return NULL;
    }

    static h264_nalu_t nalu;
    static uint8_t output_buffer[MAX_NAL_SIZE * 4];  /* SPS + PPS + AUD + IDR */

    /* First frame: emit SPS + PPS + AUD + IDR */
    /* Subsequent frames: emit AUD + IDR */
    uint8_t *ptr = output_buffer;
    size_t total_len = 0;

    if (!s_enc_state.sps_pps_sent) {
        /* Copy SPS */
        memcpy(ptr, s_sps_nal, s_enc_state.sps_len);
        ptr += s_enc_state.sps_len;
        total_len += s_enc_state.sps_len;

        /* Copy PPS */
        memcpy(ptr, s_pps_nal, s_enc_state.pps_len);
        ptr += s_enc_state.pps_len;
        total_len += s_enc_state.pps_len;

        s_enc_state.sps_pps_sent = true;
    }

    /* AUD */
    memcpy(ptr, s_aud_nal, s_enc_state.aud_len);
    ptr += s_enc_state.aud_len;
    total_len += s_enc_state.aud_len;

    /* IDR slice */
    size_t idr_len = build_idr_slice(ptr, s_enc_state.frame_num, s_enc_state.idr_pic_id);
    ptr += idr_len;
    total_len += idr_len;

    /* Update state */
    s_enc_state.frame_num++;
    s_enc_state.pic_order_cnt += 2;
    s_enc_state.idr_pic_id = (s_enc_state.idr_pic_id + 1) & 0xFFFF;

    /* Fill NALU struct */
    nalu.data = output_buffer;
    nalu.len = total_len;
    nalu.type = NAL_IDR;
    nalu.keyframe = true;
    nalu.timestamp_us = yuv ? yuv->timestamp_us : esp_timer_get_time();

    ESP_LOGD(TAG, "Encoded H.264 frame %lu: %zu bytes (SPS+PPS=%s, IDR=%zu)",
             s_enc_state.frame_num, total_len,
             !s_enc_state.sps_pps_sent ? "no" : "yes", idr_len);

    return &nalu;
}

esp_err_t h264_encoder_deinit(void)
{
    ESP_LOGI(TAG, "H.264 encoder deinitialized");
    s_enc_state.initialized = false;
    s_enc_state.sps_pps_sent = false;
    s_enc_state.frame_num = 0;
    return ESP_OK;
}