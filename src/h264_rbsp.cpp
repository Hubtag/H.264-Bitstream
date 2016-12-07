#include "h264_rbsp.h"
#include <math.h>
#include <stdlib.h>

#ifdef _cplusplus
extern "C"{
#endif

int32_t map_se(uint32_t in){
    // 0 => 0
    // 1 => 1
    // 2 => -1
    // 3 => 2
    // 4 => -2
    // k => k/2 + k%2 * (k%2 == 0 ? -1 : 1)
    int remainder = in % 2;
    int32_t value = in / 2 + remainder;
    remainder = ( remainder << 1 ) - 1;
    // 0 << 1 is 0. 0 - 1 is -1.
    // 1 << 1 is 2. 2 - 1 is 1.
    return value * remainder;
}

int nal_to_rbsp( char * data, size_t length ){
    size_t in, out, count;
    in = out = count = 0;
    // A NAL unit may not contain the byte sequences
    // 00 00 00, 00 00 01, or 00 00 02.
    // The byte sequence 00 00 03 is converted to 00 00.
    while( in < length ){
        if( count == 2 ){
            // We have two leading null bytes
            if( data[in] < 3 ){
                // Invalid sequence
                return -1;
            }
            else if( data[in] == 3 && in + 1 < length ){
                ++in;
                if( data[in] > 3){
                    // Invalid sequence
                    return -1;
                }
            }
            count = 0;
        }
        if( data[in] == 0 ){
            ++count;
        }
        else{
            count = 0;
        }
        if( count >= 3 ){
            return -1;
        }
        data[out++] = data[in++];
    }
    return out;
}

// Pretty much just a pump for the stream to make sure the bits are extracted,
// Even though we don't do anything with them.
int read_scaling_list( bs_stream_t stream, int32_t * delta_scale_thunk, size_t size ){
    int32_t last_scale = 8;
    int32_t next_scale = 8;
    int32_t delta_scale;
    for( size_t i = 0; i < size; ++i ){
        if( next_scale != 0 ){
            delta_scale = map_se( bs_read_exp_golomb( stream ) );
            next_scale = ( last_scale + delta_scale + 256 ) % 256;
        }
        last_scale = ( next_scale == 0 ) ? last_scale : next_scale;
    }
    return 0;
}

int read_hrd_parameters( bs_stream_t stream, struct hrd_parameters * params ){
    memset( params, 0, sizeof * params );
    params->cpb_cnt_minus1 = bs_read_exp_golomb( stream );
    params->bit_rate_scale = bs_read_uint( stream, 4 );
    params->cpb_size_scale = bs_read_uint( stream, 4 );
    for( size_t i = 0; i <= params->cpb_cnt_minus1; ++i ){
        if( i >= H264_ARRAY_MAX ){
            (void)bs_read_exp_golomb( stream );
            (void)bs_read_exp_golomb( stream );
            (void)bs_read_bit( stream );
            if( i >= H264_LOOP_MAX ){
                return -1;
            }
        }
        else{
            params->bit_rate_value_minus1[i] = bs_read_exp_golomb( stream );
            params->cpb_size_value_minus1[i] = bs_read_exp_golomb( stream );
            params->cbr_flag[i] = bs_read_bit( stream );
        }
    }
    params->initial_cbp_removal_delay_length_minus1 = bs_read_uint( stream, 5 );
    params->cpb_removal_delay_length_minus1 = bs_read_uint( stream, 5 );
    params->dpb_output_delay_length_minus1 = bs_read_uint( stream, 5 );
    params->time_offset_length = bs_read_uint( stream, 5 );
    return 0;
}

int read_vui_parameters( bs_stream_t stream, struct vui_parameters * params ){
    memset( params, 0, sizeof * params );
    params->matrix_coefficients = 2;
    params->color_primaries = 2;
    params->transfer_characteristics = 2;
    params->video_format = 5;

    if( bs_read_bit( stream ) ){
        params->aspect_ratio_idc = bs_read_uint( stream, 8 );
        if( params->aspect_ratio_idc == H264_EXTENDED_SAR ){
            params->sar_width = bs_read_uint( stream, 16 );
            params->sar_height = bs_read_uint( stream, 16 );
        }
    }
    if( bs_read_bit( stream ) ){
        params->overscan_appropriate = bs_read_bit( stream );
    }
    if( bs_read_bit( stream ) ){
        params->video_format = bs_read_uint( stream, 3 );
        params->video_full_range = bs_read_bit( stream );
        if( bs_read_bit( stream ) ){
            params->color_description_present = true;
            params->color_primaries = bs_read_uint( stream, 8 );
            params->transfer_characteristics = bs_read_uint( stream, 8 );
            params->matrix_coefficients = bs_read_uint( stream, 8 );
        }
    }
    if( bs_read_bit( stream ) ){
        params->chroma_loc_info_present = true;
        params->chroma_sample_loc_type_top_field = bs_read_exp_golomb( stream );
        params->chroma_sample_loc_type_bottom_field = bs_read_exp_golomb( stream );
    }
    if( bs_read_bit( stream ) ){
        params->num_units_in_tick = bs_read_uint( stream, 32 );
        params->time_scale = bs_read_uint( stream, 32 );
        params->fixed_frame_rate = bs_read_bit( stream );
    }
    bool present = false;
    if( bs_read_bit( stream ) ){
        present = true;
        read_hrd_parameters( stream, &params->nal_hrd );
    }
    if( bs_read_bit( stream ) ){
        present = true;
        read_hrd_parameters( stream, &params->vcl_hrd );
    }
    if( present ){
        params->low_delay_hrd = bs_read_bit( stream );
    }
    params->pic_struct_present = bs_read_bit( stream );
    if( bs_read_bit( stream ) ){
        params->motion_vectors_over_pic_boundaries = bs_read_bit( stream );
        params->max_bytes_per_pic_denom = bs_read_exp_golomb( stream );
        params->max_bits_per_mb_denom = bs_read_exp_golomb( stream );
        params->log2_max_mv_length_horizontal = bs_read_exp_golomb( stream );
        params->log2_max_mv_length_vertical = bs_read_exp_golomb( stream );
        params->max_num_reorder_frames = bs_read_exp_golomb( stream );
        params->max_dec_frame_buffering = bs_read_exp_golomb( stream );
    }
    return 0;
}


int read_seq_parameter_set( bs_stream_t stream, struct seq_parameter_set * params ){
    memset( params, 0, sizeof * params );
    params->chroma_format_idc = 1;
    params->profile_idc = bs_read_uint( stream, 8 );
    params->constraint_set0 = bs_read_bit( stream );
    params->constraint_set1 = bs_read_bit( stream );
    params->constraint_set2 = bs_read_bit( stream );
    params->constraint_set3 = bs_read_bit( stream );
    params->constraint_set4 = bs_read_bit( stream );
    params->constraint_set5 = bs_read_bit( stream );
    if( bs_read_uint( stream, 2 ) != 0 ){
        return -1;
    }
    params->level_idc = bs_read_uint( stream, 8 );
    params->seq_parameter_set_id = bs_read_exp_golomb( stream );
    switch( params->profile_idc ){
        case 100: case 110: case 122: case 244: case 44:  case 83: case 86:
        case 118: case 128: case 138: case 139: case 134: case 135:
            params->chroma_format_idc = bs_read_exp_golomb( stream );
            if( params->chroma_format_idc == 3 ){
                params->separate_color_plane = bs_read_bit( stream );
            }
            params->bit_depth_luma_minus8 = bs_read_exp_golomb( stream );
            params->bit_depth_chroma_minus8 = bs_read_exp_golomb( stream );
            params->qpprime_y_zero_transform_bypass = bs_read_bit( stream );
            if( bs_read_bit( stream ) == 1 ){
                for( size_t i = 0; i < ( params->chroma_format_idc != 3 ? 8 : 12 ); ++i ){
                    //This check should be optimized out
                    if( i >= H264_ARRAY_MAX ){
                        return -1;
                    }
                    if( bs_read_bit( stream ) ){
                        if( i < 6 ){
                            read_scaling_list( stream, params->delta_scale + i, 16 );
                        }
                        else {
                            read_scaling_list( stream, params->delta_scale + i, 64 );
                        }
                    }
                }
            }
        default: break;
    }
    params->log2_max_frame_num_minus4 = bs_read_exp_golomb( stream );
    params->pic_order_cnt_type = bs_read_exp_golomb( stream );
    if( params->pic_order_cnt_type == 0 ){
        params->log2_max_pic_order_cnt_lsb_minus4 = bs_read_exp_golomb( stream );
    }
    else if( params->pic_order_cnt_type == 1 ){
        params->delta_pic_order_always_zero = bs_read_bit( stream );
        params->offset_for_non_ref_pic = map_se( bs_read_exp_golomb( stream ) );
        params->offset_for_top_to_bottom_field = map_se( bs_read_exp_golomb( stream ) );
        params->num_ref_frames_in_pic_order_cnt_cycle = bs_read_exp_golomb( stream );
        for( size_t i = 0; i < params->num_ref_frames_in_pic_order_cnt_cycle; ++i ){
            if( i >= H264_ARRAY_MAX ){
                (void)bs_read_exp_golomb( stream );
            }
            else{
                params->offset_for_ref_frame[i] = map_se( bs_read_exp_golomb( stream ) );
            }
        }
    }
    params->max_num_ref_frames = bs_read_exp_golomb( stream );
    params->gaps_in_frame_num_value_allowed = bs_read_bit( stream );
    params->pic_width_in_mbs_minus1 = bs_read_exp_golomb( stream );
    params->pic_height_in_map_units_minus1 = bs_read_exp_golomb( stream );
    params->frame_mbs_only = bs_read_bit( stream );
    if( !params->frame_mbs_only ){
        params->mb_adaptive_frame_field = bs_read_bit( stream );
    }
    params->direct_8x8_inference = bs_read_bit( stream );
    if( (params->frame_cropping_flag = bs_read_bit( stream )) ){
        params->frame_crop_left_offset = bs_read_exp_golomb( stream );
        params->frame_crop_right_offset = bs_read_exp_golomb( stream );
        params->frame_crop_top_offset = bs_read_exp_golomb( stream );
        params->frame_crop_bottom_offset = bs_read_exp_golomb( stream );
    }
    if( bs_read_bit( stream ) ){
        read_vui_parameters( stream, &params->vui );
    }
    if( bs_read_bit( stream ) ){
        bs_seek( stream, ( 8 - bs_tell(stream) % 8 ) % 8, BS_SEEK_OFFSET );
    }
    return 0;
}

bool more_rbsp_data( bs_stream_t stream ){
    size_t offset = bs_tell( stream );
    if( bs_remaining( stream ) == 0 ){
        return false;
    }
    size_t set_offset = offset;
    // Find the last set of 8 bits which has a bit set.
    while( bs_remaining( stream ) > 0 ){
        size_t new_offset = bs_tell( stream );
        if( bs_read_uint( stream, 8 ) > 0 ){
            set_offset = new_offset;
        }
    }
    // Find the index of the bit found.
    bs_seek( stream, set_offset, BS_SEEK_SET );
    for( size_t i = 0; i < 8; ++i ){
        size_t new_offset = bs_tell( stream );
        if( bs_read_bit( stream ) ){
            set_offset = new_offset;
        }
    }
    // Return to the original offset.
    bs_seek( stream, offset, BS_SEEK_SET );
    return set_offset - offset > 0;
}

int read_pic_parameter_set( bs_stream_t stream, struct pic_parameter_set * params, struct seq_parameter_set * seq_params ){
    memset( params, 0, sizeof * params );
    params->pic_parameter_set_id = bs_read_exp_golomb( stream );
    params->seq_parameter_set_id = bs_read_exp_golomb( stream );
    params->entropy_coding_mode = bs_read_bit( stream );
    params->bottom_field_pic_order_in_frame_present = bs_read_bit( stream );
    params->num_slice_groups_minus1 = bs_read_exp_golomb( stream );
    if( params->num_slice_groups_minus1 > 0 ){
        params->slice_group_map_type = bs_read_exp_golomb( stream );
        if( params->slice_group_map_type == 0 ){
            // Yes, this is <= (less than or equal)
            for( size_t i = 0; i <= params->num_slice_groups_minus1; ++i ){
                uint32_t temp = bs_read_exp_golomb( stream );
                if( i < H264_ARRAY_MAX ){
                    params->run_length_minus1[i] = temp;
                }
            }
        }
        else if( params->slice_group_map_type == 2 ){
            // Yes, this is < (less than)
            for( size_t i = 0; i < params->num_slice_groups_minus1; ++i ){
                uint32_t temp1 = bs_read_exp_golomb( stream );
                uint32_t temp2 = bs_read_exp_golomb( stream );
                if( i < H264_ARRAY_MAX ){
                    params->top_left[i] = temp1;
                    params->bottom_right[i] = temp2;
                }
            }
        }
        else if(    params->slice_group_map_type == 3 ||
                    params->slice_group_map_type == 3 ||
                    params->slice_group_map_type == 4 ) {
            params->slice_group_change_direction_flag = bs_read_bit( stream );
            params->slice_group_change_rate_minus1 = bs_read_exp_golomb( stream );
        }
        else if( params->slice_group_map_type == 6 ){
            params->pic_size_in_map_units_minus1 = bs_read_exp_golomb( stream );
            size_t param_len = ceil( log( params->num_slice_groups_minus1 + 1 ) / log( 2 ) );
            // Yes, this is <= (less than or equal)
            for( size_t i = 0; i <= params->pic_size_in_map_units_minus1; ++i ){
                uint32_t temp = bs_read_uint( stream, param_len );
                if( i < H264_ARRAY_MAX ){
                    params->slice_group_id[i] = temp;
                }
            }
        }
    }
    params->num_ref_idx_l0_default_active_minus1 = bs_read_exp_golomb( stream );
    params->num_ref_idx_l1_default_active_minus1 = bs_read_exp_golomb( stream );
    params->weighted_pred_flag = bs_read_bit( stream );
    params->weighted_bipred_idc = bs_read_uint( stream, 2 );
    params->pic_init_qp_minus26 = map_se( bs_read_exp_golomb( stream ) );
    params->pic_init_qs_minus26 = map_se( bs_read_exp_golomb( stream ) );
    params->chroma_qp_index_offset = map_se( bs_read_exp_golomb( stream ) );
    params->deblocking_filter_control_present = bs_read_bit( stream );
    params->constrained_intra_pred = bs_read_bit( stream );
    params->redundant_pic_cnt_present = bs_read_bit( stream );
    if( more_rbsp_data( stream ) ){
        params->transform_8x8_mode_flag = bs_read_bit( stream );
        params->pic_scaling_matrix_present_flag = bs_read_bit( stream );
        if( params->pic_scaling_matrix_present_flag ){
            size_t dest = 6 + ((seq_params->chroma_format_idc != 3) ? 2 : 6) * params->transform_8x8_mode_flag;
            for( size_t i = 0; i < dest; ++i ){
                bool temp_present = bs_read_bit( stream );
                int32_t temp_delta = 0;
                if( temp_present ){
                    if( i < 6 ){
                        read_scaling_list( stream, &temp_delta, 16 );
                    }
                    else{
                        read_scaling_list( stream, &temp_delta, 64 );
                    }
                }
                if( i < H264_ARRAY_MAX ){
                    params->pic_scaling_list_present_flag[i] = temp_present;
                    params->delta_scale[i] = temp_delta;
                }
            }
        }
        params->second_chroma_qp_index_offset = map_se( bs_read_exp_golomb( stream ) );
    }
    if( bs_read_bit( stream ) ){
        bs_seek( stream, ( 8 - bs_tell(stream) % 8 ) % 8, BS_SEEK_OFFSET );
    }
    return 0;
}
#include <stdio.h>
#include <stdarg.h>

const char * get_color_prim_name( size_t color_prim ){
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
const char * get_color_trans_name( size_t color_trans ){
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


const char * get_color_matrix_name( size_t color_matrix ){
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
const char * get_video_fmt_name( size_t fmt ){
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



const char * get_profile_name(size_t profile){
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

const char * get_pix_name(struct seq_parameter_set * seq){
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


bool get_profile_support(size_t profile){
    switch(profile){
        case 66: case 77: case 100: case 110:
        case 122: case 244:
            return true;
        default:
            return false;
    }
}

size_t write_fmt( char * buffer, size_t len, size_t offset, const char * format, ... ){
    va_list args;
    va_start(args, format);
    size_t write_amt = vsnprintf(nullptr, 0, format, args);
    va_end(args);
    if( buffer && offset < len ){
        va_start(args, format);
        vsnprintf(buffer + offset, len - offset, format, args);
        va_end(args);
    }
    return write_amt;
}

size_t get_ffmpeg_params(char * buffer, size_t len, struct avcc_data * data ){
    if( data->pps_count == 0 || data->sps_count == 0 ){
        return 0;
    }
    struct pic_parameter_set * pic = data->pps;
    struct seq_parameter_set * seq = data->sps;

    size_t out_len = 0;
    out_len += write_fmt( buffer, len, out_len, "-pix_fmt %s ", get_pix_name( seq ) );
    out_len += write_fmt( buffer, len, out_len, "-profile:v %s ", get_profile_name( seq->profile_idc ) );
    out_len += write_fmt( buffer, len, out_len, "-level %d.%d ", seq->level_idc / 10, seq->level_idc % 10 );
    out_len += write_fmt( buffer, len, out_len, "-x264-params \"" );
    if( out_len < len ){
        out_len += get_x264_params( buffer + out_len, len - out_len, pic, seq );
    }
    else{
        out_len += get_x264_params( nullptr, 0, pic, seq );
    }
    out_len += write_fmt( buffer, len, out_len, "\"" );
    return out_len;
}

size_t get_x264_params(char * buffer, size_t len, struct pic_parameter_set * pic, struct seq_parameter_set * seq ){
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

void * safe_realloc( void * data, size_t new_size, size_t * old_size ){
    if( new_size < *old_size ){
        return data;
    }
    if( new_size > 100000 ){
        free( data );
        return nullptr;
    }
    void * new_data = realloc( data, new_size );
    if( new_data == nullptr ){
        free( data );
        return nullptr;
    }
    *old_size = new_size;
    return new_data;
}

bool verify_nal_type( bs_stream_t stream, size_t ref, size_t type ){
    if( bs_read_bit( stream ) ){
        return false;
    }
    if( !bs_read_uint( stream, 2 ) != !ref ){ //Verify that if ref > 0, then nal_ref_idc > 0 as well (and the converse)
        return false;
    }
    return type == bs_read_uint( stream, 5 );
}

int read_avcc_data( bs_stream_t stream, struct avcc_data * data ){
    uint32_t version = 0;
    size_t buffer_size = 0;
    void * buffer = safe_realloc(nullptr, 10, &buffer_size);
    while( version == 0 && bs_remaining( stream ) > 0 ){
        version = bs_read_uint( stream, 8 );
    }
    if( version != 1 ){
        return -1;
    }
    data->profile = bs_read_uint( stream, 8 );
    data->compat = bs_read_uint( stream, 8 );
    data->level = bs_read_uint( stream, 8 );
    if( bs_read_uint( stream, 6 ) != 63 ){ //63 is the least sig. 6 bits set
        return -1;
    }
    data->nalu_length_size_minus1 = bs_read_uint( stream, 2 );
    if( bs_read_uint( stream, 3 ) != 7 ){ //7 is the least sig. 3 bits set
        return -1;
    }
    data->sps_count = bs_read_uint( stream, 5 );
    for( size_t i = 0; i < data->sps_count; ++i ){
        size_t len = bs_read_uint( stream, 16 );
        size_t offset = bs_tell( stream );
        if( i < AVCC_MAX_SPS ){
            buffer = safe_realloc( buffer, len, &buffer_size );
            if( buffer == nullptr ){
                return -1;
            }
            bs_read_bytes( stream, buffer, len );
            size_t new_len = nal_to_rbsp((char*)buffer, len);
            bs_stream_t new_stream = bs_create( buffer, new_len * 8, 0 );
            if( verify_nal_type( new_stream, 1, 7 ) ){
                read_seq_parameter_set( new_stream, &data->sps[i] );
            }
            bs_destroy( new_stream );
        }
        bs_seek( stream, offset + len, BS_SEEK_SET );
    }
    data->pps_count = bs_read_uint( stream, 8 );
    for( size_t i = 0; i < data->pps_count; ++i ){
        size_t len = bs_read_uint( stream, 16 );
        size_t offset = bs_tell( stream );
        if( i < AVCC_MAX_PPS ){
            struct seq_parameter_set * parent = data->sps + i;
            if( i < data->sps_count ){
                parent = data->sps + data->sps_count - 1;
            }
            buffer = safe_realloc( buffer, len, &buffer_size );
            if( buffer == nullptr ){
                return -1;
            }
            bs_read_bytes( stream, buffer, len );
            size_t new_len = nal_to_rbsp((char*)buffer, len);
            bs_stream_t new_stream = bs_create( buffer, new_len * 8, 0 );
            if( verify_nal_type( new_stream, 1, 8 ) ){
                read_pic_parameter_set( stream, &data->pps[i], parent );
            }
            bs_destroy( new_stream );
        }
        bs_seek( stream, offset + len, BS_SEEK_SET );
    }
    return 0;
}

#ifdef _cplusplus
}
#endif
