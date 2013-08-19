/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <algorithm>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include "utils/Logger.hpp"
#include "Project.hpp"
#include "eclipse/EclipseParser.hpp"
#include "cornerpoint/Tessellator.hpp"
#include "FooBarParser.hpp"
#include "render/GridTessBridge.hpp"
#include "render/GridFieldBridge.hpp"

using std::vector;
using std::string;
using std::list;

static const std::string package = "Project";

template<typename REAL>
Project<REAL>::Project(const std::string filename,
                       int refine_i,
                       int refine_j,
                       int refine_k )
{
    Logger log = getLogger( "package.constructor" );

    m_geometry_type = GEOMETRY_NONE;
    m_cornerpoint_geometry.m_rx = 1;
    m_cornerpoint_geometry.m_ry = 1;
    m_cornerpoint_geometry.m_rz = 1;

    if( filename.empty() ) {
        throw std::runtime_error( "Empty filename" );
    }

    // check if file exists
    int fd = open( filename.c_str(), O_RDONLY );
    if( fd < 0 ) {
        throw std::runtime_error( strerror( errno ) );
    }
    close( fd );

    // Extract suffix
    size_t dot = filename.find_last_of( '.' );
    if( dot == std::string::npos ) {
        throw std::runtime_error( "Filename has no suffix" );
    }
    string suffix = filename.substr( dot + 1u );
    for( auto it=suffix.begin(); it!=suffix.end(); ++it ) {
      *it = toupper( *it );
    }

    if( suffix == "GTXT" ) {
        File file;
        file.m_filetype = FOOBAR_TXT_GRID_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_front( file );
        LOGGER_DEBUG( log, "Found GTXT file" );
    }
    else if( suffix == "GEOMETRY" ) {
        File file;
        file.m_filetype = FOOBAR_GRID_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_front( file );
        LOGGER_DEBUG( log, "Found GEOMETRY file" );
    }
    else if( suffix == "EGRID" ) {
        File file;
        file.m_filetype = ECLIPSE_EGRID_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_front( file );
        LOGGER_DEBUG( log, "Found EGRID file" );

        string stem = filename.substr( 0, dot );
        string unrst = stem + ".UNRST";
        int fd = open( unrst.c_str(), O_RDONLY );
        if( fd >= 0 ) {
            close( fd );
            File file;
            file.m_filetype = ECLIPSE_UNIFIED_RESTART_FILE;
            file.m_timestep = -1;
            file.m_path     = unrst;
            m_unprocessed_files.push_back( file );
            LOGGER_DEBUG( log, "Found unified restart file " << unrst );
        }
        else {
            // Search for restart steps in separate files
            for(int step=0; true; step++) {
                std::stringstream o;
                o << stem << ".X";
                o.width( 4 );
                o.fill( '0' );
                o << step;

                int fd = open( o.str().c_str(), O_RDONLY );
                if( fd < 0 ) {
                    break;
                }
                close( fd );

                File file;
                file.m_filetype = ECLIPSE_RESTART_FILE;
                file.m_timestep = step;
                file.m_path     = o.str();
                m_unprocessed_files.push_back( file );
                LOGGER_DEBUG( log, "Found restart file " << o.str() );
            }
        }
    }
    else {
        throw std::runtime_error( "Unknown suffix " + suffix );
    }
    refresh( refine_i, refine_j, refine_k );
}



template<typename REAL>
void
Project<REAL>::addFile( const string& filename )
{
    Logger log = getLogger( "Project.addFile" );

    size_t dot = filename.find_last_of( '.' );
    if( dot == std::string::npos ) {
        LOGGER_ERROR( log, "Unable to find suffix dot on filename '" << filename << "'" );
        return;
    }
    string suffix = filename.substr( dot + 1u );
    for( auto it=suffix.begin(); it!=suffix.end(); ++it ) {
      *it = toupper( *it );
    }

    if( suffix == "EGRID" ) {
        File file;
        file.m_filetype = ECLIPSE_EGRID_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_front( file );
    }
    else if( suffix == "GTXT" ) {
        File file;
        file.m_filetype = FOOBAR_TXT_GRID_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_front( file );
    }
    else if( suffix == "GEOMETRY" ) {
        File file;
        file.m_filetype = FOOBAR_GRID_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_front( file );
    }
    else if( suffix.size() > 2 && suffix[0] == 'X' && isdigit( suffix[1] ) ) {
        int index = 0;
        for( auto it = ++suffix.begin(); it != suffix.end(); ++it ) {
            if( !isdigit( *it ) ) {
                LOGGER_ERROR( log, "Error parsing timestep in suffix on file '"<< filename << "'" );
                return;
            }
            index = index*10 + (*it - '0' );
        }
        File file;
        file.m_filetype = ECLIPSE_RESTART_FILE;
        file.m_timestep = index;
        file.m_path     = filename;
        m_unprocessed_files.push_back( file );
    }
    else if( suffix == "UNRST" ) {
        File file;
        file.m_filetype = ECLIPSE_UNIFIED_RESTART_FILE;
        file.m_timestep = -1;
        file.m_path     = filename;
        m_unprocessed_files.push_back( file );
    }
    else {
        LOGGER_ERROR( log, "Suffix '" << suffix << "' of file '" << filename << "' not recognized" );
        return;
    }

    for(size_t i=0; i<m_report_steps.size(); i++ ) {
        LOGGER_DEBUG( log, "report step seqnum " << m_report_steps[i].m_seqnum );


    }


}

template<typename REAL>
void
Project<REAL>::bakeCornerpointGeometry()
{
    REAL minxy[3];
    REAL maxxy[3];
    for(uint k=0; k<2; k++) {
        minxy[k] = maxxy[k] = m_cornerpoint_geometry.m_coord[k];
    }

    uint nx = m_cornerpoint_geometry.m_nx;
    uint ny = m_cornerpoint_geometry.m_ny;
    uint nz = m_cornerpoint_geometry.m_nz;
    for(uint j=0; j<=ny; j++) {
        for(uint i=0; i<=nx; i++) {
            REAL* p = m_cornerpoint_geometry.m_coord.data() + 6*(j*(nx+1) + i);
            minxy[0] = std::min( minxy[0], std::min( p[0], p[3] ) );
            maxxy[0] = std::max( maxxy[0], std::max( p[0], p[3] ) );
            minxy[1] = std::min( minxy[1], std::min( p[1], p[4] ) );
            maxxy[1] = std::max( maxxy[1], std::max( p[1], p[4] ) );
        }
    }

    REAL minz, maxz;
    for(uint i=0; i<nx*ny*nz; i++) {
        if( m_cornerpoint_geometry.m_actnum[i] != 0 ) {
            minz = maxz = m_cornerpoint_geometry.m_zcorn[8*i];
        }
    }
    for(uint i=0; i<nx*ny*nz; i++) {
        if( m_cornerpoint_geometry.m_actnum[i] != 0 ) {
            for(uint k=0; k<8; k++) {
                minz = std::min( minz, m_cornerpoint_geometry.m_zcorn[8*i+k] );
                maxz = std::max( maxz, m_cornerpoint_geometry.m_zcorn[8*i+k] );
            }
        }
    }

#if 0
    m_cornerpoint_geometry.m_xyscale = 1.f;
    m_cornerpoint_geometry.m_zscale = 1.f;
#else

    REAL shift[3];
    shift[0] = 0.5f*(minxy[0]+maxxy[0]);
    shift[1] = 0.5f*(minxy[1]+maxxy[1]);
    shift[2] = 0.5f*(minz+maxz);

    m_cornerpoint_geometry.m_xyscale = std::max( maxxy[0]-minxy[0],
                                                 maxxy[1]-minxy[1] );
    m_cornerpoint_geometry.m_zscale = maxz-minz;
    REAL scale[3] = {
        1.f/m_cornerpoint_geometry.m_xyscale,
        1.f/m_cornerpoint_geometry.m_xyscale,
        1.f/m_cornerpoint_geometry.m_zscale
    };
    for(uint j=0; j<=ny; j++) {
        for(uint i=0; i<=nx; i++) {
            for(uint k=0; k<2; k++) {
                for(uint l=0; l<3; l++) {
                     REAL* p = m_cornerpoint_geometry.m_coord.data() + 6*(j*(nx+1) + i) + 3*k + l;
                     *p = scale[l]*(*p - shift[l]);
                }
            }
        }
    }
    for(uint i=0; i<8*nx*ny*nz; i++) {
        m_cornerpoint_geometry.m_zcorn[i] = scale[2]*(m_cornerpoint_geometry.m_zcorn[i] - shift[2]);
   }
#endif
}


template<typename REAL>
void
Project<REAL>::refineCornerpointGeometry( unsigned int rx,
                                          unsigned int ry,
                                          unsigned int rz )
{
#if 0
    rx = 3;
    ry = 3;
    rz = 3;
#endif
    if( (rx == 1 ) && (ry == 1 ) && (rz == 1 ) ) {
        return;
    }

    uint old_nx = m_cornerpoint_geometry.m_nx;
    uint old_ny = m_cornerpoint_geometry.m_ny;
    uint old_nz = m_cornerpoint_geometry.m_nz;
    uint new_nx = rx*old_nx;
    uint new_ny = ry*old_ny;
    uint new_nz = rz*old_nz;
    const std::vector<REAL>& old_coord = m_cornerpoint_geometry.m_coord;
    const std::vector<REAL>& old_zcorn = m_cornerpoint_geometry.m_zcorn;
    const std::vector<int>& old_actnum = m_cornerpoint_geometry.m_actnum;
    std::vector<REAL> new_coord( 6*(new_nx+1)*(new_ny+1) );
    std::vector<REAL> new_zcorn( 8*new_nx * new_ny * new_nz );
    std::vector<int>  new_actnum( new_nx * new_ny * new_nz );

    for(uint j=0; j<=new_ny; j++ ) {
        uint jm = j/ry;
        uint jp = (j+ry-1)/ry;
        float jr = glm::fract( (float)j/(float)ry );
        for(int i=0; i<=new_nx; i++ ) {
            uint im = i/rx;
            uint ip = (i+rx-1)/rx;
            float ir = glm::fract( (float)i/(float)rx );

            for(int k=0; k<6; k++) {
                new_coord[ 6*((new_nx+1)*j+i)+k] =
                        old_coord[ 6*((old_nx+1)*jm+im)+k ]*(1.f-jr)*(1.f-ir) +
                        old_coord[ 6*((old_nx+1)*jm+ip)+k ]*(1.f-jr)*ir +
                        old_coord[ 6*((old_nx+1)*jp+im)+k ]*jr*(1.f-ir) +
                        old_coord[ 6*((old_nx+1)*jp+ip)+k ]*jr*ir;

            }
        }
    }

    int active = 0;
    std::vector<uint> remap( old_nx*old_ny*old_nz);
    for( uint old_k=0; old_k<old_nz; old_k++ ) {
        for( uint old_j=0; old_j<old_ny; old_j++ ) {
            for( uint old_i=0; old_i<old_nx; old_i++ ) {
                uint l = old_nx*old_ny*old_k + old_nx*old_j + old_i;
                if( old_actnum[l] != 0 ) {
                    remap[ l ] = active++;
                }
                else {
                    remap[l ] = ~0u;
                }
            }
        }
    }

    m_cornerpoint_geometry.m_refine_map_compact.clear();
    for( int new_k=0; new_k<new_nz; new_k++ ) {
        uint old_k = new_k/rz;
        float krm = glm::fract( (float)new_k/(float)rz );
        float krp = glm::fract( (float)(new_k+1.f)/(float)rz);


        for( int new_j=0; new_j<new_ny; new_j++ ) {
            uint old_j = new_j/ry;
            for( int new_i=0; new_i<new_nx; new_i++ ) {
                uint old_i = new_i/rx;

                float z000 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+0)  + 2*old_nx*(2*old_j+0) + 2*old_i+0 ];
                float z001 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+0)  + 2*old_nx*(2*old_j+0) + 2*old_i+1 ];
                float z010 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+0)  + 2*old_nx*(2*old_j+1) + 2*old_i+0 ];
                float z011 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+0)  + 2*old_nx*(2*old_j+1) + 2*old_i+1 ];
                float z100 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+1)  + 2*old_nx*(2*old_j+0) + 2*old_i+0 ];
                float z101 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+1)  + 2*old_nx*(2*old_j+0) + 2*old_i+1 ];
                float z110 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+1)  + 2*old_nx*(2*old_j+1) + 2*old_i+0 ];
                float z111 = old_zcorn[ 2*old_nx*2*old_ny*(2*old_k+1)  + 2*old_nx*(2*old_j+1) + 2*old_i+1 ];

                for(int kk=0; kk<2; kk++) {
                    for(int jj=0; jj<2; jj++) {
                        for(int ii=0; ii<2; ii++) {
                            float xt = (float)((new_i % rx)+ii)/(float)rx;
                            float yt = (float)((new_j % ry)+jj)/(float)ry;
                            float zt = (float)((new_k % rz)+kk)/(float)rz;

                            float z = z000*(1.f-zt)*(1.f-yt)*(1.f-xt)
                                    + z001*(1.f-zt)*(1.f-yt)*xt
                                    + z010*(1.f-zt)*yt*(1.f-xt)
                                    + z011*(1.f-zt)*yt*xt
                                    + z100*zt*(1.f-yt)*(1.f-xt)
                                    + z101*zt*(1.f-yt)*xt
                                    + z110*zt*yt*(1.f-xt)
                                    + z111*zt*yt*xt;

                            new_zcorn[ 2*new_nx*2*new_ny*(2*new_k+kk) + 2*new_nx*(2*new_j+jj) + 2*new_i+ii ] = z;
                        }
                    }
                }
                if( old_actnum[ old_nx*old_ny*old_k + old_nx*old_j + old_i ] != 0 ) {
                    m_cornerpoint_geometry.m_refine_map_compact.push_back( remap[old_nx*old_ny*old_k + old_nx*old_j + old_i] );
                }

                new_actnum[ new_nx*new_ny*new_k  + new_nx*new_j + new_i ] =
                        old_actnum[ old_nx*old_ny*old_k + old_nx*old_j + old_i ];

            }
        }
    }
    m_cornerpoint_geometry.m_rx = rx;
    m_cornerpoint_geometry.m_ry = ry;
    m_cornerpoint_geometry.m_rz = rz;

/*    m_cornerpoint_geometry.m_nx = rx*old_nx;
    m_cornerpoint_geometry.m_ny = ry*old_ny;
    m_cornerpoint_geometry.m_nz = rz*old_nz;
    m_cornerpoint_geometry.m_rx = 1;
    m_cornerpoint_geometry.m_ry = 1;
    m_cornerpoint_geometry.m_rz = 1;
*/
    m_cornerpoint_geometry.m_coord.swap( new_coord );
    m_cornerpoint_geometry.m_zcorn.swap( new_zcorn );
    m_cornerpoint_geometry.m_actnum.swap( new_actnum );
}

template<typename REAL>
void
Project<REAL>::refresh( int rx, int ry, int rz )
{
    Logger log = getLogger( "Project.refresh" );

    if( m_geometry_type == GEOMETRY_NONE ) {
        for( auto it = m_unprocessed_files.begin(); it!=m_unprocessed_files.end(); ++it ) {

            if( it->m_filetype == ECLIPSE_EGRID_FILE ) {
                try {
                    unsigned int nx, ny, nz, nr;
                    std::vector<REAL>  coord;
                    std::vector<REAL>  zcorn;
                    std::vector<int>   actnum;
                    eclipse::Properties properties;

                    eclipse::parseEGrid( nx, ny, nz, nr, coord, zcorn, actnum, properties, it->m_path );

                    m_geometry_type = GEOMETRY_CORNERPOINT_GRID;
                    m_cornerpoint_geometry.m_nx = nx;
                    m_cornerpoint_geometry.m_ny = ny;
                    m_cornerpoint_geometry.m_nz = nz;
                    m_cornerpoint_geometry.m_nr = nr;
                    m_cornerpoint_geometry.m_coord.swap( coord );
                    m_cornerpoint_geometry.m_zcorn.swap( zcorn );
                    m_cornerpoint_geometry.m_actnum.swap( actnum );
                    bakeCornerpointGeometry();
                    refineCornerpointGeometry( rx, ry, rz );
                }
                catch( const std::runtime_error& e ) {
                    LOGGER_ERROR( log, it->m_path << ": Parse error: " << e.what() );
                    break;
                }
                m_unprocessed_files.erase( it );

                LOGGER_DEBUG( log, "nx=" << m_cornerpoint_geometry.m_nx
                              << ", ny=" << m_cornerpoint_geometry.m_nx
                              << ", nz=" << m_cornerpoint_geometry.m_nz );
                break;
            }
            else if( it->m_filetype == FOOBAR_GRID_FILE ) {
                try {
                    unsigned int nx, ny, nz;
                    std::vector<REAL>  coord;
                    std::vector<REAL>  zcorn;
                    std::vector<int>   actnum;

                    FooBarParser::parseGeometry( nx,
                                                 ny,
                                                 nz,
                                                 coord,
                                                 zcorn,
                                                 actnum,
                                                 it->m_path );

                    m_geometry_type = GEOMETRY_CORNERPOINT_GRID;
                    m_cornerpoint_geometry.m_nx = nx;
                    m_cornerpoint_geometry.m_ny = ny;
                    m_cornerpoint_geometry.m_nz = nz;
                    m_cornerpoint_geometry.m_nr = 1;
                    m_cornerpoint_geometry.m_coord.swap( coord );
                    m_cornerpoint_geometry.m_zcorn.swap( zcorn );
                    m_cornerpoint_geometry.m_actnum.swap( actnum );
                    bakeCornerpointGeometry();
                    refineCornerpointGeometry( rx, ry, rz );
                }
                catch( const std::runtime_error& e ) {
                    LOGGER_ERROR( log, it->m_path << ": Parse error: " << e.what() );
                    break;
                }
                m_unprocessed_files.erase( it );
                break;
            }
            else if( it->m_filetype == FOOBAR_TXT_GRID_FILE ) {

                try {
                    unsigned int nx, ny, nz;
                    std::vector<REAL>  coord;
                    std::vector<REAL>  zcorn;
                    std::vector<int>   actnum;

                    FooBarParser::parseTxtGeometry( nx,
                                                    ny,
                                                    nz,
                                                    coord,
                                                    zcorn,
                                                    actnum,
                                                    it->m_path );

                    m_geometry_type = GEOMETRY_CORNERPOINT_GRID;
                    m_cornerpoint_geometry.m_nx = nx;
                    m_cornerpoint_geometry.m_ny = ny;
                    m_cornerpoint_geometry.m_nz = nz;
                    m_cornerpoint_geometry.m_nr = 1;
                    m_cornerpoint_geometry.m_coord.swap( coord );
                    m_cornerpoint_geometry.m_zcorn.swap( zcorn );
                    m_cornerpoint_geometry.m_actnum.swap( actnum );
                    bakeCornerpointGeometry();
                    refineCornerpointGeometry( rx, ry, rz );
                }
                catch( const std::runtime_error& e ) {
                    LOGGER_ERROR( log, it->m_path << ": Parse error: " << e.what() );
                    break;
                }


                m_unprocessed_files.erase( it );
//                m_geometry_set = true;
                break;
            }
        }
    }
    if( m_unprocessed_files.empty() ) {
        return;
    }
    if( m_geometry_type == GEOMETRY_NONE ) {
        LOGGER_ERROR( log, "Cannot process restart files without geometry" );
    }

    for( auto it = m_unprocessed_files.begin(); it!=m_unprocessed_files.end(); ++it ) {


        if( it->m_filetype == ECLIPSE_UNIFIED_RESTART_FILE ) {
            try {
                std::list<eclipse::ReportStep> report_steps;
                eclipse::parseUnifiedRestartFile( report_steps,
                                                  m_cornerpoint_geometry.m_actnum,
                                                  it->m_path );
                for( auto jt=report_steps.begin(); jt!=report_steps.end(); ++jt ) {
                    import( *jt, it->m_path );
                }
            }
            catch( const std::runtime_error& e ) {
                LOGGER_ERROR( log, it->m_path << ": Parse error: " << e.what() );
                break;
            }
       }
        else if( it->m_filetype == ECLIPSE_RESTART_FILE ) {
            try {
                std::list<eclipse::ReportStep> report_steps;
                eclipse::parseRestartFile( report_steps,
                                           m_cornerpoint_geometry.m_actnum,
                                           it->m_path );
                for( auto jt=report_steps.begin(); jt!=report_steps.end(); ++jt ) {
                    import( *jt, it->m_path );
                }
            }
            catch( const std::runtime_error& e ) {
                LOGGER_ERROR( log, it->m_path << ": Parse error: " << e.what() );
                break;
            }
        }
    }
    m_unprocessed_files.clear();

    for( unsigned int i=0; i< m_report_steps.size(); i++ ) {
        LOGGER_TRACE( log, "report step seqnum " <<  m_report_steps[i].m_seqnum );
        for( unsigned int j=0; j<m_solution_names.size(); j++ ) {
            LOGGER_TRACE( log, "  " << m_solution_names[j] << ": " <<
                          (m_report_steps[i].m_solutions[j].m_reader != READER_NONE ? "available" : "unavailable" ) );
        }
    }
}

template<typename REAL>
const bool
Project<REAL>::wellDefined(  const unsigned int report_step_ix,
                             const unsigned int well_ix  ) const
{
    Logger log = getLogger( package + ".wellDefined" );
    if( m_report_steps.size() <= report_step_ix ) {
        return false;
    }
    if( m_report_steps[ report_step_ix ].m_wells.size() <= well_ix ) {
        return false;
    }
    return m_report_steps[ report_step_ix ].m_wells[ well_ix ].m_defined;
}


template<typename REAL>
void
Project<REAL>::addWell( const eclipse::Well& ewell,
                       const unsigned int sequence_number )
{
    Logger log = getLogger( "Project.addWell.Eclipse" );


    ReportStep& report_step = reportStepBySeqNum( sequence_number );
    bool dump_well = false && ewell.m_name == "F-1H";
    if( !dump_well ) {
        //continue;
    }

    if( dump_well ) {
        LOGGER_DEBUG( log, "  Well '" << ewell.m_name <<
                      "', head=[" << ewell.m_head_i <<
                      ", " << ewell.m_head_j <<
                      ", " << ewell.m_head_k <<
                      "]  (" << report_step.m_date << ")" );
    }

    unsigned int well_index = 0u;
    auto it = m_well_name_lut.find( ewell.m_name );
    if( it == m_well_name_lut.end() ) {
        well_index = m_well_names.size();
        m_well_names.push_back( ewell.m_name );
        m_well_name_lut[ ewell.m_name ] = well_index;
    }
    else {
        well_index = it->second;
    }

    if( well_index <= report_step.m_wells.size() ) {
        unsigned int a = report_step.m_wells.size();
        report_step.m_wells.resize( well_index+1 );
        for( unsigned int i=a; i<report_step.m_wells.size(); i++ ) {
            report_step.m_wells[i].m_defined = false;
        }
    }


    Well& well = report_step.m_wells[ well_index ];
    well.m_defined = true;
    well.m_name = ewell.m_name;

    cornerPointCellCentroid( well.m_head,
                             ewell.m_head_i,
                             ewell.m_head_j,
                             ewell.m_head_k );

    well.m_branches.resize( ewell.m_branches.size() );
    for( size_t j=0; j<ewell.m_branches.size(); j++ ) {
        if( dump_well ) {
            LOGGER_DEBUG( log, "    Branch " << j );
        }
        well.m_branches[j].resize( 3*ewell.m_branches[j].size() );
        for( size_t k=0; k<ewell.m_branches[j].size(); k++ ) {
            cornerPointCellCentroid( well.m_branches[j].data() + 3*k,
                                     ewell.m_branches[j][k].m_i,
                                     ewell.m_branches[j][k].m_j,
                                     ewell.m_branches[j][k].m_k );
            if( dump_well ) {
                LOGGER_DEBUG( log, "        [" <<
                              ewell.m_branches[j][k].m_i << ", " <<
                              ewell.m_branches[j][k].m_j << ", " <<
                              ewell.m_branches[j][k].m_k << "], segment=" <<
                              ewell.m_branches[j][k].m_segment << ", pos="<<
                              well.m_branches[j].at(3*k + 0) << ", " <<
                              well.m_branches[j].at(3*k + 1) << ", " <<
                              well.m_branches[j].at(3*k + 2) );
            }

        }
    }
}

template<typename REAL>
void
Project<REAL>::import( const eclipse::ReportStep& e_step,
                       const std::string path )
{
    Logger log = getLogger( "Project.Eclipse.Reportstep.import" );

    ReportStep& report_step = reportStepBySeqNum( e_step.m_sequence_number );

    std::stringstream o;
    o << e_step.m_date.m_year  << "-"
      << (e_step.m_date.m_month < 10 ? "0" : "" ) << e_step.m_date.m_month << "-"
      << (e_step.m_date.m_day < 10 ? "0" : "" ) << e_step.m_date.m_day;
    report_step.m_date = o.str();


    //report_step.m_wells.resize( e_step.m_wells.size() );
    for( unsigned int i=0; i<e_step.m_wells.size(); i++ ) {
        addWell( e_step.m_wells[i], e_step.m_sequence_number );
        /*
        const Eclipse::Well& ewell = e_step.m_wells[i];
        bool dump_well =  ewell.m_name == "F-1H";
        if( !dump_well ) {
            //continue;
        }

        if( dump_well ) {
            LOGGER_DEBUG( log, "  Well '" << ewell.m_name <<
                          "', head=[" << ewell.m_head_i <<
                          ", " << ewell.m_head_j <<
                          ", " << ewell.m_head_k <<
                          "]  (" << report_step.m_date << ")" );
        }
        Well& well = report_step.m_wells[i];
        well.m_name = ewell.m_name;


        cornerPointCellCentroid( well.m_head,
                                 ewell.m_head_i,
                                 ewell.m_head_j,
                                 ewell.m_head_k );

        LOGGER_DEBUG( log, "old: " << well.m_branches.size() );
        well.m_branches.resize( ewell.m_branches.size() );
        for( size_t j=0; j<ewell.m_branches.size(); j++ ) {
            if( dump_well ) {
                LOGGER_DEBUG( log, "    Branch " << j );
            }
            well.m_branches[j].resize( 3*ewell.m_branches[j].size() );
            for( size_t k=0; k<ewell.m_branches[j].size(); k++ ) {
                cornerPointCellCentroid( well.m_branches[j].data() + 3*k,
                                         ewell.m_branches[j][k].m_i,
                                         ewell.m_branches[j][k].m_j,
                                         ewell.m_branches[j][k].m_k );
                if( dump_well ) {
                    LOGGER_DEBUG( log, "        [" <<
                                  ewell.m_branches[j][k].m_i << ", " <<
                                  ewell.m_branches[j][k].m_j << ", " <<
                                  ewell.m_branches[j][k].m_k << "], segment=" <<
                                  ewell.m_branches[j][k].m_segment << ", pos="<<
                                  well.m_branches[j].at(3*k + 0) << ", " <<
                                  well.m_branches[j].at(3*k + 1) << ", " <<
                                  well.m_branches[j].at(3*k + 2) );
                }

            }
        }
        */
    }

    for( auto kt=e_step.m_solutions.begin(); kt!=e_step.m_solutions.end(); ++kt ) {
        const eclipse::Solution& e_solution = *kt;

        Solution solution;
        solution.m_reader = READER_UNFORMATTED_ECLIPSE;
        solution.m_path = path;
        solution.m_location.m_unformatted_eclipse = e_solution.m_location;
        addSolution( solution,
                     e_solution.m_name,
                     e_step.m_sequence_number );

    }
}


template<typename REAL>
void
Project<REAL>::cornerPointCellCentroid( REAL*               p,
                                        const unsigned int  i,
                                        const unsigned int  j,
                                        const unsigned int  k )
{

    const unsigned int ni = m_cornerpoint_geometry.m_nx;
    const unsigned int nj = m_cornerpoint_geometry.m_ny;

    size_t global_ix = i + ni*(j + k*nj);
    if( m_cornerpoint_geometry.m_actnum[ global_ix ] == 0 ) {
        Logger log = getLogger( "Project.cornerPointCellCentroid" );
        LOGGER_ERROR( log, "cell is inactive" );
        p[0] = p[1] = p[2] = 0.f;
    }
    else {
        REAL point[3] = { 0.f, 0.f, 0.f };


        for(int kk=0; kk<2; kk++) {
            for(int jj=0; jj<2; jj++) {
                for(int ii=0; ii<2; ii++) {

                    int coord_ix = 6*( (i+ii) +
                                       (j+jj)*(ni+1) +
                                       (   0)*(ni+1)*(nj+1) );
                    float x1 = m_cornerpoint_geometry.m_coord[ coord_ix + 0 ];
                    float y1 = m_cornerpoint_geometry.m_coord[ coord_ix + 1 ];
                    float z1 = m_cornerpoint_geometry.m_coord[ coord_ix + 2 ];
                    float x2 = m_cornerpoint_geometry.m_coord[ coord_ix + 3 ];
                    float y2 = m_cornerpoint_geometry.m_coord[ coord_ix + 4 ];
                    float z2 = m_cornerpoint_geometry.m_coord[ coord_ix + 5 ];

                    int zcorn_ix = ( 2*i+ii ) +
                                   ( 2*j+jj )*(2*ni) +
                                   ( 2*k+kk )*(2*ni*2*nj);
                    float z = m_cornerpoint_geometry.m_zcorn[ zcorn_ix ];
                    float a = (z-z1)/(z2-z1);
                    float b = 1.f-a;
                    point[0] += (b*x1 + a*x2);
                    point[1] += (b*y1 + a*y2);
                    point[2] += z;
                }
            }
        }
        p[0] = point[0]/8.f;
        p[1] = point[1]/8.f;
        p[2] = point[2]/8.f;
    }

}


template<typename REAL>
unsigned int
Project<REAL>::getSolutionIndex( const std::string& name )
{
    auto it = m_solution_name_lut.find( name );
    if( it != m_solution_name_lut.end() ) {
        return it->second;
    }
    else {
        unsigned int index = m_solution_names.size();
        m_solution_name_lut[ name ] = index;
        m_solution_names.push_back( name );
        for( auto it=m_report_steps.begin(); it!=m_report_steps.end(); ++it ) {
            it->m_solutions.resize( it->m_solutions.size() + 1 );
            it->m_solutions.back().m_reader = READER_NONE;
        }
        return index;
    }
}

template<typename REAL>
typename Project<REAL>::ReportStep&
Project<REAL>::reportStepBySeqNum( unsigned int seqnum )
{
    for(auto it=m_report_steps.begin(); it!=m_report_steps.end(); ++it ) {
        if( it->m_seqnum == seqnum ) {
            return *it;
        }
    }
    m_report_steps.push_back( ReportStep() );
    ReportStep& step = m_report_steps.back();
    step.m_seqnum = seqnum;
    step.m_solutions.resize( m_solution_names.size() );
    for(auto it=step.m_solutions.begin(); it!=step.m_solutions.end(); ++it ) {
        it->m_reader = READER_NONE;
    }
    std::sort( m_report_steps.begin(),
               m_report_steps.end(),
               []( const ReportStep& a, const ReportStep& b ) { return a.m_seqnum < b.m_seqnum; } );

    for(auto it=m_report_steps.begin(); it!=m_report_steps.end(); ++it ) {
        if( it->m_seqnum == seqnum ) {
            return *it;
        }
    }
    // shouldn't happen, but makes the compiler happy
    return m_report_steps.back();
}


template<typename REAL>
void
Project<REAL>::addSolution( const Solution&     solution,
                            const std::string&  name,
                            const unsigned int  seqnum )
{
    unsigned int sol_ix = getSolutionIndex( name );
    ReportStep& step = reportStepBySeqNum( seqnum );
    step.m_solutions[sol_ix] = solution;
}


template<typename REAL>
unsigned int
Project<REAL>::solutions() const
{
    return m_solution_names.size();
}

template<typename REAL>
const std::string&
Project<REAL>::solutionName( unsigned int name_index ) const
{
    return m_solution_names.at( name_index );
}

template<typename REAL>
unsigned int
Project<REAL>::reportSteps() const
{
    return m_report_steps.size();
}

template<typename REAL>
unsigned int
Project<REAL>::nx() const
{
    return m_cornerpoint_geometry.m_rx*m_cornerpoint_geometry.m_nx;
}

template<typename REAL>
unsigned int
Project<REAL>::ny() const
{
    return m_cornerpoint_geometry.m_ry*m_cornerpoint_geometry.m_ny;
}

template<typename REAL>
unsigned int
Project<REAL>::nz() const
{
    return m_cornerpoint_geometry.m_rz*m_cornerpoint_geometry.m_nz;
}



template<typename REAL>
const std::vector<typename Project<REAL>::Well>&
Project<REAL>::wells( const unsigned int step )
{
    if( step >= m_report_steps.size() ) {
        throw std::runtime_error( "Illegal report step" );
    }
    return m_report_steps[ step ].m_wells;
}



template<typename REAL>
template<typename Bridge>
void
Project<REAL>::field( Bridge& bridge, const unsigned int solution, const unsigned int step )
{
    if( solution >= m_solution_names.size()) {
        throw std::runtime_error( "Illegal solution index" );
    }
    if( step >= m_report_steps.size() ) {
        throw std::runtime_error( "Illegal report step" );
    }
    Solution& sol = m_report_steps[ step ].m_solutions[ solution ];
    if( sol.m_reader == READER_UNFORMATTED_ECLIPSE ) {
        eclipse::Reader reader( sol.m_path );

        if( (m_cornerpoint_geometry.m_rx == 1) &&
            (m_cornerpoint_geometry.m_ry == 1) &&
            (m_cornerpoint_geometry.m_rz == 1) )
        {
            reader.blockContent( bridge.values(),
                                 bridge.minimum(),
                                 bridge.maximum(),
                                 sol.m_location.m_unformatted_eclipse );
            std::cerr << "BLRAGH\n";
        }
        else {
            std::vector< REAL > tmp( m_cornerpoint_geometry.m_nx*
                                     m_cornerpoint_geometry.m_ny*
                                     m_cornerpoint_geometry.m_ny );
            reader.blockContent( tmp.data(),
                                 bridge.minimum(),
                                 bridge.maximum(),
                                 sol.m_location.m_unformatted_eclipse );

            for(size_t i=0; i<m_cornerpoint_geometry.m_refine_map_compact.size(); i++ ) {
                bridge.values()[i] = tmp[ m_cornerpoint_geometry.m_refine_map_compact[i] ];
            }
            std::cerr << "MOO " << m_cornerpoint_geometry.m_refine_map_compact.size() << "\n";
/*
            uint rx = m_cornerpoint_geometry.m_rx;
            uint ry = m_cornerpoint_geometry.m_ry;
            uint rz = m_cornerpoint_geometry.m_rz;
            uint old_nx = m_cornerpoint_geometry.m_nx;
            uint old_ny = m_cornerpoint_geometry.m_ny;
            uint old_nz = m_cornerpoint_geometry.m_nz;
            uint new_nx = rx*old_nx;
            uint new_ny = ry*old_ny;
            uint new_nz = rz*old_nz;


            for( int new_k=0; new_k<new_nz; new_k++ ) {
                uint old_k = new_k/rz;
                for( int new_j=0; new_j<new_ny; new_j++ ) {
                    uint old_j = new_j/ry;
                    for( int new_i=0; new_i<new_nx; new_i++ ) {
                        uint old_i = new_i/rx;
                        bridge.values()[ new_nx*new_ny*new_k  + new_nx*new_j + new_i ] =
                                tmp[ old_nx*old_ny*old_k + old_nx*old_j + old_i ];

                    }
                }
            }
*/
        }
    }
    else {
        throw std::runtime_error( "No valid field for this solution and report step" );
    }
}

template<typename REAL>
bool
Project<REAL>::solution( Solution& solution,
                         const uint solution_ix,
                         const uint report_step )
{
    Logger log = getLogger( package + ".solution" );
    if( solution_ix >= m_solution_names.size() ) {
        LOGGER_ERROR( log, "Illegal solution index " << solution_ix );
        return false;
    }
    if( report_step >= m_report_steps.size() ) {
        LOGGER_ERROR( log, "Illegal report step " << report_step );
        return false;
    }
    solution = m_report_steps[ report_step ].m_solutions[ solution_ix ];
    return true;
}

template<typename REAL>
const std::string&
Project<REAL>::reportStepDate( unsigned int step ) const
{
    static const std::string illegal = "Illegal report step";
    if( m_report_steps.size() <= step ) {
        return illegal;
    }
    else {
        return m_report_steps[ step ].m_date;
    }
}


template class Project<float>;
//template void Project<float>::geometry<GridTessBridge>( GridTessBridge& );
template void Project<float>::field<render::GridFieldBridge>( render::GridFieldBridge&, const unsigned int, const unsigned int );
