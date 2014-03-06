/* Copyright STIFTELSEN SINTEF 2013
 * 
 * This file is part of FRView.
 * FRView is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * FRView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <string>
#include <list>
#include <vector>
#include <memory>
#include <unordered_map>
#include "dataset/AbstractDataSource.hpp"
#include "dataset/PolyhedralDataInterface.hpp"
#include "dataset/WellDataInterface.hpp"
#include "eclipse/Eclipse.hpp"

namespace dataset {
    class PolyhedralMeshSource;


    
class Project
        : public AbstractDataSource,
          public WellDataInterace
{
public:
    typedef float REAL;
    typedef render::GridTessBridge      Tessellation;   ///< Kill when adding Polyhedral inheritance
    typedef render::GridFieldBridge     Field;          ///< \todo Rename type.
    
    enum GeometryType {
        GEOMETRY_NONE,
        GEOMETRY_CORNERPOINT_GRID,
        GEOMETRY_POLYHEDRAL_MESH
    };

    Project(const std::string filename,
             int refine_i = 1,
             int refine_j = 1,
             int refine_k = 1 );
    
    virtual
    ~Project();


    GeometryType
    geometryType() const
    { return m_geometry_type; }

    template<typename Bridge>
    void
    field( Bridge& bridge, const unsigned int solution, const unsigned int step );


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
    cornerPointXYScale() const;

    REAL
    cornerPointZScale() const;

    // -------------------------------------------------------------------------
    /** \name Implementation of PolyhedralDataInterface */
    /** @{ */

    virtual
    void
    tessellation( Tessellation&                                  tessellation,
                  boost::shared_ptr<tinia::model::ExposedModel>  model,
                  const std::string&                             progress_description_key,
                  const std::string&                             progress_counter_key );

    /** @} */

    // -------------------------------------------------------------------------
    /** \name Implementation of WellDataInterface */
    /** @{ */
    
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

    const std::vector<Well>&
    wells( const unsigned int step );

    /** @} */
    
    // -------------------------------------------------------------------------
    


    // HACK. Public for debug gfx purposes.

    enum SolutionReader {
        READER_NONE,
        READER_UNFORMATTED_ECLIPSE,
        READER_FROM_SOURCE // Let source class handle reading
    };
    struct Solution {
        SolutionReader                              m_reader;
        std::string                                 m_path;
        union {
            eclipse::Block                          m_unformatted_eclipse;
            size_t                                  m_source_index;
        }                                           m_location;
    };

    std::shared_ptr<PolyhedralMeshSource>
    source() { return m_polyhedral_mesh_source; }
    
    const std::shared_ptr<PolyhedralMeshSource>
    source() const { return m_polyhedral_mesh_source; }

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
        VTK_XML_VTU_FILE,
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

    struct {
        std::vector<REAL>                               m_vertices;
        std::vector<int>                                m_tetrahedra;
    }                                               m_tetrahedral_geometry;
            
    
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

    std::shared_ptr<PolyhedralMeshSource>   m_polyhedral_mesh_source;
    
    
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

    // Used for sorting
    static bool compareReportStep(const ReportStep& a, const ReportStep& b) {
	return a.m_seqnum < b.m_seqnum;
    }
};

} // of namespace dataset
