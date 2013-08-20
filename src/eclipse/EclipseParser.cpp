/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <stdexcept>
#include "utils/Logger.hpp"
#include "EclipseReader.hpp"
#include "EclipseParser.hpp"

namespace eclipse {
    using std::string;
    using std::vector;
    using std::list;



static
void
parseRestartStep( std::list<ReportStep>&            report_steps,
                  Reader&                           reader,
                  const std::list<Block>::iterator& first,
                  const std::list<Block>::iterator& last,
                  const std::vector<int>&           actnum,
                  const unsigned int                seqnum )
{
    Logger log = getLogger( "Eclipse.parseRestartStep" );

    unsigned int nx = 0;
    unsigned int ny = 0;
    unsigned int nz = 0;
    unsigned int nactive = 0;

    unsigned int nwell = 0;     // number of wells
    unsigned int ncwma = 0;     // max no of completions per well
    //unsigned int nwgmax = 0;    // max no of wells in any well group
    //unsigned int ngmaxz = 0;   // max no of wells in field
    unsigned int niwelz = 0;     // no of data elements per well in IWEL array
    unsigned int nzwelz = 0;
    unsigned int niconz = 0;     // no of data elements per completion in ICON array
    //unsigned int nigrpz = 0;    // no of data elements per group in IGRP array

    unsigned int nswlmx = 0;    // max no of segmented wells
    unsigned int nsegmx = 0;    // max no of segments per well
    unsigned int nisegz = 0;    // no of data elements per segment in ISEG array.


    std::vector<int>            iseg;
    std::vector<int>            iwel;
    std::vector<std::string>    zwel;
    std::vector<int>            icon;

    report_steps.push_back( ReportStep() );
    ReportStep& step = report_steps.back();
    step.m_sequence_number = seqnum;

    for(auto it=first; it!=last; ++it ) {
        if( it->m_keyword == KEYWORD_INTEHEAD ) {
            std::vector<int> intehead;
            reader.blockContent( intehead, *it );
            if( intehead.size() < 95 ) {
                throw std::runtime_error( "INTEHEAD < 95 elements" );
            }
            step.m_properties["isnum"] = intehead[ ITEM_INTEHEAD_ISNUM ];

            switch( intehead[ ITEM_INTEHEAD_UNITS ] ) {
            case 1:
                step.m_properties["units"] = "metric";
                break;
            case 2:
                step.m_properties["units"] = "field";
                break;
            case 3:
                step.m_properties["units"] = "lab";
                break;
            default:
                break;
            }
            nx = intehead[ ITEM_INTEHEAD_NX ];
            ny = intehead[ ITEM_INTEHEAD_NY ];
            nz = intehead[ ITEM_INTEHEAD_NZ ];
            nactive = intehead[ ITEM_INTEHEAD_NACTIV ];

            step.m_properties["nx"] = intehead[ ITEM_INTEHEAD_NX ];
            step.m_properties["ny"] = intehead[ ITEM_INTEHEAD_NY ];
            step.m_properties["nz"] = intehead[ ITEM_INTEHEAD_NZ ];
            step.m_properties["nactive"] = intehead[ ITEM_INTEHEAD_NACTIV ];
            switch( intehead[ ITEM_INTEHEAD_IPHS ] ) {
            case 1:
                step.m_properties["iphs"] = "oil";
                break;
            case 2:
                step.m_properties["iphs"] = "water";
                break;
            case 3:
                step.m_properties["iphs"] = "oil/water";
                break;
            case 4:
                step.m_properties["iphs"] = "gas";
                break;
            case 5:
                step.m_properties["iphs"] = "oil/gas";
                break;
            case 6:
                step.m_properties["iphs"] = "gas/water";
                break;
            case 7:
                step.m_properties["iphs"] = "oil/water/gas";
                break;
            default:
                break;
            }
            nwell  = intehead[16];      // Number of wells
            ncwma  = intehead[17];      // Max # completions per well.
            //nwgmax = intehead[19];      // Max number of wells in any well group.
            //ngmaxz = intehead[20];      // Maximum number of groups in field.
            niwelz = intehead[24];      // Number of data elements per well in IWEL.
            nzwelz = intehead[27];      // Number of 8-character strings per well in ZWEL
            niconz = intehead[32];      // Number of data elements per completion in ICON.
            //nigrpz = intehead[36];      // Number of data elements per group in ZGRP.


            step.m_properties["nwell"] = intehead[ ITEM_INTEHEAD_NWELLS ];
            step.m_properties["ncwmax"] = intehead[ ITEM_INTEHEAD_NCWMAX ];
            step.m_properties["nwgmax"] = intehead[ ITEM_INTEHEAD_NWGMAX ];
            step.m_properties["ngmaxz"] = intehead[ ITEM_INTEHEAD_NGMAXZ ];
            step.m_properties["niwel"] = intehead[ ITEM_INTEHEAD_NIWELZ ];
            step.m_properties["nzwel"] = intehead[ ITEM_INTEHEAD_NZWELZ ];
            step.m_properties["nicon"] = intehead[ ITEM_INTEHEAD_NICONZ ];
            step.m_properties["nigrpz"] = intehead[ ITEM_INTEHEAD_NIGRPZ ];

            step.m_date.m_day   = intehead[ ITEM_INTEHEAD_IDAY ];
            step.m_date.m_month = intehead[ ITEM_INTEHEAD_IMON ];
            step.m_date.m_year  = intehead[ ITEM_INTEHEAD_IYEAR ];
            switch( intehead[ ITEM_INTEHEAD_IPROG ] ) {
            case 100:
                step.m_properties["simulator"] = "eclipse 100";
                break;
            case 300:
                step.m_properties["simulator"] = "eclipse 300";
                break;
            case 500:
                step.m_properties["simulator"] = "eclipse 300 (thermal option)";
                break;
            }
            if( intehead.size() > 180 ) {
                nswlmx = intehead[ ITEM_INTEHEAD_NSWLMX ];
                nsegmx = intehead[ ITEM_INTEHEAD_NSEGMX ];
                nisegz = intehead[ ITEM_INTEHEAD_NISEGZ ];
                step.m_properties["nswlmx"] = intehead[ ITEM_INTEHEAD_NSWLMX ];
                step.m_properties["nsegmx"] = intehead[ ITEM_INTEHEAD_NSEGMX ];
                step.m_properties["nisegz"] = intehead[ ITEM_INTEHEAD_NISEGZ ];
            }
        }
        else if( it->m_keyword == KEYWORD_LOGIHEAD ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_DOUBHEAD ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_IGRP ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_ISEG ) {
            reader.blockContent( iseg, *it );
        }
        else if( it->m_keyword == KEYWORD_IWEL ) {  // well info
            reader.blockContent( iwel, *it );

        }
        else if( it->m_keyword == KEYWORD_ZWEL ) {  // well names
            reader.blockContent( zwel, *it );
        }
        else if( it->m_keyword == KEYWORD_ICON ) {
            reader.blockContent( icon, *it );
        }
        else if( it->m_keyword == KEYWORD_HIDDEN ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_ZTRACER ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_STARTSOL ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_ENDSOL ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_DRAINAGE ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_LGRNAMES ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_LGR ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_LGRHEADI ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_LGRHEADQ ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_LGRHEADD ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_LGRJOIN ) {
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_ENDLGR ) {
            // currently ignored
        }
        else if( ( it->m_datatype == TYPE_FLOAT || it->m_datatype == TYPE_DOUBLE ) &&
                 ( it->m_count == nx*ny*nz || it->m_count == nactive ) )
        {
            step.m_solutions.push_back( Solution() );
            Solution& solution = step.m_solutions.back();
            solution.m_name = it->m_keyword != KEYWORD_UNKNOWN
                            ? keywordString( it->m_keyword )
                            : string( it->m_keyword_string, it->m_keyword_string+8 );
            solution.m_location = *it;
        }
        else {
            //LOGGER_WARN( log, "Unknown keyword: " << string( it->m_keyword_string, it->m_keyword_string+8 ) );
        }
    }

    LOGGER_INFO( log, "Parsed report step " << step.m_sequence_number );

    // extract well info
    if( nwell != 0 ) {
        if( actnum.size() != nx*ny*nz ) {
            LOGGER_FATAL( log, "Dimension mismatch, actnum.size()=" << actnum.size() << ", nx*ny*nz=" << (nx*ny*nz) );
            return;
        }

        if( iwel.size() != niwelz*nwell  ) {
            throw std::runtime_error( "IWEL.size != NIWEL*NWELL" );
        }
        if( zwel.size() != nzwelz * nwell ) {
            throw std::runtime_error( "ZWEL.size != NZWEL*NWELL" );
        }
        if( icon.size() != niconz*ncwma*nwell ) {
            throw std::runtime_error( "ICON.size != NICON*NCWMA*NWELL" );
        }
        if( nsegmx > 1 ) {
            LOGGER_INFO( log, "DSADSADSADSDS" );
            if( iseg.size() != nisegz*nsegmx*nswlmx ) {
                LOGGER_FATAL( log, "seg.size()=" << iseg.size()
                              << ", nisegz=" << nisegz
                              << ", nsegmx=" << nsegmx
                              << ", nswlmx=" << nswlmx
                              << ", expected=" << (nisegz*nsegmx*nswlmx) );
                throw std::runtime_error( "ISEG.size != NISEGZ*NSEGMX*NSWLMX" );
            }
        }

        if( step.m_wells.size() != nwell ) {
            step.m_wells.resize( nwell );
        }

        for(unsigned int i=0; i<nwell; i++ ) {
            const unsigned int iwel_offset = niwelz * i;
            const unsigned int zwel_offset = nzwelz * i;


            Well& well = step.m_wells[i];
            well.m_head_i = iwel[ iwel_offset + ITEM_IWEL_WELLHEAD_I ]-1;
            well.m_head_j = iwel[ iwel_offset + ITEM_IWEL_WELLHEAD_J ]-1;
            well.m_head_k = iwel[ iwel_offset + ITEM_IWEL_WELLHEAD_K ]-1;

            if( (nx <= well.m_head_i) || (ny <= well.m_head_j) ) {
                LOGGER_ERROR( log, "Well head indices [" <<
                              well.m_head_i << ", " <<
                              well.m_head_j << "] is out of range (nx=" <<
                              nx << ", ny=" <<
                              ny << "), giving up." );
                step.m_wells.clear();
                break;
            }

            well.m_head_min_k = well.m_head_max_k = ~0u;
            for( unsigned int k=0; k<nz; k++ ) {
                if( actnum[well.m_head_i + nx*well.m_head_j + k*nx*ny] != 0 ) {
                    well.m_head_min_k = k;
                    well.m_head_max_k = k;
                    break;
                }
            }
            if( nz <= well.m_head_min_k ) {
                LOGGER_ERROR( log, "No active cells in well head column, giving up" );
                step.m_wells.clear();
                break;
            }
            for( unsigned int k=well.m_head_min_k; k<nz; k++ ) {
                if( actnum[well.m_head_i + nx*well.m_head_j + k*nx*ny] != 0 ) {
                    well.m_head_max_k = k;
                }
            }

            if(  (nz <= well.m_head_k) ) {
                LOGGER_DEBUG( log, "Well head has illegal k-coordinate, snapping to k-min." );
                well.m_head_k = well.m_head_min_k;
            }


            int ncomp = iwel[ niwelz*i + ITEM_IWEL_N_COMP ];
            well.m_group = iwel[ iwel_offset + ITEM_IWEL_GRP_IX ];
            switch( iwel[ iwel_offset + ITEM_IWEL_TYPE ] ) {
            case 1: well.m_type = WELL_PRODUCER; break;
            case 2: well.m_type = WELL_OIL_INJECTION; break;
            case 3: well.m_type = WELL_WATER_INJECTION; break;
            case 4: well.m_type = WELL_GAS_INJECTION; break;
            default:
                LOGGER_FATAL( log, "ILLEGAL WELL TYPE: well_type=" << iwel[ iwel_offset+6] );
                well.m_type = WELL_PRODUCER;
                //throw std::runtime_error( "Illegal well type in IWEL" );
            }
            well.m_open = iwel[ iwel_offset + ITEM_IWEL_STATUS ] > 0;
            if( iwel[ iwel_offset + ITEM_IWEL_SEG ] > 0 ) {

            }

            unsigned int seg_well_no = iwel[ iwel_offset + ITEM_IWEL_SEG ];
            bool msw = seg_well_no > 0 ;
            if( msw ) {
                seg_well_no = seg_well_no -1;
            }



            // well name
            well.m_name.clear();
            for(unsigned int k=0; k<nzwelz; k++) {
                well.m_name += zwel[ zwel_offset+k ];
            }
            size_t pos = well.m_name.find_last_not_of( ' ' );
            if( pos != std::string::npos ) {
                well.m_name = well.m_name.substr( 0, pos+1 );
            }

            for(int comp_no=0; comp_no<ncomp; comp_no++ ) {
                Completion completion;

                const unsigned int icon_offset = niconz*(ncwma*i + comp_no);
                completion.m_i                = icon[ icon_offset + ITEM_ICON_I       ]-1;
                completion.m_j                = icon[ icon_offset + ITEM_ICON_J       ]-1;
                completion.m_k                = icon[ icon_offset + ITEM_ICON_K       ]-1;
                if( (nx <= completion.m_i) || (ny <= completion.m_j) || (nz <= completion.m_k) ) {
                    LOGGER_ERROR( log, "Completion has illegal coordinate [" <<
                                  completion.m_i << ", " <<
                                  completion.m_j << ", " <<
                                  completion.m_k << "], nx="<<
                                  nx << ", ny=" <<
                                  ny << ", nz=" <<
                                  nz << ", skipping." );
                    continue;
                }

                if( actnum[ completion.m_i + nx*completion.m_j + nx*ny*completion.m_k ] == 0 ) {
                    LOGGER_ERROR( log, "Completion in inactive cell ["  <<
                                  completion.m_i << ", " <<
                                  completion.m_j << ", " <<
                                  completion.m_k << "], skipping." );
                    continue;
                }

                completion.m_open             = icon[ icon_offset + ITEM_ICON_STATUS  ] > 0;
                switch( icon[ icon_offset + ITEM_ICON_PDIR ] ) {
                case 1: completion.m_penetration = PENETRATION_X; break;
                case 2: completion.m_penetration = PENETRATION_Y; break;
                case 3: completion.m_penetration = PENETRATION_Z; break;
                case 4: completion.m_penetration = PENETRATION_FRACTURED_X; break;
                case 5: completion.m_penetration = PENETRATION_FRACTURED_Y; break;
                default:completion.m_penetration = PENETRATION_Z; break;
                }

                // determine branch
                int branch = 0;
                completion.m_segment = icon[ icon_offset + ITEM_ICON_SEGMENT ]-1;
                if( completion.m_segment >= 0 ) {
                    const unsigned int iseg_offset = nisegz*( nsegmx*seg_well_no + completion.m_segment );
                    branch = iseg[ iseg_offset + ITEM_ISEG_BRANCH ];
                }

                if( (int)well.m_branches.size() <= branch ) {
                    well.m_branches.resize( branch+1 );
                }
                well.m_branches[ branch ].push_back( completion );
            }
        }
    }


    if( step.m_wells.empty() ) {
        LOGGER_INFO( log, "No wells" );
    }
    else {
        for( size_t w=0; w<step.m_wells.size(); w++ ) {
#if 0
            const Well& well = step.m_wells[w];
            LOGGER_INFO( log, "Well " << w << ": " << well.m_name
                         << ", head: [" << well.m_head_i
                         << ", " << well.m_head_j
                         << ", " << well.m_head_k << "]" );
#endif
        }
    }
}


void
parseRestartFile( std::list<ReportStep>&  report_steps,
                  const std::vector<int>& actnum,
                  const std::string&      path )
{
    Logger log = getLogger( "Eclipse.parseUnifiedRestartFile" );
    size_t s = path.find_last_not_of( "0123456789" );
    if( s == std::string::npos ) {
        s = 0;
    }
    else {
        s++;
    }
    if( s >= path.size() ) {
        throw std::runtime_error( "unable to determine report step from file name" );
    }
    unsigned int seqnum = 0u;
    for( ; s < path.size(); s++ ) {
        seqnum = 10u*seqnum + (unsigned int)(path[s]-'0');
    }
    Reader reader( path );
    list<Block> blocks = reader.blocks();
    parseRestartStep( report_steps,
                      reader,
                      blocks.begin(),
                      blocks.end(),
                      actnum,
                      seqnum );
}

void
parseUnifiedRestartFile( std::list<ReportStep>&  report_steps,
                         const std::vector<int>& actnum,
                         const std::string&      path )
{
    Logger log = getLogger( "Eclipse.parseUnifiedRestartFile" );
    Reader reader( path );
    list<Block> blocks = reader.blocks();
    auto prev = blocks.begin();
    while( prev != blocks.end() ) {
        if( prev->m_keyword != KEYWORD_SEQNUM ) {
            throw std::runtime_error( "expected SEQNUM" );
        }
        std::vector<int> seqnum;
        reader.blockContent( seqnum, *prev++ );
        if( seqnum.size() < 1 ) {
            throw std::runtime_error( "SEQNUM too small" );
        }
        auto next = prev;
        while( next != blocks.end() && next->m_keyword != KEYWORD_SEQNUM ) {
            next++;
        }
        parseRestartStep( report_steps,
                          reader,
                          prev,
                          next,
                          actnum,
                          seqnum[0] );
        prev = next;
    }
}



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
            const std::string&          path )
{
    Logger log = getLogger( "Eclipse.parseEGrid" );
    Reader reader( path );
    list<Block> blocks = reader.blocks();


    auto it = blocks.begin();

    int grid_type;

    // FILEHEAD
    if( it != blocks.end() && it->m_keyword == KEYWORD_FILEHEAD ) {
        vector<int> filehead;
        reader.blockContent( filehead, *it );
        if( filehead.size() < 6 ) {
            throw std::runtime_error( "FILEHEAD too small" );
        }

        properties["version_number"]          = filehead[0];
        properties["release_year"]            = filehead[1];
        properties["backwards_compatibility"] = filehead[3];

        grid_type = filehead[4];
        switch( grid_type ) {
        case 0:
            properties["grid_type"] = "corner_point";
            break;
        case 1:
            properties["grid_type"] = "unstructured";
            break;
        case 2:
            properties["grid_type"] = "hybrid";
            break;
        default:
            throw std::runtime_error( "unknown grid type" );
        }
        switch( filehead[5] ) {
        case 0:
            properties["single_porosity"] = 1;
            break;
        case 1:
            properties["dual_porosity"] = 1;
            break;
        case 2:
            properties["dual_permeability"] = 1;
            break;
        default:
            throw std::runtime_error( "unknown porosity" );
        }
        it++;
    }
    else {
        throw std::runtime_error( "expected FILEHEAD" );
    }

    if( it != blocks.end() && it->m_keyword == KEYWORD_MAPUNITS ) {
        std::vector<std::string> mapunits;
        reader.blockContent( mapunits, *it );
        if( mapunits.size() < 1 ) {
            throw std::runtime_error( "MAPUNITS too small" );
        }
        size_t np = mapunits[0].find_last_not_of( ' ' );
        if( np != std::string::npos ) {
            properties["map_units"] = mapunits[0].substr(0, np+1 );
        }
        it++;
    }

    if( it != blocks.end() && it->m_keyword == KEYWORD_MAPAXES ) {
        it++;
    }

    if( it != blocks.end() && it->m_keyword == KEYWORD_GRIDUNIT ) {
        it++;
    }


    if( it != blocks.end() && it->m_keyword == KEYWORD_GRIDHEAD ) {
        std::vector<int> gridhead;
        reader.blockContent( gridhead, *it++ );
        if( gridhead.size() < 100 ) {
            throw std::runtime_error( "GRIDHEAD too small" );
        }

        nx = gridhead[1];
        ny = gridhead[2];
        nz = gridhead[3];
        if( gridhead[0] == 1 ) {        // --- corner point
            nr = gridhead[24];
            properties["nseg"]    = gridhead[25];
            properties["ntheta"]  = gridhead[26];
            properties["lower_i"] = gridhead[27];
            properties["lower_j"] = gridhead[28];
            properties["lower_k"] = gridhead[29];
            properties["upper_i"] = gridhead[30];
            properties["upper_j"] = gridhead[31];
            properties["upper_k"] = gridhead[32];

            if( it != blocks.end() && it->m_keyword == KEYWORD_BOXORIG ) {
                std::vector<int> boxorig;
                reader.blockContent( boxorig, *it++ );

                if( boxorig.size() < 3 ) {
                    throw std::runtime_error( "BOXORIG too small" );
                }
                properties[ "boxorig_i"] = boxorig[0];
                properties[ "boxorig_j"] = boxorig[1];
                properties[ "boxorig_k"] = boxorig[2];
                it++;
            }

            if( it != blocks.end() && it->m_keyword == KEYWORD_COORD ) {
                reader.blockContent( coord, *it++ );

                if( coord.size() != 6*(ny+1)*(nx+1)*nr ) {
                    throw std::runtime_error( "COORD of illegal size" );
                }
            }
            else {
                throw std::runtime_error( "expected COORD" );
            }

            if( it != blocks.end() && it->m_keyword == KEYWORD_COORDSYS ) {
                it++;
            }

            if( it != blocks.end() && it->m_keyword == KEYWORD_ZCORN ) {
                reader.blockContent( zcorn, *it++ );
                if( zcorn.size() != 2*nx*2*ny*2*nz ) {
                    throw std::runtime_error( "ZCORN of illegal size" );
                }
            }
            else {
                throw std::runtime_error( "expected ZCORN" );
            }

            if( it != blocks.end() && it->m_keyword == KEYWORD_ACTNUM ) {
                if( it->m_count == 0 ) {
                    LOGGER_WARN( log, "Encountered ACTNUM with no entries, assuming all blocks are active." );
                    actnum.resize( nx*ny*nz );
                    for(size_t i=0; i<actnum.size(); i++ ) {
                        actnum[i] = (i+1);
                    }
                }
                else {
                    reader.blockContent( actnum, *it++ );
                    if( actnum.size() != nx*ny*nz ) {
                        throw std::runtime_error( "ACTNUM of illegal size" );
                    }
                }
            }
            else {
                actnum.resize( nx*ny*nz );
                std::fill( actnum.begin(), actnum.end(), 1 );
            }

            if( it != blocks.end() && it->m_keyword == KEYWORD_CORSNUM ) {
                it++;
            }

            if( it != blocks.end() && it->m_keyword == KEYWORD_HOSTNUM ) {
                it++;
            }

        }
        else {
            throw std::runtime_error( "unsupported grid type" );
        }




        it++;
    }
    else {
        throw std::runtime_error( "expected GRIDHEAD" );
    }
}

template void parseEGrid( unsigned int&, unsigned int&, unsigned int&, unsigned int&, std::vector<float>&, std::vector<float>&, std::vector<int>&, Properties&, const std::string& );
template void parseEGrid( unsigned int&, unsigned int&, unsigned int&, unsigned int&, std::vector<double>&, std::vector<double>&, std::vector<int>&, Properties&, const std::string& );



} // of namespace Eclipse
