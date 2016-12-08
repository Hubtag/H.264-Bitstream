#include "h264_bs.h"
#include "gtest/gtest.h"


TEST(H264_BitStream, ReadSeqParamSet){
    const char * data = "\x64\x00\x1F\xAC\xD9\x40\x50\x05\xBB\x01\x6A\x04"
                        "\x04\x12\x80\x00\x00\x00\x80\x00\x00"
                        "\x19\x07\x8C\x18\xCB\x01\x00\x06\x68\xEB"
                        "\xE3\xCB\x22\xC0\x00\x00\x00\x3D\x09\x00"
                        "\x73\xEB\x00\x00";
    struct h264_seq_parameter_set params;
    bs_stream_t stream = bs_create( data, 41*8, 0 );
    EXPECT_EQ( h264_read_seq_parameter_set( stream, &params ), 0 );
    bs_destroy( stream );
}

TEST(H264_BitStream, NAL_To_RBSP){
    {
        char test[] = "This is valid \0\0\03\0\0\03\0";
        char result[] = "This is valid \0\0\0\0\0";
        EXPECT_EQ( h264_nal_to_rbsp(test, sizeof test - 1 ), sizeof result - 1 );
        EXPECT_EQ( memcmp(test, result, sizeof result - 1), 0 );
    }
    {
        char test[] = "This is invalid \0\0\0\0\03\0";
        EXPECT_EQ( h264_nal_to_rbsp(test, sizeof test - 1 ), -1 );
    }
    {
        char test[] = "This is valid \0\0\03\01\0\03\0";
        char result[] = "This is valid \0\0\01\0\03\0";
        EXPECT_EQ( h264_nal_to_rbsp(test, sizeof test - 1 ), sizeof result - 1 );
        EXPECT_EQ( memcmp(test, result, sizeof result - 1), 0 );
    }
    {
        char test[] = "This is invalid \0\0\01\0\03\0";
        EXPECT_EQ( h264_nal_to_rbsp(test, sizeof test - 1 ), -1 );
    }
    {
        char test[] = "This is valid \0\0\03\01\0\0\03\0";
        char result[] = "This is valid \0\0\01\0\0\0";
        EXPECT_EQ( h264_nal_to_rbsp(test, sizeof test - 1 ), sizeof result - 1 );
        EXPECT_EQ( memcmp(test, result, sizeof result - 1), 0 );
    }
    {
        char test[] = "This is invalid \0\0\03\0\0\0\03\0";
        EXPECT_EQ( h264_nal_to_rbsp(test, sizeof test - 1 ), -1 );
    }
}

TEST(H264_BitStream, ReadSeqParamSetNAL){
    char data[] =   "\x4d\x40\x29\xff\xe1\x00\x1b\x67\x4d\x40"
                    "\x29\xec\xa0\x50\x17\xfc\xb3\x50\x10\xd0"
                    "\x64\x00\x00\x03\x00\x04\x00\x00\x1f\x40"
                    "\x3c\x60\xc6\x58\x01\x00\x04\x68\xef\xbc"
                    "\x80";
    size_t len = h264_nal_to_rbsp( data, sizeof data );
    struct h264_seq_parameter_set params;
    bs_stream_t stream = bs_create( data, len * 8, 0 );
    EXPECT_EQ( h264_read_seq_parameter_set( stream, &params ), 0 );
    bs_destroy( stream );
    struct h264_seq_parameter_set compare;
    memset( &compare, 0, sizeof compare );
    compare.profile_idc = 77;
    compare.constraint_set1 = 1;
    compare.level_idc = 41;
    compare.chroma_format_idc = 1;
    compare.gaps_in_frame_num_value_allowed = true;
    compare.frame_mbs_only = true;
    compare.direct_8x8_inference = true;
    compare.frame_cropping_flag = true;
    compare.frame_crop_left_offset = 15;
    compare.frame_crop_right_offset = 218;
    compare.frame_crop_top_offset = 6;
    compare.frame_crop_bottom_offset = 1;
    EXPECT_EQ( memcmp( &params, &compare, sizeof compare ), 0 );
}
TEST(H264_BitStream, ReadAVCC){
    char data[] =   "\x00\x00\x00\x00\x01\x7A\x54\x29\xFF\xE1\x00\x5F\x67\x7A\x54\x29"
                    "\x00\x7D\x04\x94\x0C\x98\x00\x01\x91\x00\x5C\x80\x1F\xFD\x80\x01"
                    "\x81\xC8\x00\x0C\x0E\x60\xAE\x00\x4D\x30\x00\x00\x11\x92\xFB\x6E"
                    "\x10\x01\xB6\x75\xFF\x92\x0D\x82\x69\x79\xBD\x90\x01\xC0\x00\x02"
                    "\x5A\xD1\x00\x00\x35\x0C\xA0\x17\x8C\x29\xC0\x00\x00\x9A\x5B\x5C"
                    "\x03\xE0\x1F\x20\xB4\x16\xC6\x00\xB0\x10\x62\x82\xC1\x08\x01\x37"
                    "\x80\x01\x81\xD0\x00\x43\x88\x0C\x80\xB6\x00\x01\x00\x1E\x68\x00"
                    "\x26\x98\x01\x34\xF4\xC0\x09\xA6\x00\x02\xC9\xBC\x00\x9A\x60\x03"
                    "\x60\x70\x00\x9A\x50\x1E\xC0\x58\xF8\x00\x16\x00";

    struct h264_avcc_data avcc_data;
    bs_stream_t stream = bs_create( data, (sizeof data - 1) * 8, 0 );
    EXPECT_EQ( h264_read_avcc_data( stream, &avcc_data ), 0 );
    bs_destroy( stream );

    struct h264_seq_parameter_set cmp_sps;
    struct h264_pic_parameter_set cmp_pps;
    memset( &cmp_sps, 0, sizeof cmp_sps );
    memset( &cmp_pps, 0, sizeof cmp_pps );
    cmp_sps.profile_idc = 122;
    cmp_sps.constraint_set1 = 1;
    cmp_sps.constraint_set3 = 1;
    cmp_sps.constraint_set5 = 1;
    cmp_sps.level_idc = 41;
    cmp_sps.seq_parameter_set_id = 999;
    cmp_sps.chroma_format_idc = 3;
    cmp_sps.separate_color_plane = true;
    cmp_sps.bit_depth_luma_minus8 = 4;
    cmp_sps.bit_depth_chroma_minus8 = 99;
    cmp_sps.qpprime_y_zero_transform_bypass = true;
    cmp_sps.log2_max_frame_num_minus4 = 99;
    cmp_sps.pic_order_cnt_type = 1;
    cmp_sps.delta_pic_order_always_zero = false;
    cmp_sps.offset_for_non_ref_pic = -92;
    cmp_sps.offset_for_top_to_bottom_field = -1023;
    cmp_sps.num_ref_frames_in_pic_order_cnt_cycle = 2;
    cmp_sps.offset_for_ref_frame[0] = 12345;
    cmp_sps.offset_for_ref_frame[1] = -12345;
    cmp_sps.max_num_ref_frames = 42;
    cmp_sps.gaps_in_frame_num_value_allowed = true;
    cmp_sps.pic_width_in_mbs_minus1 = 1234;
    cmp_sps.pic_height_in_map_units_minus1 = 9213914;
    cmp_sps.frame_mbs_only = false;
    cmp_sps.mb_adaptive_frame_field = true;
    cmp_sps.direct_8x8_inference = true;
    cmp_sps.frame_cropping_flag = true;
    cmp_sps.frame_crop_left_offset = 15;
    cmp_sps.frame_crop_right_offset = 218;
    cmp_sps.frame_crop_top_offset = 6;
    cmp_sps.frame_crop_bottom_offset = 1;
    cmp_sps.vui.aspect_ratio_idc = H264_EXTENDED_SAR;
    cmp_sps.vui.sar_width = 9243;
    cmp_sps.vui.sar_height = 1234;
    cmp_sps.vui.overscan_appropriate = true;
    cmp_sps.vui.video_format = 4;
    cmp_sps.vui.video_full_range = true;
    cmp_sps.vui.color_description_present = true;
    cmp_sps.vui.color_primaries = 123;
    cmp_sps.vui.transfer_characteristics = 32;
    cmp_sps.vui.matrix_coefficients = 3;
    cmp_sps.vui.chroma_loc_info_present = true;
    cmp_sps.vui.chroma_sample_loc_type_top_field = 1234567;
    cmp_sps.vui.chroma_sample_loc_type_bottom_field = 54321;
    cmp_sps.vui.num_units_in_tick = 12345678;
    cmp_sps.vui.time_scale = 1234;
    cmp_sps.vui.fixed_frame_rate = true;
    cmp_sps.vui.nal_hrd.cpb_cnt_minus1 = 2;
    cmp_sps.vui.nal_hrd.bit_rate_scale = 5;
    cmp_sps.vui.nal_hrd.cpb_size_scale = 12;
    cmp_sps.vui.nal_hrd.bit_rate_value_minus1[0] = 123;
    cmp_sps.vui.nal_hrd.cpb_size_value_minus1[0] = 123;
    cmp_sps.vui.nal_hrd.cbr_flag[0] = true;
    cmp_sps.vui.nal_hrd.bit_rate_value_minus1[1] = 44;
    cmp_sps.vui.nal_hrd.cpb_size_value_minus1[1] = 44;
    cmp_sps.vui.nal_hrd.cbr_flag[1] = true;
    cmp_sps.vui.nal_hrd.bit_rate_value_minus1[2] = 11;
    cmp_sps.vui.nal_hrd.cpb_size_value_minus1[2] = 175;
    cmp_sps.vui.nal_hrd.cbr_flag[2] = false;
    cmp_sps.vui.nal_hrd.initial_cbp_removal_delay_length_minus1 = 4;
    cmp_sps.vui.nal_hrd.cpb_removal_delay_length_minus1 = 3;
    cmp_sps.vui.nal_hrd.dpb_output_delay_length_minus1 = 2;
    cmp_sps.vui.nal_hrd.time_offset_length = 16;
    cmp_sps.vui.low_delay_hrd = true;
    cmp_sps.vui.pic_struct_present = false;
    cmp_sps.vui.motion_vectors_over_pic_boundaries = true;
    cmp_sps.vui.max_bytes_per_pic_denom = 32;
    cmp_sps.vui.max_bits_per_mb_denom = 1245;
    cmp_sps.vui.log2_max_mv_length_horizontal = 12345;
    cmp_sps.vui.log2_max_mv_length_vertical = 4321;
    cmp_sps.vui.max_num_reorder_frames = 99;
    cmp_sps.vui.max_dec_frame_buffering = 44;

    cmp_pps.pic_parameter_set_id = 1234;
    cmp_pps.seq_parameter_set_id = 1234;
    cmp_pps.entropy_coding_mode = true;
    cmp_pps.bottom_field_pic_order_in_frame_present = true;
    cmp_pps.num_slice_groups_minus1 = 1;
    cmp_pps.slice_group_map_type = 2;
    cmp_pps.top_left[0] = 1234;
    cmp_pps.bottom_right[0] = 45678;
    cmp_pps.num_ref_idx_l0_default_active_minus1 = 1234;
    cmp_pps.num_ref_idx_l1_default_active_minus1 = 3456;
    cmp_pps.weighted_pred_flag = true;
    cmp_pps.weighted_bipred_idc = 2;
    cmp_pps.pic_init_qp_minus26 = -1234;
    cmp_pps.pic_init_qs_minus26 = 123;
    cmp_pps.chroma_qp_index_offset = 44;
    cmp_pps.deblocking_filter_control_present = true;
    cmp_pps.constrained_intra_pred = true;
    cmp_pps.redundant_pic_cnt_present = true;
    cmp_pps.transform_8x8_mode_flag = true;
    cmp_pps.pic_scaling_matrix_present_flag = true;
    cmp_pps.second_chroma_qp_index_offset = -2;
    EXPECT_EQ( memcmp( avcc_data.sps, &cmp_sps, sizeof cmp_sps ), 0 );
    EXPECT_EQ( memcmp( avcc_data.pps, &cmp_pps, sizeof cmp_pps ), 0 );
}
