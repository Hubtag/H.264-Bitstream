#include "gtest/gtest.h"
#include "h264_bs.h"

size_t get_b_short(const char * buf){
    size_t base = (unsigned char)buf[0];
    base <<= 8;
    base += (unsigned char)buf[1];
    return base;
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
