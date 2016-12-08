#ifndef H264_FFMPEG_H
#define H264_FFMPEG_H

#include "bitstream.h"
#include "h264_bs.h"
#include <stdint.h>
#include <stdbool.h>

#define H264_ARRAY_MAX 16
#define H264_LOOP_MAX 500
#define H264_EXTENDED_SAR 255

#ifdef __cplusplus
extern "C"{
#endif

size_t h264_get_x264_params(char * buffer, size_t len, struct h264_avcc_data * data );
size_t h264_get_ffmpeg_params(char * buffer, size_t len, struct h264_avcc_data * data );

#ifdef __cplusplus
}
#endif
#endif
