#include "gtest/gtest.h"
#include "h264_rbsp.h"

size_t get_b_short(const char * buf){
    size_t base = (unsigned char)buf[0];
    base <<= 8;
    base += (unsigned char)buf[1];
    return base;
}

int main(int argc, char* argv[]){
    //char sps_dat[] = "\x64\x00\x1F\xAC\xD9\x40\x50\x05\xBB\x01\x10\x00\x00\x03\x00\x10\x00\x00\x03\x03\x20\xF1\x83\x19\x60";
    //char sps_dat[] = "\x4d\x40\x29\xff\xe1\x00\x1b\x67\x4d\x40\x29\xec\xa0\x50\x17\xfc\xb3\x50\x10\xd0\x64\x00\x00\x03\x00\x04\x00\x00\x1f\x40\x3c\x60\xc6\x58\x01\x00\x04\x68\xef\xbc\x80";
    //char pps_dat[] = "\xEB\xE3\xCB\x22\xC0";
    char * sps_dat;
    char * pps_dat;
    char all_dat[] = "\x00\x00\x00\x00\x01\x4d\x40\x28\xff\xe1\x00\x1d\x67\x4d\x40\x28\xec\xa0\x3c\x01\x13\xf2\xcd\xc0\x40\x80\x90\x00\x00\x03\x00\x10\x00\x00\x03\x03\xc0\xf1\x83\x19\x60\x01\x00\x04\x68\xef\xbc\x80";

    size_t all_dat_len;
    for( all_dat_len = 0; all_dat[all_dat_len] == 0 && all_dat_len < sizeof all_dat; ++all_dat_len ){
    }
    all_dat_len = all_dat_len * 2 + 2;

    sps_dat = all_dat + all_dat_len;
    int sps_size = get_b_short(sps_dat);
    sps_dat += 2;
    pps_dat = sps_dat + sps_size + 1;
    int pps_size = get_b_short(pps_dat);
    pps_dat += 2;

    sps_size = nal_to_rbsp( sps_dat, sps_size );
    pps_size = nal_to_rbsp( pps_dat, pps_size );

    bs_stream_t sps_stream = bs_create( ++sps_dat, --sps_size * 8, 0 );
    bs_stream_t pps_stream = bs_create( ++pps_dat, --pps_size * 8, 0 );

    struct seq_parameter_set sps;
    struct pic_parameter_set pps;
    read_seq_parameter_set( sps_stream, &sps );
    read_pic_parameter_set( pps_stream, &pps, &sps );

    char buffer[20000];
    size_t len = get_ffmpeg_params( buffer, 20000, &pps, &sps );
    printf("%s\n", buffer);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
