#pragma once
#include <vector>
#include <boost/utility.hpp>

class GridTess;
class GridField;

class GridFieldBridge : public boost::noncopyable
{
    friend class GridField;
public:
    typedef float REAL;

    GridFieldBridge( GridField& owner, GridTess& specifier );

    ~GridFieldBridge();

    std::vector<REAL>&
    values() { return m_values; }


protected:
    GridField&          m_owner;
    GridTess&           m_specifier;
    std::vector<REAL>   m_values;
    REAL                m_min_value;
    REAL                m_max_value;
};

