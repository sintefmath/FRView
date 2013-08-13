#include <iostream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <cstring>
#include "Logger.hpp"
#include "EclipseReader.hpp"

namespace Eclipse {

using std::string;
using std::vector;
using std::list;


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


template<typename DST, typename SRC>
struct TypeTraits
{
    static const DataType m_src_type;
    static void copy( std::vector<DST>& dst, size_t offset, const unsigned char* src, const unsigned int stride, const unsigned int count );
};

template<typename DST>
struct TypeTraits<DST, bool>
{
    static const DataType m_src_type = TYPE_INTEGER;
    static void copy( std::vector<DST>& dst, size_t offset, const unsigned char* src, const unsigned int stride, const unsigned int count )
    {
        const int32_t* int_src = reinterpret_cast<const int32_t*>( src );
        for( unsigned int i=0; i<count; i++ ) {
            dst[offset++] = (*int_src++) != 0;
        }
    }
};

template<typename DST>
struct TypeTraits<DST, std::string>
{
    static const DataType m_src_type = TYPE_STRING;
    static void copy( std::vector<DST>& dst, size_t offset, const unsigned char* src, const unsigned int stride, const unsigned int count )
    {
        for( unsigned int i=0; i<count; i++ ) {
            dst[offset++] = std::string( src, src+stride );
            src += stride;
        }
    }
};


template<typename DST>
struct TypeTraits<DST, int32_t>
{
    static const DataType m_src_type = TYPE_INTEGER;
    static void copy( std::vector<DST>& dst, size_t offset, const unsigned char* src, const unsigned int stride, const unsigned int count )
    {
        for( unsigned int i=0; i<count; i++ ) {
            union {
                int32_t        m_i;
                unsigned char  m_c[4];
            }                  m_v;
#if LITTLE_ENDIAN
            m_v.m_c[3] = *src++; m_v.m_c[2] = *src++; m_v.m_c[1] = *src++; m_v.m_c[0] = *src++;
#else
            m_v.m_c[0] = *src++; m_v.m_c[1] = *src++; m_v.m_c[2] = *src++; m_v.m_c[3] = *src++;
#endif
            dst[offset++] = m_v.m_i;
        }
    }
};

template<typename DST>
struct TypeTraits<DST, float>
{
    static const DataType m_src_type = TYPE_FLOAT;
    static void copy( std::vector<DST>& dst, size_t offset, const unsigned char* src, const unsigned int stride, const unsigned int count )
    {
        for( unsigned int i=0; i<count; i++ ) {
            union {
                float          m_f;
                unsigned char  m_c[4];
            }                  m_v;
#if LITTLE_ENDIAN
            m_v.m_c[3] = *src++; m_v.m_c[2] = *src++; m_v.m_c[1] = *src++; m_v.m_c[0] = *src++;
#else
            m_v.m_c[0] = *src++; m_v.m_c[1] = *src++; m_v.m_c[2] = *src++; m_v.m_c[3] = *src++;
#endif
            dst[offset++] = m_v.m_f;
        }
    }
};

template<typename DST>
struct TypeTraits<DST, double>
{
    static const DataType m_src_type = TYPE_DOUBLE;
    static void copy( std::vector<DST>& dst, size_t offset, const unsigned char* src, const unsigned int stride, const unsigned int count )
    {
        for( unsigned int i=0; i<count; i++ ) {
            union {
                double         m_d;
                unsigned char  m_c[8];
            }                  m_v;
#if LITTLE_ENDIAN
            m_v.m_c[7] = *src++; m_v.m_c[6] = *src++; m_v.m_c[5] = *src++; m_v.m_c[4] = *src++;
            m_v.m_c[3] = *src++; m_v.m_c[2] = *src++; m_v.m_c[1] = *src++; m_v.m_c[0] = *src++;
#else
            m_v.m_c[0] = *src++; m_v.m_c[1] = *src++; m_v.m_c[2] = *src++; m_v.m_c[3] = *src++;
            m_v.m_c[4] = *src++; m_v.m_c[5] = *src++; m_v.m_c[6] = *src++; m_v.m_c[7] = *src++;
#endif
            dst[offset++] = m_v.m_d;
        }
    }
};


template<class DST, class SRC>
static
void
blockContentIterateOverRecords( std::vector<DST>&     dst,
                                const unsigned char*  block_payload,
                                const unsigned int    typesize,
                                const unsigned int    count,
                                const unsigned int    records,
                                const unsigned int    record_count )
{
    size_t record_size = typesize*record_count + 8; // payload + record head & tail
    for( unsigned int r=0; r<records; r++ ) {
        const unsigned char* src_ptr = block_payload + r*record_size + 4; // first head
        unsigned int dst_offset = r*record_count;
        unsigned int n = std::min( record_count, count - dst_offset );
        TypeTraits<DST,SRC>::copy( dst, dst_offset, src_ptr, typesize, n );
    }
}

template<class DST>
void
Reader::blockContent( std::vector<DST>& content,
                      const Block& block )
{
    if( m_fd < 0 ) {
        throw std::runtime_error( "object in invalid state" );
    }
    Logger log = getLogger( "EclipseFileReader.blockContent" );

    // set up mapping of file into memory. start address must be on a page
    // boundary.
    size_t page = block.m_offset/m_pagesize;
    size_t page_start = m_pagesize*page;
    size_t page_offset = block.m_offset - page_start;
    size_t bytes_to_map = page_offset + block.m_size;
    const unsigned char* raw =
            reinterpret_cast<const unsigned char*>( mmap( NULL,
                                                          bytes_to_map,
                                                          PROT_READ,
                                                          MAP_PRIVATE | MAP_NORESERVE,
                                                          m_fd,
                                                          page_start ) );
    if( raw == MAP_FAILED ) {
        string error(strerror(errno));
        cleanup();
        throw std::runtime_error( "mmap() failed: " + error );
    }
    // hint to the kernel that we will read this memory sequentially
    if( madvise( const_cast<unsigned char*>(raw), bytes_to_map, MADV_SEQUENTIAL ) != 0 ) {
        LOGGER_WARN( log, "madvice() failed: " << strerror(errno) );
    }

    // read contents
    content.resize( block.m_count );



    if( block.m_datatype == TypeTraits<DST,DST>::m_src_type ) {
        blockContentIterateOverRecords<DST,DST>( content,
                                                 raw + page_offset,
                                                 block.m_typesize,
                                                 block.m_count,
                                                 block.m_records,
                                                 block.m_record_size );
    }
    else if( TypeTraits<DST,DST>::m_src_type == TYPE_FLOAT && block.m_datatype == TYPE_DOUBLE ) {
        blockContentIterateOverRecords<DST,double>( content,
                                                   raw + page_offset,
                                                   block.m_typesize,
                                                   block.m_count,
                                                   block.m_records,
                                                   block.m_record_size );
    }
    else if( TypeTraits<DST,DST>::m_src_type == TYPE_DOUBLE && block.m_datatype == TYPE_FLOAT ) {
        blockContentIterateOverRecords<DST,float>( content,
                                                   raw + page_offset,
                                                   block.m_typesize,
                                                   block.m_count,
                                                   block.m_records,
                                                   block.m_record_size );
    }
    else if( TypeTraits<DST,DST>::m_src_type == TYPE_INTEGER && block.m_datatype == TYPE_BOOL ) {
        blockContentIterateOverRecords<DST,bool>( content,
                                                  raw + page_offset,
                                                  block.m_typesize,
                                                  block.m_count,
                                                  block.m_records,
                                                  block.m_record_size );
    }
    else {
        throw std::runtime_error( "Type mismatch: dst=" +
                                  typeString(TypeTraits<DST,DST>::m_src_type ) +
                                  ", src=" +
                                  typeString(block.m_datatype) );
    }


    if( munmap(const_cast<unsigned char*>(raw), bytes_to_map ) != 0 ) {
        string error(strerror(errno));
        cleanup();
        throw std::runtime_error( "munmap() failed: " + error );
    }
}


template void Reader::blockContent( std::vector<std::string>&, const Block& );
template void Reader::blockContent( std::vector<bool>&, const Block& );
template void Reader::blockContent( std::vector<int>&, const Block& );
template void Reader::blockContent( std::vector<float>&, const Block& );
template void Reader::blockContent( std::vector<double>&, const Block& );



} // of namespace Eclipse
