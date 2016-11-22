#ifndef BS_BITSTREAM_H
#define BS_BITSTREAM_H
#include <string.h>

/*! \addtogroup bitstream Bitstream
    @{
*/

/*! \struct bs_stream_t
    \brief  An object used to read data from a buffer bit by bit.
*/
typedef struct bs_stream * bs_stream_t;

/*! \enum   bs_stream_t
    \brief  Used to drive seek operations inside of a bitstream.
*/
typedef enum{
    BS_SEEK_SET,    //!< Seek to the absolute position provided by the offset relative to the start of the stream.
    BS_SEEK_OFFSET, //!< Seek to a position relative to the current offset.
    BS_SEEK_END     //!< Seek to the absolute position provided by the offset relative to the end of the stream.
} bs_seek_t;

/*! @} */

/*! \brief      Creates a bitstream backed by \a buffer.
    \param      buffer      The buffer which holds the data to access.
    \param      len_bits    The length of the buffer in *bits*
    \param      start_pos   \parblock
                            The offset to start at in the stream.

                            A positive offset will begin the stream that many bits into the stream from the start.

                            A negative offset will begin the stream that many bits back from the end of the stream.

                            If `abs(start_pos)` is greater than \a len_bits, the result is undefined.
                            \endparblock
    \return     A bitstream backed by \a buffer.
    \memberof   bs_stream_t
*/
bs_stream_t bs_create( const void * buffer, size_t len_bits, int start_pos );

/*! \brief      Destroys a bitstream.
    \param      stream  The bitstream to destroy.
    \noreturn
    \memberof   bs_stream_t
*/
void bs_destroy( bs_stream_t stream );

/*! \brief      Returns the current read offset into the bitstream.
    \param      stream  The bitstream to check.
    \return     The current read offset in the bitstream.
    \memberof   bs_stream_t
*/
size_t bs_tell( bs_stream_t stream );

/*! \brief      Returns the remaining number of bits in the stream.
    \param      stream  The bitstream to check.
    \return     The remaining number of bits in the stream.
    \memberof   bs_stream_t
*/
size_t bs_remaining( bs_stream_t stream );

/*! \brief      Moves the read offset inside of a bitstream.
    \param      stream  The bitstream to seek in.
    \param      offset  The distance to move the current offset.
    \param      whence  The position to apply the offset relative to.
    \remarks    Seeking to a position outside of the stream will result in the offset being clamped to the
                nearest valid index (seeking to offset -10 will result in moving to offset 0, seeking to
                end + 10 will result in moving to end).
    \noreturn
    \memberof   bs_stream_t
*/
void bs_seek( bs_stream_t stream, int offset, bs_seek_t whence );

/*! \brief      Reads an unsigned integer from the bitstream.
    \param      stream  The bitstream to read from.
    \param      bits    The number of bits to read. Undefined for bits > 8 * sizeof( unsigned int ).
    \return     Returns the value of the bits which were read as an unsigned integer.
    \remarks    Attempting to read more bits than remain in the stream will zero-fill the least significant
                bits in the returned number.
    \memberof   bs_stream_t
*/
unsigned int bs_read_uint( bs_stream_t stream, size_t bits );

/*! \brief      Reads a 2's complement signed integer from the bitstream.
    \param      stream  The bitstream to read from.
    \param      bits    The number of bits to read. Undefined for bits > 8 * sizeof( int ).
    \return     Returns the value of the bits which were read as a 2's complement signed integer.
    \remarks    Attempting to read more bits than remain in the stream will zero-fill the least significant
                bits in the returned number.
    \memberof   bs_stream_t
*/
int bs_read_int( bs_stream_t stream, size_t bits );

/*! \brief      Reads a number encoded with Exponential-Golomb coding from the bitstream.
    \param      stream  The bitstream to read from.
    \return     Returns the value of the bits which were read as a Exponential-Golomb coded number.
    \remarks    Attempting to read more bits than remain in the stream will result in a return value of (size_t) -1;
    \memberof   bs_stream_t
*/
unsigned int bs_read_exp_golomb( bs_stream_t stream );


#endif
