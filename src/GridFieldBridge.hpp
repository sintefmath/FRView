/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
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

    REAL*
    values() { return m_memory; }

    //std::vector<REAL>&
    //values() { return m_values; }

    REAL&
    minimum() { return m_min_value; }

    REAL&
    maximum() { return m_max_value; }

protected:
    GridField&          m_owner;
    GridTess&           m_specifier;
    std::vector<REAL>   m_values;
    REAL*               m_memory;
    REAL                m_min_value;
    REAL                m_max_value;
};

