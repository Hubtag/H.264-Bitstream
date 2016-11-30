#ifndef H264_RBSP_H
#define H264_RBSP_H

#include "bitstream.h"
#include <stdint.h>
#include <stdbool.h>

#define H264_ARRAY_MAX 16
#define H264_LOOP_MAX 500
#define H264_EXTENDED_SAR 255

#ifdef _cplusplus
extern "C"{
#endif



struct hrd_parameters{
    uint32_t cpb_cnt_minus1;
    uint8_t bit_rate_scale;
    uint8_t cpb_size_scale;
    uint32_t bit_rate_value_minus1[H264_ARRAY_MAX];
    uint32_t cpb_size_value_minus1[H264_ARRAY_MAX];
    bool cbr_flag[H264_ARRAY_MAX];
    uint8_t initial_cbp_removal_delay_length_minus1;
    uint8_t cpb_removal_delay_length_minus1;
    uint8_t dpb_output_delay_length_minus1;
    uint8_t time_offset_length;
};

struct vui_parameters{
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    bool overscan_appropriate;
    uint8_t video_format;
    bool video_full_range;
    uint8_t color_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
    uint32_t chroma_sample_loc_type_top_field;
    uint32_t chroma_sample_loc_type_bottom_field;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    bool fixed_frame_rate;
    struct hrd_parameters nal_hrd;
    struct hrd_parameters vcl_hrd;
    bool low_delay_hrd;
    bool pic_struct_present;
    bool motion_vectors_over_pic_boundaries;
    uint32_t max_bytes_per_pic_denom;
    uint32_t max_bits_per_mb_denom;
    uint32_t log2_max_mv_length_horizontal;
    uint32_t log2_max_mv_length_vertical;
    uint32_t max_num_reorder_frames;
    uint32_t max_dec_frame_buffering;
};

struct seq_parameter_set{
    uint8_t profile_idc;
    bool constraint_set0;
    bool constraint_set1;
    bool constraint_set2;
    bool constraint_set3;
    bool constraint_set4;
    bool constraint_set5;
    uint8_t level_idc;
    uint32_t seq_parameter_set_id;
    uint32_t chroma_format_idc;
    bool separate_color_plane;
    uint32_t bit_depth_luma_minus8;
    uint32_t bit_depth_chroma_minus8;
    bool qpprime_y_zero_transform_bypass;
    int32_t delta_scale[H264_ARRAY_MAX];
    uint32_t log2_max_frame_num_minus4;
    uint8_t pic_order_cnt_type;
    uint32_t log2_max_pic_order_cnt_lsb_minus4;
    bool delta_pic_order_always_zero;
    int32_t offset_for_non_ref_pic;
    int32_t offset_for_top_to_bottom_field;
    uint32_t num_ref_frames_in_pic_order_cnt_cycle;
    int32_t offset_for_ref_frame[H264_ARRAY_MAX];
    uint32_t max_num_ref_frames;
    bool gaps_in_frame_num_value_allowed;
    uint32_t pic_width_in_mbs_minus1;
    uint32_t pic_height_in_map_units_minus1;
    bool frame_mbs_only;
    bool mb_adaptive_frame_field;
    bool direct_8x8_inference;
    bool frame_cropping_flag;
    uint32_t frame_crop_left_offset;
    uint32_t frame_crop_right_offset;
    uint32_t frame_crop_top_offset;
    uint32_t frame_crop_bottom_offset;
    struct vui_parameters vui;
};

struct pic_parameter_set{
    uint32_t pic_parameter_set_id;
    uint32_t seq_parameter_set_id;
    bool entropy_coding_mode;
    bool bottom_field_pic_order_in_frame_present;
    uint32_t num_slice_groups_minus1;
    uint32_t slice_group_map_type;
    uint32_t run_length_minus1[H264_ARRAY_MAX];
    uint32_t top_left[H264_ARRAY_MAX];
    uint32_t bottom_right[H264_ARRAY_MAX];
    bool slice_group_change_direction_flag;
    uint32_t slice_group_change_rate_minus1;
    uint32_t pic_size_in_map_units_minus1;
    uint32_t slice_group_id[H264_ARRAY_MAX];
    uint32_t num_ref_idx_l0_default_active_minus1;
    uint32_t num_ref_idx_l1_default_active_minus1;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_idc;
    int32_t pic_init_qp_minus26;
    int32_t pic_init_qs_minus26;
    int32_t chroma_qp_index_offset;
    bool deblocking_filter_control_present;
    bool constrained_intra_pred;
    bool redundant_pic_cnt_present;
    bool transform_8x8_mode_flag;
    bool pic_scaling_matrix_present_flag;
    bool pic_scaling_list_present_flag[H264_ARRAY_MAX];
    int32_t delta_scale[H264_ARRAY_MAX];
    int32_t second_chroma_qp_index_offset;
};

int32_t map_se(uint32_t in);

int nal_to_rbsp( char * data, size_t length );

int read_scaling_list( bs_stream_t stream, int32_t * delta_scale_thunk, size_t size );

int read_hrd_parameters( bs_stream_t stream, struct hrd_parameters * params );

int read_vui_parameters( bs_stream_t stream, struct vui_parameters * params );

int read_seq_parameter_set( bs_stream_t stream, struct seq_parameter_set * params );

#ifdef _cplusplus
}
#endif
#endif
