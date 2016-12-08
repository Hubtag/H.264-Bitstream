#include "bitstream.h"
#include <stdlib.h>

#ifdef _cplusplus
extern "C"{
#endif

struct bs_stream{
    const unsigned char * data;
    size_t offset;
    size_t length;
};

bs_stream_t bs_create( const void * buffer, size_t len_bits, int start_pos ){
    bs_stream_t ret = (bs_stream_t) malloc(sizeof *ret);
    ret->data = (const unsigned char *)buffer;
    if( start_pos >= 0 ){
        ret->offset = start_pos;
    }
    else{
        ret->offset = len_bits - -start_pos;
    }
    ret->length = len_bits;
    return ret;
}

void bs_destroy( bs_stream_t stream ){
    if( stream ){
        free(stream);
    }
}

size_t bs_tell( bs_stream_t stream ){
    return stream->offset;
}

size_t bs_remaining( bs_stream_t stream ){
    return stream->length - stream->offset;
}

void bs_seek( bs_stream_t stream, int offset, bs_seek_t whence ){

    switch(whence){
        case BS_SEEK_OFFSET:
            if( offset < 0 ){
                if( stream->offset > (size_t) -offset ){
                    stream->offset += offset;
                }
                else{
                    stream->offset = 0;
                }
            }
            else{
                if( stream->offset + offset < stream->length ){
                    stream->offset += offset;
                }
                else{
                    stream->offset = stream->length;
                }
            }
            break;
        case BS_SEEK_END:
            if( offset < 0 ){
                offset = 0;
            }
            if( stream->length > (size_t) offset ){
                stream->offset = stream->length - offset;
            }
            else{
                stream->offset = 0;
            }
            break;
        case BS_SEEK_SET:
            if( offset < 0 ){
                offset = 0;
            }
            if( stream->length > (size_t) offset ){
                stream->offset = offset;
            }
            else{
                stream->offset = 0;
            }
            break;
        default: break;
    }
}

unsigned int bs_read_bit( bs_stream_t stream ){
    size_t byte = stream->offset / 8;
    size_t bit = stream->offset % 8;
    if( stream->offset < stream->length ){
        stream->offset++;
        return ((stream->data[byte] >> ( 7 - bit )) & 1) != 0;
    }
    return 0;
}

unsigned int bs_read_uint( bs_stream_t stream, size_t bits ){
    size_t byte = stream->offset / 8;
    size_t bit = stream->offset % 8;
    size_t rounded_bits = 8 - (bits + bit) % 8;
    if( rounded_bits == 8 ){
        rounded_bits = 0;
    }
    rounded_bits += bits;
    unsigned int ret = 0;
    // Fill the MSBs with the next bits to read
    ret = (stream->data[byte] << bit) & 0xFF;
    ret >>= bit;
    // While we're byte aligned, push whole bytes
    size_t i;
    for( i = 1; i * 8 + 8 - bit <= rounded_bits && (i + byte) * 8 < stream->length; i++ ){
        ret <<= 8;
        ret += stream->data[byte + i];
    }
    if( byte * 8 + bit + bits >= stream->length ){
        ret <<= bits - i * 8 + bit;
    }
    else{
        ret >>= i * 8 + 8 - bit - bits - 8;
    }
    stream->offset += bits;
    if( stream->offset > stream->length ){
        stream->offset = stream->length;
    }
    return ret;
}

int bs_read_int( bs_stream_t stream, size_t bits ){
    unsigned int ret = bs_read_uint( stream, bits );
    unsigned int mask = (1 << bits) - 1;
    if( (ret & mask) != 0 ){
        return -(ret & (mask - 1));
    }
    return ret;
}

unsigned int bs_read_exp_golomb( bs_stream_t stream ){
    size_t byte = stream->offset / 8;
    size_t bit = stream->offset % 8;
    size_t len = 0;
    unsigned int test = 0;
    // Fill the MSBs with the next bits to read
    test = (stream->data[byte] << bit) & 0xFF;
    while( test == 0 && ++byte * 8 < stream->length ){
        len += 8-bit;
        bit = 0;
        test = stream->data[byte];
    }
    if( byte * 8 > stream->length ){
        stream->offset = stream->length;
        return (unsigned int)-1;
    }

    size_t log2 = 8;
    while( test > 0 && log2 > 0 ){
        test >>= 1;
        log2--;
    }
    len += log2;
    if( len + stream->offset > stream->length ){
        stream->offset = stream->length;
        return (unsigned int)-1;
    }
    stream->offset += len;
    if( stream->offset > stream->length ){
        stream->offset = stream->length;
    }
    return bs_read_uint( stream, len + 1 ) - 1;
}

size_t bs_read_bytes( bs_stream_t stream, void * dest, size_t len ){
    unsigned char * out = (unsigned char *) dest;
    size_t byte = stream->offset / 8;
    size_t bit = stream->offset % 8;
    size_t read_len = len;
    memset( dest, 0, len );
    if( stream->offset / 8 + read_len > stream->length / 8 ){
        read_len = stream->length / 8 - stream->offset / 8;
    }
    memcpy( out, stream->data + byte, read_len );
    //If we're not byte aligned, we need to shift everything to become byte aligned.
    if( bit != 0 ){
        //Shift everything over
        size_t mask = ( 1 << bit ) - 1;
        for( size_t i = 0; i + 1 < read_len; ++i ){
            out[i] = (out[i] << bit) | ((out[i + 1] >> (8 - bit)) & mask );
        }
        //Get the last byte
        unsigned char last = 0;
        if( stream->offset + read_len * 8 < stream->length ){
            last = stream->data[byte + read_len];
        }
        //Fill it in
        if( read_len > 0 ){
            out[read_len - 1] = (out[read_len - 1] << bit) | ((last >> (8 - bit)) & mask );
        }
        else if( len > 0 ){
            out[0] = last << bit ;
        }
    }
    return read_len;
}

#ifdef _cplusplus
}
#endif
