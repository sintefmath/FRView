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


template<typename REAL>
void
parseEGrid( unsigned int&               nx,
            unsigned int&               ny,
            unsigned int&               nz,
            unsigned int&               nr,
            std::vector<REAL>&          coord,
            std::vector<REAL>&          zcorn,
            std::vector<int>&           actnum,
            Properties&                 properties,
            const std::string&          path );

void
parseRestartFile( std::list<ReportStep>&  report_steps,
                  const std::string&      path );

void
parseUnifiedRestartFile( std::list<ReportStep>&  report_steps,
                         const std::string&      path );


} // of namespace Eclipse

