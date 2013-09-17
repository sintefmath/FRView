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

class GridTess;
class GridField;

class FooBarParser
{
public:


    static
    GridField*
    parseField( GridTess* tess, const std::string& filename );

    static
    void
    parseGeometry( unsigned int&         nx,
                   unsigned int&         ny,
                   unsigned int&         nz,
                   std::vector<float>&   coord,
                   std::vector<float>&   zcorn,
                   std::vector<int>&     actnum,
                   const std::string&    filename );

    static
    void
    parseTxtGeometry( unsigned int&         nx,
                      unsigned int&         ny,
                      unsigned int&         nz,
                      std::vector<float>&   coord,
                      std::vector<float>&   zcorn,
                      std::vector<int>&     actnum,
                      const std::string&    filename );

protected:

};
