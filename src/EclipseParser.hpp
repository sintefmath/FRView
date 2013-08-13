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

#define ITEM_INTEHEAD_ISNUM     0   ///< Timestamp
#define ITEM_INTEHEAD_UNITS     2   ///< Unit type (1: metric, 2: field, 3: Lab).
#define ITEM_INTEHEAD_NX        8   ///< Grid I dimension.
#define ITEM_INTEHEAD_NY        9   ///< Grid J dimension.
#define ITEM_INTEHEAD_NZ        10  ///< Grid K dimension.
#define ITEM_INTEHEAD_NACTIV    11  ///< Number of active cells.
#define ITEM_INTEHEAD_IPHS      14  ///< Phase indicator(1: oil, 2: water, 3: oil/water, 4: gas, 5: oil/gas, 6: gas/water, 7: oil/water/gas).
#define ITEM_INTEHEAD_NWELLS    16  ///< Number of wells.
#define ITEM_INTEHEAD_NCWMAX    17  ///< Maximum number of completions per well.
#define ITEM_INTEHEAD_NWGMAX    19  ///< Maximum number of wells in any well group.
#define ITEM_INTEHEAD_NGMAXZ    20  ///< Maximum number of groups in field.
#define ITEM_INTEHEAD_NIWELZ    24  ///< Number of data elements per well in IWEL.
#define ITEM_INTEHEAD_NSWELZ    25  ///< Number of data elements in SWEL.
#define ITEM_INTEHEAD_NXWELZ    26  ///< Number of data elements in XWEL.
#define ITEM_INTEHEAD_NZWELZ    27  ///< Number of 8-char words per well in ZWEL.
#define ITEM_INTEHEAD_NICONZ    32  ///< Number of data elements per completion in ICON.
#define ITEM_INTEHEAD_NSCONZ    33  ///< Number of data elements per completion in SCON.
#define ITEM_INTEHEAD_NXCONZ    34  ///< Number of data elements per completion in XCON
#define ITEM_INTEHEAD_NIGRPZ    36  ///< Number of data elements per group in IGRP.
#define ITEM_INTEHEAD_NSGRPZ    37  ///< Number of data elements per group in SGRP.
#define ITEM_INTEHEAD_NXGRPZ    38  ///< Number of data elements per group in XGRP
#define ITEM_INTEHEAD_NZGRPZ    39  ///< Number of data elements per group in ZGRP.
#define ITEM_INTEHEAD_IDAY      64  ///< Calendar day at this report time.
#define ITEM_INTEHEAD_IMON      65  ///< Calendar day at this report time.
#define ITEM_INTEHEAD_IYEAR     66  ///< Calendar day at this report time.
#define ITEM_INTEHEAD_IPROG     94  ///< Simulation program id (100: eclipse 100, 300: eclipse 300, 500: eclipse 300 thermal option).
#define ITEM_INTEHEAD_NSWLMX    175 ///< Maximum number of segmented wells.
#define ITEM_INTEHEAD_NSEGMX    176 ///< Maximum number of segments per well.
#define ITEM_INTEHEAD_NISEGZ    178 ///< Number of entries per segment in ISEG.

// ISEG[ NISEGZ * NSEGMX * NSWLMX ]
#define ITEM_ISEG_OUTLET        1   ///< Outlet segment number (=0 for segment nearest wellhead).
#define ITEM_ISEG_BRANCH        3   ///< Branch for this segment (=1 for main stem, 0 if not active segment).

// IWEL[ NIWELZ * NWELLS ]
#define ITEM_IWEL_WELLHEAD_I    0   ///< Wellhead I-coordinate.
#define ITEM_IWEL_WELLHEAD_J    1   ///< Wellhead J-coordinate.
#define ITEM_IWEL_WELLHEAD_K    2   ///< Wellhead K-coordinate.
#define ITEM_IWEL_N_COMP        4   ///< Number of completion connections for this well.
#define ITEM_IWEL_GRP_IX        5   ///< Group index.
#define ITEM_IWEL_TYPE          6   ///< Well type (1: producer, 2: oil injection, 3: water injection, 4: gas injection).
#define ITEM_IWEL_STATUS        10  ///< Well status ( >0 open, <= 0 shut).
#define ITEM_IWEL_LGR_IX        42  ///< LGR index for a well with local completions.
#define ITEM_IWEL_FRICT         48  ///< Friction well flag, non-zero for horizontal well.
#define ITEM_IWEL_SEG           70  ///< Segmented well number (=0 for ordinary wells).

// ICON[ NICONZ * NCWMAX * NWELLS ]
#define ITEM_ICON_IC            0   ///< Well connection index ICON(1,IC,IWELL)=IC, -IC if not in current LGR.
#define ITEM_ICON_I             1   ///< Completion I-coordinate (<=0 if not in this LGR).
#define ITEM_ICON_J             2   ///< Completion J-coordinate (<=0 if not in this LGR).
#define ITEM_ICON_K             3   ///< Completion K-coordinate (<=0 if not in this LGR).
#define ITEM_ICON_STATUS        5   ///< Connection status (0> open, <=0 shut).
#define ITEM_ICON_PDIR          13  ///< Penetration direction.
#define ITEM_ICON_SEGMENT       14  ///< Segment number containing connection (=0 for ordinary wells).



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
                  const std::vector<int>& actnum,
                  const std::string&      path );

void
parseUnifiedRestartFile( std::list<ReportStep>&  report_steps,
                         const std::vector<int>& actnum,
                         const std::string&      path );


} // of namespace Eclipse

