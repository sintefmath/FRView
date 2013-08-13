#pragma once
#include <string>
#include <vector>
#include <list>

#include "Eclipse.hpp"

namespace Eclipse {


class Reader : public boost::noncopyable
{
public:
    Reader( const std::string& path );

    ~Reader();

    std::list<Block>
    blocks();

    template<class T>
    void
    blockContent( std::vector<T>& content, const Block& block );

private:
    Reader();

    void
    cleanup();

    const std::string   m_path;
    int                 m_fd;
    size_t              m_filesize;
    size_t              m_pagesize;
};

} // of namespace Eclipse
