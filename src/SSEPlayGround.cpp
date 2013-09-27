/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <malloc.h>
#include <iostream>
#include <string.h>
#include <limits>
#include <time.h>
#include <xmmintrin.h>


static const int prefetch = 32;

static
__m128i
endianSwap32( const __m128i byte_mask, const __m128i value ) __attribute__((always_inline,const,pure));

static
__m128i
endianSwap32( const __m128i byte_mask, const __m128i value )
{
    __m128i u = _mm_srli_si128( _mm_and_si128( byte_mask, value ), 1 );
    __m128i l = _mm_and_si128( byte_mask, _mm_slli_si128( value, 1 ) );
    __m128i r = _mm_or_si128( u, l );
    r = _mm_shufflehi_epi16( r, _MM_SHUFFLE(2, 3, 0, 1) );
    r = _mm_shufflelo_epi16( r, _MM_SHUFFLE(2, 3, 0, 1) );
    return r;
}

template<unsigned int shift>
static
__m128i
copy32s( __m128i* out,
         __m128i* in,
         __m128& minimum,
         __m128& maximum,
         const __m128i bytemask,
         const unsigned int qws,
         const bool tail ) __attribute__((always_inline));

template<unsigned int shift>
static
__m128i
copy32s( __m128i* out,
         __m128i* in,
         __m128& minimum,
         __m128& maximum,
         const __m128i bytemask,
         const unsigned int qws,
         const bool tail )
{
    const unsigned int rshift = 4*shift;
    const unsigned int lshift = 16-rshift;
    __m128i curr;

    curr = _mm_load_si128( in++ );
    curr = _mm_srli_si128( curr, rshift );
    for(size_t i=0; i<qws; i++) {
        _mm_prefetch( in + prefetch, _MM_HINT_T0 );
        __m128i next = _mm_load_si128( in++ );
        curr = _mm_or_si128( curr, _mm_slli_si128( next, lshift ) );

        curr = endianSwap32( bytemask, curr );
        _mm_stream_si128( out++, curr );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(curr) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(curr) );
        curr = _mm_srli_si128( next, rshift );
    }
    // have 32 bits left, get more if tail is 2 or 3
    if( tail ) {
        curr = _mm_or_si128( curr,
                             _mm_slli_si128( _mm_load_si128( in++ ),
                                             lshift ) );
    }
    return curr;
}

template<>
__m128i
copy32s<0>( __m128i* out,
            __m128i* in,
            __m128& minimum,
            __m128& maximum,
            const __m128i bytemask,
            const unsigned int qws,
            const bool tail )
{
    __m128i curr;
    for(size_t i=0; i<qws; i++) {
        _mm_prefetch( in + prefetch, _MM_HINT_T0 );
        curr = _mm_load_si128( in++ );
        curr = endianSwap32( bytemask, curr );
        _mm_stream_si128( out++, curr );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(curr) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(curr) );
    }
    // have 0 bits left, get 128 if any is needed
    if( tail ) {
        curr = _mm_load_si128( in++ );
    }
    return curr;
}


template<unsigned int shift>
static
void
copy32sFull( __m128i* out,
         __m128i* in,
         __m128& minimum,
         __m128& maximum,
         const __m128i bytemask,
         const unsigned int qws ) __attribute__((always_inline,hot));

template<unsigned int shift>
static
void
copy32sFull( __m128i* out,
         __m128i* in,
         __m128& minimum,
         __m128& maximum,
         const __m128i bytemask,
         const unsigned int qws )
{
    const unsigned int rshift = 4*shift;
    const unsigned int lshift = 16-rshift;
    __m128i curr = _mm_srli_si128( _mm_load_si128( in++ ), rshift );
    for(size_t i=0; i<qws/2; i++) {
        _mm_prefetch( in + prefetch, _MM_HINT_T0 );
        __m128i next = _mm_load_si128( in++ );
        curr = _mm_or_si128( curr, _mm_slli_si128( next, lshift ) );

        curr = endianSwap32( bytemask, curr );
        _mm_stream_si128( out++, curr );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(curr) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(curr) );
        curr = _mm_srli_si128( next, rshift );

        _mm_prefetch( in + prefetch, _MM_HINT_T0 );
        next = _mm_load_si128( in++ );
        curr = _mm_or_si128( curr, _mm_slli_si128( next, lshift ) );

        curr = endianSwap32( bytemask, curr );
        _mm_stream_si128( out++, curr );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(curr) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(curr) );
        curr = _mm_srli_si128( next, rshift );
    }
}

template<>
void
copy32sFull<0>( __m128i* out,
            __m128i* in,
            __m128& minimum,
            __m128& maximum,
            const __m128i bytemask,
            const unsigned int qws )
{
    static const int prefetch = 32;
    __m128i curr;
    for(size_t i=0; i<qws/2; i++) {
        _mm_prefetch( in + prefetch, _MM_HINT_T0 );
        curr = _mm_load_si128( in++ );
        curr = endianSwap32( bytemask, curr );
        _mm_stream_si128( out++, curr );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(curr) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(curr) );

        _mm_prefetch( in + prefetch, _MM_HINT_T0 );
        curr = _mm_load_si128( in++ );
        curr = endianSwap32( bytemask, curr );
        _mm_stream_si128( out++, curr );
        minimum = _mm_min_ps( minimum, _mm_castsi128_ps(curr) );
        maximum = _mm_max_ps( maximum, _mm_castsi128_ps(curr) );
    }
}



void
foobar( float* src, float* dst, float& min, float& max, size_t count )
{
    __m128i bytemask = _mm_set1_epi32( 0xff00ff00u );
    __m128  minimum = _mm_set1_ps(  std::numeric_limits<float>::max() );
    __m128  maximum = _mm_set1_ps( -std::numeric_limits<float>::max() );

    // process full records. Max size of records is 1000 elements, which is
    // 250 quad floats. Since we process full quadwords, we do not need to care
    // about any tails.
    unsigned int full_records = count/1000;
    for(unsigned int i=0; i<full_records; i++ ) {
        float* src_r = src + 1000*i;
        const unsigned int shift = ((unsigned long int)src_r & 0xfu)/(sizeof(unsigned int));
        __m128i* in = (__m128i*)(src_r - shift);
        __m128i* out = (__m128i*)(dst+1000*i);
        switch( shift ) {
        case 0:
            copy32sFull<0>( out, in, minimum, maximum, bytemask, 250 );
            break;
        case 1:
            copy32sFull<1>( out, in, minimum, maximum, bytemask, 250 );
            break;
        case 2:
            copy32sFull<2>( out, in, minimum, maximum, bytemask, 250 );
            break;
        case 3:
            copy32sFull<3>( out, in, minimum, maximum, bytemask, 250 );
            break;
        }
    }

    // Process the last partial record (if required). This might have a tail.
    size_t remaining = count - full_records*1000ul;
    if( remaining != 0 ) {
        float* src_r = src + 1000*full_records;
        const unsigned int shift = ((unsigned long int)src_r & 0xfu)/(sizeof(unsigned int));
        __m128i* in = (__m128i*)(src_r - shift);
        __m128i* out = (__m128i*)(dst+1000*full_records);

        unsigned int tailN = remaining&0x3;
        size_t qws = remaining/4u;

        __m128i tail;
        switch( shift ) {
        case 0:
            tail = copy32s<0>( out, in, minimum, maximum, bytemask, qws, tailN > 0 );
            break;
        case 1:
            tail = copy32s<1>( out, in, minimum, maximum, bytemask, qws, false );
            break;
        case 2:
            tail = copy32s<2>( out, in, minimum, maximum, bytemask, qws, tailN > 2 );
            break;
        case 3:
            tail = copy32s<3>( out, in, minimum, maximum, bytemask, qws, tailN > 1 );
            break;
        }
        if( tailN ) {
            __m128i* out = (__m128i*)(dst+1000*full_records+4*qws);
            tail = endianSwap32( bytemask, tail );
            switch( tailN ) {
            case 1:
                tail = _mm_shuffle_epi32( tail, _MM_SHUFFLE(0,0,0,0));
                break;
            case 2:
                tail = _mm_shuffle_epi32( tail, _MM_SHUFFLE(1,1,1,0));
                break;
            case 3:
                tail = _mm_shuffle_epi32( tail, _MM_SHUFFLE(2,2,1,0));
                break;
            }
            _mm_store_si128( out++, tail );
            minimum = _mm_min_ps( minimum, _mm_castsi128_ps(tail) );
            maximum = _mm_max_ps( maximum, _mm_castsi128_ps(tail) );
        }
    }


    // Reduce and store
    minimum = _mm_min_ps( _mm_shuffle_ps( minimum, minimum, _MM_SHUFFLE( 2, 3, 0, 1) ), minimum );
    minimum = _mm_min_ss( _mm_shuffle_ps( minimum, minimum, _MM_SHUFFLE( 2, 2, 2, 2) ), minimum );
    _mm_store_ss( &min, minimum );

    maximum = _mm_max_ps( _mm_shuffle_ps( maximum, maximum, _MM_SHUFFLE( 2, 3, 0, 1) ), maximum );
    maximum = _mm_max_ss( _mm_shuffle_ps( maximum, maximum, _MM_SHUFFLE( 2, 2, 2, 2) ), maximum );
    _mm_store_ss( &max, maximum );
}




int
main( int argc, char** argv )
{
#ifdef __SSE2__
    std::cerr << "compiled with sse2.\n";
#endif

    unsigned int N = 1024*1024*1;



    float* src = (float*)malloc( sizeof(float)*2*N );
    for( unsigned int i=0; i<N+16; i++ ) {
        union {
            unsigned char b[4];
            unsigned int i;
            float f;
        } v;
        v.f = i;
        std::swap( v.b[1], v.b[2] );
        std::swap( v.b[0], v.b[3] );
        src[i] = v.f;
    }

    float* __attribute__((aligned(16))) dst = (float*)memalign( 16,  sizeof(float)*2*N );


    for(unsigned int j=0; j<4; j++ ) {
        for(unsigned int i=0; i<4; i++ ) {
            memset( dst, ~0u, sizeof(float)*2*N );

            float min, max;

            timespec start, stop;

            clock_gettime( CLOCK_MONOTONIC, &start );
            foobar( src+i, dst, min, max, N+j );
            clock_gettime( CLOCK_MONOTONIC, &stop );

            bool fail = false;
            for(unsigned int k=0; k<N+j; k++) {
                fail = fail || (dst[k] != (k+i) );
                if( fail ) {
                    std::cerr << "failed at k=" << k << "\n";
                    break;
                }
            }
            if( min != i ) {
                std::cerr << "fail: min=" << (int)min << ", should be " << i << "\n";
                fail = true;
            }
            if( max != i+j+N-1 ) {
                std::cerr << "fail: max=" << (int)max << ", should be " << (i+j+N-1) << "\n";
                fail = true;
            }


            timespec delta;
            if( (stop.tv_nsec-start.tv_nsec) < 0 ) {
                delta.tv_sec = stop.tv_sec - start.tv_sec - 1;
                delta.tv_nsec = 1000000000 + stop.tv_nsec - start.tv_nsec;
            }
            else {
                delta.tv_sec = stop.tv_sec - start.tv_sec;
                delta.tv_nsec = stop.tv_nsec - start.tv_nsec;
            }
            double dt = delta.tv_sec + 1e-9*delta.tv_nsec;
            double mbs = ((double)(N*sizeof(float))/dt)/(1024*1024);


            std::cerr << "shift=" << i
                      << ", tail=" << j
                      << ", " << dt << " secs"
                      << "(" << mbs << ") mb/s, "
                      << (fail?"FAIL":"SUCCESS") << "\n";


        }
    }





    return 0;
}
