/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#pragma once

#include <vector>
#include "GridTess.hpp"
#include "EclipseReader.hpp"

template<typename Bridge>
class CornerPointTessellator
{
public:
    typedef typename Bridge::REAL REAL;

    static
    void
    triangulate( Bridge&                   bridge,
                 const unsigned int        nx,
                 const unsigned int        ny,
                 const unsigned int        nz,
                 const unsigned int        nr,
                 const std::vector<REAL>&  coord,
                 const std::vector<REAL>&  zcorn,
                 const std::vector<int>&   actnum );

private:
    static const float          epsilon;
    static const unsigned int   end_flag;
    static const unsigned int   end_mask;



    /** A boundary between cells along a wall between two pillars.
      *
      * The boundary can be on either (or both) sides of the wall, i.e.,
      * m_cell_over and m_cell_under can be identical for one index but not for
      * both.
      */
    struct WallLine
    {
        /** Vertex index of end points at pillar 0 and pillar 1. */
        unsigned int    m_ends[2];
        /** Which side this boundary belongs to (1=side a, 2=side b, 3=both). */
        unsigned int    m_side;
        /** Minimum p1 index of all lines above, used for early exit in intersection search. */
        unsigned int    m_cutoff;
        /** The index of the cell over (increasing k) this boundary, for side a and b. */
        unsigned int    m_cell_over[2];
        /** The index of the cell under (decreasing k) this boundary, for side a and b. */
        unsigned int    m_cell_under[2];
        /** True if cells above are logical neighbours. */
        bool            m_match_over;
    };

    /** An intersection between two boundary lines. */
    struct Intersection
    {
        /** Vertex index of intersection position.*/
        unsigned int    m_vtx_ix;
        /** Boundary with min index at pillar 0 (and max index at pillar 1) */
        unsigned int    m_upwrd_bndry_ix;
        /** Boundary with max index at pillar 1 (and min index at pillar 0) */
        unsigned int    m_dnwrd_bndry_ix;
        /** Minimum distance between intersection and pillar 0. */
        float           m_distance;
        /** Next intersection along downwards boundary. */
        unsigned int    m_nxt_dnwrd_isec_ix;
        /** Next intersection along upwards boundary. */
        unsigned int    m_nxt_upwrd_isec_ix;
    };

    // cell layout:
    //
    //      p(i,j) +------------+ p(i+1,j)
    //             | I0J0  I1J0 |
    //             |            |
    //             |   c(i,j)   |
    //             |            |
    //             | I0J1  I1J1 |
    //    p(i,j+1) +------------+ p(i+1,j+1)
    enum {
        CELL_CORNER_O11 = 0,
        CELL_CORNER_O01 = 1,
        CELL_CORNER_O10 = 2,
        CELL_CORNER_O00 = 3
    };
    enum {
        CELL_WALL_O00_D10 = 0,
        CELL_WALL_O00_D01 = 1,
        CELL_WALL_O10_D01 = 2,
        CELL_WALL_O01_D10 = 3
    };

    static
    void
    findActiveCellsInColumn( unsigned int*       active_cell_list,
                             unsigned int&       active_cell_count,
                             const int* const    actnum,
                             const unsigned int  stride,
                             const unsigned int  nz );

    static
    void
    uniquePillarVertices( Bridge&                     bridge,
                          std::vector<unsigned int>&  adjacent_cells,
                          unsigned int*               zcorn_ix_00,
                          unsigned int*               zcorn_ix_01,
                          unsigned int*               zcorn_ix_10,
                          unsigned int*               zcorn_ix_11,
                          const REAL* const           zcorn_00,
                          const REAL* const           zcorn_01,
                          const REAL* const           zcorn_10,
                          const REAL* const           zcorn_11,
                          const unsigned int          stride,
                          const REAL* const           coord,
                          const unsigned int* const   active_cell_list_00,
                          const unsigned int* const   active_cell_list_01,
                          const unsigned int* const   active_cell_list_10,
                          const unsigned int* const   active_cell_list_11,
                          const unsigned int          active_cell_count_00,
                          const unsigned int          active_cell_count_01,
                          const unsigned int          active_cell_count_10,
                          const unsigned int          active_cell_count_11 );

    static
    void
    pillarEdges( Bridge&                          bridge,
                 const unsigned int               offset,
                 const std::vector<unsigned int>& adjacent_cells,
                 const int*                       cell_map_00,
                 const int*                       cell_map_01,
                 const int*                       cell_map_10,
                 const int*                       cell_map_11,
                 const unsigned int               stride );


    /** Extracts all line segments across a pillar wall. */
    static
    void
    extractWallLines( std::vector<WallLine>&      boundary_lines,
                      unsigned int*               boundary_line_index_a,
                      unsigned int*               boundary_line_index_b,
                      const unsigned int* const   zcorn_ix_a_0,
                      const unsigned int* const   zcorn_ix_a_1,
                      const unsigned int* const   zcorn_ix_b_0,
                      const unsigned int* const   zcorn_ix_b_1,
                      const unsigned int* const   active_cell_list_a,
                      const unsigned int* const   active_cell_list_b,
                      const unsigned int          active_cell_count_a,
                      const unsigned int          active_cell_count_b,
                      const int* const            cell_map_a,
                      const int* const            cell_map_b,
                      const unsigned int          stride,
                      const unsigned int          adjacent_stride );

    /** Find intersections on a pillar wall, defining line segments for affected edges. */
    static
    void
    intersectWallLines( Bridge&                       bridge,
                        std::vector<Intersection>&    intersections,
                        std::vector<unsigned int>&    chains,
                        unsigned int*                 chain_offsets,
                        const std::vector<WallLine>&  wall_lines,
                        const float*                  pillar_a,
                        const float*                  pillar_b );

    /** Extract wireframe edges across a pillar wall. */
    static
    void
    wallEdges( Bridge&                           bridge,
               const std::vector<WallLine>&      boundaries,
               const std::vector<Intersection>&  intersections,
               const std::vector<unsigned int>&  chains,
               const unsigned int* const         chain_offsets );


    /** Stitch the wall between two pillars, simple case where there are no intersections. */
    static
    void
    stitchPillarsNoIntersections( Bridge&                       bridge,
                                  const typename Bridge::Orientation  orientation,
                                  const std::vector<WallLine>&  boundaries );


    /** Stitch the wall between two pillars, do handle intersections. */
    static
    void
    stitchPillarsHandleIntersections(  Bridge&                             bridge,
                                       const typename Bridge::Orientation  orientation,
                                       const std::vector<WallLine>&        boundaries,
                                       const std::vector<Intersection>&    intersections,
                                       const std::vector<unsigned int>&    chains,
                                       const unsigned int* const           chain_offsets );


    static
    void
    intersectionArcsPolygon( Bridge& bridge,
                            std::vector<typename Bridge::Segment>&  segments,
                            const typename Bridge::Orientation  orientation,
                            const std::vector<Intersection>&    intersections,
                            const unsigned int first_upper_isec,
                            const unsigned int first_lower_isec );

    /** Stitch the quadrilaterals between cells in a stack of cells. */
    static
    void
    stitchTopBottom( Bridge&                     bridge,
                     const unsigned int* const   ci0j0_active_cell_list,
                     const unsigned int          ci0j0_active_cell_count,
                     const unsigned int* const   ci0j0_p00_p10_wall_line_ix,
                     const unsigned int* const   ci0j0_p00_p01_wall_line_ix,
                     const unsigned int* const   ci0j0_p10_p11_wall_line_ix,
                     const unsigned int* const   ci0j0_p01_p11_wall_line_ix,
                     const unsigned int* const   ci0j0_i0j0_zcorn_ix,
                     const unsigned int* const   ci0j0_i1j0_zcorn_ix,
                     const unsigned int* const   ci0j0_i0j1_zcorn_ix,
                     const unsigned int* const   ci0j0_i1j1_zcorn_ix,
                     const unsigned int* const   pi1j0_along_i_chains,
                     const unsigned int* const   pi1j0_along_i_chain_offsets,
                     const unsigned int* const   pi1j1_along_i_chains,
                     const unsigned int* const   pi1j1_along_i_chain_offsets,
                     const unsigned int* const   pi0j1_along_j_chains,
                     const unsigned int* const   pi0j1_along_j_chain_offsets,
                     const unsigned int* const   pi1j1_along_j_chains,
                     const unsigned int* const   pi1j1_along_j_chain_offsets,
                     const int* const            cell_map,
                     const unsigned int          stride );



    static
    unsigned int
    segmentIntersection( Bridge& bridge,
                         const unsigned int a0, const unsigned int a1,
                         const unsigned int b0, const unsigned int b1,
                         const float* pillar_a,
                         const float* pillar_b );

    static
    REAL
    distancePointLineSquared( Bridge& bridge,
                              const unsigned int p,
                              const unsigned int l0,
                              const unsigned int l1 );


};
