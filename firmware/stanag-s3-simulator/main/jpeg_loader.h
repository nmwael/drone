/*
 * JPEG Loader - SPIFFS load + HW JPEG decode to YUV420
 */

#ifndef JPEG_LOADER_H
#define JPEG_LOADER_H

#include "stanag_types.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t jpeg_loader_init(void);
jpeg_frame_t* jpeg_loader_get_next(void);
yuv420_frame_t* jpeg_loader_decode(jpeg_frame_t *frame);
void jpeg_loader_free_yuv(yuv420_frame_t *yuv);
esp_err_t jpeg_loader_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // JPEG_LOADER_H