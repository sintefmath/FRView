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
#include <list>
#include <vector>
#include <boost/utility.hpp>
#include <unordered_map>
#include "eclipse/Eclipse.hpp"

namespace dataset {

template<typename REAL>
class Project : public boost::noncopyable
{
public:
    enum GeometryType {
        GEOMETRY_NONE,
        GEOMETRY_CORNERPOINT_GRID
    };

    Project(const std::string filename,
             int refine_i = 1,
             int refine_j = 1,
             int refine_k = 1 );


    GeometryType
    geometryType() const
    { return m_geometry_type; }

    template<typename Bridge>
    void
    field( Bridge& bridge, const unsigned int solution, const unsigned int step );

    const unsigned int
    wellCount() const { return m_well_names.size(); }


    const std::string&
    wellName( unsigned int well_ix ) const { return m_well_names[ well_ix ]; }

    const bool
    wellDefined( const unsigned int report_step_ix,
                 const unsigned int well_ix ) const;

    const float*
    wellHeadPosition( const unsigned int report_step_ix,
                      const unsigned int well_ix ) const
    {
        return m_report_steps[ report_step_ix ].m_wells[ well_ix ].m_head;
    }

    const unsigned int
    wellBranchCount( const unsigned int report_step_ix,
                     const unsigned int well_ix ) const
    {
        return m_report_steps[ report_step_ix ].m_wells[ well_ix ].m_branches.size();
    }

    const std::vector<float>&
    wellBranchPositions( const unsigned int report_step_ix,
                         const unsigned int well_ix,
                         const unsigned int branch_ix )
    {
        return m_report_steps[ report_step_ix ].m_wells[ well_ix ].m_branches[ branch_ix ];
    }


    unsigned int
    solutions() const;

    const std::string&
    solutionName( unsigned int name_index ) const;

    const std::string&
    reportStepDate( unsigned int step ) const;

    unsigned int
    reportSteps() const;

    unsigned int
    reportStepSeqNum( unsigned int step_index ) const;

    unsigned int
    nx() const;

    unsigned int
    ny() const;

    unsigned int
    nz() const;

    unsigned int
    nr() const { return m_cornerpoint_geometry.m_nr; }

    const std::vector<REAL>
    cornerPointCoord() const { return m_cornerpoint_geometry.m_coord; }

    const std::vector<REAL>
    cornerPointZCorn() const { return m_cornerpoint_geometry.m_zcorn; }

    const std::vector<int>
    cornerPointActNum() const { return m_cornerpoint_geometry.m_actnum; }


    REAL
    cornerPointXYScale() const { return m_cornerpoint_geometry.m_xyscale; }

    REAL
    cornerPointZScale() const { return m_cornerpoint_geometry.m_zscale; }

    struct Well
    {
        bool                                        m_defined;
        std::string                                 m_name;
        float                                       m_head[3];
        std::vector< std::vector<float> >           m_branches;
    };

    // HACK, will be replaced by bridge API
    const std::vector<Well>&
    wells( const unsigned int step );

    // HACK. Public for debug gfx purposes.

    enum SolutionReader {
        READER_NONE,
        READER_UNFORMATTED_ECLIPSE
    };
    struct Solution {
        SolutionReader                              m_reader;
        std::string                                 m_path;
        union {
            eclipse::Block                          m_unformatted_eclipse;
        }                                           m_location;
    };

    const std::vector<int>&
    fieldRemap() const { return m_cornerpoint_geometry.m_refine_map_compact; }

    bool
    solution( Solution& solution,
              const uint solution_ix,
              const uint report_step );

private:
    enum FileType {
        ECLIPSE_EGRID_FILE,
        FOOBAR_GRID_FILE,
        FOOBAR_TXT_GRID_FILE,
        ECLIPSE_RESTART_FILE,
        ECLIPSE_UNIFIED_RESTART_FILE
    };

    struct File {
        FileType                                    m_filetype;
        int                                         m_timestep;
        std::string                                 m_path;
    };
    struct {
        unsigned int                                    m_nx;
        unsigned int                                    m_ny;
        unsigned int                                    m_nz;
        unsigned int                                    m_rx;
        unsigned int                                    m_ry;
        unsigned int                                    m_rz;
        unsigned int                                    m_nr;
        REAL                                            m_xyscale;
        REAL                                            m_zscale;
        std::vector<REAL>                               m_coord;
        std::vector<REAL>                               m_zcorn;
        std::vector<int>                                m_actnum;
        std::vector<int>                                m_refine_map_compact;
    }                                               m_cornerpoint_geometry;

    struct ReportStep {
        unsigned int                                m_seqnum;
        std::string                                 m_date;
        std::vector<Solution>                       m_solutions;
        std::vector<Well>                           m_wells;
    };

    GeometryType                                    m_geometry_type;

    std::vector<std::string>                        m_solution_names;
    std::unordered_map<std::string,unsigned int>    m_solution_name_lut;
    std::vector<std::string>                        m_well_names;
    std::unordered_map<std::string,unsigned int>    m_well_name_lut;

    std::vector<ReportStep>                         m_report_steps;
    std::list<File>                                 m_unprocessed_files;

    void
    refineCornerpointGeometry( unsigned int rx,
                               unsigned int ry,
                               unsigned int rz );

    void
    bakeCornerpointGeometry();

    ReportStep&
    reportStepBySeqNum( unsigned int seqnum );

    unsigned int
    getSolutionIndex( const std::string& name );

    void
    addSolution( const Solution&     solution,
                 const std::string&  name,
                 const unsigned int  report_step );


    void
    cornerPointCellCentroid( REAL*               p,
                             const unsigned int  i,
                             const unsigned int  j,
                             const unsigned int  k );

    void
    addFile( const std::string& file );

    void
    import( const eclipse::ReportStep& e_step,
            const std::string path );

    void
    addWell( const eclipse::Well& ewell,
            const unsigned int sequence_number );


    void
    refresh( int rx, int ry, int rz );

};

} // of namespace dataset
