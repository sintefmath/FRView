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

template<typename Bridge>
class CornerPointTessellatorSimple
{
public:
    static void
    triangulate( Bridge&                                    bridge,
                 const unsigned int                         nx,
                 const unsigned int                         ny,
                 const unsigned int                         nz,
                 const unsigned int                         nr,
                 const std::vector<typename Bridge::REAL>&  coord,
                 const std::vector<typename Bridge::REAL>&  zcorn,
                 const std::vector<int>&                    actnum );

private:

};
