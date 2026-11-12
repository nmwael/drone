/*
 * JPEG Loader - Loads JPEG frames from SPIFFS
 * For ESP32-S3 (no HW JPEG decode in v5.5.6), creates dummy YUV frames
 * Real JPEG files are still loaded from SPIFFS for pipeline testing
 */

#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "stanag_types.h"

static const char *TAG = "JPEG_LOAD";

#define NUM_FRAMES 5
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720

// Forward declarations
void jpeg_loader_free_yuv(yuv420_frame_t *yuv);
yuv420_frame_t* jpeg_loader_create_dummy_yuv(void);

static jpeg_frame_t s_frames[NUM_FRAMES];
static int s_current_frame = 0;
static bool s_spiffs_mounted = false;
static bool s_frames_loaded = false;

// YUV buffer for decoded frames
static uint8_t *s_yuv_buffer = NULL;
static size_t s_yuv_buffer_size = 0;

esp_err_t jpeg_loader_init(void)
{
    ESP_LOGI(TAG, "Initializing JPEG loader (SPIFFS + dummy YUV for ESP32-S3)...");

    // Mount SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Check SPIFFS info
    size_t total = 0, used = 0;
    ret = esp_spiffs_info("spiffs", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS info failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS: total=%d, used=%d, free=%d", total, used, total - used);
    }

    // Pre-allocate YUV buffer (1280x720 I420 = 1280*720*1.5 = 1,382,400 bytes)
    s_yuv_buffer_size = FRAME_WIDTH * FRAME_HEIGHT * 3 / 2;
    s_yuv_buffer = heap_caps_malloc(s_yuv_buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_yuv_buffer) {
        ESP_LOGW(TAG, "Failed to alloc YUV buffer in PSRAM, trying internal RAM");
        s_yuv_buffer = heap_caps_malloc(s_yuv_buffer_size, MALLOC_CAP_8BIT);
    }
    if (!s_yuv_buffer) {
        ESP_LOGE(TAG, "Failed to alloc YUV buffer");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "YUV buffer allocated: %d bytes", s_yuv_buffer_size);

    s_spiffs_mounted = true;
    ESP_LOGI(TAG, "JPEG loader initialized (using dummy YUV for ESP32-S3)");
    return ESP_OK;
}

jpeg_frame_t* jpeg_loader_get_next(void)
{
    if (!s_spiffs_mounted) {
        ESP_LOGE(TAG, "SPIFFS not mounted");
        return NULL;
    }

    jpeg_frame_t *frame = &s_frames[s_current_frame];
    s_current_frame = (s_current_frame + 1) % NUM_FRAMES;

    // Load JPEG file from SPIFFS
    char filename[64];
    snprintf(filename, sizeof(filename), "/spiffs/frame_%d.jpg", s_current_frame + 1);

    FILE *f = fopen(filename, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", filename);
        return NULL;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0 || fsize > 1024*1024) {
        ESP_LOGE(TAG, "Invalid JPEG file size: %ld", fsize);
        fclose(f);
        return NULL;
    }

    // Allocate frame data buffer
    if (frame->data) {
        free(frame->data);
    }
    frame->data = malloc(fsize);
    if (!frame->data) {
        ESP_LOGE(TAG, "Failed to alloc frame data (%ld bytes)", fsize);
        fclose(f);
        return NULL;
    }

    // Read JPEG data
    size_t read = fread(frame->data, 1, fsize, f);
    fclose(f);

    if (read != (size_t)fsize) {
        ESP_LOGE(TAG, "Failed to read full JPEG: %zu/%ld", read, fsize);
        free(frame->data);
        frame->data = NULL;
        return NULL;
    }

    frame->len = fsize;
    frame->width = FRAME_WIDTH;
    frame->height = FRAME_HEIGHT;
    frame->index = s_current_frame;

    ESP_LOGI(TAG, "Loaded JPEG frame %d: %zu bytes from %s", frame->index + 1, frame->len, filename);
    return frame;
}

yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame)
{
    if (!frame || !frame->data || frame->len == 0) {
        ESP_LOGE(TAG, "Invalid frame");
        return NULL;
    }

    // For ESP32-S3 (no HW JPEG decode in v5.5.6), create dummy YUV
    // In production, this would use HW JPEG decoder on supported chips
    ESP_LOGW(TAG, "ESP32-S3: No HW JPEG decode in v5.5.6, using dummy YUV (frame %d, %zu bytes JPEG)", 
             frame->index + 1, frame->len);
    return jpeg_loader_create_dummy_yuv();
}

// Create dummy YUV frame for testing when JPEG decode is not available
yuv420_frame_t* jpeg_loader_create_dummy_yuv(void)
{
    static yuv420_frame_t yuv;
    yuv.y = s_yuv_buffer;
    yuv.u = s_yuv_buffer + FRAME_WIDTH * FRAME_HEIGHT;
    yuv.v = s_yuv_buffer + FRAME_WIDTH * FRAME_HEIGHT + (FRAME_WIDTH * FRAME_HEIGHT / 4);
    yuv.width = FRAME_WIDTH;
    yuv.height = FRAME_HEIGHT;
    yuv.stride_y = FRAME_WIDTH;
    yuv.stride_uv = FRAME_WIDTH / 2;
    yuv.timestamp_us = esp_timer_get_time();

    // Fill with test pattern (Y=gradient, U=128, V=128)
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
        yuv.y[i] = (i % 256);
    }
    for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT / 4; i++) {
        yuv.u[i] = 128;
        yuv.v[i] = 128;
    }

    ESP_LOGD(TAG, "Created dummy YUV frame (%dx%d)", FRAME_WIDTH, FRAME_HEIGHT);
    return &yuv;
}

void jpeg_loader_free_yuv(yuv420_frame_t *yuv)
{
    // Static buffer, nothing to free
    (void)yuv;
}