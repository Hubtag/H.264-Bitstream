#include "bin.h"
#include "gtest/gtest.h"
#include "bitstream.h"



TEST(Bitstream, ReadZero){
    char data[] = b00000000 b00000000 b00000000 b00000000;
    bs_stream_t stream = bs_create(data, sizeof data * 8, 0);
    EXPECT_EQ(bs_read_uint(stream, 32), 0);
    // Read over end
    EXPECT_EQ(bs_read_uint(stream, 32), 0);
    bs_destroy( stream );
}

TEST(Bitstream, ReadSaturated){
    unsigned char data[] = b11111111 b11111111 b11111111 b11111111;
    bs_stream_t stream = bs_create(data, sizeof data * 8, 0);
    EXPECT_EQ(bs_read_uint(stream, 32), 0xFFFFFFFFu);
    // Read over end
    EXPECT_EQ(bs_read_uint(stream, 32), 0);
    bs_destroy( stream );
}

TEST(Bitstream, ReadHalfSaturated){
    unsigned char data[] = b11111111 b11111111;
    bs_stream_t stream = bs_create(data, sizeof data * 8 - 8, 0);
    EXPECT_EQ(bs_read_uint(stream, 32), 0xFFFF0000u);
    // Read over end
    EXPECT_EQ(bs_read_uint(stream, 32), 0);
    bs_destroy( stream );
}

TEST(Bitstream, ReadLoop){
    unsigned char data[] = b11111111 b11111111 b11111111 b11111111;
    for( size_t i = 0; i < 32; ++i ){
        bs_stream_t stream = bs_create(data, sizeof data * 8 - 8, i);
        EXPECT_EQ(bs_read_uint(stream, 32), 0xFFFFFFFFu << i & 0xFFFFFFFFu);
        bs_destroy( stream );
    }
    for( size_t i = 1; i < 32; ++i ){
        bs_stream_t stream = bs_create(data, sizeof data * 8 - 8, 0);
        EXPECT_EQ(bs_read_uint(stream, i), 0xFFFFFFFFu >> (32 - i));
        bs_destroy( stream );
    }
    unsigned char data2[] = b00000100 b00100010 b10111011 b11011111; //04 22 BB DF
    unsigned int test = 0x0422BBDF;

    for( size_t i = 0; i < 32; ++i ){
        bs_stream_t stream = bs_create(data2, sizeof data2 * 8 - 8, i);
        EXPECT_EQ(bs_read_uint(stream, 32), test << i & 0xFFFFFFFFu);
        bs_destroy( stream );
    }
}

TEST(Bitstream, ReadExpGolomb){
    unsigned char data[] = b00000010 b01101100 b11100000 b00000111 b00110110;
    unsigned int test1 = *b01001101 - 1;
    unsigned int test2 = *b00000111 - 1;
    unsigned int test3 = 0x735;
    bs_stream_t stream = bs_create(data, sizeof data * 8 - 8, 0);
    EXPECT_EQ( bs_read_exp_golomb(stream), test1);
    EXPECT_EQ( bs_read_exp_golomb(stream), 0);
    EXPECT_EQ( bs_read_exp_golomb(stream), test2);
    EXPECT_EQ( bs_read_exp_golomb(stream), test3);
    bs_destroy( stream );
}



int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
