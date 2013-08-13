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
#include "Eclipse.hpp"

template<typename REAL>
class Project : public boost::noncopyable
{
public:

    Project();

    template<class Container>
    Project( const Container& files );

    template<typename Bridge>
    void
    geometry( Bridge& bridge );

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
    struct {
        unsigned int                                    m_nx;
        unsigned int                                    m_ny;
        unsigned int                                    m_nz;
        unsigned int                                    m_nr;
        std::vector<REAL>                               m_coord;
        std::vector<REAL>                               m_zcorn;
        std::vector<int>                                m_actnum;
    }                                               m_cornerpoint_geometry;


private:
    enum FileType {
        ECLIPSE_EGRID_FILE,
        FOOBAR_GRID_FILE,
        FOOBAR_TXT_GRID_FILE,
        ECLIPSE_RESTART_FILE,
        ECLIPSE_UNIFIED_RESTART_FILE
    };
    enum SolutionReader {
        READER_NONE,
        READER_UNFORMATTED_ECLIPSE
    };
    enum GeometryType {
        GEOMETRY_NONE,
        GEOMETRY_CORNERPOINT_GRID
    };

    struct File {
        FileType                                    m_filetype;
        int                                         m_timestep;
        std::string                                 m_path;
    };
    struct Solution {
        SolutionReader                              m_reader;
        std::string                                 m_path;
        union {
            Eclipse::Block                          m_unformatted_eclipse;
        }                                           m_location;
    };


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
    import( const Eclipse::ReportStep& e_step,
            const std::string path );

    void
    addWell( const Eclipse::Well& ewell,
            const unsigned int sequence_number );


    void
    refresh();

};

template<typename REAL>
template<class Container>
Project<REAL>::Project( const Container& files )
    : m_geometry_type( GEOMETRY_NONE )
{
    for( auto it=files.begin(); it!=files.end(); ++it ) {
        addFile( *it );
    }
    refresh();
}
