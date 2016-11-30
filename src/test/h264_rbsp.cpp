#include "../h264_rbsp.h"
#include "gtest/gtest.h"


TEST(H264_BitStream, ReadSeqParamSet){
    const char * data = "\x64\x00\x1F\xAC\xD9\x40\x50\x05\xBB\x01\x6A\x04"
                        "\x04\x12\x80\x00\x00\x00\x80\x00\x00"
                        "\x19\x07\x8C\x18\xCB\x01\x00\x06\x68\xEB"
                        "\xE3\xCB\x22\xC0\x00\x00\x00\x3D\x09\x00"
                        "\x73\xEB\x00\x00";
    struct seq_parameter_set params;
    bs_stream_t stream = bs_create( data, 41*8, 0 );
    EXPECT_EQ( read_seq_parameter_set( stream, &params ), 0 );
    bs_destroy( stream );
}

TEST(H264_BitStream, NAL_To_RBSP){
    {
        char test[] = "This is valid \0\0\03\0\0\03\0";
        char result[] = "This is valid \0\0\0\0\0";
        EXPECT_EQ( nal_to_rbsp(test, sizeof test - 1 ), sizeof result - 1 );
        EXPECT_EQ( memcmp(test, result, sizeof result - 1), 0 );
    }
    {
        char test[] = "This is invalid \0\0\0\0\03\0";
        EXPECT_EQ( nal_to_rbsp(test, sizeof test - 1 ), -1 );
    }
    {
        char test[] = "This is valid \0\0\03\01\0\03\0";
        char result[] = "This is valid \0\0\01\0\03\0";
        EXPECT_EQ( nal_to_rbsp(test, sizeof test - 1 ), sizeof result - 1 );
        EXPECT_EQ( memcmp(test, result, sizeof result - 1), 0 );
    }
    {
        char test[] = "This is invalid \0\0\01\0\03\0";
        EXPECT_EQ( nal_to_rbsp(test, sizeof test - 1 ), -1 );
    }
    {
        char test[] = "This is valid \0\0\03\01\0\0\03\0";
        char result[] = "This is valid \0\0\01\0\0\0";
        EXPECT_EQ( nal_to_rbsp(test, sizeof test - 1 ), sizeof result - 1 );
        EXPECT_EQ( memcmp(test, result, sizeof result - 1), 0 );
    }
    {
        char test[] = "This is invalid \0\0\03\0\0\0\03\0";
        EXPECT_EQ( nal_to_rbsp(test, sizeof test - 1 ), -1 );
    }
}

TEST(H264_BitStream, ReadSeqParamSetNAL){
    char data[] =   "\x4d\x40\x29\xff\xe1\x00\x1b\x67\x4d\x40"
                    "\x29\xec\xa0\x50\x17\xfc\xb3\x50\x10\xd0"
                    "\x64\x00\x00\x03\x00\x04\x00\x00\x1f\x40"
                    "\x3c\x60\xc6\x58\x01\x00\x04\x68\xef\xbc"
                    "\x80";
    size_t len = nal_to_rbsp( data, sizeof data );
    struct seq_parameter_set params;
    bs_stream_t stream = bs_create( data, len * 8, 0 );
    EXPECT_EQ( read_seq_parameter_set( stream, &params ), 0 );
    bs_destroy( stream );
    struct seq_parameter_set compare;
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