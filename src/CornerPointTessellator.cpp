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
#include "Logger.hpp"
#include "PerfTimer.hpp"
#include "GridTessBridge.hpp"
#include "PolygonTessellator.hpp"
#include "CornerPointTessellator.hpp"
#ifdef CHECK_INVARIANTS
#include "CellSanityChecker.hpp"
#endif

using std::vector;
static const std::string package = "CornerPointTessellator";

template<typename Tessellation>
CornerPointTessellator<Tessellation>::CornerPointTessellator( Tessellation& tessellation )
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
CornerPointTessellator<Tessellation>::triangulate( const Index             nx,
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
        if( (j!=0) && (j%16==0) ) {
            LOGGER_DEBUG( log, "Tessellating grid... " << ((j*100)/ny) << "%" );
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
                                                      wall_lines );
                    }
                    else {
                        stitchPillarsHandleIntersections( ORIENTATION_I,
                                                          wall_lines,
                                                          wall_line_intersections,
                                                          pi0jm1_d01_chains,
                                                          pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i );
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
                                                      wall_lines );
                    }
                    else {
                        stitchPillarsHandleIntersections( ORIENTATION_J,
                                                          wall_lines,
                                                          wall_line_intersections,
                                                          pi0j0_d10_chains,
                                                          pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1) );
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

    m_tessellation.process();
}

template<typename Tessellation>
typename CornerPointTessellator<Tessellation>::Index
CornerPointTessellator<Tessellation>::segmentIntersection( const Index a0ix, const Index a1ix,
                                                           const Index b0ix, const Index b1ix,
                                                           const SrcReal* pillar_a,
                                                           const SrcReal* pillar_b )
{
    Logger log = getLogger( package + ".segmentIntersection" );

    Real s1_a_z = m_tessellation.vertex( a0ix ).z();
    Real s1_b_z = m_tessellation.vertex( a1ix ).z();
    Real s2_a_z = m_tessellation.vertex( b0ix ).z();
    Real s2_b_z = m_tessellation.vertex( b1ix ).z();

    Real m_z = 0.25f*( s1_a_z + s1_b_z + s2_a_z + s2_b_z );
    s1_a_z -= m_z;
    s1_b_z -= m_z;
    s2_a_z -= m_z;
    s2_b_z -= m_z;



    Real s = (s2_a_z - s1_a_z)/( (s1_b_z-s1_a_z) - (s2_b_z-s2_a_z) );
    if( s < 0.f || 1.f < s ) {
        LOGGER_ERROR( log, "Inter-pillar blend "<<s<<" is outside [0,1]");
    }

    Real z = (1.f-s)*s1_a_z + s*s1_b_z + m_z;

    Real ta = (z-pillar_a[2])/(pillar_a[5]-pillar_a[2]);
    Real tb = (z-pillar_b[2])/(pillar_b[5]-pillar_b[2]);

    Real iax = (1.f-ta)*pillar_a[0] + ta*pillar_a[3];
    Real ibx = (1.f-tb)*pillar_b[0] + tb*pillar_b[3];
    Real ix = (1.f-s)*iax + s*ibx;

    Real iay = (1.f-ta)*pillar_a[1] + ta*pillar_a[4];
    Real iby = (1.f-tb)*pillar_b[1] + tb*pillar_b[4];
    Real iy = (1.f-s)*iay + s*iby;

    glm::vec3 i = glm::vec3( ix, iy, z );


#ifdef CHECK_INVARIANTS
    glm::vec3 v1( m_tessellation.vertex( a0ix ).x(), m_tessellation.vertex( a0ix ).y(), m_tessellation.vertex( a0ix ).z() );
    glm::vec3 v2( m_tessellation.vertex( a1ix ).x(), m_tessellation.vertex( a1ix ).y(), m_tessellation.vertex( a1ix ).z() );
    glm::vec3 v3( m_tessellation.vertex( b0ix ).x(), m_tessellation.vertex( b0ix ).y(), m_tessellation.vertex( b0ix ).z() );
    glm::vec3 v4( m_tessellation.vertex( b1ix ).x(), m_tessellation.vertex( b1ix ).y(), m_tessellation.vertex( b1ix ).z() );

    glm::vec3 v12 = glm::normalize( v2-v1 );
    glm::vec3 v1i = glm::normalize( i-v1 );
    glm::vec3 vi2 = glm::normalize( v2-i );
    if( (glm::dot( v12,v1i) < 0.0 ) || (glm::dot(v12,vi2) < 0.0 ) ) {
        LOGGER_ERROR( log, "v12 is wild"
                      << ", v12=" << glm::to_string( v2-v1 )
                      << ", v1i=" << glm::to_string( i-v1 )
                      << ", vi2=" << glm::to_string( v2-i ) );
    }

    glm::vec3 v34 = glm::normalize( v4-v3 );
    glm::vec3 v3i = glm::normalize( i-v3 );
    glm::vec3 vi4 = glm::normalize( v4-i );
    if( (glm::dot( v34,v3i) < 0.0 ) || (glm::dot(v34,vi4) < 0.0 ) ) {
        LOGGER_ERROR( log, "v34 is wild" );
    }


#endif
    return m_tessellation.addVertex( Real4( i.x, i.y, i.z ) );
}



template<typename Tessellation>
void
CornerPointTessellator<Tessellation>::findActiveCellsInColumn( Index*            active_cell_list,
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
CornerPointTessellator<Tessellation>::pillarEdges( const Index           offset,
                                                   const vector<Index>&  adjacent_cells,
                                                   const Index*          cell_map_00,
                                                   const Index*          cell_map_01,
                                                   const Index*          cell_map_10,
                                                   const Index*          cell_map_11,
                                                   const Index           stride )
{
    for( size_t i=1; i<adjacent_cells.size()/4; i++) {
        const Index* adj = adjacent_cells.data() + 4*i;
        if( (adj[0] != IllegalIndex ) ||
            (adj[1] != IllegalIndex ) ||
            (adj[2] != IllegalIndex ) ||
            (adj[3] != IllegalIndex ) )
        {
            const Index cells[4] = { adj[0] != IllegalIndex ? cell_map_00[ stride*adj[0] ] : IllegalIndex,
                                            adj[1] != IllegalIndex ? cell_map_01[ stride*adj[1] ] : IllegalIndex,
                                            adj[2] != IllegalIndex ? cell_map_10[ stride*adj[2] ] : IllegalIndex,
                                            adj[3] != IllegalIndex ? cell_map_11[ stride*adj[3] ] : IllegalIndex
                                          };
            m_tessellation.addEdge( offset + i - 1 , offset + i,
                                    cells[0], cells[1], cells[2], cells[3] );
        }
    }
}


template<typename Tessellation>
void
CornerPointTessellator<Tessellation>::uniquePillarVertices( vector<Index>&      adjacent_cells,
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


    //Index cells_below[4] = { IllegalIndex, IllegalIndex, IllegalIndex, IllegalIndex };



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
            for( Index l=0; l<4; l++ ) {
                if( i[l] < 2*active_cell_count[l] ) {
                    const Index ii = i[l]>>1;
                    const Index ij = i[l]&1;

                    const Index k = active_cell_list[l][ ii ];
                    Real z = zcorn[l][ 4*stride*(2*k + ij) ];

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
                //adjacent_cells.push_back( cells_below[0] );
                //adjacent_cells.push_back( cells_below[1] );
                //adjacent_cells.push_back( cells_below[2] );
                //adjacent_cells.push_back( cells_below[3] );
                first = false;
            }
            const Index ii = i[smallest_l]>>1;
            const Index ij = i[smallest_l]&1;
            //cells_below[ smallest_l ] = ij == 0 ? smallest_k : IllegalIndex;
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
CornerPointTessellator<Tessellation>::stitchTopBottom( const Index* const   ci0j0_active_cell_list,
                                                 const Index          ci0j0_active_cell_count,
                                                 const Index* const   o01_d10_wall_line_ix,
                                                 const Index* const   o00_d10_wall_line_ix,
                                                 const Index* const   o10_d01_wall_line_ix,
                                                 const Index* const   o00_d01_wall_line_ix,
                                                 const Index* const   o00_zcorn_ix,
                                                 const Index* const   o10_zcorn_ix,
                                                 const Index* const   o01_zcorn_ix,
                                                 const Index* const   o11_zcorn_ix,
                                                 const Index* const   o01_d10_chains,
                                                 const Index* const   o01_d10_chain_offsets,
                                                 const Index* const   o00_d10_chains,
                                                 const Index* const   o00_d10_chain_offsets,
                                                 const Index* const   o10_d01_chains,
                                                 const Index* const   o10_d01_chain_offsets,
                                                 const Index* const   o00_d01_chains,
                                                 const Index* const   o00_d01_chain_offsets,
                                                 const Index* const            cell_map,
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
            helper.back().m_fault = last_k + 1 != k;
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

    vector<Segment> segments;
    for( auto it=helper.begin(); it!=helper.end(); ++it ) {
        segments.clear();
        const Index b_ix = it->m_index;

        // (0,0) -> (0,1)
        segments.push_back( Segment( o00_zcorn_ix[ b_ix ], true, true ) );
        Index o00_d01_0 = o00_d01_chain_offsets[ o00_d01_wall_line_ix[ b_ix ] ];
        Index o00_d01_1 = o00_d01_chain_offsets[ o00_d01_wall_line_ix[ b_ix ] + 1 ];
        for( Index i=o00_d01_0; i<o00_d01_1; i++ ) {
            segments.push_back( Segment( o00_d01_chains[i], true, true ) );
        }

        // (0,1) -> (1,1)
        segments.push_back( Segment( o01_zcorn_ix[ b_ix ], true, true ) );
        Index o01_d10_0 = o01_d10_chain_offsets[ o01_d10_wall_line_ix[ b_ix ] ];
        Index o01_d10_1 = o01_d10_chain_offsets[ o01_d10_wall_line_ix[ b_ix ] + 1 ];
        for( Index i=o01_d10_0; i<o01_d10_1; i++ ) {
            segments.push_back( Segment( o01_d10_chains[i], true, true ) );
        }

        // (1,1) -> (1,0)
        segments.push_back( Segment( o11_zcorn_ix[ b_ix ], true, true ) );
        Index o10_d01_0 = o10_d01_chain_offsets[ o10_d01_wall_line_ix[ b_ix ] ];
        Index o10_d01_1 = o10_d01_chain_offsets[ o10_d01_wall_line_ix[ b_ix ] + 1 ];
        for( Index i=o10_d01_1; i>o10_d01_0; i-- ) {
            segments.push_back( Segment( o10_d01_chains[i-1], true, true ) );
        }

        // (1,0) -> (0,0)
        segments.push_back( Segment( o10_zcorn_ix[ b_ix ], true, true ) );
        Index o00_d10_0 = o00_d10_chain_offsets[ o00_d10_wall_line_ix[ b_ix ] ];
        Index o00_d10_1 = o00_d10_chain_offsets[ o00_d10_wall_line_ix[ b_ix ] + 1 ];
        for( Index i=o00_d10_1; i>o00_d10_0; i-- ) {
            segments.push_back( Segment( o00_d10_chains[i-1], true, true ) );
        }

        m_tessellation.addPolygon( Interface( it->m_cell_above, it->m_cell_below, ORIENTATION_K, it->m_fault),
                                   segments.data(), segments.size() );
    }
}


template<typename Tessellation>
void
CornerPointTessellator<Tessellation>::wallEdges( const vector<WallLine>&      boundaries,
                                                 const vector<Intersection>&  intersections,
                                                 const vector<Index>&  chains,
                                                 const Index* const         chain_offsets )
{
    for( size_t b=0; b<boundaries.size(); b++ ) {
        const WallLine& wl = boundaries[b];
        Index cell_over[2];
        Index cell_under[2];
        cell_over[0]  = wl.m_cell_over[0];
        cell_over[1]  = wl.m_cell_over[1];
        cell_under[0] = wl.m_cell_under[0];
        cell_under[1] = wl.m_cell_under[1];
        Index other_side = wl.m_side==1 ? 0 : 1;
        Index end0 = wl.m_ends[0];
        for( size_t c=chain_offsets[b]; c!=chain_offsets[b+1]; c++ ) {
            const Intersection& i = intersections[ chains[c] ];
            const WallLine& ol = boundaries[ i.m_upwrd_bndry_ix == b ? i.m_upwrd_bndry_ix : i.m_dnwrd_bndry_ix ];
            cell_over[ other_side ]  = ol.m_cell_over[ other_side ];
            cell_under[ other_side ] = ol.m_cell_under[ other_side ];

            Index end1 = i.m_vtx_ix;
            m_tessellation.addEdge( end0, end1, cell_over[0], cell_under[0], cell_over[1], cell_under[1] );
            end0 = end1;
        }
        Index end1 = wl.m_ends[1];
        m_tessellation.addEdge( end0, end1, cell_over[0], cell_under[0], cell_over[1], cell_under[1] );
    }
}


template<typename Tessellation>
void
CornerPointTessellator<Tessellation>::stitchPillarsHandleIntersections( const Orientation                 orientation,
                                                                        const vector<WallLine>&      boundaries,
                                                                        const vector<Intersection>&  intersections,
                                                                        const vector<Index>&  chains,
                                                                        const Index* const         chain_offsets )
{
    Logger log = getLogger( package + ".stitchPillarsHandleIntersections" );

    vector<Segment> segments;

    // First, process as polygons that are attached at pillar 0. We start by
    // pairing adjacent edges and follow them out until a common intersection or
    // until they hit pillar 1.
    for( size_t b=0; (b+1u)<boundaries.size(); b++ ) {
        const WallLine& u_0_1_l = boundaries[b+1];
        const WallLine& l_0_1_l = boundaries[b];

        if( u_0_1_l.m_side == 3 && l_0_1_l.m_side == 3 ) {      // Non-fault (fully matching) interface
            Index cells[2];
            cells[0] = l_0_1_l.m_cell_over[0];
            cells[1] = l_0_1_l.m_cell_over[1];
            LOGGER_INVARIANT_EQUAL( log, cells[0], u_0_1_l.m_cell_under[0] );
            LOGGER_INVARIANT_EQUAL( log, cells[1], u_0_1_l.m_cell_under[1] );
            if( cells[0] == IllegalIndex && cells[1] == IllegalIndex ) {
                continue;
            }
            segments.clear();
            for( Index v=u_0_1_l.m_ends[0]; v>l_0_1_l.m_ends[0]; v-- ) {
                segments.push_back( Segment( v, true, true ) );
            }
            segments.push_back( Segment( l_0_1_l.m_ends[0], true, true ) );
            for( Index v=l_0_1_l.m_ends[1]; v<=u_0_1_l.m_ends[1]; v++ ) {
                segments.push_back( Segment( v, true, true ) );
            }
            m_tessellation.addPolygon( Interface( cells[0], cells[1], orientation, false ),
                                       segments.data(), segments.size() );
        }
        else {                                                  // Fault (partially matching) interface
            Index cell[2];
            cell[0] = l_0_1_l.m_cell_over[0];
            cell[1] = l_0_1_l.m_cell_over[1];
            LOGGER_INVARIANT_EQUAL( log, u_0_1_l.m_cell_under[0], cell[0] );
            LOGGER_INVARIANT_EQUAL( log, u_0_1_l.m_cell_under[1], cell[1] );

            if( cell[0] == IllegalIndex && cell[1] == IllegalIndex ) {
                continue;
            }
            segments.clear();

            // follow upper arc backwards from towards pilalr1 to pillar 0
            bool process_pillar_1 = false;
            Index u_v_1;
            if( chain_offsets[b+1] == chain_offsets[b+2] ) {
                // First intersection is on pillar
                u_v_1 = u_0_1_l.m_ends[1];
                process_pillar_1 = true;
            }
            else {
                // First intersection is on wall
                const Intersection& u_1_isec = intersections[ chains[ chain_offsets[b+1] ] ];
                if( u_1_isec.m_dnwrd_bndry_ix != (b+1) ) {
                    // We haven't turned downwards yet
                    const WallLine& u_1_2_l = boundaries[ u_1_isec.m_dnwrd_bndry_ix ];
                    NextIntersection ni = u_1_isec.m_nxt_dnwrd_isec_ix;
                    Index u_v_2;
                    if( ni.isIntersection() ) {
                        // Second intersection is on wall
                        u_v_2 = intersections[ ni.intersection() ].m_vtx_ix;
                    }
                    else {
                        // Second intersection is on pillar
                        u_v_2 = ni.vertex();
                        process_pillar_1 = true;
                    }

                    segments.push_back( Segment( u_v_2, u_1_2_l.m_side ) );
                }
                u_v_1 = u_1_isec.m_vtx_ix;
            }
            segments.push_back( Segment( u_v_1, u_0_1_l.m_side ) );

            // from (b+1).end[0] to (b).end[0]
            for(Index v=boundaries[b+1].m_ends[0]; v>boundaries[b].m_ends[0]; v-- ) {
                segments.push_back( Segment(v, true, true)  );
            }
            segments.push_back( Segment(boundaries[b].m_ends[0], boundaries[b].m_side )  );

            // follow lower arc forwards from pillar 0 towards pillar 1
            if( chain_offsets[b] == chain_offsets[b+1] ) {
                // First intersection on pillar
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                if( segments.front().vertex() == boundaries[b].m_ends[1] ) {
                    // Loop is closed
                    process_pillar_1 = false;
                }
                else {
                    segments.push_back( Segment( boundaries[b].m_ends[1], true, true ) );
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
                    const WallLine& l_1_2_l = boundaries[ l_1_isec.m_upwrd_bndry_ix ];
                    segments.push_back( Segment( l_1_isec.m_vtx_ix, l_1_2_l.m_side ) );

                    NextIntersection l_2_ix = l_1_isec.m_nxt_upwrd_isec_ix;
                    if( l_2_ix.isIntersection() ) {
                        // Second intersection is on wall, and we should have closed the loop
                        const Intersection& l_2_isec = intersections[ l_2_ix.intersection() ];
                        LOGGER_INVARIANT_EQUAL( log, process_pillar_1, false );
                        LOGGER_INVARIANT_EQUAL( log, segments.front().vertex(), l_2_isec.m_vtx_ix );
                    }
                    else if( segments.front().vertex() == l_2_ix.vertex() ) {
                        // Second intersection is on pillar, and matches upper loop
                        LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                        process_pillar_1 = false;
                    }
                    else {
                        // Second intersection is on pillar, no match, so we must close the loop
                        LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                        segments.push_back( Segment( l_2_ix.vertex(), true, true) );
                    }
                }
            }
            if( process_pillar_1 ) {
                Index b = segments.front().vertex();
                Index a = segments.back().vertex();
                for(Index v=a+1; v<b; v++ ) {
                    segments.push_back( Segment( v, true, true ) );
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
        const WallLine& u_0_1_l = boundaries[ is.m_upwrd_bndry_ix ];
        const WallLine& l_0_1_l = boundaries[ is.m_dnwrd_bndry_ix ];

        Index side1 = u_0_1_l.m_side - 1u;
        Index side2 = l_0_1_l.m_side - 1u;

        Index cell[2];
        cell[ side1 ] = u_0_1_l.m_cell_under[ side1 ];
        cell[ side2 ] = l_0_1_l.m_cell_over[  side2 ];
        if( cell[0] == IllegalIndex && cell[1] == IllegalIndex ) {
            continue;   // Empty hole
        }

        segments.clear();

        // Follow upper arc backwards
        NextIntersection u_1_ix = is.m_nxt_upwrd_isec_ix;
        bool process_pillar_1 = false;
        Index v_1;
        if( u_1_ix.isIntersection() ) {         // First intersection is on wall
            const Intersection& u_1_isec = intersections[ u_1_ix.intersection() ];
            const WallLine& u_1_2_l = boundaries[ u_1_isec.m_dnwrd_bndry_ix ];
            NextIntersection u_2_ix = u_1_isec.m_nxt_dnwrd_isec_ix;
            Index v_2;
            if( u_2_ix.isIntersection() ) {     // Second intersection is on wall
                const Intersection& isec_u_2 = intersections[ u_2_ix.intersection() ];
                v_2 = isec_u_2.m_vtx_ix;
                LOGGER_INVARIANT_EQUAL( log, cell[side2], u_1_2_l.m_cell_under[ side2 ] );
            }
            else {                              // Second intersection is on pillar
                v_2 = u_2_ix.vertex();
                process_pillar_1 = true;
            }
            segments.push_back( Segment( v_2, u_1_2_l.m_side ) );
            v_1 =  u_1_isec.m_vtx_ix;
        }
        else {                                  // First intersection is on pillar
            v_1 = u_1_ix.vertex();
            process_pillar_1 = true;
        }
        segments.push_back( Segment(  v_1, u_0_1_l.m_side ) );

        // Follow lower arc forwards
        NextIntersection l_1_ix = is.m_nxt_dnwrd_isec_ix;
        segments.push_back( Segment( is.m_vtx_ix, l_0_1_l.m_side ) );

        if( l_1_ix.isIntersection() ) {
            // First intersection is on wall, cannot be u_1_ix.
            const Intersection& l_1_isec = intersections[ l_1_ix.intersection() ];
            const WallLine& l_1_2_l = boundaries[ l_1_isec.m_upwrd_bndry_ix ];

            segments.push_back( Segment( l_1_isec.m_vtx_ix, l_1_2_l.m_side ) );

            NextIntersection l_2_ix = intersections[l_1_ix.intersection()].m_nxt_upwrd_isec_ix;
            if( l_2_ix.isIntersection() ) {
                // Second intersection is on wall, must be identical to upper arc
                const Intersection& l_2_isec = intersections[ l_2_ix.intersection() ];
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, false );
                LOGGER_INVARIANT_EQUAL( log, cell[side1], l_1_2_l.m_cell_over[ side1 ] );
                LOGGER_INVARIANT_EQUAL( log, segments.front().vertex(),l_2_isec.m_vtx_ix );
            }
            else if( segments.front().vertex() == l_2_ix.vertex() ) {
                // Second intersection is on pillar and closes loop
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                process_pillar_1 = false;
            }
            else {
                // Second intersection is on pillar and doesn't close loop
                LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
                segments.push_back( Segment( l_2_ix.vertex(), true, true ) );
            }
        }
        else if( segments.front().vertex() == l_1_ix.vertex() ) {
            // First intersection is on pillar and closes loop
            LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
            process_pillar_1 = false;
        }
        else {
            // First intersection is on pillar and doesn't close loop
            LOGGER_INVARIANT_EQUAL( log, process_pillar_1, true );
            segments.push_back( Segment( l_1_ix.vertex(), true, true) );
        }

        if( process_pillar_1 ) {
            // Add vertices along pillar 1
            Index b = segments.front().vertex();
            Index a = segments.back().vertex();
            for(Index v=a+1; v<b; v++ ) {
                segments.push_back( Segment( v, true, true ) );
            }
        }

        m_tessellation.addPolygon( Interface( cell[0], cell[1],orientation, true),
                                   segments.data(), segments.size() );
    }
}


template<typename Tessellation>
void
CornerPointTessellator<Tessellation>::stitchPillarsNoIntersections( const Orientation             orientation,
                                                                    const vector<WallLine>&  boundaries )
{
    Logger log = getLogger( package + ".stitchPillarsNoIntersections" );

    if( boundaries.empty() ) {
        return;
    }
    vector<Segment> segments;

    bool edge_under[2] = {
        boundaries[0].m_cell_over[0] != IllegalIndex,
        boundaries[0].m_cell_over[1] != IllegalIndex,
    };
    Index l[2] = {
            boundaries[0].m_ends[0],
            boundaries[0].m_ends[1]
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
            for( Index v=l[1]; v<=u[1]; v++ ) {
                segments.push_back( Segment( v, true, true ) );
            }
            segments.back().setEdges( edge_over[0], edge_over[1] );
            //segments.back().m_flags = (edge_over[0]?2:0) | (edge_over[1]?1:0);
            for( Index v=u[0]+1; v>l[0]; v-- ) {
                segments.push_back( Segment(v-1, true, true ) );
            }
            segments.back().setEdges( edge_under[0], edge_under[1] );
            //segments.back().m_flags = (edge_under[0]?2:0) | (edge_under[1]?1:0);
            m_tessellation.addPolygon( interface,
                                       segments.data(),
                                       segments.size() );
        }
        l[0] = u[0];
        l[1] = u[1];
        edge_under[0] = edge_over[0];
        edge_under[1] = edge_over[1];
    }
}


template<typename Tessellation>
void
CornerPointTessellator<Tessellation>::extractWallLines( vector<WallLine>&   wall_lines,
                                                        Index*              boundary_line_index_a,
                                                        Index*              boundary_line_index_b,
                                                        const Index* const  zcorn_ix_a_0,
                                                        const Index* const  zcorn_ix_a_1,
                                                        const Index* const  zcorn_ix_b_0,
                                                        const Index* const  zcorn_ix_b_1,
                                                        const Index* const  active_cell_list_a,
                                                        const Index* const  active_cell_list_b,
                                                        const Index         active_cell_count_a,
                                                        const Index         active_cell_count_b,
                                                        const Index* const  cell_map_a,
                                                        const Index* const  cell_map_b,
                                                        const Index         stride,
                                                        const Index         adjacent_stride )
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
            wall_lines.resize( wall_lines.size() + 1 );
            WallLine& l = wall_lines.back();
            l.m_ends[0] = smallest_p0;
            l.m_ends[1] = smallest_p1;
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
CornerPointTessellator<Tessellation>::intersectWallLines( vector<Intersection>&    wall_line_intersections,
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
                const Index vtx_ix = segmentIntersection( lowest_at_p0.m_ends[0], lowest_at_p0.m_ends[1],
                                                                 larger_at_p0.m_ends[0], larger_at_p0.m_ends[1],
                                                                 pillar_a,
                                                                 pillar_b );


                chains_tmp[lower].push_back( is_ix );
                chains_tmp[upper].push_front( is_ix );

                wall_line_intersections.resize( wall_line_intersections.size()+1 );
                Intersection& i = wall_line_intersections.back();
                i.m_vtx_ix = vtx_ix;
                i.m_upwrd_bndry_ix = lower;
                i.m_dnwrd_bndry_ix = upper;
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


template class CornerPointTessellator< PolygonTessellator<GridTessBridge> >;
