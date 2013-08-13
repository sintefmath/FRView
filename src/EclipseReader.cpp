/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <cstring>
#include <limits>
#include "Logger.hpp"
#include "PerfTimer.hpp"
#include "EclipseReader.hpp"
#ifdef __SSE2__
#include <xmmintrin.h>
#warning "Using SSE2 intrinsics"
#endif
#ifdef __SSSE3__
#include <tmmintrin.h>
#warning "Using SSSE3 intrinsics"
#endif

namespace Eclipse {

using std::string;
using std::vector;
using std::list;

// --- SSE2 Helpers ------------------------------------------------------------
#ifdef __SSE2__
static __m128i endianSwap32( const __m128i byte_mask, const __m128i value )
__attribute__((always_inline,const,pure));

/** Swap endianness of four 32-bit values stored in an SSE register. */

static __m128i endianSwap32( const __m128i byte_mask,
                             const __m128i value )
{
#ifdef __SSSE3__
    // SSSE3 has a byte-shuffle instruction which does everything.
    return _mm_shuffle_epi8( value, byte_mask );
#else
    // On SSE2, we swap bytes using masks and shifts, and then swap words using
    // shuffle on the high and low quadwords.
    __m128i u = _mm_srli_si128( _mm_and_si128( byte_mask, value ), 1 );
    __m128i l = _mm_and_si128( byte_mask, _mm_slli_si128( value, 1 ) );
    __m128i r = _mm_or_si128( u, l );
    r = _mm_shufflehi_epi16( r, _MM_SHUFFLE(2, 3, 0, 1) );
    r = _mm_shufflelo_epi16( r, _MM_SHUFFLE(2, 3, 0, 1) );
    return r;
#endif
}

/** Copies 20 floats from aligned src to aligned dst, finding min and max. */
template<unsigned int shift> static void copy32x20s( __m128i*           out,
                                                     __m128&            minimum,
                                                     __m128&            maximum,
                                                     const __m128i*     in,
                                                     const __m128i      bytemask,
                                                     const unsigned int qws5 )
__attribute__((always_inline,hot));

template<unsigned int shift> static void copy32x20s( __m128i*           out,
                                                     __m128&            minimum,
                                                     __m128&            maximum,
                                                     const __m128i*     in,
                                                     const __m128i      bytemask,
                                                     const unsigned int qws5 )
{
    const unsigned int rshift = 4*shift;
    const unsigned int lshift = 16-rshift;
    __m128i rem = endianSwap32( bytemask, _mm_srli_si128( _mm_load_si128( in++ ), rshift ) );

    for(size_t i=0; i<qws5; i++) {
        __m128i a1 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i a3 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i a5 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i a7 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i a9 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
//        _mm_prefetch( in+5, _MM_HINT_T0 );

        __m128i a0 = _mm_or_si128( rem,
                                   _mm_slli_si128( a1, lshift ) );
        __m128i a2 = _mm_or_si128( _mm_srli_si128( a1, rshift ),
                                   _mm_slli_si128( a3, lshift ) );
        __m128i a4 = _mm_or_si128( _mm_srli_si128( a3, rshift ),
                                   _mm_slli_si128( a5, lshift ) );
        __m128i a6 = _mm_or_si128( _mm_srli_si128( a5, rshift ),
                                   _mm_slli_si128( a7, lshift ) );
        __m128i a8 = _mm_or_si128( _mm_srli_si128( a7, rshift ),
                                   _mm_slli_si128( a9, lshift ) );

        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(a0) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(a2) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(a4) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(a6) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(a8) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(a0) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(a2) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(a4) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(a6) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(a8) );

        _mm_stream_si128( out++, a0 );
        _mm_stream_si128( out++, a2 );
        _mm_stream_si128( out++, a4 );
        _mm_stream_si128( out++, a6 );
        _mm_stream_si128( out++, a8 );
        rem = _mm_srli_si128( a9, rshift );
    }
}

template<> void copy32x20s<0>( __m128i* out,
                                __m128& minimum,
                                __m128& maximum,
                                const __m128i* in,
                                const __m128i bytemask,
                                const unsigned int qws5 )
{
    for(size_t i=0; i<qws5; i++) {
        __m128i in0 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i in1 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i in2 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i in3 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
        __m128i in4 = endianSwap32( bytemask, _mm_load_si128( in++ ) );
//        _mm_prefetch( in+5, _MM_HINT_T0 );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(in0) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(in1) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(in2) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(in3) );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(in4) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(in0) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(in1) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(in2) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(in3) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(in4) );
        _mm_stream_si128( out++, in0 );
        _mm_stream_si128( out++, in1 );
        _mm_stream_si128( out++, in2 );
        _mm_stream_si128( out++, in3 );
        _mm_stream_si128( out++, in4 );
    }
}
#endif
// -----------------------------------------------------------------------------



Reader::Reader(const std::string &path)
    : m_path(path),
      m_fd(-1)
{
    m_fd = open( m_path.c_str(), O_RDONLY );
    if( m_fd < 0 ) {
        string error(strerror(errno));
        cleanup();
        throw std::runtime_error( "open() failed: " + error );
    }
    struct stat finfo;
    if( fstat( m_fd, &finfo ) != 0 ) {
        string error(strerror(errno));
        cleanup();
        throw std::runtime_error( "fstat() failed: " + error );
    }
    m_filesize = finfo.st_size;

    m_pagesize = sysconf( _SC_PAGE_SIZE );

    //Logger log = getLogger( "Eclipse.Reader.Reader" );
    //LOGGER_DEBUG( log, "page size = " << m_pagesize << " bytes" );
}

Reader::~Reader()
{
    cleanup();
}

void
Reader::cleanup()
{
    if( m_fd >= 0 ) {
        close( m_fd );
        m_fd = -1;
    }
}

list<Block>
Reader::blocks()
{
    if( m_fd < 0 ) {
        throw std::runtime_error( "object in invalid state" );
    }

    Logger log = getLogger( "Eclipse.Reader.blocks" );
    list<Block> blocks;

    size_t offset = 4u;
    while( offset < m_filesize) {
//        LOGGER_DEBUG( log, "checking at " << offset );

        // Seek to payload of block header
        off_t o = lseek( m_fd, offset, SEEK_SET );
        if( o == -1 ) {
            string error(strerror(errno));
            cleanup();
            throw std::runtime_error( "lseek() failed: " + error );
        }
        if( o < 0 || ((size_t)o != offset ) ) {
            cleanup();
            throw std::runtime_error( "lseek() returned wrong offset" );
        }


        // Header:
        // - size of record (int, 4 bytes)
        // - chunk keyword (chars, 8 bytes)
        // - number of elements in block (int, 4 bytes)
        // - data type of elements in block (chars, 4 bytes)
        // - size of record repeated (int, 4 bytes)

        char head[16];
        ssize_t n = read( m_fd, head, 16 );
        if( n == -1 ) {
            string error(strerror(errno));
            cleanup();
            throw std::runtime_error( "read() failed: " + error );
        }
        else if( n != 16 ) {
            cleanup();
            throw std::runtime_error( "premature end of file" );
        }

        // Populate block
        Block block;
        memcpy( block.m_keyword_string, head, 8 );
        block.m_keyword = keyword( head );
        //if( block.m_keyword == KEYWORD_UNKNOWN ) {
        //    LOGGER_DEBUG( log, "'" << string( head, head+8 ) << "'" );
        //}
        block.m_count =
                0x01000000u*(unsigned int)(((unsigned char*)head)[8+0]) +
                0x00010000u*(unsigned int)(((unsigned char*)head)[8+1]) +
                0x00000100u*(unsigned int)(((unsigned char*)head)[8+2]) +
                0x00000001u*(unsigned int)(((unsigned char*)head)[8+3]);

        if( strncmp( "INTE", head+12, 4 ) == 0 ) {
            block.m_datatype    = TYPE_INTEGER;
            block.m_typesize    = 4;
            block.m_record_size = 1000;
        }
        else if( strncmp( "REAL", head+12, 4 ) == 0 ) {
            block.m_datatype    = TYPE_FLOAT;
            block.m_typesize    = 4;
            block.m_record_size = 1000;
        }
        else if( strncmp( "LOGI", head+12, 4 ) == 0) {
            block.m_datatype    = TYPE_BOOL;
            block.m_typesize    = 4;
            block.m_record_size = 1000;
        }
        else if( strncmp( "DOUB", head+12, 4  ) == 0 ) {
            block.m_datatype    = TYPE_DOUBLE;
            block.m_typesize    = 8;
            block.m_record_size = 1000;
        }
        else if( strncmp( "CHAR", head+12, 4  ) == 0 ) {
            block.m_datatype    = TYPE_STRING;
            block.m_typesize    = 8;
            block.m_record_size = 105;
        }
        else if( strncmp( "MESS", head+12, 4  ) == 0 ) {
            block.m_datatype    = TYPE_MESSAGE;
            block.m_typesize    = 8;
            block.m_record_size = 105;
        }
        else if( (strncmp( "C0", head+12, 2 ) == 0 ) &&
                 isdigit( head[ 14 ] ) &&
                 isdigit( head[ 15 ] ) )
        {
            block.m_datatype    = TYPE_STRING;
            block.m_typesize    = (head[14]-'0')*10 + (head[15]-'0');
            block.m_record_size = 105;
        }
        else {
            throw std::runtime_error( "unknown type '" + string( head+12, head+16 ) + "'" );
        }
        block.m_offset = offset + 16 + 4;
        block.m_records = (block.m_count + block.m_record_size - 1)/block.m_record_size;
        block.m_size = 8*block.m_records                // records head and tail
                     + block.m_count*block.m_typesize;  // block data contents

        offset += 24 + block.m_size;

//        LOGGER_DEBUG( log, "keyword=" << keywordString(block.m_keyword) << ", next at " << offset );

        blocks.push_back( block );

    }
    return blocks;
}



Reader::Map::Map(Reader& parent, const Block& block)
    : m_page( block.m_offset / parent.m_pagesize ),
      m_page_start( m_page * parent.m_pagesize ),
      m_page_offset( block.m_offset - m_page_start ),
      m_bytes_to_map( m_page_offset + block.m_size )
{
    static const std::string func = "Eclipse.Reader.Map.Map";

    if( parent.m_fd < 0 ) {
        throw std::runtime_error( func + ": Invalid file descriptor." );
    }

    m_map = static_cast<unsigned char*>( mmap( NULL,
                                               m_bytes_to_map,
                                               PROT_READ,
                                               MAP_PRIVATE | MAP_NORESERVE,
                                               parent.m_fd,
                                               m_page_start ) );
    if( m_map == MAP_FAILED ) {
        std::string error(strerror(errno));
        throw std::runtime_error( func  + ": mmap() failed: " + error );
    }

    // hint to the kernel that we will read this memory sequentially
    if( madvise( m_map, m_bytes_to_map, MADV_SEQUENTIAL ) != 0 ) {
        Logger log = getLogger( func );
        LOGGER_WARN( log, "madvice() failed: " << strerror(errno) );
    }
}

Reader::Map::~Map()
{
    static const std::string func = "Eclipse.Reader.Map.~Map";

    if( m_map != MAP_FAILED ) {
        if( munmap( m_map, m_bytes_to_map ) != 0 ) {
            std::string error( strerror( errno ) );
            throw std::runtime_error( func + "; munmap failed: " + error );
        }
    }
}





void
Reader::blockContent( std::vector<bool>& content, const Block& block )
{
    static const std::string func = "Eclipse.Reader.blockContent.bool";
    Logger log = getLogger( func );
    LOGGER_DEBUG( log, "invoked on " +  typeString(block.m_datatype) );
    try {
        if( block.m_datatype != TYPE_BOOL ) {
            throw std::runtime_error( func + ": Illegal block type: " + typeString(block.m_datatype) );
        }
        content.resize( block.m_count );
        Map map( *this, block );

        static const size_t  elements_per_record = 1000u;

        const unsigned int* src =  reinterpret_cast<const unsigned int*>( map.bytes() + 4 );   // first head
        size_t elements_left = block.m_count;

        size_t offset = 0u;
        for( unsigned int r=0; r<block.m_records; r++ ) {
            unsigned int n = std::min( elements_per_record, elements_left );
            for( unsigned int i=0; i<n; i++ ) {
                content[ offset++ ] = (*src++ != 0u);
            }
            src += 2; // skip record head and tail
            elements_left -= elements_per_record;
        }
    }
    catch( std::runtime_error& e ) {
        cleanup();
        throw e;
    }
}

void
Reader::blockContent( std::vector<std::string>& content, const Block& block )
{
    static const std::string func = "Eclipse.Reader.blockContent.string";
    Logger log = getLogger( func );
    LOGGER_DEBUG( log, "invoked on " +  typeString(block.m_datatype) );
    try {
        if( block.m_datatype != TYPE_STRING ) {
            throw std::runtime_error( func + ": Illegal block type: " + typeString(block.m_datatype) );
        }
        content.resize( block.m_count );
        Map map( *this, block );

        const unsigned char* src =  map.bytes() + 4;
        size_t elements_left = block.m_count;

        size_t offset = 0u;
        static const size_t  elements_per_record = 105u;
        for( unsigned int r=0; r<block.m_records; r++ ) {
            unsigned int n = std::min( elements_per_record, elements_left );
            for( unsigned int i=0; i<n; i++ ) {
                content[ offset++ ] = std::string( src, src+block.m_typesize );
                src += block.m_typesize;
            }
            src += 8;                               // skip record head and tail
            elements_left -= elements_per_record;
        }
    }
    catch( std::runtime_error& e ) {
        cleanup();
        throw e;
    }
}

void
Reader::blockContent( std::vector<int>& content, const Block& block )
{
    static_assert( sizeof(unsigned int) == 4, "uint is not 4 bytes" );

    static const std::string func = "Eclipse.Reader.blockContent.int";
    Logger log = getLogger( func );
    try {
        content.resize( block.m_count );

        if( block.m_datatype != TYPE_INTEGER ) {
            throw std::runtime_error( func + ": Illegal block type: " + typeString(block.m_datatype) );
        }

        Map map( *this, block );

        static const size_t  elements_per_record = 1000u;
        int* dst = content.data();
        const unsigned int* src =  reinterpret_cast<const unsigned int*>( map.bytes() + 4 );   // first head
        size_t elements_left = block.m_count;

        PerfTimer start;
        for( unsigned int r=0; r<block.m_records; r++ ) {
            unsigned int n = std::min( elements_per_record, elements_left );
            for( unsigned int i=0; i<n; i++ ) {
                union {
                    unsigned int    ui;
                    int             i;
                } v;
                v.ui = *src++;
#if LITTLE_ENDIAN
                v.ui = ((v.ui >> 16u)&0x0000ffffu) | ((v.ui << 16u)&0xffff0000u);
                v.ui = ((v.ui >> 8u )&0x00ff00ffu) | ((v.ui << 8u )&0xff00ff00u);
#endif
                *dst++ = v.i;
            }
            src += 2; // skip record head and tail
            elements_left -= elements_per_record;
        }
        PerfTimer stop;
        double dt = PerfTimer::delta( start, stop );
        LOGGER_DEBUG( log, "invoked on " << typeString(block.m_datatype)
                      << " (" << ((double)(block.m_count*sizeof(float))/dt)/(1024*1024) << " mb/s)" );

    }
    catch( std::runtime_error& e ) {
        cleanup();
        throw e;
    }
}





void
Reader::blockContent( float*  __attribute__((aligned(16))) content,
                      float&                               minimum,
                      float&                               maximum,
                      const Block&                         block )
{
    static_assert( sizeof(unsigned int) == 4, "uint is not 4 bytes" );
    static_assert( sizeof(float) == 4,        "float is not 4 bytes" );

    static const std::string func = "Eclipse.Reader.blockContent.float.minmax";
    Logger log = getLogger( func );
    if( block.m_count == 0 ) {
        LOGGER_DEBUG( log, "invoked on " +  typeString(block.m_datatype) );
        return;
    }

    try {
        Map map( *this, block );
        if( block.m_datatype == TYPE_FLOAT ) {
            static const size_t  elements_per_record = 1000u;

            PerfTimer start;

#ifdef __SSE2__
            // sse2-optimized copy loop ----------------------------------------
            __m128  min = _mm_set1_ps(  std::numeric_limits<float>::max() );
            __m128  max = _mm_set1_ps( -std::numeric_limits<float>::max() );
#ifdef __SSSE3__
            static const __m128i bytemask = _mm_set_epi8( 12, 13, 14, 15,
                                                          8, 9, 10, 11,
                                                          4, 5, 6, 7,
                                                          0, 1, 2, 3 );
#else
            static const __m128i bytemask = _mm_set1_epi32( 0xff00ff00u );
#endif
            __m128i* out = reinterpret_cast<__m128i*>( content );
            size_t N = block.m_count;
            unsigned int records = (N + elements_per_record - 1u)/elements_per_record;
            const float* in_u = reinterpret_cast<const float*>( map.bytes() + 4 );
            for(unsigned int r=0; r<records; r++ ) {
                unsigned int shift = (reinterpret_cast<unsigned long int>( in_u )>>2)&0x3;
                const __m128i* in = reinterpret_cast<const __m128i*>( in_u - shift );
                unsigned int chunks = (r+1<records) ? elements_per_record/(4*5) : N/(4*5);
                switch( shift ) {
                case 0: copy32x20s<0>( out, min, max, in, bytemask, chunks ); break;
                case 1: copy32x20s<1>( out, min, max, in, bytemask, chunks ); break;
                case 2: copy32x20s<2>( out, min, max, in, bytemask, chunks ); break;
                case 3: copy32x20s<3>( out, min, max, in, bytemask, chunks ); break;
                }
                out += 5*chunks;
                in_u += 4*5*chunks+2;
                N -= 4*5*chunks;
            }
            if( N > 0 ) {   // have trailing quad-floats
                in_u -= 2;
                for( size_t i=0; i<N/4; i++ ) {
                    __m128i a = _mm_loadu_si128( reinterpret_cast<const __m128i*>( in_u ) );
                    a = endianSwap32( bytemask, a );
                    min = _mm_min_ps( min, _mm_castsi128_ps(a) );
                    max = _mm_max_ps( max, _mm_castsi128_ps(a) );
                    _mm_stream_si128( out++, a );
                    in_u += 4;
                }
                size_t tail_n = N & 3;
                if( tail_n ) {  // have trailing floats
                    __m128i a = _mm_loadu_si128( reinterpret_cast<const __m128i*>( in_u ) );
                    a = endianSwap32( bytemask, a );
                    switch( tail_n ) {
                    case 1: a = _mm_shuffle_epi32( a, _MM_SHUFFLE(0,0,0,0)); break;
                    case 2: a = _mm_shuffle_epi32( a, _MM_SHUFFLE(1,1,1,0)); break;
                    case 3: a = _mm_shuffle_epi32( a, _MM_SHUFFLE(2,2,1,0)); break;
                    }
                    min = _mm_min_ps( min, _mm_castsi128_ps(a) );
                    max = _mm_max_ps( max, _mm_castsi128_ps(a) );
                    _mm_stream_si128( out++, a );
                }
            }
            min = _mm_min_ps( _mm_shuffle_ps( min, min, _MM_SHUFFLE( 2, 3, 0, 1) ), min );
            min = _mm_min_ss( _mm_shuffle_ps( min, min, _MM_SHUFFLE( 2, 2, 2, 2) ), min );
            _mm_store_ss( &minimum, min );
            max = _mm_max_ps( _mm_shuffle_ps( max, max, _MM_SHUFFLE( 2, 3, 0, 1) ), max );
            max = _mm_max_ss( _mm_shuffle_ps( max, max, _MM_SHUFFLE( 2, 2, 2, 2) ), max );
            _mm_store_ss( &maximum, max );

            if( 0 ) {
                const unsigned int* src =  reinterpret_cast<const unsigned int*>( map.bytes() + 4 );
                float* dst = content;
                size_t elements_left = block.m_count;
                float min =  std::numeric_limits<float>::max();
                float max = -std::numeric_limits<float>::max();
                for( unsigned int r=0; r<block.m_records; r++ ) {
                    unsigned int n = std::min( elements_per_record, elements_left );
                    for( unsigned int i=0; i<n; i++ ) {
                        union {
                            unsigned int    ui;
                            float           f;
                        } v;
                        v.ui = *src++;
                        v.ui = ((v.ui >> 16u)&0x0000ffffu) | ((v.ui << 16u)&0xffff0000u);
                        v.ui = ((v.ui >> 8u )&0x00ff00ffu) | ((v.ui << 8u )&0xff00ff00u);
                        if( *dst++ != v.f ) {
                            LOGGER_ERROR( log, "Failed at r=" << r << ", i=" << i );
                            break;
                        }
                        min = v.f < min ? v.f : min;
                        max = max < v.f ? v.f : max;
                    }
                    src += 2; // skip record head and tail
                    elements_left -= elements_per_record;
                }
                if( minimum != min ) {
                    LOGGER_ERROR( log, "min is wrong" );
                }
                if( maximum != max ) {
                    LOGGER_ERROR( log, "max is wrong" );
                }
                LOGGER_DEBUG( log, "Checked output." );
            }
#else
            // non-specialized copy loop ---------------------------------------
            const unsigned int* src =  reinterpret_cast<const unsigned int*>( map.bytes() + 4 );
            float* dst = content;
            size_t elements_left = block.m_count;
            float min =  std::numeric_limits<float>::max();
            float max = -std::numeric_limits<float>::max();
            for( unsigned int r=0; r<block.m_records; r++ ) {
                unsigned int n = std::min( elements_per_record, elements_left );
                for( unsigned int i=0; i<n; i++ ) {
                    union {
                        unsigned int    ui;
                        float           f;
                    } v;
                    v.ui = *src++;
#if LITTLE_ENDIAN
                    v.ui = ((v.ui >> 16u)&0x0000ffffu) | ((v.ui << 16u)&0xffff0000u);
                    v.ui = ((v.ui >> 8u )&0x00ff00ffu) | ((v.ui << 8u )&0xff00ff00u);
#endif
                    *dst++ = v.f;
                    min = v.f < min ? v.f : min;
                    max = max < v.f ? v.f : max;
                }
                src += 2; // skip record head and tail
                elements_left -= elements_per_record;
            }
            minimum = min;
            maximum = max;
#endif
            PerfTimer stop;
            double dt = PerfTimer::delta( start, stop );
            LOGGER_DEBUG( log, "invoked on " << typeString(block.m_datatype)
                          << " (" << ((double)(block.m_count*sizeof(float))/dt)/(1024*1024) << " mb/s)" );
        }
        else {
            throw std::runtime_error( func + ": Illegal block type: " + typeString(block.m_datatype) );
        }
    }
    catch( std::runtime_error& e ) {
        cleanup();
        throw e;
    }

}


void
Reader::blockContent( std::vector<float>& content, const Block& block )
{
    static_assert( sizeof(unsigned int) == 4,      "unsigned int is not 4 bytes" );
    static_assert( sizeof(float) == 4,             "float is not 4 bytes" );
    static_assert( sizeof(double) == 8,            "double is not 8 bytes" );

    static const std::string func = "Eclipse.Reader.blockContent.float";
    Logger log = getLogger( func );
    LOGGER_DEBUG( log, "invoked on " +  typeString(block.m_datatype) );
    try {
        content.resize( block.m_count );

        Map map( *this, block );
        static const size_t  elements_per_record = 1000u;
        float* dst = content.data();
        size_t elements_left = block.m_count;

        const unsigned int* src =  reinterpret_cast<const unsigned int*>( map.bytes() + 4 );   // first head
        if( block.m_datatype == TYPE_FLOAT ) {
            for( unsigned int r=0; r<block.m_records; r++ ) {
                unsigned int n = std::min( elements_per_record, elements_left );
                for( unsigned int i=0; i<n; i++ ) {
                    union {
                        unsigned int    ui;
                        float           f;
                    } v;
                    v.ui = *src++;
#if LITTLE_ENDIAN
                    v.ui = ((v.ui >> 16u)&0x0000ffffu) | ((v.ui << 16u)&0xffff0000u);
                    v.ui = ((v.ui >> 8u )&0x00ff00ffu) | ((v.ui << 8u )&0xff00ff00u);
#endif
                    *dst++ = v.f;
                }
                src += 2; // skip record head and tail
                elements_left -= elements_per_record;
            }
        }

        else if( block.m_datatype == TYPE_DOUBLE ) {
            for( unsigned int r=0; r<block.m_records; r++ ) {
                unsigned int n = std::min( elements_per_record, elements_left );
                for( unsigned int i=0; i<n; i++ ) {
                    union {
                        unsigned int    ui[2];
                        double          d;
                    } v;
                    v.ui[0] = *src++;
                    v.ui[1] = *src++;
#if LITTLE_ENDIAN
                    unsigned int t = v.ui[0];
                    v.ui[0] = v.ui[1];
                    v.ui[1] = t;
                    v.ui[0] = ((v.ui[0] >> 16u)&0x0000ffffu) | ((v.ui[0] << 16u)&0xffff0000u);
                    v.ui[1] = ((v.ui[1] >> 16u)&0x0000ffffu) | ((v.ui[1] << 16u)&0xffff0000u);
                    v.ui[0] = ((v.ui[0] >> 8u )&0x00ff00ffu) | ((v.ui[0] << 8u )&0xff00ff00u);
                    v.ui[1] = ((v.ui[1] >> 8u )&0x00ff00ffu) | ((v.ui[1] << 8u )&0xff00ff00u);
#endif
                    *dst++ = v.d;
                }
                src += 2; // skip record head and tail
                elements_left -= elements_per_record;
            }
        }

        else {
            throw std::runtime_error( func + ": Illegal block type: " + typeString(block.m_datatype) );
        }
    }
    catch( std::runtime_error& e ) {
        cleanup();
        throw e;
    }
}

void
Reader::blockContent( std::vector<double>& content, const Block& block )
{
    static const std::string func = "Eclipse.Reader.blockContent.double";
    Logger log = getLogger( func );
    LOGGER_DEBUG( log, "invoked on " +  typeString(block.m_datatype) );

    try {

        content.resize( block.m_count );

        Map map( *this, block );
        static const size_t  elements_per_record = 1000u;
        double* dst = content.data();
        size_t elements_left = block.m_count;

        const unsigned int* src =  reinterpret_cast<const unsigned int*>( map.bytes() + 4 );   // first head
        if( block.m_datatype == TYPE_FLOAT ) {
            for( unsigned int r=0; r<block.m_records; r++ ) {
                unsigned int n = std::min( elements_per_record, elements_left );
                for( unsigned int i=0; i<n; i++ ) {
                    union {
                        unsigned int    ui;
                        float           f;
                    } v;
                    v.ui = *src++;
#if LITTLE_ENDIAN
                    v.ui = ((v.ui >> 16u)&0x0000ffffu) | ((v.ui << 16u)&0xffff0000u);
                    v.ui = ((v.ui >> 8u )&0x00ff00ffu) | ((v.ui << 8u )&0xff00ff00u);
#endif
                    *dst++ = v.f;
                }
                src += 2; // skip record head and tail
                elements_left -= elements_per_record;
            }
        }

        else if( block.m_datatype == TYPE_DOUBLE ) {
            for( unsigned int r=0; r<block.m_records; r++ ) {
                unsigned int n = std::min( elements_per_record, elements_left );
                for( unsigned int i=0; i<n; i++ ) {
                    union {
                        unsigned int    ui[2];
                        double          d;
                    } v;
                    v.ui[0] = *src++;
                    v.ui[1] = *src++;
#if LITTLE_ENDIAN
                    unsigned int t = v.ui[0];
                    v.ui[0] = v.ui[1];
                    v.ui[1] = t;
                    v.ui[0] = ((v.ui[0] >> 16u)&0x0000ffffu) | ((v.ui[0] << 16u)&0xffff0000u);
                    v.ui[1] = ((v.ui[1] >> 16u)&0x0000ffffu) | ((v.ui[1] << 16u)&0xffff0000u);
                    v.ui[0] = ((v.ui[0] >> 8u )&0x00ff00ffu) | ((v.ui[0] << 8u )&0xff00ff00u);
                    v.ui[1] = ((v.ui[1] >> 8u )&0x00ff00ffu) | ((v.ui[1] << 8u )&0xff00ff00u);
#endif
                    *dst++ = v.d;
                }
                src += 2; // skip record head and tail
                elements_left -= elements_per_record;
            }
        }

        else {
            throw std::runtime_error( func + ": Illegal block type: " + typeString(block.m_datatype) );
        }
    }
    catch( std::runtime_error& e ) {
        cleanup();
        throw e;
    }
}


// Format notes:
// w: number of positions used
// m: minimum number of positions used
// d: number of digits right of the decimal point.
// e: number of digits in the exponent part

// Iw Iw.m          Integers
// Fw.d             Real, decimal form
// Ew.d Ew.dEe      Real, exponential form
// ESw.d ESw.dEe    Real, scientific form
// ENw.d ENw.dEe    Real, engineering form
// Lw               Logicals
// A Aw             Characters
// nX               Horizontal positioning
// Tc TLc TRc       Tabbing
// /                Vertical
// r(...)           Grouping
// :                Format scanning control
// S SP SS          Sign control
// BN BZ            Blank control

// Header: (1X, 1X, A8, 1X, 1X, I11, 1X, 1X, A4)
// "  AAAAAAAA  DDDDDDDDDDD  AAAA"




} // of namespace Eclipse
