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

TEST(Bitstream, ReadBits){
    unsigned char data[] = b00000010 b01101100 b11100000 b00000111 b00110110;
    unsigned int data2[] = {0,0,0,0,0,0,1,0,0,1,1,0,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1,1,0,1,1,0};
    bs_stream_t stream = bs_create(data, sizeof data * 8 - 8, 0);
    for( size_t i = 0; i < sizeof data2 / sizeof data2[0]; ++i ){
        EXPECT_EQ( bs_read_bit(stream), data2[i]);
    }
    bs_destroy( stream );
}

TEST(Bitstream, ReadBuffer){
    unsigned char data[] = b00000010 b01101100 b11100000 b00000111 b00110110;
    //Ugly, I know. But it's simple.
    const char * data2[] = {
        b00000010 b01101100 b11100000 b00000111 b00110110,
        b00000100 b11011001 b11000000 b00001110 b01101100,
        b00001001 b10110011 b10000000 b00011100 b11011000,
        b00010011 b01100111 b00000000 b00111001 b10110000,
        b00100110 b11001110 b00000000 b01110011 b01100000,
        b01001101 b10011100 b00000000 b11100110 b11000000,
        b10011011 b00111000 b00000001 b11001101 b10000000,
        b00110110 b01110000 b00000011 b10011011 b00000000,
        b01101100 b11100000 b00000111 b00110110 b00000000,
        b11011001 b11000000 b00001110 b01101100 b00000000,
        b10110011 b10000000 b00011100 b11011000 b00000000,
        b01100111 b00000000 b00111001 b10110000 b00000000,
        b11001110 b00000000 b01110011 b01100000 b00000000,
        b10011100 b00000000 b11100110 b11000000 b00000000,
        b00111000 b00000001 b11001101 b10000000 b00000000,
        b01110000 b00000011 b10011011 b00000000 b00000000,
        b11100000 b00000111 b00110110 b00000000 b00000000,
        b11000000 b00001110 b01101100 b00000000 b00000000,
        b10000000 b00011100 b11011000 b00000000 b00000000,
        b00000000 b00111001 b10110000 b00000000 b00000000,
        b00000000 b01110011 b01100000 b00000000 b00000000,
        b00000000 b11100110 b11000000 b00000000 b00000000,
        b00000001 b11001101 b10000000 b00000000 b00000000,
        b00000011 b10011011 b00000000 b00000000 b00000000,
        b00000111 b00110110 b00000000 b00000000 b00000000,
        b00001110 b01101100 b00000000 b00000000 b00000000,
        b00011100 b11011000 b00000000 b00000000 b00000000,
        b00111001 b10110000 b00000000 b00000000 b00000000,
        b01110011 b01100000 b00000000 b00000000 b00000000,
        b11100110 b11000000 b00000000 b00000000 b00000000,
        b11001101 b10000000 b00000000 b00000000 b00000000,
        b10011011 b00000000 b00000000 b00000000 b00000000,
        b00110110 b00000000 b00000000 b00000000 b00000000,
        b01101100 b00000000 b00000000 b00000000 b00000000,
        b11011000 b00000000 b00000000 b00000000 b00000000,
        b10110000 b00000000 b00000000 b00000000 b00000000,
        b01100000 b00000000 b00000000 b00000000 b00000000,
        b11000000 b00000000 b00000000 b00000000 b00000000,
        b10000000 b00000000 b00000000 b00000000 b00000000,
        b00000000 b00000000 b00000000 b00000000 b00000000
    };

    char buffer[600];
    bs_stream_t stream = bs_create(data, sizeof data * 8 - 8, 0);
    for( size_t i = 0; i < sizeof data2 / sizeof data2[0]; ++i ){
        bs_seek( stream, i, BS_SEEK_SET );
        bs_read_bytes( stream, buffer, i + 5 );
        EXPECT_EQ( memcmp( buffer, data2[i], 5 ), 0 );

    }
    bs_destroy( stream );
}
