/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once

#include <memory>
#include <vector>
#include <tinia/model/ExposedModel.hpp>
#include "GridTess.hpp"
#include "EclipseReader.hpp"


template<typename Tessellation>
class CornerPointTessellator
{
public:
    /** \name Forwarding
      * Forwarding of tessellation types and constants. */
    /** \@{ */
    typedef float                               SrcReal;
    typedef typename Tessellation::Real         Real;
    typedef typename Tessellation::Index        Index;
    typedef typename Tessellation::Real4        Real4;
    typedef typename Tessellation::Orientation  Orientation;
    typedef typename Tessellation::Segment      Segment;
    typedef typename Tessellation::Interface    Interface;
    static const Orientation ORIENTATION_I = Tessellation::ORIENTATION_I;
    static const Orientation ORIENTATION_J = Tessellation::ORIENTATION_J;
    static const Orientation ORIENTATION_K = Tessellation::ORIENTATION_K;
    static const Index IllegalIndex = (~(Index)0u);
    /** @} */

    /** Constructor, attaches itself to an empty tessellation. */
    CornerPointTessellator( Tessellation& tessellation );

    void
    tessellate( std::shared_ptr<tinia::model::ExposedModel> model,
                const std::string& what_key,
                const std::string& progress_key,
                const Index                  nx,
                const Index                  ny,
                const Index                  nz,
                const Index                  nr,
                const std::vector<SrcReal>&  coord,
                const std::vector<SrcReal>&  zcorn,
                const std::vector<int>&      actnum );

private:

    /** Helper struct used for ordering intersection paths along inbetween pillar walls.
      *
      * Everything is currently encoded as an uint32, where the single upper bit
      * is used to signal if the index is a vertex index or a intersection
      * index. Maximum index is thus restricted to 2^31-1.
      */
    struct NextIntersection
    {
        static inline NextIntersection intersection( Index ix ) { NextIntersection ni; ni.m_value = ix; return ni; }
        static inline NextIntersection vertex( Index ix ) { NextIntersection ni; ni.m_value = ix | (1u<<31u); return ni; }
        inline NextIntersection() : m_value( IllegalIndex ) {}
        inline bool isVertex() const { return (m_value & (1u<<31u)) != 0u; }
        inline bool isIntersection() const { return (m_value & (1u<<31u)) == 0u; }
        inline Index vertex() const { return m_value & (~(1u<<31u)); }
        inline Index intersection() const { return m_value; }
        Index        m_value;
    };

    /** A boundary between cells along a wall between two pillars.
      *
      * The boundary can be on either (or both) sides of the wall, i.e.,
      * m_cell_over and m_cell_under can be identical for one index but not for
      * both.
      */
    struct WallLine
    {
        Index               m_ends[2];          ///< Vertex index of end points at pillar 0 and pillar 1.
        Index               m_normals[2];       ///< Normal vector at edge end points.
        Index               m_cell_over[2];     ///< The index of the cell over (increasing k) this boundary, for side a and b.
        Index               m_cell_under[2];    ///< The index of the cell under (decreasing k) this boundary, for side a and b.
        Index               m_cutoff;           ///< Minimum p1 index of all lines above, used for early exit in intersection search.
        unsigned short int  m_side;             ///< Which side this boundary belongs to (1=side a, 2=side b, 3=both).
        bool                m_match_over;       ///< True if cells above are logical neighbours.
        bool                m_fault;            ///< True if edge is part of a fault.
    };

    /** An intersection between two boundary lines. */
    struct Intersection
    {
        Index             m_vtx_ix;             ///< Vertex index of intersection position.
        Index             m_upwrd_bndry_ix;     ///< Boundary with min index at pillar 0 (and max index at pillar 1).
        Index             m_dnwrd_bndry_ix;     ///< Boundary with max index at pillar 1 (and min index at pillar 0).
        NextIntersection  m_nxt_dnwrd_isec_ix;  ///< Next intersection along downwards boundary.
        NextIntersection  m_nxt_upwrd_isec_ix;  ///< Next intersection along upwards boundary.
        Index             m_n;                  ///< Index of normal vector.
        //        Real              m_u;
//        Real              m_z;
    };

    /** Enumeration of logical cell corners in IJ-plane. */
    enum {
        CELL_CORNER_O11 = 0,    ///< Corner (1,1) of cell c(i,j).
        CELL_CORNER_O01 = 1,    ///< Corner (0,1) of cell c(i,j).
        CELL_CORNER_O10 = 2,    ///< Corner (1,0) of cell c(i,j).
        CELL_CORNER_O00 = 3     ///< Corner (0,0) of cell c(i,j).
    };

    /** Enumeration of locical cell edges in the IJ-plane. */
    enum {
        CELL_WALL_O00_D10 = 0,  ///< Edge [(0,0),(1,0)] of cell c(i,j).
        CELL_WALL_O00_D01 = 1,  ///< Edge [(0,0),(0,1)] of cell c(i,j).
        CELL_WALL_O10_D01 = 2,  ///< Edge [(1,0),(1,1)] of cell c(i,j).
        CELL_WALL_O01_D10 = 3   ///< Edge [(0,1),(1,1)] of cell c(i,j).
    };

    Tessellation&                   m_tessellation;

    void
    findActiveCellsInColumn( Index*            active_cell_list,
                             Index&            active_cell_count,
                             const int* const  actnum,
                             const Index       stride,
                             const Index       nz );

    void
    uniquePillarVertices( std::vector<Index>&   adjacent_cells,
                          Index*                zcorn_ix_00,
                          Index*                zcorn_ix_01,
                          Index*                zcorn_ix_10,
                          Index*                zcorn_ix_11,
                          const SrcReal* const  zcorn_00,
                          const SrcReal* const  zcorn_01,
                          const SrcReal* const  zcorn_10,
                          const SrcReal* const  zcorn_11,
                          const Index           stride,
                          const SrcReal* const  coord,
                          const Index* const    active_cell_list_00,
                          const Index* const    active_cell_list_01,
                          const Index* const    active_cell_list_10,
                          const Index* const    active_cell_list_11,
                          const Index           active_cell_count_00,
                          const Index           active_cell_count_01,
                          const Index           active_cell_count_10,
                          const Index           active_cell_count_11 );

    void
    pillarEdges( const Index               offset,
                 const std::vector<Index>& adjacent_cells,
                 const Index*              cell_map_00,
                 const Index*              cell_map_01,
                 const Index*              cell_map_10,
                 const Index*              cell_map_11,
                 const Index               stride );


    /** Extracts all line segments across a pillar wall. */
    void
    extractWallLines( std::vector<WallLine>&      boundary_lines,
                      Index*               boundary_line_index_a,
                      Index*               boundary_line_index_b,
                      const Index* const   zcorn_ix_a_0,
                      const Index* const   zcorn_ix_a_1,
                      const Index* const   zcorn_ix_b_0,
                      const Index* const   zcorn_ix_b_1,
                      const Index* const   active_cell_list_a,
                      const Index* const   active_cell_list_b,
                      const Index          active_cell_count_a,
                      const Index          active_cell_count_b,
                      const Index* const    cell_map_a,
                      const Index* const    cell_map_b,
                      const SrcReal* const  o0_coord,
                      const SrcReal* const  o1_coord,
                      const Index           stride,
                      const Index           adjacent_stride );

    /** Find intersections on a pillar wall, defining line segments for affected edges. */
    void
    intersectWallLines( std::vector<Intersection>&    intersections,
                        std::vector<Index>&    chains,
                        Index*                 chain_offsets,
                        const std::vector<WallLine>&  wall_lines,
                        const float*                  pillar_a,
                        const float*                  pillar_b );

    /** Extract wireframe edges across a pillar wall. */
    void
    wallEdges( const std::vector<WallLine>&      boundaries,
               const std::vector<Intersection>&  intersections,
               const std::vector<Index>&  chains,
               const Index* const         chain_offsets );


    /** Stitch the wall between two pillars, simple case where there are no intersections. */
    void
    stitchPillarsNoIntersections( const Orientation             orientation,
                                  const std::vector<WallLine>&  boundaries,
                                  const SrcReal* const          o0_coord,
                                  const SrcReal* const          o1_coord );


    /** Stitch the wall between two pillars, do handle intersections. */
    void
    stitchPillarsHandleIntersections( const Orientation                 orientation,
                                      const std::vector<WallLine>&      boundaries,
                                      const std::vector<Intersection>&  intersections,
                                      const std::vector<Index>&         chains,
                                      const Index* const                chain_offsets,
                                      const SrcReal* const              o0_coord,
                                      const SrcReal* const              o1_coord );

    /** Stitch the quadrilaterals between cells in a stack of cells. */
    void
    stitchTopBottom( const Index* const   ci0j0_active_cell_list,
                     const Index          ci0j0_active_cell_count,
                     const Index* const   ci0j0_p00_p10_wall_line_ix,
                     const Index* const   ci0j0_p00_p01_wall_line_ix,
                     const Index* const   ci0j0_p10_p11_wall_line_ix,
                     const Index* const   ci0j0_p01_p11_wall_line_ix,
                     const Index* const   ci0j0_i0j0_zcorn_ix,
                     const Index* const   ci0j0_i1j0_zcorn_ix,
                     const Index* const   ci0j0_i0j1_zcorn_ix,
                     const Index* const   ci0j0_i1j1_zcorn_ix,
                     const SrcReal* const  o00_coord,
                     const SrcReal* const  o10_coord,
                     const SrcReal* const  o01_coord,
                     const SrcReal* const  o11_coord,
                     const Index* const   pi1j0_along_i_chains,
                     const Index* const   pi1j0_along_i_chain_offsets,
                     const Index* const   pi1j1_along_i_chains,
                     const Index* const   pi1j1_along_i_chain_offsets,
                     const Index* const   pi0j1_along_j_chains,
                     const Index* const   pi0j1_along_j_chain_offsets,
                     const Index* const   pi1j1_along_j_chains,
                     const Index* const   pi1j1_along_j_chain_offsets,
                     const Index* const            cell_map,
                     const Index          stride );



};
