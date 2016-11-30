#include "h264_rbsp.h"

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
            params->color_primaries = bs_read_uint( stream, 8 );
            params->transfer_characteristics = bs_read_uint( stream, 8 );
            params->matrix_coefficients = bs_read_uint( stream, 8 );
        }
    }
    if( bs_read_bit( stream ) ){
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

#ifdef _cplusplus
}
#endif
