/*
 * JPEG Loader - SPIFFS load + HW JPEG decode to YUV420
 */

#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_jpeg.h"
#include "esp_heap_caps.h"
#include "stanag_types.h"

static const char *TAG = "JPEG_LOAD";

#define NUM_FRAMES 10
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720
#define YUV420_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3 / 2)

static jpeg_frame_t s_frames[NUM_FRAMES];
static int s_current_frame = 0;
static bool s_spiffs_mounted = false;

esp_err_t jpeg_loader_init(void)
{
    ESP_LOGI(TAG, "Mounting SPIFFS...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_spiffs_mounted = true;

    // Load frame metadata
    for (int i = 0; i < NUM_FRAMES; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/spiffs/frame_%d.jpg", i);

        FILE *f = fopen(path, "rb");
        if (!f) {
            ESP_LOGE(TAG, "Missing frame: %s", path);
            return ESP_ERR_NOT_FOUND;
        }

        fseek(f, 0, SEEK_END);
        size_t len = ftell(f);
        fseek(f, 0, SEEK_SET);

        s_frames[i].data = heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!s_frames[i].data) {
            fclose(f);
            ESP_LOGE(TAG, "Failed to alloc %zu bytes for frame %d", len, i);
            return ESP_ERR_NO_MEM;
        }

        fread(s_frames[i].data, 1, len, f);
        fclose(f);

        s_frames[i].len = len;
        s_frames[i].width = FRAME_WIDTH;
        s_frames[i].height = FRAME_HEIGHT;
        s_frames[i].index = i;

        ESP_LOGI(TAG, "Loaded frame %d: %zu bytes", i, len);
    }

    ESP_LOGI(TAG, "All %d frames loaded into PSRAM", NUM_FRAMES);
    return ESP_OK;
}

jpeg_frame_t* jpeg_loader_get_next(void)
{
    jpeg_frame_t *frame = &s_frames[s_current_frame];
    s_current_frame = (s_current_frame + 1) % NUM_FRAMES;
    return frame;
}

yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame)
{
    // Allocate YUV420 buffer in PSRAM
    yuv420_frame_t *yuv = heap_caps_calloc(1, sizeof(yuv420_frame_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!yuv) {
        ESP_LOGE(TAG, "Failed to alloc YUV420 struct");
        return NULL;
    }

    size_t yuv_size = FRAME_WIDTH * FRAME_HEIGHT * 3 / 2;
    yuv->y = heap_caps_malloc(FRAME_WIDTH * FRAME_HEIGHT, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    yuv->u = heap_caps_malloc(FRAME_WIDTH * FRAME_HEIGHT / 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    yuv->v = heap_caps_malloc(FRAME_WIDTH * FRAME_HEIGHT / 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!yuv->y || !yuv->u || !yuv->v) {
        ESP_LOGE(TAG, "Failed to alloc YUV planes");
        jpeg_loader_free_yuv(yuv);
        return NULL;
    }

    yuv->width = FRAME_WIDTH;
    yuv->height = FRAME_HEIGHT;
    yuv->stride_y = FRAME_WIDTH;
    yuv->stride_uv = FRAME_WIDTH / 2;
    yuv->timestamp_us = esp_timer_get_time();

    // Configure HW JPEG decoder
    esp_jpeg_decoder_cfg_t dec_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_YUV420,
        .output_buffer = {
            .data = yuv->y,
            .size = yuv_size
        },
        .upscale_output = false,
        .downscale_output = false
    };

    esp_jpeg_image_t src = {
        .data = frame->data,
        .len = frame->len,
        .width = FRAME_WIDTH,
        .height = FRAME_HEIGHT
    };

    esp_jpeg_decoder_handle_t decoder;
    esp_err_t ret = esp_jpeg_decoder_create(&dec_cfg, &decoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG decoder create failed: %s", esp_err_to_name(ret));
        jpeg_loader_free_yuv(yuv);
        return NULL;
    }

    ret = esp_jpeg_decoder_process(decoder, &src, &dec_cfg);
    esp_jpeg_decoder_destroy(decoder);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(ret));
        jpeg_loader_free_yuv(yuv);
        return NULL;
    }

    // Note: In real implementation, decoder outputs to separate Y/U/V planes
    // This is simplified - actual ESP JPEG decoder outputs planar YUV420
    ESP_LOGD(TAG, "Decoded frame %d to YUV420 (%dx%d)", frame->index, FRAME_WIDTH, FRAME_HEIGHT);

    return yuv;
}

void jpeg_loader_free_yuv(yuv420_frame_t *yuv)
{
    if (yuv) {
        free(yuv->y);
        free(yuv->u);
        free(yuv->v);
        free(yuv);
    }
}