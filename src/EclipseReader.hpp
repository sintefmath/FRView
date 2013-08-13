/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <list>

#include "Eclipse.hpp"

namespace Eclipse {


/** Block reader abstraction for Eclipse files.
  *
  * Abstracts away the Fortran sentinels, the division of blocks into records,
  * as well as endianess. Does not in any way actually interpret the contents of
  * the blocks (which is handled by the parser).
  *
  */
class Reader : public boost::noncopyable
{
public:

    /** Create a reader for the file at a specific path. */
    Reader( const std::string& path );

    ~Reader();

    /** Get the blocks that this file contains.
      *
      * Scans through the file, determining the set of blocks the file contains.
      *
      * \returns The list of blocks that this file contains.
      */
    std::list<Block>
    blocks();

    /** Read a block of booleans from file.
      *
      * \param[out] content  Data storage.
      * \param[in]  block    Specifies position of data in file.
      * \throws std::runtime_error If block contents are not booleans.
      * \throws std::runtime_error If unable to mmap the file.
      * \note Resizes content to appropriate size.
      */
    void
    blockContent( std::vector<bool>& content, const Block& block );

    /** Read a block of strings from file.
      *
      * \param[out] content  Data storage.
      * \param[in]  block    Specifies position of data in file.
      * \throws std::runtime_error If block contents are not strings.
      * \throws std::runtime_error If unable to mmap the file.
      * \note Resizes content to appropriate size.
      */
    void
    blockContent( std::vector<std::string>& content, const Block& block );

    /** Read a block of integers from file.
      *
      * \param[out] content  Data storage.
      * \param[in]  block    Specifies position of data in file.
      * \throws std::runtime_error If block contents are not integers.
      * \throws std::runtime_error If unable to mmap the file.
      * \note Resizes content to appropriate size.
      */
    void
    blockContent( std::vector<int>& content, const Block& block );

    /** Read a block of floats or doubles from file.
      *
      * \param[out] content  Data storage.
      * \param[in]  block    Specifies position of data in file.
      * \throws std::runtime_error If block contents are not float or doubles.
      * \throws std::runtime_error If unable to mmap the file.
      * \note Resizes content to appropriate size.
      */
    void
    blockContent( std::vector<float>& content, const Block& block );

    /** Read a block of floats or doubles from file.
      *
      * \param[out] content  Data storage.
      * \param[in]  block    Specifies position of data in file.
      * \throws std::runtime_error If block contents are not float or doubles.
      * \throws std::runtime_error If unable to mmap the file.
      * \note Resizes content to appropriate size.
      */
    void
    blockContent( std::vector<double>& content, const Block& block );

    /** Stream-read a block of floats or doubles from file, determining min and max.
      *
      * \param[out] content  Data storage.
      * \param[out] minimum  The smallest value encountered.
      * \param[out] maximum  The largest value encountered.
      * \param[in]  block    Specifies position of data in file.
      * \throws std::runtime_error If block contents are not float or doubles.
      * \throws std::runtime_error If unable to mmap the file.
      */
    void
    blockContent( float* __attribute__((aligned(16)))   content,
                  float&                                minimum,
                  float&                                maximum,
                  const Block&                          block );


private:
    /** RAII helper class to mmap a block.
      *
      * Mmap requires that the map'ed region starts at a page boundary, so we
      * find the last page boundary before the block's data.
      */
    class Map
    {
    public:
        Map( Reader&        parent,
             const Block&   block );

        ~Map();

        const unsigned char*
        bytes() const
        { return m_map + m_page_offset; }

    private:
        size_t          m_page;
        size_t          m_page_start;
        size_t          m_page_offset;
        size_t          m_bytes_to_map;
        unsigned char*  m_map;
    };

    Reader();

    void
    cleanup();

    const std::string   m_path;
    int                 m_fd;
    size_t              m_filesize;
    size_t              m_pagesize;
};

} // of namespace Eclipse
