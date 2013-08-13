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
#include "EclipseParser.hpp"
#include "GridTessBridge.hpp"
#include "GridFieldBridge.hpp"
#include "Logger.hpp"
#include "Project.hpp"
#include "CornerPointTessellator.hpp"
#include "FooBarParser.hpp"

using std::vector;
using std::string;
using std::list;


template<typename REAL>
Project<REAL>::Project()
    : m_geometry_type( GEOMETRY_NONE )
{
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
Project<REAL>::refresh( )
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
                    Eclipse::Properties properties;

                    Eclipse::parseEGrid( nx, ny, nz, nr, coord, zcorn, actnum, properties, it->m_path );

                    m_geometry_type = GEOMETRY_CORNERPOINT_GRID;
                    m_cornerpoint_geometry.m_nx = nx;
                    m_cornerpoint_geometry.m_ny = ny;
                    m_cornerpoint_geometry.m_nz = nz;
                    m_cornerpoint_geometry.m_nr = nr;
                    m_cornerpoint_geometry.m_coord.swap( coord );
                    m_cornerpoint_geometry.m_zcorn.swap( zcorn );
                    m_cornerpoint_geometry.m_actnum.swap( actnum );
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
                std::list<Eclipse::ReportStep> report_steps;
                Eclipse::parseUnifiedRestartFile( report_steps, it->m_path );
                for( auto jt=report_steps.begin(); jt!=report_steps.end(); ++jt ) {
                    Eclipse::ReportStep& e_step = *jt;

                    ReportStep& report_step = reportStepBySeqNum( e_step.m_sequence_number );
                    report_step.m_wells.resize( e_step.m_wells.size() );
                    for( unsigned int i=0; i<report_step.m_wells.size(); i++ ) {
                        Eclipse::Well& ewell = e_step.m_wells[i];
                        Well& well = report_step.m_wells[i];
                        well.m_name = ewell.m_name;

                        if( ewell.m_head_i > 0 && ewell.m_head_j > 0 && ewell.m_head_k > 0 ) {
                            float v[3];
                            cornerPointCellCentroid( v,
                                                     ewell.m_head_i-1,
                                                     ewell.m_head_j-1,
                                                     ewell.m_head_k-1 );
                            well.m_points.push_back( v[0] );
                            well.m_points.push_back( v[1] );
                            well.m_points.push_back( v[2] );
                        }

                        for(unsigned int j=0; j<ewell.m_completions.size(); j++ ) {
                            Eclipse::Completion& ecompletion = ewell.m_completions[j];

                            if( ecompletion.m_i > 0 && ecompletion.m_j > 0 && ecompletion.m_k > 0 ) {
                                float v[3];
                                cornerPointCellCentroid( v,
                                                         ecompletion.m_i-1,
                                                         ecompletion.m_j-1,
                                                         ecompletion.m_k-1 );
                                well.m_points.push_back( v[0] );
                                well.m_points.push_back( v[1] );
                                well.m_points.push_back( v[2] );
                            }
                        }
                    }


                    for( auto kt=e_step.m_solutions.begin(); kt!=e_step.m_solutions.end(); ++kt ) {
                        Eclipse::Solution& e_solution = *kt;

                        Solution solution;
                        solution.m_reader = READER_UNFORMATTED_ECLIPSE;
                        solution.m_path = it->m_path;
                        solution.m_location.m_unformatted_eclipse = e_solution.m_location;
                        addSolution( solution,
                                     e_solution.m_name,
                                     e_step.m_sequence_number );

                    }
                }
            }
            catch( const std::runtime_error& e ) {
                LOGGER_ERROR( log, it->m_path << ": Parse error: " << e.what() );
                break;
            }
       }
        else if( it->m_filetype == ECLIPSE_RESTART_FILE ) {
            try {
                std::list<Eclipse::ReportStep> report_steps;
                Eclipse::parseRestartFile( report_steps, it->m_path );
                for( auto jt=report_steps.begin(); jt!=report_steps.end(); ++jt ) {
                    Eclipse::ReportStep& e_step = *jt;


                    ReportStep& report_step = reportStepBySeqNum( e_step.m_sequence_number );
                    report_step.m_wells.resize( e_step.m_wells.size() );
                    for( unsigned int i=0; i<report_step.m_wells.size(); i++ ) {
                        Eclipse::Well& ewell = e_step.m_wells[i];
                        Well& well = report_step.m_wells[i];
                        well.m_name = ewell.m_name;

                        if( ewell.m_head_i > 0 && ewell.m_head_j > 0 && ewell.m_head_k > 0 ) {
                            float v[3];
                            cornerPointCellCentroid( v,
                                                     ewell.m_head_i-1,
                                                     ewell.m_head_j-1,
                                                     ewell.m_head_k-1 );
                            well.m_points.push_back( v[0] );
                            well.m_points.push_back( v[1] );
                            well.m_points.push_back( v[2] );
                        }

                        for(unsigned int j=0; j<ewell.m_completions.size(); j++ ) {
                            Eclipse::Completion& ecompletion = ewell.m_completions[j];

                            if( ecompletion.m_i > 0 && ecompletion.m_j > 0 && ecompletion.m_k > 0 ) {
                                float v[3];
                                cornerPointCellCentroid( v,
                                                         ecompletion.m_i-1,
                                                         ecompletion.m_j-1,
                                                         ecompletion.m_k-1 );
                                well.m_points.push_back( v[0] );
                                well.m_points.push_back( v[1] );
                                well.m_points.push_back( v[2] );
                            }
                        }
                    }


                    for( auto kt=e_step.m_solutions.begin(); kt!=e_step.m_solutions.end(); ++kt ) {
                        Eclipse::Solution& e_solution = *kt;
                        Solution solution;
                        solution.m_reader = READER_UNFORMATTED_ECLIPSE;
                        solution.m_path = it->m_path;
                        solution.m_location.m_unformatted_eclipse = e_solution.m_location;

                        addSolution( solution,
                                     e_solution.m_name,
                                     e_step.m_sequence_number );

                    }
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
        LOGGER_DEBUG( log, "report step seqnum " <<  m_report_steps[i].m_seqnum );
        for( unsigned int j=0; j<m_solution_names.size(); j++ ) {
            LOGGER_DEBUG( log, "  " << m_solution_names[j] << ": " <<
                          (m_report_steps[i].m_solutions[j].m_reader != READER_NONE ? "available" : "unavailable" ) );
        }
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
    return m_cornerpoint_geometry.m_nx;
}

template<typename REAL>
unsigned int
Project<REAL>::ny() const
{
    return m_cornerpoint_geometry.m_ny;
}

template<typename REAL>
unsigned int
Project<REAL>::nz() const
{
    return m_cornerpoint_geometry.m_nz;
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
Project<REAL>::geometry( Bridge& bridge )
{
    switch( m_geometry_type ) {
    case GEOMETRY_NONE:
        throw std::runtime_error( "no geometry set" );
        break;
    case GEOMETRY_CORNERPOINT_GRID:
        CornerPointTessellator<Bridge>::triangulate( bridge,
                                                     m_cornerpoint_geometry.m_nx,
                                                     m_cornerpoint_geometry.m_ny,
                                                     m_cornerpoint_geometry.m_nz,
                                                     m_cornerpoint_geometry.m_nr,
                                                     m_cornerpoint_geometry.m_coord,
                                                     m_cornerpoint_geometry.m_zcorn,
                                                     m_cornerpoint_geometry.m_actnum );
        break;
    }
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
        Eclipse::Reader reader( sol.m_path );
        reader.blockContent( bridge.values(),
                             bridge.minimum(),
                             bridge.maximum(),
                             sol.m_location.m_unformatted_eclipse );
    }
    else {
        throw std::runtime_error( "No valid field for this solution and report step" );
    }
}


template class Project<float>;
template void Project<float>::geometry<GridTessBridge>( GridTessBridge& );
template void Project<float>::field<GridFieldBridge>( GridFieldBridge&, const unsigned int, const unsigned int );
