/*
 * JPEG Loader - Stub Implementation (SPIFFS load, no HW JPEG decode)
 */

#include <string.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "stanag_types.h"

static const char *TAG = "JPEG_LOAD";

#define NUM_FRAMES 10
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720

// Forward declarations
void jpeg_loader_free_yuv(yuv420_frame_t *yuv);

static jpeg_frame_t s_frames[NUM_FRAMES];
static int s_current_frame = 0;
static bool s_spiffs_mounted = false;
static bool s_frames_loaded = false;

esp_err_t jpeg_loader_init(void)
{
    ESP_LOGI(TAG, "Initializing JPEG loader (stub)...");

    // Mount SPIFFS
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
    ESP_LOGI(TAG, "SPIFFS mounted (stub - no actual frames)");
    return ESP_OK;
}

jpeg_frame_t* jpeg_loader_get_next(void)
{
    jpeg_frame_t *frame = &s_frames[s_current_frame];
    s_current_frame = (s_current_frame + 1) % NUM_FRAMES;
    
    // Return a dummy frame if not loaded
    if (!s_frames_loaded) {
        static uint8_t dummy_data[1024] = {0};
        frame->data = dummy_data;
        frame->len = sizeof(dummy_data);
        frame->width = FRAME_WIDTH;
        frame->height = FRAME_HEIGHT;
        frame->index = s_current_frame;
    }
    
    return frame;
}

yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame)
{
    (void)frame;
    
    // Allocate dummy YUV420 frame in PSRAM
    yuv420_frame_t *yuv = heap_caps_calloc(1, sizeof(yuv420_frame_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!yuv) {
        ESP_LOGE(TAG, "Failed to alloc YUV420 struct");
        return NULL;
    }

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

    ESP_LOGD(TAG, "Created dummy YUV420 frame (%dx%d)", FRAME_WIDTH, FRAME_HEIGHT);
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