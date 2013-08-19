/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <list>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "utils/Logger.hpp"
#include "utils/PerfTimer.hpp"
#include "cornerpoint/Tessellator.hpp"
#include "cornerpoint/PillarFloorSampler.hpp"
#include "cornerpoint/PillarWallSampler.hpp"
#ifdef CHECK_INVARIANTS
#include "CellSanityChecker.hpp"
#endif
#include "render/GridTessBridge.hpp"

namespace cornerpoint {
    using std::vector;
    static const std::string package = "CornerPointTessellator";

template<typename Tessellation>
Tessellator<Tessellation>::Tessellator( Tessellation& tessellation )
    : m_tessellation( tessellation )
{}

// pillar layout
//   p(i-1,j-1) ----- p(i  ,j-1) ----- p(i+1,j-1) --> i (inner loop)
//      |                 |||               |
//      |    C(i-1,j-1)   |||  C(i  ,j-1)   |
//      |         0       |||       1       |
//   p(i-1,j  ) ===== P(i  ,j  ) ----- p(i+1,j  )
//      |                  |                |
//      |    C(i-1,j  )    |   C(i  ,j  )   |
//      |         2        |        3       |
//   p(i-1,j+1) ----- p(i  ,j+1) ----- p(i+1,j+1)
//      |
//      V
//      j (outer loop)
//
// Loop over j=0..ny:
// - Loop over i=0..nx:
//   - Find active cells in cell column c(i,j):
//     - result in jm0_active_cell[i+1].
//   - Find all unique vertices along pillar p(i,j):
//     - uses cells columns c(i-1,j-1), c(i,j-1), c(i-1,j), and c(i,j).
//     - result in jm1_zcorn_ix[ i-1 ] @ I1J1               -> c(i-1, j-1 )
//     - result in jm1_zcorn_ix[ i   ] @ I0J1               -> c(i,   j-1 )
//     - result in jm0_zcorn_ix[ i-1 ] @ I1J0               -> c(i-1, j   )
//     - result in jm0_zcorn_ix[ i   ] @ I0J0               -> c(i,   j   )
//
//   - Handle wall between pillars p(i,j-1) and p(i,j) (a = c(i-1,j-1), b = c(i,j-1)):
//     - Find wall lines on wall between p(i,j-1) and p(i,j)
//       - input from jm1_zcorn_ix[ i-1 ] @ I1J0            -> c(i-1, j-1 )
//       - input from jm1_zcorn_ix[ i-1 ] @ I1J1            -> c(i-1, j-1 )
//       - input from jm1_zcorn_ix[ i   ] @ I0J0            -> c(i,   j-1 )
//       - input from jm1_zcorn_ix[ i   ] @ I0J1            -> c(i,   j-1 )
//       - result in jm1_wall_line_ix[ i-1 ] @ I1J0_I1J1    -> c(i-1, j-1 )
//       - result in jm1_wall_line_ix[ i   ] @ I0J0_I0J1    -> c(i,   j-1 )
//     - Find and insert intersections on wall
//       - result in pi0jm1_pi0j0_chains[ i ]               -> p(i,j-1)
//     - Triangulate wall
//     - Remap intersection indices to vertex indices
//       - input and result in pi0jm1_pi0j0_chains[ i ]
//
//   - Handle wall between pillars p(i-1,j) and p(i,j) (a = c(i-1,j), b  = c(i-1,j-1)):
//     - Find wall lines on wall between  p(i-1,j) and p(i,j):
//       - input from jm0_zcorn_ix[ i-1 ] @ I0J0            -> c(i-1, j   )
//       - input from jm0_zcorn_ix[ i-1 ] @ I1J0            -> c(i-1, j   )
//       - input from jm1_zcorn_ix[ i-1 ] @ I0J1            -> c(i-1, j-1 )
//       - input from jm1_zcorn_ix[ i-1 ] @ I1J1            -> c(i-1, j-1 )
//       - result in jm0_wall_line_ix[ i-1 ] @ I0J0_I1J0    -> c(i-1, j   )
//       - result in jm1_wall_line_ix[ i-1 ] @ I0J1_I1J1    -> c(i-1, j-1 )
//     - Find and insert intersections on wall
//       - result in pi0j0_pi1j0_chains[i-1]                -> p(i-1,j)
//     - Triangulate wall
//     - Remap intersection indices to vertex indices
//       - input and result in pi0j0_pi1j0_chains
//
//   - Stitch top and bottom of cell c(i-1,j-1)
//     - pi0j0_d10_chains[i-1]      -> [ p(i-1,j  ) + (1,0) ] -> WALL_O01_D10  (this j)
//     - pi0jm1_d10_chains[i-1]     -> [ p(i-1,j-1) + (1,0) ] -> WALL_O00_D10  (prev j)
//     - pi0jm1_d01_chains[ i ]     -> [ p(i  ,j-1) + (0,1) ] -> WALL_O10_D01  (this i)
//     - pi0jm1_d01_chains[ i-1 ]   -> [ p(i-1,j-1) + (0,1) ] -> WALL_O00_D01  (prev i)

//   p(i-1,j-1) ----- p(i  ,j-1) ----- p(i+1,j-1) --> i (inner loop)
//      |                 |||               |
//      |    C(i-1,j-1)   |||  C(i  ,j-1)   |
//      |         0       |||       1       |
//   p(i-1,j  ) ===== P(i  ,j  ) ----- p(i+1,j  )
//      |                  |                |
//      |    C(i-1,j  )    |   C(i  ,j  )   |
//      |         2        |        3       |
//   p(i-1,j+1) ----- p(i  ,j+1) ----- p(i+1,j+1)



template<typename Tessellation>
void
Tessellator<Tessellation>::tessellate( boost::shared_ptr<tinia::model::ExposedModel> model,
                                                  const std::string& what_key,
                                                  const std::string& progress_key,
                                                  const Index             nx,
                                                  const Index             ny,
                                                  const Index             nz,
                                                  const Index             nr,
                                                  const vector<SrcReal>&  coord,
                                                  const vector<SrcReal>&  zcorn,
                                                  const vector<int>&      actnum )
{
    Logger log = getLogger( package + ".triangulate" );

    static const Index chain_offset_stride = 4*nz+1;

    // enumeration of active cells is different from how we traverse the grid.
    vector<Index> cell_map( nx*ny*nz, IllegalIndex );
    Index active_cells = 0;
    for( size_t i=0; i<actnum.size(); i++ ) {
        if( actnum[i] != 0 ) {
            cell_map[i] = active_cells++;
        }
    }
    m_tessellation.setCellCount( active_cells );
    m_tessellation.reserveVertices( 8*active_cells );
    m_tessellation.reserveEdges( 12*active_cells );
    m_tessellation.reserveTriangles( 2*6*active_cells );

    m_tessellation.addNormal( Real4( 1.f, 0.f, 0.f ) );

    LOGGER_DEBUG( log, "active cells = " << active_cells <<
                       " ("  << ((100.f*active_cells)/actnum.size()) << "%)." );


    vector<Index> pi0jm1_d01_chains;
    vector<Index> pi0jm1_d01_chain_offsets( (nx+1)*chain_offset_stride, IllegalIndex );
    vector<Index> pi0jm1_d10_chains;
    vector<Index> pi0jm1_d10_chain_offsets( nx*chain_offset_stride, IllegalIndex );
    vector<Index> pi0j0_d10_chains;
    vector<Index> pi0j0_d10_chain_offsets( nx*chain_offset_stride, IllegalIndex );
    vector<Index> jm1_active_cell_list( (nx+2)*(nz) );
    vector<Index> jm1_active_cell_count( (nx+2), 0u );
    vector<Index> jm0_active_cell_list( (nx+2)*(nz) );
    vector<Index> jm0_active_cell_count( (nx+2), 0u );
    vector<Index> jm1_zcorn_ix( nx*4*2*nz );
    vector<Index> jm0_zcorn_ix( nx*4*2*nz );
    vector<Index> jm1_wall_line_ix( nx*4*2*nz );
    vector<Index> jm0_wall_line_ix( nx*4*2*nz );

    LOGGER_DEBUG( log, "Tessellating grid..." );

    PerfTimer start;
    for( Index j=0; j<ny+1; j++ ) {
        int pn = (j-1*100)/ny;
        int cn = (j*100)/ny;
        if( pn != cn ) {
            std::stringstream o;
            o << "Tessellating grid... (" << cn << "%)";
            model->updateElement<std::string>( what_key, o.str() );
            model->updateElement<int>( progress_key, std::min( 100u, ((j*100)/ny) ) );
        }

        pi0jm1_d01_chains.clear();
        pi0j0_d10_chains.clear();

#if CHECK_INVARIANTS
        std::fill( jm0_active_cell_list.begin(), jm0_active_cell_list.end(), IllegalIndex );
        std::fill( jm0_active_cell_count.begin(), jm0_active_cell_count.end(), IllegalIndex );
        std::fill( pi0jm1_d01_chain_offsets.begin(), pi0jm1_d01_chain_offsets.end(), IllegalIndex );
        std::fill( pi0j0_d10_chain_offsets.begin(), pi0j0_d10_chain_offsets.end(), IllegalIndex );
        std::fill( jm0_wall_line_ix.begin(), jm0_wall_line_ix.end(), ~1u );
#endif


        // Iterate over all i's.
        jm0_active_cell_count[0] = 0u;
        for( Index i=0; i<nx+1; i++ ) {

            try {
                // Determine the active cells in the column
                if( i < nx && j < ny ) {
                    findActiveCellsInColumn( jm0_active_cell_list.data() + nz*(i+1),
                                             jm0_active_cell_count[i+1],
                                             actnum.data() + i + nx*j,
                                             nx*ny,
                                             nz );
                }
                else {
                    jm0_active_cell_count[ i+1 ] = 0u;
                }

                Index vertex_pillar_start = m_tessellation.vertices();
                vector<Index> adjacent_cells;

                // Determine the unique set of vertices on a pillar
                uniquePillarVertices( adjacent_cells,
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O11 ),   // c(i-1,j-1) @ (1,1)
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i+0) + CELL_CORNER_O01 ),   // c(i,  j-1) @ (0,1)
                                      jm0_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O10 ),   // c(i-1,j  ) @ (1,0)
                                      jm0_zcorn_ix.data() + 2*nz*( 4*(i+0) + CELL_CORNER_O00 ),   // c(i,  j  ) @ (0,0)
                                      zcorn.data() +  2*(i-1) + 1  + 2*nx*( 2*(j-1) + 1 ),
                                      zcorn.data() +  2*(i  ) + 0  + 2*nx*( 2*(j-1) + 1 ),
                                      zcorn.data() +  2*(i-1) + 1  + 2*nx*( 2*(j  ) + 0 ),
                                      zcorn.data() +  2*(i  ) + 0  + 2*nx*( 2*(j  ) + 0 ),
                                      nx*ny,
                                      coord.data() + 6*( i + (nx+1)*j ),
                                      jm1_active_cell_list.data() + nz*(i+ 0 ),
                                      jm1_active_cell_list.data() + nz*(i+ 1 ),
                                      jm0_active_cell_list.data() + nz*(i+ 0 ),
                                      jm0_active_cell_list.data() + nz*(i+ 1 ),
                                      jm1_active_cell_count[ i+0 ],
                                      jm1_active_cell_count[ i+1 ],
                                      jm0_active_cell_count[ i+0 ],
                                      jm0_active_cell_count[ i+1 ] );


                // Edges along pillar
                pillarEdges( vertex_pillar_start,
                             adjacent_cells,
                             cell_map.data() + (i-1) + nx*(j-1),
                             cell_map.data() + (i  ) + nx*(j-1),
                             cell_map.data() + (i-1) + nx*(j  ),
                             cell_map.data() + (i  ) + nx*(j  ),
                             nx*ny );

                // Process pillar [p(i,j-1) and p(i,j)] along j (with c(i-1,j-1) and c(i,j-1) abutting)
                if( 0 < j ) {
                    vector<WallLine> wall_lines;
                    vector<Intersection> wall_line_intersections;

                    extractWallLines( wall_lines,
                                      jm1_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O10_D01),
                                      jm1_wall_line_ix.data() + 2*nz*( 4*(i+0) + CELL_WALL_O00_D01),
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i-1)+ CELL_CORNER_O10 ),
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i-1)+ CELL_CORNER_O11 ),
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i+0)+ CELL_CORNER_O00 ),
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i+0)+ CELL_CORNER_O01 ),
                                      jm1_active_cell_list.data() + nz*(i+0),
                                      jm1_active_cell_list.data() + nz*(i+1),
                                      jm1_active_cell_count[ i+0 ],
                                      jm1_active_cell_count[ i+1 ],
                                      cell_map.data() + (i-1) + nx*(j-1),
                                      cell_map.data() + (i  ) + nx*(j-1),
                                      coord.data() + 6*( i + (nx+1)*(j-1) ),
                                      coord.data() + 6*( i + (nx+1)*j ),
                                      nx*ny,
                                      1u );

                    intersectWallLines( wall_line_intersections,
                                        pi0jm1_d01_chains,
                                        pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i,
                                        wall_lines,
                                        coord.data() + 6*( i + (nx+1)*(j-1) ),
                                        coord.data() + 6*( i + (nx+1)*j )
                                        );

                    wallEdges( wall_lines,
                               wall_line_intersections,
                               pi0jm1_d01_chains,
                               pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i );
#if 1
                    if( wall_line_intersections.empty() ) {
                        stitchPillarsNoIntersections( ORIENTATION_I,
                                                      wall_lines,
                                                      coord.data() + 6*( (i) + (nx+1)*(j-1) ),
                                                      coord.data() + 6*( (i) + (nx+1)*(j  ) ) );
                    }
                    else {
                        stitchPillarsHandleIntersections( ORIENTATION_I,
                                                          wall_lines,
                                                          wall_line_intersections,
                                                          pi0jm1_d01_chains,
                                                          pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i,
                                                          coord.data() + 6*( (i) + (nx+1)*(j-1) ),
                                                          coord.data() + 6*( (i) + (nx+1)*(j  ) ) );
                    }
#endif

#ifdef CHECK_INVARIANTS
                    // Sanity check
                    for( Index l=0; l<wall_lines.size(); l++ ) {
                        const Index a = pi0jm1_d01_chain_offsets[chain_offset_stride*i + l + 0 ];
                        const Index b = pi0jm1_d01_chain_offsets[chain_offset_stride*i + l + 1 ];
                        for( Index c = a; c<b; c++ ) {
                            const Index i = pi0jm1_d01_chains[ c ];
                            Intersection& isec = wall_line_intersections[ i ];
                            LOGGER_INVARIANT_EITHER_EQUAL( log, l, isec.m_upwrd_bndry_ix, l, isec.m_dnwrd_bndry_ix );
                        }
                    }
#endif

                    // replace intersection indices with vertex indices.
                    const Index c0 = pi0jm1_d01_chain_offsets[chain_offset_stride*i ];
                    const Index c1 = pi0jm1_d01_chain_offsets[chain_offset_stride*i+wall_lines.size() ];
                    for( Index k=c0; k<c1; k++ ) {
                        pi0jm1_d01_chains[k] = wall_line_intersections[ pi0jm1_d01_chains[k] ].m_vtx_ix;
                    }

                }


                if( 0 < i ) {
                    // Stitch pillar wall between pillars p(i-1,j) and p(i,j)
                    //      |                  |
                    //      |    c(i-1,j-1)    |
                    //      |                  |
                    //   p(i-1,j) --------- p(i,j) --> i
                    //      |                  |
                    //      |    c(i-1,j  )    |
                    //    j V                  |
                    vector<WallLine> wall_lines;
                    vector<Intersection> wall_line_intersections;

                    extractWallLines( wall_lines,
                                      jm0_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O00_D10),
                                      jm1_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O01_D10),
                                      jm0_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O00 ),
                                      jm0_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O10 ),
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O01 ),
                                      jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O11 ),
                                      jm0_active_cell_list.data() + nz*(i+0),
                                      jm1_active_cell_list.data() + nz*(i+0),
                                      jm0_active_cell_count[ i+0 ],
                                      jm1_active_cell_count[ i+0 ],
                                      cell_map.data() + (i-1) + nx*(j  ),
                                      cell_map.data() + (i-1) + nx*(j-1),
                                      coord.data() + 6*( (i-1) + (nx+1)*j ),
                                      coord.data() + 6*( i + (nx+1)*j ),
                                      nx*ny,
                                      nx );
                    intersectWallLines( wall_line_intersections,
                                        pi0j0_d10_chains,
                                        pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1),
                                        wall_lines,
                                        coord.data() + 6*( (i-1) + (nx+1)*j ),
                                        coord.data() + 6*( i + (nx+1)*j )
                                        );

                    wallEdges( wall_lines,
                               wall_line_intersections,
                               pi0j0_d10_chains,
                               pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1) );

                    if( wall_line_intersections.empty() ) {
                        stitchPillarsNoIntersections( ORIENTATION_J,
                                                      wall_lines,
                                                      coord.data() + 6*( (i-1) + (nx+1)*(j) ),
                                                      coord.data() + 6*( (i  ) + (nx+1)*(j) ) );
                    }
                    else {
                        stitchPillarsHandleIntersections( ORIENTATION_J,
                                                          wall_lines,
                                                          wall_line_intersections,
                                                          pi0j0_d10_chains,
                                                          pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1),
                                                          coord.data() + 6*( (i-1) + (nx+1)*(j) ),
                                                          coord.data() + 6*( (i  ) + (nx+1)*(j) ) );
                    }

#ifdef CHECK_INVARIANTS
                    // Sanity check
                    for( Index l=0; l<wall_lines.size(); l++ ) {
                        const Index a = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1) + l + 0 ];
                        const Index b = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1) + l + 1 ];
                        for( Index c = a; c<b; c++ ) {
                            const Index i = pi0j0_d10_chains[c];
                            Intersection& isec = wall_line_intersections[ i ];
                            LOGGER_INVARIANT_EITHER_EQUAL( log, l, isec.m_upwrd_bndry_ix, l, isec.m_dnwrd_bndry_ix );
                        }
                    }
#endif
                    // replace intersection indices with vertex indices.
                    const Index c0 = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1) ];
                    const Index c1 = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1)+wall_lines.size() ];

                    for( Index k=c0; k<c1; k++ ) {
                        pi0j0_d10_chains[k] = wall_line_intersections[ pi0j0_d10_chains[k] ].m_vtx_ix;
                    }
                }

                // For each full cell
                if( i>0 && j>0) {
#if 1
                    stitchTopBottom( jm1_active_cell_list.data() + (nz*i),
                                     jm1_active_cell_count[i],
                                     jm1_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O01_D10 ),
                                     jm1_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O00_D10 ),
                                     jm1_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O10_D01 ),
                                     jm1_wall_line_ix.data() + 2*nz*( 4*(i-1) + CELL_WALL_O00_D01 ),
                                     jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O00 ),
                                     jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O10 ),
                                     jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O01 ),
                                     jm1_zcorn_ix.data() + 2*nz*( 4*(i-1) + CELL_CORNER_O11 ),
                                     coord.data() + 6*( (i-1) + (nx+1)*(j-1) ),
                                     coord.data() + 6*( (i  ) + (nx+1)*(j-1) ),
                                     coord.data() + 6*( (i-1) + (nx+1)*(j  ) ),
                                     coord.data() + 6*( (i  ) + (nx+1)*(j  ) ),
                                     pi0j0_d10_chains.data(),
                                     pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1),
                                     pi0jm1_d10_chains.data(),
                                     pi0jm1_d10_chain_offsets.data() + chain_offset_stride*(i-1),
                                     pi0jm1_d01_chains.data(),
                                     pi0jm1_d01_chain_offsets.data() + chain_offset_stride*(i),
                                     pi0jm1_d01_chains.data(),
                                     pi0jm1_d01_chain_offsets.data() + chain_offset_stride*(i-1),
                                     cell_map.data() + (i-1) + nx*(j-1),
                                     nx*ny );
#endif
                    // process cell column c(i-1,j-1)
                    for( Index m=0; m < jm1_active_cell_count[ i ]; m++ ) {
                        const Index k = jm1_active_cell_list[ (nz*i) + m];
                        const size_t gix = i-1 + nx*((j-1) + k*ny);
                        m_tessellation.setCell( cell_map[gix],
                                                gix,
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O00 ) + m ) + 0 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O10 ) + m ) + 0 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O01 ) + m ) + 0 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O11 ) + m ) + 0 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O00 ) + m ) + 1 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O10 ) + m ) + 1 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O01 ) + m ) + 1 ],
                                                jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O11 ) + m ) + 1 ] );

                    }
                }
            }
            catch( std::runtime_error& e ) {
                std::stringstream o;
                o << "j=" << j << ",i="<<i<<':' << e.what();
                throw std::runtime_error( o.str() );
            }
        }
        pi0jm1_d10_chains.swap( pi0j0_d10_chains );
        pi0jm1_d10_chain_offsets.swap( pi0j0_d10_chain_offsets );
        jm1_zcorn_ix.swap( jm0_zcorn_ix );
        jm0_active_cell_list.swap( jm1_active_cell_list );
        jm0_active_cell_count.swap( jm1_active_cell_count );
        jm0_wall_line_ix.swap( jm1_wall_line_ix );
    }
    PerfTimer stop;
    double t = PerfTimer::delta(start, stop);
    LOGGER_DEBUG( log, "Tessellating grid... done (" << t << " secs)" );

}


template<typename Tessellation>
void
Tessellator<Tessellation>::findActiveCellsInColumn( Index*            active_cell_list,
                                                               Index&            active_cell_count,
                                                               const int* const  actnum,
                                                               const Index       stride,
                                                               const Index       nz )
{
    Index n = 0;
    for( Index k=0; k<nz; k++ ) {
        if( actnum[ stride*k ] != 0 ) {
            active_cell_list[n++] = k;
        }
    }
    active_cell_count = n;
}


template<typename Tessellation>
void
Tessellator<Tessellation>::pillarEdges( const Index           offset,
                                                   const vector<Index>&  adjacent_cells,
                                                   const Index*          cell_map_00,
                                                   const Index*          cell_map_01,
                                                   const Index*          cell_map_10,
                                                   const Index*          cell_map_11,
                                                   const Index           stride )
{
    for( size_t i=1; i<adjacent_cells.size()/4; i++) {
        const Index* adj_p = adjacent_cells.data() + 4*(i-1);
        const Index* adj_c = adjacent_cells.data() + 4*i;
        if( (adj_c[0] != IllegalIndex ) ||
            (adj_c[1] != IllegalIndex ) ||
            (adj_c[2] != IllegalIndex ) ||
            (adj_c[3] != IllegalIndex ) )
        {
            // Check if k matches across pillar walls

            bool regular_p =
                    (adj_p[0] == adj_p[1]) &&
                    (adj_p[1] == adj_p[2]) &&
                    (adj_p[2] == adj_p[3]);
            bool regular_c =
                    (adj_c[0] == adj_c[1]) &&
                    (adj_c[1] == adj_c[2]) &&
                    (adj_c[2] == adj_c[3]);
            const Index fault = (!regular_p || !regular_c) ? (1u<<31u) : 0u;



            const Index cells[4] = { adj_c[0] != IllegalIndex ? cell_map_00[ stride*adj_c[0] ] : IllegalIndex,
                                     adj_c[1] != IllegalIndex ? cell_map_01[ stride*adj_c[1] ] : IllegalIndex,
                                     adj_c[2] != IllegalIndex ? cell_map_10[ stride*adj_c[2] ] : IllegalIndex,
                                     adj_c[3] != IllegalIndex ? cell_map_11[ stride*adj_c[3] ] : IllegalIndex
                                   };
            m_tessellation.addEdge( offset + i - 1 , offset + i,
                                    cells[0] | fault,
                                    cells[1] | fault,
                                    cells[2] | fault,
                                    cells[3] | fault );
        }
    }
}


template<typename Tessellation>
void
Tessellator<Tessellation>::uniquePillarVertices( vector<Index>&      adjacent_cells,
                                                            Index*              zcorn_ix_00,
                                                            Index*              zcorn_ix_01,
                                                            Index*              zcorn_ix_10,
                                                            Index*              zcorn_ix_11,
                                                            const SrcReal* const   zcorn_00,
                                                            const SrcReal* const   zcorn_01,
                                                            const SrcReal* const   zcorn_10,
                                                            const SrcReal* const   zcorn_11,
                                                            const Index         stride,
                                                            const SrcReal* const   coord,
                                                            const Index* const  active_cell_list_00,
                                                            const Index* const  active_cell_list_01,
                                                            const Index* const  active_cell_list_10,
                                                            const Index* const  active_cell_list_11,
                                                            const Index         active_cell_count_00,
                                                            const Index         active_cell_count_01,
                                                            const Index         active_cell_count_10,
                                                            const Index         active_cell_count_11 )
{
    Logger log = getLogger( package + ".uniquePillarVertices" );
    const bool snap_logical_neighbours = true;


    static const Real maxf = std::numeric_limits<Real>::max();
    static const Real minf = -std::numeric_limits<Real>::max();
    static const Real epsf = std::numeric_limits<Real>::epsilon();
    const Real x1 = coord[ 0 ];
    const Real y1 = coord[ 1 ];
    const Real z1 = coord[ 2 ];
    const Real x2 = coord[ 3 ];
    const Real y2 = coord[ 4 ];
    const Real z2 = coord[ 5 ];

    Index* zcorn_ix[4] = { zcorn_ix_00, zcorn_ix_01, zcorn_ix_10, zcorn_ix_11 };
    const Real* zcorn[4] = { zcorn_00, zcorn_01, zcorn_10, zcorn_11 };
    const Index* active_cell_list[4] = { active_cell_list_00, active_cell_list_01, active_cell_list_10,  active_cell_list_11 };
    const Index  active_cell_count[4] = { active_cell_count_00, active_cell_count_01, active_cell_count_10, active_cell_count_11 };

    adjacent_cells.clear();
    adjacent_cells.reserve( 2*active_cell_count_00 +
                            2*active_cell_count_01 +
                            2*active_cell_count_10 +
                            2*active_cell_count_11 );

    bool ascending  = false;
    bool descending = false;
    bool any        = false;
    for( Index l=0; l<4; l++) {
        const Index n = active_cell_count[l];
        if( n > 0u ) {
            const Index k0 = active_cell_list[l][0];
            const Index kn = active_cell_list[l][n-1u];
            const Real z0 = zcorn[l][ 4*stride*(2*k0+0) ];
            const Real zn = zcorn[l][ 4*stride*(2*kn+1) ];
            if( z0 < zn ) {
                ascending = true;
            }
            else if( zn < z0 ) {
                descending = true;
            }
            any = true;
        }
    }
    if( !any ) {
        return;    // no vertices
    }
    if( ascending && descending ) {
        throw std::runtime_error( "uniquePillarVertices: zcorn's are both ascending and descending" );
    }


    // We keep track of which cell is below the cornerpoint in all quadrants.
    Index cells_below[4] = { IllegalIndex, IllegalIndex, IllegalIndex, IllegalIndex };

    if( ascending ) {
        bool         first   = true;
        Real        curr_z  = minf;
        Index curr_ix = ~0;
        Index i[4]    = { 0, 0, 0, 0 };
        Index last_k[4] = { IllegalIndex-1u, IllegalIndex-1u, IllegalIndex-1u, IllegalIndex-1u };
        while(1) {
            Real smallest_z = maxf;
            Index smallest_l = IllegalIndex;
            Index smallest_k = IllegalIndex;

            // search through the four quadrants for the unprocessed corner-point
            // with the smallest z-value.
            for( Index l=0; l<4; l++ ) {
                // look at the next active cell in quadrant
                if( i[l] < 2*active_cell_count[l] ) {
                    const Index ii = i[l]>>1;   // cell no
                    const Index ij = i[l]&1;    // 0 -> enter, 1->exit

                    const Index k = active_cell_list[l][ ii ];
                    Real z = zcorn[l][ 4*stride*(2*k + ij) ];

                    // optionally force logically top-down neighbours to share
                    // cornerpoints
                    if( snap_logical_neighbours && (ij==0) && (last_k[l]+1 == k) ) {
                        z = zcorn[l][ 4*stride*(2*last_k[l] + 1) ];
                    }

                    if( z < smallest_z ) {
                        smallest_z = z;
                        smallest_l = l;
                        smallest_k = k;
                    }
                }
            }

            if( 3 < smallest_l ) {
                break;
            }

            if( first || curr_z+epsf < smallest_z ) {
                // create new vertex
                curr_z = smallest_z;
                const Real a = (smallest_z-z1)/(z2-z1);
                const Real b = 1.f - a;

                curr_ix = m_tessellation.addVertex( Real4( b*x1 + a*x2,
                                                           b*y1 + a*y2,
                                                           smallest_z ) );
                adjacent_cells.push_back( cells_below[0] );
                adjacent_cells.push_back( cells_below[1] );
                adjacent_cells.push_back( cells_below[2] );
                adjacent_cells.push_back( cells_below[3] );
                first = false;
            }
            const Index ii = i[smallest_l]>>1;
            const Index ij = i[smallest_l]&1;
            cells_below[ smallest_l ] = ij == 0 ? smallest_k : IllegalIndex;
            zcorn_ix[ smallest_l ][ 2*ii + ij ] = curr_ix;
            i[smallest_l]++;
            last_k[ smallest_l ] = smallest_k;
        }
    }
    else {
        bool         first   = true;
        Real        curr_z  = maxf;
        Index curr_ix = ~0;
        Index i[4]    = { 0, 0, 0, 0 };
        Index last_k[4] = { IllegalIndex-1u, IllegalIndex-1u, IllegalIndex-1u, IllegalIndex-1u };
        while(1) {
            Real largest_z = minf;
            Index largest_l = ~0;
            Index largest_k = ~0;
            for( Index l=0; l<4; l++ ) {
                if( i[l] < 2*active_cell_count[l] ) {
                    const Index ii = i[l]>>1;
                    const Index ij = i[l]&1;

                    const Index k = active_cell_list[l][ ii ];
                    Real z = zcorn[l][ 4*stride*(2*k + ij) ];
                    if( snap_logical_neighbours && (ij==0) && (last_k[l]+1 == k) ) {
                        z = zcorn[l][ 4*stride*(2*last_k[l] + 1) ];
                    }
                    if( largest_z < z ) {
                        largest_z = z;
                        largest_l = l;
                        largest_k = k;
                    }
                }
            }

            if( 3 < largest_l ) {
                break;
            }
            if( first || largest_z < curr_z-epsf  ) {
                // create new vertex
                first = false;
                curr_z = largest_z;
                const Real a = (largest_z-z1)/(z2-z1);
                const Real b = 1.f - a;
                curr_ix = m_tessellation.addVertex( Real4( b*x1 + a*x2,
                                                           b*y1 + a*y2,
                                                           largest_z ) );
            }
            const Index ii = i[largest_l]>>1;
            const Index ij = i[largest_l]&1;
            zcorn_ix[ largest_l ][ 2*ii + ij ] = curr_ix;
            i[largest_l]++;
            last_k[ largest_l ] = largest_k;
        }
    }
}



template<typename Tessellation>
void
Tessellator<Tessellation>::stitchTopBottom( const Index* const   ci0j0_active_cell_list,
                                                       const Index          ci0j0_active_cell_count,
                                                       const Index* const   o01_d10_wall_line_ix,
                                                       const Index* const   o00_d10_wall_line_ix,
                                                       const Index* const   o10_d01_wall_line_ix,
                                                       const Index* const   o00_d01_wall_line_ix,
                                                       const Index* const   o00_zcorn_ix,
                                                       const Index* const   o10_zcorn_ix,
                                                       const Index* const   o01_zcorn_ix,
                                                       const Index* const   o11_zcorn_ix,
                                                       const SrcReal* const  o00_coord,
                                                       const SrcReal* const  o10_coord,
                                                       const SrcReal* const  o01_coord,
                                                       const SrcReal* const  o11_coord,
                                                       const Index* const   o01_d10_chains,
                                                       const Index* const   o01_d10_chain_offsets,
                                                       const Index* const   o00_d10_chains,
                                                       const Index* const   o00_d10_chain_offsets,
                                                       const Index* const   o10_d01_chains,
                                                       const Index* const   o10_d01_chain_offsets,
                                                       const Index* const   o00_d01_chains,
                                                       const Index* const   o00_d01_chain_offsets,
                                                       const Index* const   cell_map,
                                                       const Index          stride )
{
    struct StitchTopBottomHelper {
        Index m_index;
        Index m_cell_below;
        Index m_cell_above;
        bool         m_fault;
        bool         m_emit_00_01_10;
        bool         m_emit_11_10_01;
    };

    Logger log = getLogger( package + ".stitchTopBottom" );



    vector<StitchTopBottomHelper> helper;
    helper.reserve( 2*ci0j0_active_cell_count );

    Index last_top_00 = IllegalIndex;
    Index last_top_01 = IllegalIndex;
    Index last_top_10 = IllegalIndex;
    Index last_top_11 = IllegalIndex;

    Index last_k = IllegalIndex-1u;
    for( Index m=0; m<ci0j0_active_cell_count; m++ ) {
        const Index k = ci0j0_active_cell_list[m];
        const Index cell = cell_map[ stride*k ];

        const Index i000 = o00_zcorn_ix[2*m + 0];
        const Index i100 = o10_zcorn_ix[2*m + 0];
        const Index i010 = o01_zcorn_ix[2*m + 0];
        const Index i110 = o11_zcorn_ix[2*m + 0];
        const Index i001 = o00_zcorn_ix[2*m + 1];
        const Index i101 = o10_zcorn_ix[2*m + 1];
        const Index i011 = o01_zcorn_ix[2*m + 1];
        const Index i111 = o11_zcorn_ix[2*m + 1];


        bool degenerate_00_01_10 = (i000 == i001 ) &&
                                   (i010 == i011 ) &&
                                   (i100 == i101 );

        bool degenerate_11_10_01 = (i110 == i111 ) &&
                                   (i100 == i101 ) &&
                                   (i010 == i011 );
        if( degenerate_00_01_10 && degenerate_11_10_01 ) {
            continue;
        }

        bool attached_00_01_10 = (last_top_00 == i000) &&
                                 (last_top_10 == i100) &&
                                 (last_top_01 == i010);
        bool attached_11_10_01 = (last_top_11 == i110) &&
                                 (last_top_10 == i100) &&
                                 (last_top_01 == i010);

        bool attached = attached_00_01_10 && attached_11_10_01;

#ifdef DEBUG
        if( !attached ) {
            if( last_k+1 == k ) {
                LOGGER_WARN( log, "Logical top-down neighbours are not geometrical neighbours" );
            }
        }
#endif

        // Bottom
        if( helper.empty() || !attached ) {
            helper.resize( helper.size() + 1 );
            helper.back().m_index = 2*m+0;
            helper.back().m_cell_below = IllegalIndex;
            helper.back().m_cell_above = cell;
            helper.back().m_fault = false; // we define boundary as not fault
        }
        else {
            // cells are attached
            helper.back().m_cell_above = cell;
            helper.back().m_fault = false; //last_k + 1 != k;
        }

        // Top
        helper.resize( helper.size() + 1 );
        helper.back().m_index = 2*m+1;
        helper.back().m_cell_below = cell;
        helper.back().m_cell_above = IllegalIndex;
        helper.back().m_fault = false; // we define boundary as not fault

        last_top_00 = i001;
        last_top_10 = i101;
        last_top_01 = i011;
        last_top_11 = i111;

        last_k = k;
    }

    PillarFloorSampler<Tessellation> floor_sampler( o00_coord, o10_coord, o01_coord, o11_coord );

    vector<Segment> segments;
    for( auto it=helper.begin(); it!=helper.end(); ++it ) {
        segments.clear();

        // cell top/bottom index
        const Index b_ix = it->m_index;

        const Real4& c00 = m_tessellation.vertex( o00_zcorn_ix[ b_ix ] );
        const Real4& c01 = m_tessellation.vertex( o01_zcorn_ix[ b_ix ] );
        const Real4& c10 = m_tessellation.vertex( o10_zcorn_ix[ b_ix ] );
        const Real4& c11 = m_tessellation.vertex( o11_zcorn_ix[ b_ix ] );
        floor_sampler.setCellFloor( c00.z(), c01.z(), c10.z(), c11.z() );

        {   // (0,0) -> (0,1)
            segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal( 0.f, 0.f, c00.z() ) ),
                                         o00_zcorn_ix[ b_ix ], 3u ) );
            Index o00_d01_0 = o00_d01_chain_offsets[ o00_d01_wall_line_ix[ b_ix ] ];
            Index o00_d01_1 = o00_d01_chain_offsets[ o00_d01_wall_line_ix[ b_ix ] + 1 ];
            for( Index i=o00_d01_0; i<o00_d01_1; i++ ) {
                const Index ii = o00_d01_chains[i];
                const Real4& p = m_tessellation.vertex( ii );
                segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal( 0.f, p.w(), p.z() ) ),
                                             ii, 3u ) );
            }
        }

        {   // (0,1) -> (1,1)
            segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal( 0.f, 1.f, c01.z() ) ),
                                         o01_zcorn_ix[ b_ix ], 3u ) );
            Index o01_d10_0 = o01_d10_chain_offsets[ o01_d10_wall_line_ix[ b_ix ] ];
            Index o01_d10_1 = o01_d10_chain_offsets[ o01_d10_wall_line_ix[ b_ix ] + 1 ];
            for( Index i=o01_d10_0; i<o01_d10_1; i++ ) {
                const Index ii = o01_d10_chains[i];
                const Real4& p = m_tessellation.vertex( ii );
                segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal(  p.w(), 1.f, p.z() ) ),
                                             o01_d10_chains[i], 3u ) );
            }
        }

        {   // (1,1) -> (1,0)
            segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal( 1.f, 1.f, c11.z() ) ),
                                          o11_zcorn_ix[ b_ix ], 3u ) );
            Index o10_d01_0 = o10_d01_chain_offsets[ o10_d01_wall_line_ix[ b_ix ] ];
            Index o10_d01_1 = o10_d01_chain_offsets[ o10_d01_wall_line_ix[ b_ix ] + 1 ];
            for( Index i=o10_d01_1; i>o10_d01_0; i-- ) {
                const Index ii = o10_d01_chains[i-1];
                const Real4& p = m_tessellation.vertex( ii );
                segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal( 1.f, p.w(), p.z() ) ),
                                             o10_d01_chains[i-1], 3u ) );
            }
        }

        {   // (1,0) -> (0,0)
            segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal(  1.f, 0.f, c10.z() ) ),
                                         o10_zcorn_ix[ b_ix ], 3u ) );
            Index o00_d10_0 = o00_d10_chain_offsets[ o00_d10_wall_line_ix[ b_ix ] ];
            Index o00_d10_1 = o00_d10_chain_offsets[ o00_d10_wall_line_ix[ b_ix ] + 1 ];
            for( Index i=o00_d10_1; i>o00_d10_0; i-- ) {
                const Index ii = o00_d10_chains[i-1];
                const Real4& p = m_tessellation.vertex( ii );

                segments.push_back( Segment( m_tessellation.addNormal( floor_sampler.normal( p.w(), 0.f, p.z() ) ),
                                             o00_d10_chains[i-1], 3u ) );
            }
        }

        m_tessellation.addPolygon( Interface( it->m_cell_above, it->m_cell_below, ORIENTATION_K, it->m_fault),
                                   segments.data(), segments.size() );
    }
}


template<typename Tessellation>
void
Tessellator<Tessellation>::wallEdges( const vector<WallLine>&      boundaries,
                                                 const vector<Intersection>&  intersections,
                                                 const vector<Index>&  chains,
                                                 const Index* const         chain_offsets )
{
    // Run through all cell boundaries
    for( size_t b=0; b<boundaries.size(); b++ ) {
        // cell over and under is correct for both sides at pillar 0.
        const WallLine& wl = boundaries[b];
        Index cell_over[2];
        Index cell_under[2];
        cell_over[0]  = wl.m_cell_over[0];
        cell_over[1]  = wl.m_cell_over[1];
        cell_under[0] = wl.m_cell_under[0];
        cell_under[1] = wl.m_cell_under[1];
        Index other_side = wl.m_side==1 ? 0 : 1;
        Index end0 = wl.m_ends[0];
        Index fault = wl.m_fault ? (1u<<31u) : 0u;
        for( size_t c=chain_offsets[b]; c!=chain_offsets[b+1]; c++ ) {
            // Run through intersections, adjust cell over and under
            const Intersection& i = intersections[ chains[c] ];
            const WallLine& ol = boundaries[ i.m_upwrd_bndry_ix == b ? i.m_upwrd_bndry_ix : i.m_dnwrd_bndry_ix ];
            cell_over[ other_side ]  = ol.m_cell_over[ other_side ];
            cell_under[ other_side ] = ol.m_cell_under[ other_side ];

            Index end1 = i.m_vtx_ix;
            m_tessellation.addEdge( end0, end1,
                                    cell_over[0]  | fault,
                                    cell_under[0] | fault,
                                    cell_over[1]  | fault,
                                    cell_under[1] | fault );
            end0 = end1;
        }
        Index end1 = wl.m_ends[1];
        m_tessellation.addEdge( end0, end1,
                                cell_over[0]  | fault,
                                cell_under[0] | fault,
                                cell_over[1]  | fault,
                                cell_under[1] | fault );
    }
}


template<typename Tessellation>
void
Tessellator<Tessellation>::stitchPillarsHandleIntersections( const Orientation            orientation,
                                                                        const vector<WallLine>&      boundaries,
                                                                        const vector<Intersection>&  intersections,
                                                                        const vector<Index>&         chains,
                                                                        const Index* const           chain_offsets,
                                                                        const SrcReal* const         o0_coord,
                                                                        const SrcReal* const         o1_coord )
{
    Logger log = getLogger( package + ".stitchPillarsHandleIntersections" );


    const Real c_x[4] = { o0_coord[0], o1_coord[0], o0_coord[3], o1_coord[3] };
    const Real c_y[4] = { o0_coord[1], o1_coord[1], o0_coord[4], o1_coord[4] };
    const Real c_z[4] = { o0_coord[2], o1_coord[2], o0_coord[5], o1_coord[5] };
    const Real s[2] = { 1.f/(c_z[2]-c_z[0]), 1.f/(c_z[3]-c_z[1]) };
    const Real dl_xy[4] = { s[0]*(c_x[2]-c_x[0]), s[1]*(c_x[3]-c_x[1]), s[0]*(c_y[2]-c_y[0]), s[1]*(c_y[3]-c_y[1]) };

    vector<Segment> segments;

    PillarWallSampler<Tessellation> wall( o0_coord, o1_coord );

    // First, process as polygons that are attached at pillar 0. We start by
    // pairing adjacent edges and follow them out until a common intersection or
    // until they hit pillar 1.
    for( size_t b=0; (b+1u)<boundaries.size(); b++ ) {
        const WallLine& line_u = boundaries[b+1];
        const WallLine& line_l = boundaries[b];

        if( line_u.m_side == 3 && line_l.m_side == 3 ) {      // Non-fault (fully matching) interface
            Index cells[2];
            cells[0] = line_l.m_cell_over[0];
            cells[1] = line_l.m_cell_over[1];
            LOGGER_INVARIANT_EQUAL( log, cells[0], line_u.m_cell_under[0] );
            LOGGER_INVARIANT_EQUAL( log, cells[1], line_u.m_cell_under[1] );
            if( cells[0] == IllegalIndex && cells[1] == IllegalIndex ) {
                continue;
            }
            segments.clear();
            segments.push_back( Segment( line_u.m_normals[0],
                                         line_u.m_ends[0],
                                         3u ) );
            if( line_u.m_ends[0] != line_l.m_ends[0] ) {
                for( Index v=line_u.m_ends[0]-1; v>line_l.m_ends[0]; v-- ) {
                    const Real4& p = m_tessellation.vertex( v );
                    const Index ni = m_tessellation.addNormal( wall.normal( 0.f, p.z() ) );
                    segments.push_back( Segment( ni, v, 3u ) );
                }
                segments.push_back( Segment( line_l.m_normals[0],
                                             line_l.m_ends[0],
                                             3u ) );
            }
            segments.push_back( Segment( line_l.m_normals[1],
                                         line_l.m_ends[1],
                                         3u ) );
            if( line_u.m_ends[1] != line_l.m_ends[1] ) {
                for( Index v=line_l.m_ends[1]+1; v<line_u.m_ends[1]; v++ ) {
                    const Real4& p = m_tessellation.vertex( v );
                    const Index ni = m_tessellation.addNormal( wall.normal( 1.f, p.z() ) );
                    segments.push_back( Segment( ni, v, 3u ) );
                }
                segments.push_back( Segment( line_u.m_normals[1],
                                             line_u.m_ends[1],
                                             3u ) );
            }
            m_tessellation.addPolygon( Interface( cells[0], cells[1], orientation,
                                                  line_u.m_fault && line_l.m_fault ),
                                       segments.data(), segments.size() );
        }
        else {                                                  // Fault (partially matching) interface
            Index cell[2];
            cell[0] = line_l.m_cell_over[0];
            cell[1] = line_l.m_cell_over[1];
            LOGGER_INVARIANT_EQUAL( log, line_u.m_cell_under[0], cell[0] );
            LOGGER_INVARIANT_EQUAL( log, line_u.m_cell_under[1], cell[1] );
            if( cell[0] == IllegalIndex && cell[1] == IllegalIndex ) {
                continue;
            }
            segments.clear();

            // follow upper arc backwards from towards pillar1 to pillar 0
            bool process_pillar_1 = false;

            if( chain_offsets[b+1] == chain_offsets[b+2] ) {  // First intersection is on pillar
                segments.push_back( Segment( line_u.m_normals[1],
                                             line_u.m_ends[1],
                                             line_u.m_side ) );
                process_pillar_1 = true;
            }
            else {
                // First intersection is on wall
                const Intersection& isec_u1 = intersections[ chains[ chain_offsets[b+1] ] ];
                if( isec_u1.m_dnwrd_bndry_ix != (b+1) ) { // We haven't turned downwards yet
                    const WallLine& line_u12 = boundaries[ isec_u1.m_dnwrd_bndry_ix ];
                    NextIntersection isec_u2_ix = isec_u1.m_nxt_dnwrd_isec_ix;
                    if( isec_u2_ix.isIntersection() ) {    // Second intersection is on wall
                        const Intersection& isec_u2 = intersections[ isec_u2_ix.intersection() ];
                        segments.push_back( Segment( isec_u2.m_n,
                                                     isec_u2.m_vtx_ix,
                                                     line_u12.m_side ) );
                    }
                    else {                         // Second intersection is on pillar
                        LOGGER_INVARIANT_EQUAL( log, line_u12.m_ends[1], isec_u2_ix.vertex() );
                        segments.push_back( Segment( line_u12.m_normals[1],
                                                     line_u12.m_ends[1],
                                                     line_u12.m_side ) );
                        process_pillar_1 = true;
                    }
                }
                segments.push_back( Segment( isec_u1.m_n, isec_u1.m_vtx_ix, line_u.m_side ) );
            }
            // Down along pillar 0
            if( boundaries[b+1].m_ends[0] != boundaries[b].m_ends[0] ) {
                segments.push_back( Segment( boundaries[b+1].m_normals[0],
                                             boundaries[b+1].m_ends[0],
                                             3u ) );
                for( Index v = boundaries[b+1].m_ends[0]-1; v>boundaries[b].m_ends[0]; v-- ) {
                    const Real4& p = m_tessellation.vertex( v );
                    const Index ni = m_tessellation.addNormal( wall.normal( 0.f, p.z() ) );
                    segments.push_back( Segment( ni, v, 3u ) );
                }
            }
            segments.push_back( Segment( boundaries[b].m_normals[0],
                                         boundaries[b].m_ends[0],
                                         boundaries[b].m_side ) );

            // follow lower arc forwards from pillar 0 towards pillar 1
            if( chain_offsets[b] == chain_offsets[b+1] ) {
                // First intersection on pillar
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                if( segments.front().vertex() == boundaries[b].m_ends[1] ) {
                    // Loop is closed
                    process_pillar_1 = false;
                }
                else {
                    segments.push_back( Segment( boundaries[b].m_normals[1],
                                                 boundaries[b].m_ends[1],
                                                 3u ) );
                }
            }
            else {
                // First intersection is on wall
                const Intersection& l_1_isec = intersections[ chains[ chain_offsets[b] ] ];
                if( l_1_isec.m_upwrd_bndry_ix == b ) {
                    // We already moving upwards, should close the loop
                    LOGGER_INVARIANT_EQUAL( log, segments.front().vertex(), l_1_isec.m_vtx_ix );
                }
                else {
                    const WallLine& line_l12 = boundaries[ l_1_isec.m_upwrd_bndry_ix ];
                    segments.push_back( Segment( l_1_isec.m_n,
                                                 l_1_isec.m_vtx_ix,
                                                 line_l12.m_side ) );
                    NextIntersection isec_l2_ix = l_1_isec.m_nxt_upwrd_isec_ix;
                    if( isec_l2_ix.isIntersection() ) {
                        // Second intersection is on wall, and we should have closed the loop
                        const Intersection& isec_l2 = intersections[ isec_l2_ix.intersection() ];
                        LOGGER_INVARIANT_EQUAL( log, process_pillar_1, false );
                        LOGGER_INVARIANT_EQUAL( log, segments.front().vertex(), isec_l2.m_vtx_ix );
                    }
                    else if( segments.front().vertex() == isec_l2_ix.vertex() ) {
                        // Second intersection is on pillar, and matches upper loop
                        LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                        process_pillar_1 = false;
                    }
                    else {
                        // Second intersection is on pillar, no match, so we must close the loop
                        LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                        LOGGER_INVARIANT_EQUAL( log, line_l12.m_ends[1], isec_l2_ix.vertex() );
                        segments.push_back( Segment( line_l12.m_normals[1],
                                                     line_l12.m_ends[1],
                                                     3u ) );
                    }
                }
            }
            if( process_pillar_1 ) {
                Index b = segments.front().vertex();
                Index a = segments.back().vertex();
                for(Index v=a+1; v<b; v++ ) {
                    const Real4& p = m_tessellation.vertex( v );
                    const Index ni = m_tessellation.addNormal( wall.normal( 0.f, p.z() ) );
                    segments.push_back( Segment( ni, v, 3u ) );
                }
            }
            m_tessellation.addPolygon( Interface( boundaries[b].m_cell_over[0],
                                                  boundaries[b].m_cell_over[1],
                                                  orientation,
                                                  true ),
                                       segments.data(), segments.size() );
        }
    }

    // Second, process all polygons that are not attached to pillar 1, but
    // origins in an intersection and extens towards pillar 1.
    // The adjacent cells of the polygon are the cell above the downward edge
    // and below the upward edge.
    for( size_t i=0; i<intersections.size(); i++ ) {
        const Intersection& is = intersections[i];
        const WallLine& line_u01 = boundaries[ is.m_upwrd_bndry_ix ];
        const WallLine& line_l01 = boundaries[ is.m_dnwrd_bndry_ix ];

        Index side1 = line_u01.m_side - 1u;
        Index side2 = line_l01.m_side - 1u;

        Index cell[2];
        cell[ side1 ] = line_u01.m_cell_under[ side1 ];
        cell[ side2 ] = line_l01.m_cell_over[  side2 ];
        if( cell[0] == IllegalIndex && cell[1] == IllegalIndex ) {
            continue;   // Empty hole
        }
        segments.clear();

        // Follow upper arc backwards

        bool process_pillar_1 = false;
        NextIntersection isec_u1_ix = is.m_nxt_upwrd_isec_ix;
        if( isec_u1_ix.isIntersection() ) {         // First intersection is on wall
            const Intersection& isec_u1 = intersections[ isec_u1_ix.intersection() ];
            const WallLine& line_u12 = boundaries[ isec_u1.m_dnwrd_bndry_ix ];
            NextIntersection isec_u2_ix = isec_u1.m_nxt_dnwrd_isec_ix;
            if( isec_u2_ix.isIntersection() ) {     // Second intersection is on wall
                const Intersection& isec_u_2 = intersections[ isec_u2_ix.intersection() ];
                segments.push_back( Segment( isec_u_2.m_n,
                                             isec_u_2.m_vtx_ix,
                                             line_u12.m_side ) );
                LOGGER_INVARIANT_EQUAL( log, cell[side2], line_u12.m_cell_under[ side2 ] );
            }
            else {                              // Second intersection is on pillar
                LOGGER_INVARIANT_EQUAL( log, line_u12.m_ends[1], isec_u2_ix.vertex() );
                segments.push_back( Segment( line_u12.m_normals[1],
                                             line_u12.m_ends[1],
                                             line_u12.m_side ) );
                process_pillar_1 = true;
            }
            segments.push_back( Segment( isec_u1.m_n,
                                         isec_u1.m_vtx_ix,
                                         line_u01.m_side ) );
        }
        else {                                  // First intersection is on pillar
            LOGGER_INVARIANT_EQUAL( log, line_u01.m_ends[1], isec_u1_ix.vertex() );
            segments.push_back( Segment( line_u01.m_normals[1],
                                         line_u01.m_ends[1],
                                         line_u01.m_side ) );
            process_pillar_1 = true;
        }

        // Follow lower arc forwards
        NextIntersection isec_l1_ix = is.m_nxt_dnwrd_isec_ix;
        segments.push_back( Segment( is.m_n,
                                     is.m_vtx_ix,
                                     line_l01.m_side ) );

        if( isec_l1_ix.isIntersection() ) {
            // First intersection is on wall, cannot be u_1_ix.
            const Intersection& isec_l1 = intersections[ isec_l1_ix.intersection() ];
            const WallLine& line_l12 = boundaries[ isec_l1.m_upwrd_bndry_ix ];

            segments.push_back( Segment( isec_l1.m_n,
                                         isec_l1.m_vtx_ix,
                                         line_l12.m_side ) );

            NextIntersection isec_l2_ix = intersections[isec_l1_ix.intersection()].m_nxt_upwrd_isec_ix;
            if( isec_l2_ix.isIntersection() ) {
                // Second intersection is on wall, must be identical to upper arc
                const Intersection& isec_l2 = intersections[ isec_l2_ix.intersection() ];
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, false );
                LOGGER_INVARIANT_EQUAL( log, cell[side1], line_l12.m_cell_over[ side1 ] );
                LOGGER_INVARIANT_EQUAL( log, segments.front().vertex(),isec_l2.m_vtx_ix );
            }
            else if( segments.front().vertex() == isec_l2_ix.vertex() ) {
                // Second intersection is on pillar and closes loop
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                process_pillar_1 = false;
            }
            else {
                // Second intersection is on pillar and doesn't close loop
                LOGGER_INVARIANT_EQUAL( log, line_l12.m_ends[1], isec_l2_ix.vertex() );
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                segments.push_back( Segment( line_l12.m_normals[1],
                                             line_l12.m_ends[1],
                                             3u ) );
            }
        }
        else if( segments.front().vertex() == isec_l1_ix.vertex() ) {
            // First intersection is on pillar and closes loop
            LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
            process_pillar_1 = false;
        }
        else {
            // First intersection is on pillar and doesn't close loop
            LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
            LOGGER_INVARIANT_EQUAL( log, line_l01.m_ends[1], isec_l1_ix.vertex() );
            segments.push_back( Segment( line_l01.m_normals[1],
                                         line_l01.m_ends[1],
                                         3u ) );
        }

        if( process_pillar_1 ) {
            // Add vertices along pillar 1
            Index b = segments.front().vertex();
            Index a = segments.back().vertex();
            for(Index v=a+1; v<b; v++ ) {
                const Real4& p = m_tessellation.vertex( v );
                const Index ni = m_tessellation.addNormal( wall.normal( 1.f, p.z() ) );
                segments.push_back( Segment( ni, v, 3u ) );
            }
        }
        m_tessellation.addPolygon( Interface( cell[0], cell[1],orientation, true),
                                   segments.data(), segments.size() );
    }
}


template<typename Tessellation>
void
Tessellator<Tessellation>::stitchPillarsNoIntersections( const Orientation        orientation,
                                                                    const vector<WallLine>&  boundaries,
                                                                    const SrcReal* const     o0_coord,
                                                                    const SrcReal* const     o1_coord  )
{
    Logger log = getLogger( package + ".stitchPillarsNoIntersections" );


    if( boundaries.empty() ) {
        return;
    }
    vector<Segment> segments;

    PillarWallSampler<Tessellation> wall( o0_coord, o1_coord );

    bool edge_under[2] = {
        boundaries[0].m_cell_over[0] != IllegalIndex,
        boundaries[0].m_cell_over[1] != IllegalIndex,
    };
    Index l[2] = {
            boundaries[0].m_ends[0],
            boundaries[0].m_ends[1]
    };
    Index nl[2] = {
        boundaries[0].m_normals[0],
        boundaries[0].m_normals[1]
    };
    for(size_t b=1; b<boundaries.size(); b++) {
        Index cells[2] = {
            boundaries[b].m_cell_under[0],
            boundaries[b].m_cell_under[1]
        };
        Index u[2] = {
            boundaries[b].m_ends[0],
            boundaries[b].m_ends[1]
        };
        Index nu[2] = {
            boundaries[b].m_normals[0],
            boundaries[b].m_normals[1]
        };
        bool edge_over[2] = {
            boundaries[b].m_cell_over[0] != cells[0],
            boundaries[b].m_cell_over[1] != cells[1]
        };
        if( cells[0] != IllegalIndex || cells[1] != IllegalIndex ) {
            Interface interface( cells[0], cells[1],
                                 orientation,
                                 (boundaries[b-1].m_side != 3) || (boundaries[b].m_side!=3) );
            LOGGER_INVARIANT_LESS_EQUAL( log, l[1], u[1] );
            LOGGER_INVARIANT_LESS_EQUAL( log, l[0], u[0] );
            segments.clear();

            // up along pillar 1
            if( l[1] == u[1] ) {
                segments.push_back( Segment( nl[1], l[1], (edge_over[0]?1u:0u)|(edge_over[1]?2u:0u) ) );
            }
            else {
                segments.push_back( Segment( nl[1], l[1], 3u ) );
                for( Index v=l[1]+1; v<u[1]; v++ ) {
                    segments.push_back( Segment( m_tessellation.addNormal( wall.normal( 1.f, m_tessellation.vertex(v).z() ) ),
                                                 v, 3u ) );
                }
                segments.push_back( Segment( nu[1], u[1], (edge_over[0]?1u:0u)|(edge_over[1]?2u:0u) ) );
            }

            // down along pillar 0
            if( l[0] == u[0] ) {
                segments.push_back( Segment( nu[0], u[0], (edge_under[0]?1u:0u)|(edge_under[1]?2u:0u) ) );
            }
            else {
                segments.push_back( Segment( nu[0], u[0], 3u ) );
                for( Index v=u[0]-1u; v>l[0]; v-- ) {
                    segments.push_back( Segment( m_tessellation.addNormal( wall.normal( 0.f, m_tessellation.vertex(v).z() ) ),
                                                 v, 3u ) );
                }
                segments.push_back( Segment( nl[0], l[0], (edge_under[0]?1u:0u)|(edge_under[1]?2u:0u) ) );
            }
            m_tessellation.addPolygon( interface,
                                       segments.data(),
                                       segments.size() );
        }
        l[0] = u[0];
        l[1] = u[1];
        nl[0] = nu[0];
        nl[1] = nu[1];
        edge_under[0] = edge_over[0];
        edge_under[1] = edge_over[1];
    }
}




template<typename Tessellation>
void
Tessellator<Tessellation>::extractWallLines( vector<WallLine>&     wall_lines,
                                                        Index*                boundary_line_index_a,
                                                        Index*                boundary_line_index_b,
                                                        const Index* const    zcorn_ix_a_0,
                                                        const Index* const    zcorn_ix_a_1,
                                                        const Index* const    zcorn_ix_b_0,
                                                        const Index* const    zcorn_ix_b_1,
                                                        const Index* const    active_cell_list_a,
                                                        const Index* const    active_cell_list_b,
                                                        const Index           active_cell_count_a,
                                                        const Index           active_cell_count_b,
                                                        const Index* const    cell_map_a,
                                                        const Index* const    cell_map_b,
                                                        const SrcReal* const  o0_coord,
                                                        const SrcReal* const  o1_coord,
                                                        const Index           stride,
                                                        const Index           adjacent_stride )
{
    // Helper struct for wall lines on one side of the wall
    struct SidedWallLine {
        Index  m_ends[2];
        Index  m_cell_under;
        Index  m_cell_over;
        Index  m_twin_under;
        Index  m_twin_over;
        Index  m_maps_to_merged_ix;
    };
    Logger log = getLogger( package + ".extractWallLines" );

    // Convert to arrays so we can use loops.
    const Index* zcorn_ix[4] = {
        zcorn_ix_a_0,
        zcorn_ix_a_1,
        zcorn_ix_b_0,
        zcorn_ix_b_1
    };
    const Index* active_cell_list[2] = {
        active_cell_list_a,
        active_cell_list_b
    };
    const Index active_cell_count[2] = {
        active_cell_count_a,
        active_cell_count_b
    };
    const Index* cell_map[2] = {
        cell_map_a,
        cell_map_b
    };
    Index* boundary_line_index[2] = {
        boundary_line_index_a,
        boundary_line_index_b,
    };


    // Step 1: Extract all wall lines on each side of the wall.
    vector<SidedWallLine> sided_wall_lines[2];
    for( Index side=0; side<2; side++) {
        vector<SidedWallLine>& current_lines = sided_wall_lines[side];

        current_lines.reserve( 2*active_cell_count[ side ] );

        for(Index m=0; m<active_cell_count[ side ]; m++ ) {
            const Index b0 = zcorn_ix[2*side+0][ 2*m + 0 ];
            const Index b1 = zcorn_ix[2*side+1][ 2*m + 0 ];
            const Index t0 = zcorn_ix[2*side+0][ 2*m + 1 ];
            const Index t1 = zcorn_ix[2*side+1][ 2*m + 1 ];
            LOGGER_INVARIANT( log, b0 != IllegalIndex );
            LOGGER_INVARIANT( log, b1 != IllegalIndex );
            LOGGER_INVARIANT( log, t0 != IllegalIndex );
            LOGGER_INVARIANT( log, t1 != IllegalIndex );

            const int k = active_cell_list[side][ m ];
            const int cell_ix  = cell_map[ side   ][ stride*k ];
            const int twin_ix =  active_cell_count[side^1] == 0 ? 0 : cell_map[ side^1 ][ stride*k ];

            if( b0 == t0 && b1 == t1 ) {
                // Cell face is degenerate. If edge is identical to edge below,
                // we don't have to do anything. Otherwise, we add an edge that
                // has no cell neither above or below.

                if( current_lines.empty() ||
                        (current_lines.back().m_ends[0] != b0 ) ||
                        (current_lines.back().m_ends[1] != b1 ) )
                {
                    // Create edge
                    current_lines.resize( current_lines.size() + 1 );
                    SidedWallLine& current_line = current_lines.back();
                    current_line.m_ends[0] = b0;
                    current_line.m_ends[1] = b1;
                    current_line.m_cell_under = IllegalIndex;
                    current_line.m_cell_over  = IllegalIndex;
                }
                boundary_line_index[side][2*m+0] = current_lines.size()-1u;
                boundary_line_index[side][2*m+1] = current_lines.size()-1u;
            }
            else {
                // Bottom edge
                if( current_lines.empty() ||
                        (current_lines.back().m_ends[0] != b0 ) ||
                        (current_lines.back().m_ends[1] != b1 ) )
                {
                    // Either first edge in column or bottom edge didn't match
                    // previous top edge; create a new edge
                    current_lines.resize( current_lines.size() + 1 );
                    SidedWallLine& top_line = current_lines.back();
                    top_line.m_ends[0]    = b0;
                    top_line.m_ends[1]    = b1;
                    top_line.m_cell_under = IllegalIndex;
                    top_line.m_cell_over  = cell_ix;
                    top_line.m_twin_under = IllegalIndex;
                    top_line.m_twin_over  = twin_ix;
                }
                else {
                    // Edge matched previous edge, recycle edge.
                    SidedWallLine& bottom_line = current_lines.back();
                    bottom_line.m_cell_over = cell_ix;
                    bottom_line.m_twin_over = twin_ix;
                }
                boundary_line_index[side][2*m+0] = current_lines.size()-1u;

                // Top edge, always created
                current_lines.resize( current_lines.size() + 1 );
                SidedWallLine& top_line = current_lines.back();
                top_line.m_ends[0]    = t0;
                top_line.m_ends[1]    = t1;
                top_line.m_cell_under = cell_ix;
                top_line.m_cell_over  = IllegalIndex;
                top_line.m_twin_under = twin_ix;
                top_line.m_twin_over  = IllegalIndex;
                boundary_line_index[side][2*m+1] = current_lines.size()-1u;
            }
        }
    }

    PillarWallSampler<Tessellation> wall( o0_coord, o1_coord );

    // Step 2: Merge the lines from the two sides.
    wall_lines.clear();
    wall_lines.reserve( sided_wall_lines[0].size() + sided_wall_lines[1].size() );
    Index i[2] = {0u, 0u };
    Index cells_below[2] = { IllegalIndex, IllegalIndex };
    Index twins_above[2] = { IllegalIndex, IllegalIndex };
    while( 1 ) {

        // check the current edge on each side to find the lexicographically smallest
        Index smallest_p0   = IllegalIndex;
        Index smallest_p1   = IllegalIndex;
        Index overall_smallest_p1      = IllegalIndex;   // the p1 of the largest edge
        Index smallest_side = IllegalIndex;
        for(Index side=0; side<2; side++) {
            const Index ii = i[side];
            const vector<SidedWallLine>& swls = sided_wall_lines[side];
            if( ii < swls.size() ) {
                const Index p0 = swls[ii].m_ends[0];
                const Index p1 = swls[ii].m_ends[1];
                overall_smallest_p1 = std::min( overall_smallest_p1, p1 );
                if( (p0 < smallest_p0 ) ||
                        ( (p0 == smallest_p0 ) &&
                          (p1 <= smallest_p1 ) ) )
                {
                    smallest_p0   = p0;
                    smallest_p1   = p1;
                    smallest_side = side;
                }
            }
        }
        if( 2 < smallest_side ) {
            // This may be triggered if edges on a side is decreasing
            LOGGER_INVARIANT( log, smallest_p0 == IllegalIndex );
            LOGGER_INVARIANT( log, smallest_p1 == IllegalIndex );
            break;
        }
        SidedWallLine& sl = sided_wall_lines[ smallest_side ][ i[smallest_side] ];


        Index cells_above[2] = { cells_below[0], cells_below[1] };
        cells_above[ smallest_side ] = sl.m_cell_over;
        twins_above[ smallest_side ] = sl.m_twin_over;

        LOGGER_INVARIANT( log, cells_below[ smallest_side ] == sl.m_cell_under );


        // Check if we have matching cell indices (used to detect faults)



        if( wall_lines.empty() ||
                (wall_lines.back().m_ends[0] != smallest_p0) ||
                (wall_lines.back().m_ends[1] != smallest_p1 ) )
        {
            const Real z0 = m_tessellation.vertex( smallest_p0 ).z();
            const Real z1 = m_tessellation.vertex( smallest_p1 ).z();


            wall_lines.resize( wall_lines.size() + 1 );
            WallLine& l = wall_lines.back();
            l.m_ends[0] = smallest_p0;
            l.m_ends[1] = smallest_p1;
            l.m_normals[0] = m_tessellation.addNormal( wall.normal( 0.f, z0 ) );
            l.m_normals[1] = m_tessellation.addNormal( wall.normal( 1.f, z1 ) );
            l.m_side = smallest_side + 1;
            l.m_cutoff = overall_smallest_p1;
            l.m_cell_over[0] = cells_above[0];
            l.m_cell_over[1] = cells_above[1];
            l.m_cell_under[0] = cells_below[0];
            l.m_cell_under[1] = cells_below[1];
            l.m_match_over = false; // cannot be match yet, needs two edges
        }
        else {
            WallLine& l = wall_lines.back();
            l.m_side = l.m_side ^ (smallest_side + 1);
            //l.m_cutoff = smallest_p1;
            l.m_cell_over[0] = cells_above[0];
            l.m_cell_over[1] = cells_above[1];
            l.m_match_over = cells_above[0] == twins_above[1];
            LOGGER_INVARIANT( log, l.m_side == 3 );
        }
        sl.m_maps_to_merged_ix = wall_lines.size()-1;

        i[ smallest_side ]++;
        cells_below[0] = cells_above[0];
        cells_below[1] = cells_above[1];
    }

    for( size_t i=0; i<wall_lines.size(); i++ ) {
        wall_lines[i].m_fault = !wall_lines[ i>0 ? i-1 : i ].m_match_over ||
                                !wall_lines[ i ].m_match_over;
    }

#ifdef CHECK_INVARIANTS
    for( Index side=0; side<2; side++) {
        for( Index m=0; m<active_cell_count[side]; m++ ) {
            for(Index i=0; i<2; i++ ) {
                SidedWallLine& sl = sided_wall_lines[side][ boundary_line_index[side][2*m+i] ];
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+0][2*m+i], sl.m_ends[0] );
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+1][2*m+i], sl.m_ends[1] );
            }
        }
    }
#endif
    // update sided indices to merged indices
    for( Index side=0; side<2; side++ ) {
        for( Index m = 0; m<2*active_cell_count[side]; m++ ) {
            LOGGER_INVARIANT( log, boundary_line_index[side][m] < sided_wall_lines[side].size() );
            const Index sided_ix = boundary_line_index[ side ][ m ];
            const Index merged_ix = sided_wall_lines[ side ][ sided_ix ].m_maps_to_merged_ix;
            boundary_line_index[ side ][ m ] = merged_ix;
        }
    }

#ifdef CHECK_INVARIANTS
    for( Index side=0; side<2; side++) {
        for( Index m=0; m<active_cell_count[side]; m++ ) {
            for(Index i=0; i<2; i++ ) {
                WallLine& l = wall_lines[ boundary_line_index[side][ 2*m+i ] ];
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+0][2*m+i], l.m_ends[0] );
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+1][2*m+i], l.m_ends[1] );
            }
        }
    }
    for(Index i=1; i<wall_lines.size(); i++ ) {
        LOGGER_INVARIANT_EQUAL( log, wall_lines[i-1].m_cell_over[0], wall_lines[i].m_cell_under[0] );
        LOGGER_INVARIANT_EQUAL( log, wall_lines[i-1].m_cell_over[1], wall_lines[i].m_cell_under[1] );
    }
#endif
}

template<typename Tessellation>
void
Tessellator<Tessellation>::intersectWallLines( vector<Intersection>&    wall_line_intersections,
                                                          vector<Index>&    chains,
                                                          Index*                 chain_offsets,
                                                          const vector<WallLine>&  wall_lines,
                                                          const float*                  pillar_a,
                                                          const float*                  pillar_b )
{
    Logger log = getLogger( package + ".intersectWallLines" );
    if( wall_lines.empty() ) {
        return;
    }

    PillarWallSampler<Tessellation> wall( pillar_a, pillar_b );

    // Step 1: Detect intersections, and insert the index of the intersection
    // at the chains of each of the two lines that intersect.
    //
    // We use two lines; 'lowest_at_p0' and 'larger_at_p0'. For these two lines
    // to intersect, 'larger_at_p0' must be smaller at p1 than lowest_at_p0.
    //
    // Since we scan from smaller indices to larger, we find intersections in an
    // particular order: All found intersections are in front (from p0 to p1)
    // for 'lowest_at_p0', while the converse is true for 'larger_at_p0', all
    // found intersections come after.
#ifdef CHECK_INVARIANTS
    const Index isec_base_offset = wall_line_intersections.size();
#endif

    vector< std::list<Index> > chains_tmp( wall_lines.size() );
    for( size_t lower = 0; lower < wall_lines.size(); lower++ ) {
        const WallLine& lowest_at_p0 = wall_lines[lower];

        // scan over edges that are above at pillar 0
        for( size_t upper = lower+1; upper < wall_lines.size(); upper++ ) {
            const WallLine& larger_at_p0 = wall_lines[upper];

            // Early exit: No edges above have an index at p1 that is smaller
            // than lowest_at_p0.
            if( larger_at_p0.m_cutoff >= lowest_at_p0.m_ends[1] ) {
                break;
            }

            // detect upper edges that are lower at pillar 1 -> intersection
            if( larger_at_p0.m_ends[1] < lowest_at_p0.m_ends[1] ) {
                // Just make sure that the early exit condition holds
                // (only relevant when we disable early exit)
                // LOGGER_INVARIANT( log, larger_at_p0.m_cutoff < lowest_at_p0.m_ends[1]);


                // We have an intersection. Since boundaries have been compacted
                // on each side of the cell column wall, the boundaries should
                // be from different sides.

                LOGGER_INVARIANT( log, (wall_lines[ upper ].m_side ^ wall_lines[ lower ].m_side) == 3 );
                const Index is_ix = wall_line_intersections.size();
                chains_tmp[lower].push_back( is_ix );
                chains_tmp[upper].push_front( is_ix );
                wall_line_intersections.resize( wall_line_intersections.size()+1 );

                Real4 ip = wall.intersect( m_tessellation.vertex( lowest_at_p0.m_ends[0] ).z(),
                                           m_tessellation.vertex( lowest_at_p0.m_ends[1] ).z(),
                                           m_tessellation.vertex( larger_at_p0.m_ends[0] ).z(),
                                           m_tessellation.vertex( larger_at_p0.m_ends[1] ).z() );

                Intersection& i = wall_line_intersections.back();
                i.m_vtx_ix = m_tessellation.addVertex( ip );
                i.m_upwrd_bndry_ix = lower;
                i.m_dnwrd_bndry_ix = upper;
                i.m_n = m_tessellation.addNormal( wall.normal( ip.w(), ip.z() ) );
            }
        }
    }
#ifdef DEBUG
    for(auto isec=wall_line_intersections.begin(); isec!=wall_line_intersections.end(); ++isec ) {
        LOGGER_INVARIANT_LESS( log, isec->m_dnwrd_bndry_ix, wall_lines.size() );
        LOGGER_INVARIANT_LESS( log, isec->m_upwrd_bndry_ix, wall_lines.size() );
    }
#endif

    // each intersection is present in two chains
    chains.reserve( chains.size() + 2u*wall_line_intersections.size() );

    // Run through the lists in chains_tmp to (i) populate the chains array, as
    // we know the size of all chains, and (ii) add m_nxt_dnwrd_isec_ix and
    // m_nxt_upwrd_isec_ix, which is used by the side wall triangulation front
    // movement code.

    for(size_t b_ix=0; b_ix<wall_lines.size(); b_ix++) {

        LOGGER_INVARIANT( log, chain_offsets[b_ix] == IllegalIndex );
        chain_offsets[b_ix] = chains.size();

        const std::list<Index>& chain = chains_tmp[b_ix];
        if( !chain.empty() ) {

            for( auto it=chain.begin(); it!=chain.end(); ++it ) {
                chains.push_back( *it );
            }

            auto p = chain.begin();
            auto c = p;
            for( c++; c != chain.end(); c++ ) {
                if( b_ix == wall_line_intersections[*p].m_dnwrd_bndry_ix ) {
                    wall_line_intersections[ *p ].m_nxt_dnwrd_isec_ix = NextIntersection::intersection( *c );
                }
                else if( b_ix == wall_line_intersections[*p].m_upwrd_bndry_ix ) {
                    wall_line_intersections[ *p ].m_nxt_upwrd_isec_ix = NextIntersection::intersection( *c );
                }
                p = c;
            }
            if( b_ix == wall_line_intersections[*p].m_dnwrd_bndry_ix ) {
                wall_line_intersections[ *p ].m_nxt_dnwrd_isec_ix = NextIntersection::vertex( wall_lines[b_ix].m_ends[1] );
            }
            else if( b_ix == wall_line_intersections[*p].m_upwrd_bndry_ix ) {
                wall_line_intersections[ *p ].m_nxt_upwrd_isec_ix = NextIntersection::vertex( wall_lines[b_ix].m_ends[1] );
            }
        }
    }
    chain_offsets[ wall_lines.size() ] = chains.size();

#ifdef CHECK_INVARIANTS
    for( Index l = 0; l < wall_lines.size(); l++ ) {
        const Index a = chain_offsets[l];
        const Index b = chain_offsets[l+1];
        for( Index c=a; c<b; c++) {
            const Index i = chains[c];
            LOGGER_INVARIANT_LESS_EQUAL( log, isec_base_offset, i );
            Intersection& isec = wall_line_intersections[ i ];
            LOGGER_INVARIANT_EITHER_EQUAL(log,
                                          isec.m_dnwrd_bndry_ix, l,
                                          isec.m_upwrd_bndry_ix, l );
        }
    }
    for( auto it = wall_line_intersections.begin(); it!=wall_line_intersections.end(); ++it ) {
        LOGGER_INVARIANT_LESS( log, it->m_upwrd_bndry_ix, wall_lines.size() );
        LOGGER_INVARIANT_LESS( log, it->m_dnwrd_bndry_ix, wall_lines.size() );
    }
#endif
}



template class Tessellator< render::GridTessBridge >;

} // of namespace cornerpoint
