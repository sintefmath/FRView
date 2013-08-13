/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <stdexcept>
#include "Logger.hpp"
#include "EclipseReader.hpp"
#include "EclipseParser.hpp"

namespace Eclipse {
    using std::string;
    using std::vector;
    using std::list;



static
void
parseRestartStep( std::list<ReportStep>&        report_steps,
                  Reader&                       reader,
                  const std::list<Block>::iterator&   first,
                  const std::list<Block>::iterator&   last,
                  const unsigned int            seqnum )
{
    Logger log = getLogger( "Eclipse.parseRestartStep" );

    unsigned int nx = 0;
    unsigned int ny = 0;
    unsigned int nz = 0;
    unsigned int nactive = 0;

    unsigned int nwell = 0;     // number of wells
    unsigned int ncwma = 0;     // max no of completions per well
    unsigned int nwgmax = 0;    // max no of wells in any well group
    unsigned int ngmaxz = 0;   // max no of wells in field
    unsigned int niwel = 0;     // no of data elements per well in IWEL array
    unsigned int nzwel = 0;
    unsigned int nicon = 0;     // no of data elements per completion in ICON array
    unsigned int nigrpz = 0;    // no of data elements per group in IGRP array

    unsigned int nswlmx = 0;    // max no of segmented wells
    unsigned int nsegmx = 0;    // max no of segments per well
    unsigned int nisegz = 0;    // no of data elements per segment in ISEG array.


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
            step.m_properties["isnum"] = intehead[0];

            switch( intehead[2] ) {
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
            nx = intehead[8];
            ny = intehead[9];
            nz = intehead[10];
            nactive = intehead[11];

            step.m_properties["nx"] = intehead[8];
            step.m_properties["ny"] = intehead[9];
            step.m_properties["nz"] = intehead[10];
            step.m_properties["nactive"] = intehead[11];
            switch( intehead[14] ) {
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
            nwell  = intehead[16];
            ncwma  = intehead[17];
            nwgmax = intehead[19];
            ngmaxz = intehead[20];
            niwel  = intehead[24];
            nzwel  = intehead[27];
            nicon  = intehead[32];
            nigrpz = intehead[36];


            step.m_properties["nwell"] = intehead[16];
            step.m_properties["ncwma"] = intehead[17];
            step.m_properties["nwgmax"] = intehead[19];
            step.m_properties["ngmaxz"] = intehead[20];
            step.m_properties["niwel"] = intehead[24];
            step.m_properties["nzwel"] = intehead[27];
            step.m_properties["nicon"] = intehead[32];
            step.m_properties["nigrpz"] = intehead[36];
            step.m_properties["iday"] = intehead[64];
            step.m_properties["imon"] = intehead[65];
            step.m_properties["iyear"] = intehead[66];
            switch( intehead[94] ) {
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
                nswlmx = intehead[175];
                nsegmx = intehead[176];
                nisegz = intehead[178];
                step.m_properties["nswlmx"] = intehead[175];
                step.m_properties["nsegmx"] = intehead[176];
                step.m_properties["nisegz"] = intehead[178];
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
            // currently ignored
        }
        else if( it->m_keyword == KEYWORD_IWEL ) {  // well info
            std::vector<int> iwel;
            reader.blockContent( iwel, *it );
            if( iwel.size() != niwel*nwell  ) {
                throw std::runtime_error( "IWEL.size != NIWEL*NWELL" );
            }
            if( step.m_wells.size() != nwell ) {
                step.m_wells.resize( nwell );
            }
            for(unsigned int i=0; i<nwell; i++ ) {
                Well& well = step.m_wells[i];
                well.m_head_i = iwel[ niwel*i + 0 ];
                well.m_head_j = iwel[ niwel*i + 1 ];
                well.m_head_k = iwel[ niwel*i + 2 ];
                well.m_completions.resize( iwel[ niwel*i + 4 ] );
                well.m_group = iwel[ niwel*i + 5 ];
                switch( iwel[ niwel*i+6] ) {
                case 1: well.m_type = WELL_PRODUCER; break;
                case 2: well.m_type = WELL_OIL_INJECTION; break;
                case 3: well.m_type = WELL_WATER_INJECTION; break;
                case 4: well.m_type = WELL_GAS_INJECTION; break;
                default:
                    LOGGER_FATAL( log, "ILLEGAL WELL TYPE: well_type=" << iwel[ niwel*i+6] );
                    well.m_type = WELL_PRODUCER;
                    //throw std::runtime_error( "Illegal well type in IWEL" );
                }
                well.m_open = iwel[ niwel*i + 10] > 0;
                well.m_segmented_well_id = iwel[ niwel*i + 70 ];
            }
        }
        else if( it->m_keyword == KEYWORD_ZWEL ) {  // well names
            std::vector<std::string> zwel;
            reader.blockContent( zwel, *it );
            if( zwel.size() != nzwel * nwell ) {
                throw std::runtime_error( "ZWEL.size != NZWEL*NWELL" );
            }
            if( step.m_wells.size() != nwell ) {
                step.m_wells.resize( nwell );
            }
            for(unsigned int i=0; i<nwell; i++ ) {
                Well& well = step.m_wells[i];
                well.m_name.clear();
                for(unsigned int k=0; k<nzwel; k++) {
                    well.m_name += zwel[ nzwel*i+k ];
                }
                size_t pos = well.m_name.find_last_not_of( ' ' );
                if( pos != std::string::npos ) {
                    well.m_name = well.m_name.substr( 0, pos+1 );
                }
            }
        }
        else if( it->m_keyword == KEYWORD_ICON ) {
            std::vector<int> icon;
            reader.blockContent( icon, *it );
            if( icon.size() != nicon*ncwma*nwell ) {
                throw std::runtime_error( "ICON.size != NICON*NCWMA*NWELL" );
            }
            if( step.m_wells.size() != nwell ) {
                step.m_wells.resize( nwell );
            }
            for(unsigned int i=0; i<nwell; i++ ) {
                Well& well = step.m_wells[i];
                for(unsigned int j=0; j<well.m_completions.size(); j++ ) {
                    unsigned int ix = icon[ nicon*(ncwma*i + j) + 0 ]-1;
                    Completion& completion = well.m_completions[j];
                    completion.m_connection_index = icon[ nicon*(ncwma*i + j) + 0 ];
                    completion.m_i                = icon[ nicon*(ncwma*i + j) + 1 ];
                    completion.m_j                = icon[ nicon*(ncwma*i + j) + 2 ];
                    completion.m_k                = icon[ nicon*(ncwma*i + j) + 3 ];
                    completion.m_open             = icon[ nicon*(ncwma*i + j) + 5 ] > 0;
                    completion.m_segment          = icon[ nicon*(ncwma*i + j) + 14 ];
                    switch( icon[ nicon*(ncwma*i + j) + 13 ] ) {
                    case 1: completion.m_penetration = PENETRATION_X; break;
                    case 2: completion.m_penetration = PENETRATION_Y; break;
                    case 3: completion.m_penetration = PENETRATION_Z; break;
                    case 4: completion.m_penetration = PENETRATION_FRACTURED_X; break;
                    case 5: completion.m_penetration = PENETRATION_FRACTURED_Y; break;
                    default:completion.m_penetration = PENETRATION_Z; break;
                    }
                }
            }
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
}


void
parseRestartFile( std::list<ReportStep>&  report_steps,
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
    parseRestartStep( report_steps, reader, blocks.begin(), blocks.end(), seqnum );
}

void
parseUnifiedRestartFile( std::list<ReportStep>&  report_steps,
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
        parseRestartStep( report_steps, reader, prev, next, seqnum[0] );
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
                reader.blockContent( actnum, *it++ );
                if( actnum.size() != nx*ny*nz ) {
                    throw std::runtime_error( "ACTNUM of illegal size" );
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
