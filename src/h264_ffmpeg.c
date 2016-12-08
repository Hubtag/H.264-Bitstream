#include "h264_bs.h"
#include "h264_ffmpeg.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _cplusplus
extern "C"{
#endif


static const char * get_color_prim_name( size_t color_prim ){
    switch( color_prim ){
        case 1:  return "bt709";
        case 2:  return "undef";
        case 4:  return "bt470m";
        case 5:  return "bt470bg";
        case 6:  return "smpte170m";
        case 7:  return "smpte240m";
        case 8:  return "film";
        case 9:  return "bt2020";
        case 10: return "smpte428";
        case 11: return "smpte431";
        case 12: return "smpte432";
        default: return "unknown";
    }
}
static const char * get_color_trans_name( size_t color_trans ){
    switch( color_trans ){
        case 1:  return "bt709";
        case 2:  return "undef";
        case 4:  return "bt470m";
        case 5:  return "bt470bg";
        case 6:  return "smpte170m";
        case 7:  return "smpte240m";
        case 8:  return "linear";
        case 9:  return "log100";
        case 10: return "log316";
        case 11: return "iec61966-2-4";
        case 12: return "bt1361e";
        case 13: return "iec61966-2-1";
        case 14: return "bt2020-10";
        case 15: return "bt2020-12";
        case 16: return "smpte2084";
        case 17: return "smpte428";
        default: return "unknown";
    }
}


static const char * get_color_matrix_name( size_t color_matrix ){
    switch( color_matrix ){
        case 0:  return "GBR";
        case 1:  return "bt709";
        case 2:  return "undef";
        case 4:  return "fcc";
        case 5:  return "bt470bg";
        case 6:  return "smpte170m";
        case 7:  return "smpte240m";
        case 8:  return "YCgCo";
        case 9:  return "bt2020nc";
        case 10: return "bt2020c";
        case 11: return "smpte2085";
        default: return "unknown";
    }
}
static const char * get_video_fmt_name( size_t fmt ){
    switch( fmt ){
        case 0:  return "component";
        case 1:  return "pal";
        case 2:  return "ntsc";
        case 3:  return "secam";
        case 4:  return "mac";
        case 5:  return "undef";
        default: return "unknown";
    }
}



static const char * get_profile_name(size_t profile){
    switch(profile){
        case 66:    return "baseline";
        case 77:    return "main";
        case 100:   return "high";
        case 110:   return "high10";
        case 122:   return "high422";
        case 244:   return "high444";
        case 88:    return "extended";
        case 44:    return "intra";
        case 83:    return "scalable_baseline";
        case 86:    return "scalable_high";
        case 128:   return "stereo_high";
        case 118:   return "multiview_high";
        case 138:   return "multiview_depth_high";
        default:    return "unknown";
    }
}

static const char * get_pix_name(struct h264_seq_parameter_set * seq){
    if( seq->profile_idc == 122 && seq->chroma_format_idc == 2 ){
        if( seq->vui.video_full_range ){
            return "yuvj422p";
        }
        return "yuv422p";
    }
    if( seq->profile_idc == 244 ){
        return "yuv424p";
    }
    if( seq->vui.video_full_range ){
        return "yuvj420p";
    }
    return "yuv420p";
}


static bool get_profile_support(size_t profile){
    switch(profile){
        case 66: case 77: case 100: case 110:
        case 122: case 244:
            return true;
        default:
            return false;
    }
}

static size_t write_fmt( char * buffer, size_t len, size_t offset, const char * format, ... ){
    va_list args;
    va_start(args, format);
    size_t write_amt = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if( buffer && offset < len ){
        va_start(args, format);
        vsnprintf(buffer + offset, len - offset, format, args);
        va_end(args);
    }
    return write_amt;
}

size_t h264_get_ffmpeg_params(char * buffer, size_t len, struct h264_avcc_data * data ){
    if( data->pps_count == 0 || data->sps_count == 0 ){
        return 0;
    }
    struct h264_seq_parameter_set * seq = data->sps;

    size_t out_len = 0;
    out_len += write_fmt( buffer, len, out_len, "-pix_fmt %s ", get_pix_name( seq ) );
    out_len += write_fmt( buffer, len, out_len, "-profile:v %s ", get_profile_name( seq->profile_idc ) );
    out_len += write_fmt( buffer, len, out_len, "-level %d.%d ", seq->level_idc / 10, seq->level_idc % 10 );
    out_len += write_fmt( buffer, len, out_len, "-x264-params \"" );
    if( out_len < len ){
        out_len += h264_get_x264_params( buffer + out_len, len - out_len, data );
    }
    else{
        out_len += h264_get_x264_params( NULL, 0, data );
    }
    out_len += write_fmt( buffer, len, out_len, "\"" );
    return out_len;
}

size_t h264_get_x264_params(char * buffer, size_t len, struct h264_avcc_data * data ){
    struct h264_pic_parameter_set * pic = data->pps;
    struct h264_seq_parameter_set * seq = data->sps;
    size_t out_len = 0;
    out_len += write_fmt( buffer, len, out_len, "chroma-qp-offset=%d:", pic->chroma_qp_index_offset + 2 );
    out_len += write_fmt( buffer, len, out_len, "constrained-intra=%d:", pic->constrained_intra_pred );
    out_len += write_fmt( buffer, len, out_len, "cabac=%d:", pic->entropy_coding_mode );
    if( seq->frame_cropping_flag ){
        out_len += write_fmt( buffer, len, out_len, "crop-rect=%d,%d,%d,%d:",
                        seq->frame_crop_left_offset / 2,
                        seq->frame_crop_top_offset / 2,
                        seq->frame_crop_right_offset / 2,
                        seq->frame_crop_bottom_offset / 2 );
    }
    if( seq->frame_mbs_only ){
        out_len += write_fmt( buffer, len, out_len, "fake-interlaced=1:" );
    }
    out_len += write_fmt( buffer, len, out_len, "level=%d.%d:", seq->level_idc / 10, seq->level_idc % 10 );
    if( seq->log2_max_frame_num_minus4 == 3 ){
        out_len += write_fmt( buffer, len, out_len, "intra-refresh=1:" );
    }
    out_len += write_fmt( buffer, len, out_len, "crf=%d:", pic->pic_init_qp_minus26 + 26 );
    //out_len += write_fmt( buffer, len, out_len, "qp=%d:", pic->pic_init_qp_minus26 + 26 );
    if( pic->pic_scaling_matrix_present_flag ){
        out_len += write_fmt( buffer, len, out_len, "cqm=jvt:" );
    }
    if( get_profile_support( seq->profile_idc ) ){
        out_len += write_fmt( buffer, len, out_len, "profile=%s:", get_profile_name( seq->profile_idc ) );
    }
    else{
        fprintf(stderr, "Incompatible profile \"%s\"", get_profile_name( seq->profile_idc ) );
        return 0;
    }
    out_len += write_fmt( buffer, len, out_len, "8x8dct=%d:", pic->transform_8x8_mode_flag );
    if( seq->vui.chroma_loc_info_present ){
        out_len += write_fmt( buffer, len, out_len, "chromaloc=%d:", seq->vui.chroma_sample_loc_type_top_field );
    }
    if( seq->vui.color_description_present ){
        out_len += write_fmt( buffer, len, out_len, "colormatrix=%s:", get_color_matrix_name( seq->vui.matrix_coefficients ) );
        out_len += write_fmt( buffer, len, out_len, "colorprim=%s:", get_color_prim_name( seq->vui.color_primaries ) );
        out_len += write_fmt( buffer, len, out_len, "transfer=%s:", get_color_trans_name( seq->vui.transfer_characteristics ) );
    }
    out_len += write_fmt( buffer, len, out_len, "force-cfr=%d:", seq->vui.fixed_frame_rate );
    if( seq->vui.overscan_appropriate ){
        out_len += write_fmt( buffer, len, out_len, "overscan=crop:" );
    }
    else{
        out_len += write_fmt( buffer, len, out_len, "overscan=show:" );
    }
    out_len += write_fmt( buffer, len, out_len, "videoformat=%s:", get_video_fmt_name( seq->vui.video_format ) );
    out_len += write_fmt( buffer, len, out_len, "no-weightb=%d:", !pic->weighted_bipred_idc );
    out_len += write_fmt( buffer, len, out_len, "weightp=%d:", pic->weighted_pred_flag );
    return out_len;
}


#ifdef _cplusplus
}
#endif
