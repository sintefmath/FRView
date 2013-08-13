#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <list>
#include <sstream>
#include "Logger.hpp"
#include "GridTessBridge.hpp"
#include "CornerPointTessellator.hpp"

template<typename Bridge> const float        CornerPointTessellator<Bridge>::epsilon  = std::numeric_limits<REAL>::epsilon();
template<typename Bridge> const unsigned int CornerPointTessellator<Bridge>::end_flag = (1u<<31u);
template<typename Bridge> const unsigned int CornerPointTessellator<Bridge>::end_mask = ~end_flag;

#define FOO


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


template<typename Bridge>
void
CornerPointTessellator<Bridge>::triangulate( Bridge& bridge,
                                             const unsigned int nx,
                                             const unsigned int ny,
                                             const unsigned int nz,
                                             const unsigned int nr,
                                             const std::vector<typename Bridge::REAL>&  coord,
                                             const std::vector<typename Bridge::REAL>&  zcorn,
                                             const std::vector<int>&  actnum )
{
    Logger log = getLogger( "CPTessFactory.triangulate" );

    static const unsigned int chain_offset_stride = 4*nz+1;

    // enumeration of active cells is different from how we traverse the grid.
    std::vector<int> cell_map( nx*ny*nz, -1 );
    unsigned int active_cells = 0;
    for( size_t i=0; i<actnum.size(); i++ ) {
        if( actnum[i] != 0 ) {
            cell_map[i] = active_cells++;
        }
    }
    bridge.setCellCount( active_cells );
    bridge.reserveVertices( 8*active_cells );
    bridge.reserveEdges( 12*active_cells );
    bridge.reserveTriangles( 2*6*active_cells );

    LOGGER_DEBUG( log, "active cells = " << active_cells <<
                       " ("  << ((100.f*active_cells)/actnum.size()) << "%)." );


    std::vector<unsigned int> pi0jm1_d01_chains;
    std::vector<unsigned int> pi0jm1_d01_chain_offsets( (nx+1)*chain_offset_stride, ~0u );
    std::vector<unsigned int> pi0jm1_d10_chains;
    std::vector<unsigned int> pi0jm1_d10_chain_offsets( nx*chain_offset_stride, ~0u );
    std::vector<unsigned int> pi0j0_d10_chains;
    std::vector<unsigned int> pi0j0_d10_chain_offsets( nx*chain_offset_stride, ~0u );
    std::vector<unsigned int> jm1_active_cell_list( (nx+2)*(nz) );
    std::vector<unsigned int> jm1_active_cell_count( (nx+2), 0u );
    std::vector<unsigned int> jm0_active_cell_list( (nx+2)*(nz) );
    std::vector<unsigned int> jm0_active_cell_count( (nx+2), 0u );
    std::vector<unsigned int> jm1_zcorn_ix( nx*4*2*nz );
    std::vector<unsigned int> jm0_zcorn_ix( nx*4*2*nz );
    std::vector<unsigned int> jm1_wall_line_ix( nx*4*2*nz );
    std::vector<unsigned int> jm0_wall_line_ix( nx*4*2*nz );
    for( unsigned int j=0; j<ny+1; j++ ) {



        pi0jm1_d01_chains.clear();
        pi0j0_d10_chains.clear();

#if CHECK_INVARIANTS
        std::fill( jm0_active_cell_list.begin(), jm0_active_cell_list.end(), ~0u );
        std::fill( jm0_active_cell_count.begin(), jm0_active_cell_count.end(), ~0u );
        std::fill( pi0jm1_d01_chain_offsets.begin(), pi0jm1_d01_chain_offsets.end(), ~0u );
        std::fill( pi0j0_d10_chain_offsets.begin(), pi0j0_d10_chain_offsets.end(), ~0u );
        std::fill( jm0_wall_line_ix.begin(), jm0_wall_line_ix.end(), ~1u );
#endif


        // Iterate over all i's.
        jm0_active_cell_count[0] = 0u;
        for( unsigned int i=0; i<nx+1; i++ ) {

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

                const unsigned int vertex_pillar_start = bridge.vertexCount();
                std::vector<unsigned int> adjacent_cells;

                // Determine the unique set of vertices on a pillar
                uniquePillarVertices( bridge,
                                      adjacent_cells,
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
                pillarEdges( bridge,
                             vertex_pillar_start,
                             adjacent_cells,
                             cell_map.data() + (i-1) + nx*(j-1),
                             cell_map.data() + (i  ) + nx*(j-1),
                             cell_map.data() + (i-1) + nx*(j  ),
                             cell_map.data() + (i  ) + nx*(j  ),
                             nx*ny );

                // Process pillar [p(i,j-1) and p(i,j)] along j (with c(i-1,j-1) and c(i,j-1) abutting)
                if( 0 < j ) {
                    std::vector<WallLine> wall_lines;
                    std::vector<Intersection> wall_line_intersections;

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
                                      nx*ny );

                    intersectWallLines( bridge,
                                        wall_line_intersections,
                                        pi0jm1_d01_chains,
                                        pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i,
                                        wall_lines );

                    wallEdges( bridge,
                               wall_lines,
                               wall_line_intersections,
                               pi0jm1_d01_chains,
                               pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i );

                    if( wall_line_intersections.empty() ) {
                        stitchPillarsNoIntersections( bridge,
                                                      Bridge::ORIENTATION_I,
                                                      wall_lines );
                    }
                    else {
                        stitchPillarsHandleIntersections( bridge,
                                                          Bridge::ORIENTATION_I,
                                                          wall_lines,
                                                          wall_line_intersections,
                                                          pi0jm1_d01_chains,
                                                          pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i );
                    }


#ifdef CHECK_INVARIANTS
                    // Sanity check
                    for( unsigned int l=0; l<wall_lines.size(); l++ ) {
                        const unsigned int a = pi0jm1_d01_chain_offsets[chain_offset_stride*i + l + 0 ];
                        const unsigned int b = pi0jm1_d01_chain_offsets[chain_offset_stride*i + l + 1 ];
                        for( unsigned int c = a; c<b; c++ ) {
                            const unsigned int i = pi0jm1_d01_chains[ c ];
                            Intersection& isec = wall_line_intersections[ i ];
                            LOGGER_INVARIANT_EITHER_EQUAL( log, l, isec.m_upwrd_bndry_ix, l, isec.m_dnwrd_bndry_ix );
                        }
                    }
#endif

                    // replace intersection indices with vertex indices.
                    const unsigned int c0 = pi0jm1_d01_chain_offsets[chain_offset_stride*i ];
                    const unsigned int c1 = pi0jm1_d01_chain_offsets[chain_offset_stride*i+wall_lines.size() ];
                    for( unsigned int k=c0; k<c1; k++ ) {
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
                    std::vector<WallLine> wall_lines;
                    std::vector<Intersection> wall_line_intersections;

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
                                      nx*ny );

                    intersectWallLines( bridge,
                                        wall_line_intersections,
                                        pi0j0_d10_chains,
                                        pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1),
                                        wall_lines );

                    wallEdges( bridge,
                               wall_lines,
                               wall_line_intersections,
                               pi0j0_d10_chains,
                               pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1) );

                    if( wall_line_intersections.empty() ) {
                        stitchPillarsNoIntersections( bridge,
                                                      Bridge::ORIENTATION_J,
                                                      wall_lines );
                    }
                    else {
                        stitchPillarsHandleIntersections( bridge,
                                                          Bridge::ORIENTATION_J,
                                                          wall_lines,
                                                          wall_line_intersections,
                                                          pi0j0_d10_chains,
                                                          pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1) );
                    }

#ifdef CHECK_INVARIANTS
                    // Sanity check
                    for( unsigned int l=0; l<wall_lines.size(); l++ ) {
                        const unsigned int a = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1) + l + 0 ];
                        const unsigned int b = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1) + l + 1 ];
                        for( unsigned int c = a; c<b; c++ ) {
                            const unsigned int i = pi0j0_d10_chains[c];
                            Intersection& isec = wall_line_intersections[ i ];
                            LOGGER_INVARIANT_EITHER_EQUAL( log, l, isec.m_upwrd_bndry_ix, l, isec.m_dnwrd_bndry_ix );
                        }
                    }
#endif
                    // replace intersection indices with vertex indices.
                    const unsigned int c0 = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1) ];
                    const unsigned int c1 = pi0j0_d10_chain_offsets[chain_offset_stride*(i-1)+wall_lines.size() ];

                    for( unsigned int k=c0; k<c1; k++ ) {
                        pi0j0_d10_chains[k] = wall_line_intersections[ pi0j0_d10_chains[k] ].m_vtx_ix;
                    }
                }

                // For each full cell
                if( i>0 && j>0) {
                    stitchTopBottom( bridge,
                                     jm1_active_cell_list.data() + (nz*i),
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

                    // process cell column c(i-1,j-1)
                    for( unsigned int m=0; m < jm1_active_cell_count[ i ]; m++ ) {
                        const unsigned int k = jm1_active_cell_list[ (nz*i) + m];
                        const size_t gix = i-1 + nx*((j-1) + k*ny);
                        bridge.setCell( cell_map[gix],
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
}

template<typename Bridge>
unsigned int
CornerPointTessellator<Bridge>::segmentIntersection( Bridge& bridge,
                                                     const unsigned int a0ix, const unsigned int a1ix,
                                                     const unsigned int b0ix, const unsigned int b1ix )
{
    Logger log = getLogger( "CPTessFactory.segmentIntersection" );

    const REAL a0[3] = { bridge.vertexX( a0ix ), bridge.vertexY( a0ix ), bridge.vertexZ( a0ix ) };
    const REAL a1[3] = { bridge.vertexX( a1ix ), bridge.vertexY( a1ix ), bridge.vertexZ( a1ix ) };
    const REAL b0[3] = { bridge.vertexX( b0ix ), bridge.vertexY( b0ix ), bridge.vertexZ( b0ix ) };
    const REAL b1[3] = { bridge.vertexX( b1ix ), bridge.vertexY( b1ix ), bridge.vertexZ( b1ix ) };
    const REAL p[3] = { a0[0],         a0[1],      a0[2]       };  // a0
    const REAL u[3] = { a1[0]-p[0],    a1[1]-p[1], a1[2]-p[2]  };  // a1-a0
    const REAL q[3] = { b0[0],         b0[1],      b0[2]       };  // b0
    const REAL v[3] = { b1[0]-q[0],    b1[1]-q[1], b1[2]-q[2]  };  // b1-b0
    const REAL w[3] = { p[0]-q[0],     p[1]-q[1],  p[2]-q[2]   };  // p-q
    const REAL a = u[0]*u[0] + u[1]*u[1] + u[2]*u[2];              // dot(u,u)
    const REAL b = u[0]*v[0] + u[1]*v[1] + u[2]*v[2];              // dot(u,v)
    const REAL c = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];              // dot(v,v)
    const REAL den = a*c-b*b;

    unsigned int retval = bridge.vertexCount();
    if( fabsf(den)  < epsilon ) {
        LOGGER_WARN( log, "segments are parallel (den=" << den << "), using midpoints instead" );
        LOGGER_WARN( log, "  Line 1: [" << a0[0] << ", " << a0[1]<< ", " << a0[2] << "]");
        LOGGER_WARN( log, "  Line 2: [" << a1[0] << ", " << a1[1]<< ", " << a1[2] << "]");
        bridge.addVertex( 0.25f*( a0[0] + a1[0] + b0[0] + b1[0] ),
                          0.25f*( a0[1] + a1[1] + b0[1] + b1[1] ),
                          0.25f*( a0[2] + a1[2] + b0[2] + b1[2] ) );
    }
    else {
        const REAL r = 1.f/den;
        const REAL d = u[0]*w[0] + u[1]*w[1] + u[2]*w[2];              // dot(u,w)
        const REAL e = v[0]*w[0] + v[1]*w[1] + v[2]*w[2];              // dot(v,w)
        REAL s = r*(b*e-c*d);
        if( s < 0.f ) {
            LOGGER_WARN( log, "s=" << s << " is outside of [0,1]");
            s = 0.f;
        }
        else if( s > 1.f ) {
            LOGGER_WARN( log, "s=" << s << " is outside of [0,1]");
            s = 1.f;
        }
        REAL t = r*(a*e-b*d);
        if( t < 0.f ) {
            LOGGER_WARN( log, "t=" << t << " is outside of [0,1]");
            t = 0.f;
        }
        else if( t > 1.f ) {
            LOGGER_WARN( log, "t=" << t << " is outside of [0,1]");
            t = 1.f;
        }
        bridge.addVertex( 0.5f*( p[0] + s*u[0] + q[0] + t*v[0] ),
                          0.5f*( p[1] + s*u[1] + q[1] + t*v[1] ),
                          0.5f*( p[2] + s*u[2] + q[2] + t*v[2] ) );
    }
    return retval;
}

template<typename Bridge>
typename CornerPointTessellator<Bridge>::REAL
CornerPointTessellator<Bridge>::distancePointLineSquared( Bridge& bridge,
                                                          const unsigned int pp,
                                                          const unsigned int l0,
                                                          const unsigned int l1 )
{
    const REAL p[3] = { bridge.vertexX( pp ), bridge.vertexY( pp ), bridge.vertexZ( pp ) };
    const REAL a[3] = { bridge.vertexX( l0 ), bridge.vertexY( l0 ), bridge.vertexZ( l0 ) };
    const REAL b[3] = { bridge.vertexX( l1 ), bridge.vertexY( l1 ), bridge.vertexZ( l1 ) };
    const REAL ab[3] = { b[0]-a[0], b[1]-a[1], b[2]-a[2] };
    const REAL pa[3] = { a[0]-p[0], a[1]-p[1], a[2]-p[2] };
    const REAL ab_x_pb[3] = {
        ab[1]*pa[2] - ab[2]*pa[1],
        ab[2]*pa[0] - ab[0]*pa[2],
        ab[0]*pa[1] - ab[1]*pa[0]
    };
    const REAL den = ab[0]*ab[0] + ab[1]*ab[1] + ab[2]*ab[2];
    if( fabsf( den ) < epsilon ) {
        Logger log = getLogger( "CPTessFactory.distancePointLineSquared" );
        LOGGER_WARN( log, "Line is degenerate, using pa instead." );
        return pa[0]*pa[0] + pa[1]*pa[1] + pa[2]*pa[2];
    }
    const REAL r = 1.f/den;
    return r*(ab_x_pb[0]*ab_x_pb[0] + ab_x_pb[1]*ab_x_pb[1] + ab_x_pb[2]*ab_x_pb[2]);
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::findActiveCellsInColumn( unsigned int*       active_cell_list,
                                                         unsigned int&       active_cell_count,
                                                         const int* const    actnum,
                                                         const unsigned int  stride,
                                                         const unsigned int  nz )
{
    unsigned int n = 0;
    for( unsigned int k=0; k<nz; k++ ) {
        if( actnum[ stride*k ] != 0 ) {
            active_cell_list[n++] = k;
        }
    }
    active_cell_count = n;
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::pillarEdges( Bridge&                          bridge,
                            const unsigned int               offset,
                            const std::vector<unsigned int>& adjacent_cells,
                            const int*                       cell_map_00,
                            const int*                       cell_map_01,
                            const int*                       cell_map_10,
                            const int*                       cell_map_11,
                            const unsigned int               stride )
{
    for( size_t i=1; i<adjacent_cells.size()/4; i++) {
        const unsigned int* adj = adjacent_cells.data() + 4*i;
        if( (adj[0] != ~0u ) ||
            (adj[1] != ~0u ) ||
            (adj[2] != ~0u ) ||
            (adj[3] != ~0u ) )
        {
            const unsigned int cells[4] = { adj[0] != ~0u ? cell_map_00[ stride*adj[0] ] : ~0u,
                                            adj[1] != ~0u ? cell_map_01[ stride*adj[1] ] : ~0u,
                                            adj[2] != ~0u ? cell_map_10[ stride*adj[2] ] : ~0u,
                                            adj[3] != ~0u ? cell_map_11[ stride*adj[3] ] : ~0u
                                          };
            bridge.addEdge(offset + i - 1 , offset + i,
                           cells[0], cells[1], cells[2], cells[3] );
        }
    }
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::uniquePillarVertices( Bridge&                     bridge,
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
                                                      const unsigned int          active_cell_count_11 )
{
    static const float maxf = std::numeric_limits<REAL>::max();
    static const float minf = std::numeric_limits<REAL>::min();
    static const float epsf = std::numeric_limits<REAL>::epsilon();
    const float x1 = coord[ 0 ];
    const float y1 = coord[ 1 ];
    const float z1 = coord[ 2 ];
    const float x2 = coord[ 3 ];
    const float y2 = coord[ 4 ];
    const float z2 = coord[ 5 ];

    unsigned int* zcorn_ix[4] = { zcorn_ix_00, zcorn_ix_01, zcorn_ix_10, zcorn_ix_11 };
    const float* zcorn[4] = { zcorn_00, zcorn_01, zcorn_10, zcorn_11 };
    const unsigned int* active_cell_list[4] = { active_cell_list_00, active_cell_list_01, active_cell_list_10,  active_cell_list_11 };
    const unsigned int  active_cell_count[4] = { active_cell_count_00, active_cell_count_01, active_cell_count_10, active_cell_count_11 };

    adjacent_cells.clear();
    adjacent_cells.reserve( 2*active_cell_count_00 +
                            2*active_cell_count_01 +
                            2*active_cell_count_10 +
                            2*active_cell_count_11 );

    bool ascending  = false;
    bool descending = false;
    bool any        = false;
    for( unsigned int l=0; l<4; l++) {
        const unsigned int n = active_cell_count[l];
        if( n > 0u ) {
            const unsigned int k0 = active_cell_list[l][0];
            const unsigned int kn = active_cell_list[l][n-1u];
            const float z0 = zcorn[l][ 4*stride*(2*k0+0) ];
            const float zn = zcorn[l][ 4*stride*(2*kn+1) ];
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

    unsigned int cells_below[4] = { ~0u, ~0u, ~0u, ~0u };

    if( ascending ) {
        bool         first   = true;
        float        curr_z  = minf;
        unsigned int curr_ix = ~0;
        unsigned int i[4]    = { 0, 0, 0, 0 };
        while(1) {
            float smallest_z = maxf;
            unsigned int smallest_l = ~0u;
            unsigned int smallest_c = ~0u;
            for( unsigned int l=0; l<4; l++ ) {
                if( i[l] < 2*active_cell_count[l] ) {
                    const unsigned int ii = i[l]>>1;
                    const unsigned int ij = i[l]&1;

                    const unsigned int k = active_cell_list[l][ ii ];
                    const float z = zcorn[l][ 4*stride*(2*k + ij) ];
                    if( z < smallest_z ) {
                        smallest_z = z;
                        smallest_l = l;
                        smallest_c = k;
                    }
                }
            }

            if( 3 < smallest_l ) {
                break;
            }

            if( first || curr_z+epsf < smallest_z ) {
                // create new vertex
                curr_ix = bridge.vertexCount();
                curr_z = smallest_z;
                const float a = (smallest_z-z1)/(z2-z1);
                const float b = 1.f - a;
                bridge.addVertex( b*x1 + a*x2, b*y1 + a*y2, smallest_z );
                adjacent_cells.push_back( cells_below[0] );
                adjacent_cells.push_back( cells_below[1] );
                adjacent_cells.push_back( cells_below[2] );
                adjacent_cells.push_back( cells_below[3] );
                first = false;
            }
            const unsigned int ii = i[smallest_l]>>1;
            const unsigned int ij = i[smallest_l]&1;
            cells_below[ smallest_l ] = ij == 0 ? smallest_c : ~0u;
            zcorn_ix[ smallest_l ][ 2*ii + ij ] = curr_ix;

            i[smallest_l]++;
        }
    }
    else {
        bool         first   = true;
        float        curr_z  = maxf;
        unsigned int curr_ix = ~0;
        unsigned int i[4]    = { 0, 0, 0, 0 };
        while(1) {
            float largest_z = minf;
            unsigned int largest_l = ~0;
            for( unsigned int l=0; l<4; l++ ) {
                if( i[l] < 2*active_cell_count[l] ) {
                    const unsigned int ii = i[l]>>1;
                    const unsigned int ij = i[l]&1;

                    const unsigned int k = active_cell_list[l][ ii ];
                    const float z = zcorn[l][ 4*stride*(2*k + ij) ];
                    if( largest_z < z ) {
                        largest_z = z;
                        largest_l = l;
                    }
                }
            }

            if( 3 < largest_l ) {
                break;
            }
            if( first || largest_z < curr_z-epsf  ) {
                // create new vertex
                first = false;
                curr_ix = bridge.vertexCount();
                curr_z = largest_z;
                const float a = (largest_z-z1)/(z2-z1);
                const float b = 1.f - a;
                bridge.addVertex( b*x1 + a*x2, b*y1 + a*y2, largest_z );
            }
            const unsigned int ii = i[largest_l]>>1;
            const unsigned int ij = i[largest_l]&1;
            zcorn_ix[ largest_l ][ 2*ii + ij ] = curr_ix;
            i[largest_l]++;
        }
    }
}

template<typename Bridge>
void
CornerPointTessellator<Bridge>::stitchTopBottom( Bridge& bridge,
                                                 const unsigned int* const   ci0j0_active_cell_list,
                                                 const unsigned int          ci0j0_active_cell_count,
                                                 const unsigned int* const   o01_d10_wall_line_ix,
                                                 const unsigned int* const   o00_d10_wall_line_ix,
                                                 const unsigned int* const   o10_d01_wall_line_ix,
                                                 const unsigned int* const   o00_d01_wall_line_ix,
                                                 const unsigned int* const   o00_zcorn_ix,
                                                 const unsigned int* const   o10_zcorn_ix,
                                                 const unsigned int* const   o01_zcorn_ix,
                                                 const unsigned int* const   o11_zcorn_ix,
                                                 const unsigned int* const   o01_d10_chains,
                                                 const unsigned int* const   chain_offsets_o01_d10,
                                                 const unsigned int* const   o00_d10_chains,
                                                 const unsigned int* const   o00_d10_chain_offsets,
                                                 const unsigned int* const   o10_d01_chains,
                                                 const unsigned int* const   chain_offsets_o10_d01,
                                                 const unsigned int* const   o00_d01_chains,
                                                 const unsigned int* const   o00_d01_chain_offsets,
                                                 const int* const            cell_map,
                                                 const unsigned int          stride )
{
    Logger log = getLogger( "CPTessFactory.stitchTopBottom" );
    struct Helper {
        unsigned int m_index;
        unsigned int m_cell_below;
        unsigned int m_cell_above;
    };

    std::vector<Helper> helper;
    helper.reserve( 2*ci0j0_active_cell_count );

    unsigned int last_top_00 = ~0u;
    unsigned int last_top_01 = ~0u;
    unsigned int last_top_10 = ~0u;
    unsigned int last_top_11 = ~0u;

    for( unsigned int m=0; m<ci0j0_active_cell_count; m++ ) {
        const unsigned int cell_current = ci0j0_active_cell_list[m];
        const unsigned int global_cell_index = cell_map[ stride*cell_current ];

        const unsigned int i000 = o00_zcorn_ix[2*m + 0];
        const unsigned int i100 = o10_zcorn_ix[2*m + 0];
        const unsigned int i010 = o01_zcorn_ix[2*m + 0];
        const unsigned int i110 = o11_zcorn_ix[2*m + 0];
        const unsigned int i001 = o00_zcorn_ix[2*m + 1];
        const unsigned int i101 = o10_zcorn_ix[2*m + 1];
        const unsigned int i011 = o01_zcorn_ix[2*m + 1];
        const unsigned int i111 = o11_zcorn_ix[2*m + 1];

        bool degenerate =
                (i000 == i001 ) &&
                (i010 == i011 ) &&
                (i100 == i101 ) &&
                (i110 == i111 );
        if( degenerate ) {
            continue;
        }

        bool attached =
                (last_top_00 == i000) &&
                (last_top_10 == i100) &&
                (last_top_01 == i010) &&
                (last_top_11 == i110);

        // Bottom
        if( helper.empty() || !attached ) {
            helper.resize( helper.size() + 1 );
            helper.back().m_index = 2*m+0;
            helper.back().m_cell_below = ~0u;
            helper.back().m_cell_above = global_cell_index;
        }
        else {
            helper.back().m_cell_above = global_cell_index;
        }

        // Top
        helper.resize( helper.size() + 1 );
        helper.back().m_index = 2*m+1;
        helper.back().m_cell_below = global_cell_index;
        helper.back().m_cell_above = ~0u;

        last_top_00 = i001;
        last_top_10 = i101;
        last_top_01 = i011;
        last_top_11 = i111;
    }


    size_t skip = 0;//std::max( 1ul, helper.size() ) - 1u;
    for( auto it=helper.begin()+skip; it!=helper.end(); ++it ) {
        const unsigned int cell_boundary_ix = it->m_index;
        const unsigned int i00 = o00_zcorn_ix[ cell_boundary_ix ];
        const unsigned int i10 = o10_zcorn_ix[ cell_boundary_ix ];
        const unsigned int i01 = o01_zcorn_ix[ cell_boundary_ix ];
        const unsigned int i11 = o11_zcorn_ix[cell_boundary_ix];
        // triangulate 00, 01, 10
        const unsigned int o00_d10_line_ix = o00_d10_wall_line_ix[ cell_boundary_ix ];
        const unsigned int o00_d01_line_ix = o00_d01_wall_line_ix[ cell_boundary_ix ];
        unsigned int base_d10 = i10;
        unsigned int base_d01 = i01;
        unsigned int curr_d10_0 = o00_d10_chain_offsets[ o00_d10_line_ix ];
        unsigned int curr_d10_1 = o00_d10_chain_offsets[ o00_d10_line_ix + 1 ];
        unsigned int curr_d01_0 = o00_d01_chain_offsets[ o00_d01_line_ix ];
        unsigned int curr_d01_1 = o00_d01_chain_offsets[ o00_d01_line_ix + 1 ];
        while( (curr_d10_0 != curr_d10_1) || (curr_d01_0 != curr_d01_1 ) ) {
            LOGGER_INVARIANT_LESS_EQUAL( log, curr_d10_0, curr_d10_1 );
            LOGGER_INVARIANT_LESS_EQUAL( log, curr_d01_0, curr_d01_1 );
            if( (curr_d10_1-curr_d10_0) > (curr_d01_1-curr_d01_0) ) {
                curr_d10_1 = curr_d10_1 - 1u;
                const unsigned int vertex_ix = o00_d10_chains[ curr_d10_1 ];
                bridge.addTriangle( Bridge::ORIENTATION_K, it->m_cell_below, it->m_cell_above, base_d10, vertex_ix, base_d01 );
                base_d10 = vertex_ix;
            }
            else {
                curr_d01_1 = curr_d01_1 - 1u;
                const unsigned int vertex_ix = o00_d01_chains[ curr_d01_1 ];
                bridge.addTriangle( Bridge::ORIENTATION_K, it->m_cell_below, it->m_cell_above, vertex_ix, base_d01, base_d10 );
                base_d01 = vertex_ix;
            }
        }
        bridge.addTriangle( Bridge::ORIENTATION_K, it->m_cell_below, it->m_cell_above, base_d10, i00, base_d01 );
        // triangulate 11, 10, 01
        const unsigned int wall_line_ix_o01_d10 = o01_d10_wall_line_ix[ cell_boundary_ix ];
        const unsigned int wall_line_ix_o10_d01 = o10_d01_wall_line_ix[ cell_boundary_ix ];
        base_d10 = i01;
        base_d01 = i10;
        curr_d10_0 = chain_offsets_o01_d10[ wall_line_ix_o01_d10 ];
        curr_d10_1 = chain_offsets_o01_d10[ wall_line_ix_o01_d10 + 1 ];
        curr_d01_0 = chain_offsets_o10_d01[ wall_line_ix_o10_d01 ];
        curr_d01_1 = chain_offsets_o10_d01[ wall_line_ix_o10_d01 + 1 ];
        while( (curr_d10_0 != curr_d10_1) || (curr_d01_0 != curr_d01_1 ) ) {
            LOGGER_INVARIANT_LESS_EQUAL( log, curr_d10_0, curr_d10_1 );
            LOGGER_INVARIANT_LESS_EQUAL( log, curr_d01_0, curr_d01_1 );
            if( (curr_d10_1-curr_d10_0) > (curr_d01_1-curr_d01_0) ) {
                const unsigned int vertex_ix = o01_d10_chains[ curr_d10_0 ];
                bridge.addTriangle( Bridge::ORIENTATION_K, it->m_cell_below, it->m_cell_above, base_d10, vertex_ix, base_d01 );
                base_d10 = vertex_ix;
                curr_d10_0 = curr_d10_0 + 1u;
            }
            else {
                const unsigned int vertex_ix = o10_d01_chains[ curr_d01_0 ];
                bridge.addTriangle( Bridge::ORIENTATION_K, it->m_cell_below, it->m_cell_above, vertex_ix, base_d01, base_d10 );
                base_d01 = vertex_ix;
                curr_d01_0 = curr_d01_0 + 1u;
            }
        }
        bridge.addTriangle( Bridge::ORIENTATION_K, it->m_cell_below, it->m_cell_above, base_d10, i11, base_d01 );
    }
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::wallEdges( Bridge&                           bridge,
                                           const std::vector<WallLine>&      boundaries,
                                           const std::vector<Intersection>&  intersections,
                                           const std::vector<unsigned int>&  chains,
                                           const unsigned int* const         chain_offsets )
{
    for( size_t b=0; b<boundaries.size(); b++ ) {
        const WallLine& wl = boundaries[b];
        unsigned int cell_over[2];
        unsigned int cell_under[2];
        cell_over[0]  = wl.m_cell_over[0];
        cell_over[1]  = wl.m_cell_over[1];
        cell_under[0] = wl.m_cell_under[0];
        cell_under[1] = wl.m_cell_under[1];
        unsigned other_side = wl.m_side==1 ? 0 : 1;
        unsigned int end0 = wl.m_ends[0];
        for( size_t c=chain_offsets[b]; c!=chain_offsets[b+1]; c++ ) {
            const Intersection& i = intersections[ chains[c] ];
            const WallLine& ol = boundaries[ i.m_upwrd_bndry_ix == b ? i.m_upwrd_bndry_ix : i.m_dnwrd_bndry_ix ];
            cell_over[ other_side ]  = ol.m_cell_over[ other_side ];
            cell_under[ other_side ] = ol.m_cell_under[ other_side ];

            unsigned int end1 = i.m_vtx_ix;
            bridge.addEdge( end0, end1, cell_over[0], cell_under[0], cell_over[1], cell_under[1] );
            end0 = end1;
        }
        unsigned int end1 = wl.m_ends[1];
        bridge.addEdge( end0, end1, cell_over[0], cell_under[0], cell_over[1], cell_under[1] );
    }
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::stitchPillarsHandleIntersections( Bridge&                             bridge,
                                                                  const typename Bridge::Orientation  orientation,
                                                                  const std::vector<WallLine>&        boundaries,
                                                                  const std::vector<Intersection>&    intersections,
                                                                  const std::vector<unsigned int>&    chains,
                                                                  const unsigned int* const           chain_offsets )
{
    Logger log = getLogger( "CPTessFactory.stitchPillarsHandleIntersections" );

    // triangulate up to first intersections and extract front
    std::vector<unsigned int> front;
    front.reserve( intersections.size() + boundaries.size() );
    unsigned int ix_p0 = 0u;
    unsigned int ix_p1 = 0u;
    bool is_isec_p1 = false;

    for(size_t b=0; b<boundaries.size(); b++) {
        const unsigned int ix_c0 = boundaries[b].m_ends[0];     // index at pillar 0
        unsigned int ix_c1, id_c1;                              // index and id at pillar 1
        bool is_isec_c1;
        if( chain_offsets[b] == chain_offsets[b+1] ) {          // no intersections
            ix_c1 = boundaries[b].m_ends[1];
            id_c1 = end_flag | ix_c1;
            is_isec_c1 = false;
        }
        else {
            id_c1 = chains[ chain_offsets[b] ];
            ix_c1 = intersections[id_c1].m_vtx_ix;
            is_isec_c1 = true;
        }
        const unsigned ccu_a = boundaries[b].m_cell_under[0];  // cells in this span
        const unsigned ccu_b = boundaries[b].m_cell_under[1];
        const bool pinch_0 = (ix_c0 == ix_p0);
        if( b > 0 && (ccu_a != ~0u || ccu_b != ~0u) ) {       // add first triangle/quad
            if( !pinch_0 ) {
                bridge.addTriangle( orientation, ccu_b, ccu_a, ix_p1, ix_c0, ix_p0 );
            }

            if( ix_p1 != ix_c1 ) {
                bridge.addTriangle( orientation, ccu_b, ccu_a, ix_c0, ix_p1, ix_c1 );
            }
        }
        if( front.empty() || front.back() != id_c1 ) {          // add to front
            front.push_back( id_c1 );
        }
        ix_p0 = ix_c0;
        ix_p1 = ix_c1;
        is_isec_p1 = is_isec_c1;
    }


    // move front forward step by step
    std::vector<unsigned int> new_front;
    new_front.reserve( intersections.size() + boundaries.size() );
    for( unsigned int iteration = 0; iteration < 1000; iteration++ ) {
        // search for intersection closest to pillar 0
        unsigned int k = ~0u;
        float d = std::numeric_limits<float>::max();
        for( unsigned int i=0; i<front.size(); i++ ) {
            if( (end_flag&front[i]) == 0 ) {
                if( intersections[ front[i] ].m_distance < d ) {
                    k = i;
                    d = intersections[ front[i] ].m_distance;
                }
            }
        }
        new_front.clear();
        if( k != ~0u ) {
            // expand at front item k
            for( unsigned int i=0; i<front.size(); i++ ) {

                // triangulate triangle below split if necessary
                if( i+1 == k ) {

                    const Intersection& isec = intersections[ front[i+1] ];

                    unsigned int id_a = front[i];
                    unsigned int id_b = front[i+1];
                    unsigned int id_c = isec.m_nxt_dnwrd_isec_ix;
                    if( id_a != id_b && id_b != id_c && id_c != id_a ) {
                        // determine if we should produce a boundary line
                        const bool is_isec_a = (id_a&end_flag)==0;
                        const bool is_isec_c = (id_c&end_flag)==0;
                        // determine vertex indices of the triangle
                        unsigned int ix_a = is_isec_a ? intersections[ id_a ].m_vtx_ix : (id_a&end_mask);
                        unsigned int ix_b = intersections[ id_b ].m_vtx_ix;
                        unsigned int ix_c = is_isec_c ? intersections[ id_c ].m_vtx_ix : (id_c&end_mask);
                        // determine which cells that face this triangle
                        LOGGER_INVARIANT( log, isec.m_dnwrd_bndry_ix < boundaries.size() );
                        LOGGER_INVARIANT( log, isec.m_upwrd_bndry_ix < boundaries.size() );
                        unsigned int side1 = boundaries[ isec.m_upwrd_bndry_ix ].m_side - 1u;
                        unsigned int side2 = boundaries[ isec.m_dnwrd_bndry_ix ].m_side - 1u;
                        LOGGER_INVARIANT( log, side1 < 2 );
                        LOGGER_INVARIANT( log, side2 < 2 );
                        LOGGER_INVARIANT( log, side1 != side2 );
                        unsigned int cells_curr[2];
                        cells_curr[ side1 ] = boundaries[ isec.m_upwrd_bndry_ix ].m_cell_under[ side1 ];
                        cells_curr[ side2 ] = boundaries[ isec.m_dnwrd_bndry_ix ].m_cell_under[ side2 ];
                        bridge.addTriangle( orientation, cells_curr[1], cells_curr[0], ix_a, ix_c, ix_b );
                    }
                    new_front.push_back( front[i] );
                }
                // split intersection and triangulate
                else if( i == k ) {
                    const Intersection& isec = intersections[ front[i] ];

                    // split from intersection into two items
                    unsigned int nxt_dnwrd_id = isec.m_nxt_dnwrd_isec_ix;
                    unsigned int nxt_upwrd_id = isec.m_nxt_upwrd_isec_ix;
                    if( new_front.empty() || new_front.back() != nxt_dnwrd_id ) {
                        new_front.push_back( nxt_dnwrd_id );
                    }
                    if( i+1 == front.size() || front[i+1] != nxt_upwrd_id ) {
                        new_front.push_back( nxt_upwrd_id );
                    }
                    // determine if we should produce a boundary line
                    const bool is_isec_b = (nxt_dnwrd_id&end_flag) == 0;
                    const bool is_isec_c = (nxt_upwrd_id&end_flag) == 0;
                    // determine vertex indices of the triangle
                    unsigned int ix_a = isec.m_vtx_ix;
                    unsigned int ix_b = is_isec_b ? intersections[ nxt_dnwrd_id ].m_vtx_ix : (nxt_dnwrd_id & end_mask);
                    unsigned int ix_c = is_isec_c ? intersections[ nxt_upwrd_id ].m_vtx_ix : (nxt_upwrd_id & end_mask);
                    // determine which cells that face this triangle
                    unsigned int side1 = boundaries[ isec.m_upwrd_bndry_ix ].m_side - 1u;
                    unsigned int side2 = boundaries[ isec.m_dnwrd_bndry_ix ].m_side - 1u;
                    LOGGER_INVARIANT( log, side1 < 2 );
                    LOGGER_INVARIANT( log, side2 < 2 );
                    LOGGER_INVARIANT( log, side1 != side2 );
                    unsigned int cells[2];
                    cells[ side1 ] = boundaries[ isec.m_upwrd_bndry_ix ].m_cell_under[ side1 ];
                    cells[ side2 ] = boundaries[ isec.m_dnwrd_bndry_ix ].m_cell_over[ side2 ];
                    bool edges_above[2];
                    edges_above[ side1 ] = boundaries[ isec.m_upwrd_bndry_ix ].m_cell_under[ side1 ]
                                        != boundaries[ isec.m_upwrd_bndry_ix ].m_cell_over[ side1 ];
                    edges_above[ side2 ] = false;
                    bool edges_below[2];
                    edges_below[ side2 ] = boundaries[ isec.m_dnwrd_bndry_ix ].m_cell_under[ side2 ]
                                        != boundaries[ isec.m_dnwrd_bndry_ix ].m_cell_over[ side2 ];
                    edges_below[ side1 ] = false;

                    bridge.addTriangle( orientation, cells[1], cells[0], ix_b, ix_c, ix_a );
                }
                // triangulate triangle above if necessary
                else if( i == k+1 ) {
                    const Intersection& isec = intersections[ front[i-1] ];

                    unsigned int id_a = front[i-1];
                    unsigned int id_b = isec.m_nxt_upwrd_isec_ix;
                    unsigned int id_c = front[i];
                    if( id_a != id_b && id_b != id_c && id_c != id_a ) {
                        // determine if we should produce a boundary line
                        const bool is_isec_b = (id_b&end_flag) == 0;
                        const bool is_isec_c = (id_c&end_flag) == 0;
                        // determine vertex indices of the triangle
                        unsigned int ix_a = isec.m_vtx_ix;
                        unsigned int ix_b = is_isec_b ? intersections[ id_b ].m_vtx_ix : (id_b&end_mask);
                        unsigned int ix_c = is_isec_c ? intersections[ id_c ].m_vtx_ix : (id_c&end_mask);
                        // determine which cells that face this triangle
                        unsigned int side1 = boundaries[ isec.m_upwrd_bndry_ix ].m_side - 1u;
                        unsigned int side2 = boundaries[ isec.m_dnwrd_bndry_ix ].m_side - 1u;
                        LOGGER_INVARIANT( log, side1 < 2 );
                        LOGGER_INVARIANT( log, side2 < 2 );
                        LOGGER_INVARIANT( log, side1 != side2 );
                        unsigned int cells[2];
                        cells[ side1 ] = boundaries[ isec.m_upwrd_bndry_ix ].m_cell_over[ side1 ];
                        cells[ side2 ] = boundaries[ isec.m_dnwrd_bndry_ix ].m_cell_over[ side2 ];

                        bridge.addTriangle( orientation, cells[1], cells[0], ix_b, ix_c, ix_a );
                    }
                    new_front.push_back( front[i] );
                }
                else {
                    new_front.push_back( front[i] );
                }

            }
        }
        else {
            break;
        }
        front.swap( new_front );
    }
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::stitchPillarsNoIntersections( Bridge&                             bridge,
                                                              const typename Bridge::Orientation  orientation,
                                                              const std::vector<WallLine>&        boundaries )
{
    unsigned int ix_p0 = 0u;
    unsigned int ix_p1 = 0u;
    for(size_t b=0; b<boundaries.size(); b++) {
        const unsigned int ix_c0 = boundaries[b].m_ends[0];     // index at pillar 0
        unsigned int ix_c1;                                  // index and id at pillar 1
        //            cco
        //
        //   ix_c0 +---b---+ ix_c1
        //         |       |
        //  pich0? |  ccu  | pinch1?
        //         |       |
        //   ix_p0 +-------+ ix_p1
        //
        //            pcu
        ix_c1 = boundaries[b].m_ends[1];
        const unsigned ccu_a = boundaries[b].m_cell_under[0];  // cells in this span
        const unsigned ccu_b = boundaries[b].m_cell_under[1];

        // check if we should have edges above or below
        const bool pinch_0 = (ix_c0 == ix_p0);
        const bool pinch_1 = (ix_c1 == ix_p1);
        if( b > 0 ) {       // add first triangle/quad
            if( !pinch_0 ) {
                bridge.addTriangle( orientation, ccu_b, ccu_a, ix_p1, ix_c0, ix_p0 );
            }
            if( !pinch_1 ) {
                bridge.addTriangle( orientation, ccu_b, ccu_a, ix_c0, ix_p1, ix_c1 );
            }
        }
        ix_p0 = ix_c0;
        ix_p1 = ix_c1;
    }
}


template<typename Bridge>
void
CornerPointTessellator<Bridge>::extractWallLines( std::vector<WallLine>&      wall_lines,
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
                                                  const unsigned int          stride )
{
    // Helper struct for wall lines on one side of the wall
    struct SidedWallLine {
        unsigned int m_ends[2];
        unsigned int m_cell_under;
        unsigned int m_cell_over;
        unsigned int m_maps_to_merged_ix;
    };
    Logger log = getLogger( "CPTessFactory.extractBoundaryLinesOnWalls" );

    // Convert to arrays so we can use loops.
    const unsigned int* zcorn_ix[4] = {
        zcorn_ix_a_0,
        zcorn_ix_a_1,
        zcorn_ix_b_0,
        zcorn_ix_b_1
    };
    const unsigned int* active_cell_list[2] = {
        active_cell_list_a,
        active_cell_list_b
    };
    const unsigned int active_cell_count[2] = {
        active_cell_count_a,
        active_cell_count_b
    };
    const int* cell_map[2] = {
        cell_map_a,
        cell_map_b
    };
    unsigned int* boundary_line_index[2] = {
        boundary_line_index_a,
        boundary_line_index_b,
    };



    // Step 1: Extract all wall lines on each side of the wall.
    std::vector<SidedWallLine> sided_wall_lines[2];
    for( unsigned int side=0; side<2; side++) {
        std::vector<SidedWallLine>& current_lines = sided_wall_lines[side];

        current_lines.reserve( 2*active_cell_count[ side ] );

        for(unsigned int m=0; m<active_cell_count[ side ]; m++ ) {
            const unsigned int b0 = zcorn_ix[2*side+0][ 2*m + 0 ];
            const unsigned int b1 = zcorn_ix[2*side+1][ 2*m + 0 ];
            const unsigned int t0 = zcorn_ix[2*side+0][ 2*m + 1 ];
            const unsigned int t1 = zcorn_ix[2*side+1][ 2*m + 1 ];
            LOGGER_INVARIANT( log, b0 != ~ 0u );
            LOGGER_INVARIANT( log, b1 != ~ 0u );
            LOGGER_INVARIANT( log, t0 != ~ 0u );
            LOGGER_INVARIANT( log, t1 != ~ 0u );

            const int k = active_cell_list[side][ m ];
            const int cell_ix = cell_map[side][ stride*k ];

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
                    current_line.m_cell_under = ~0;
                    current_line.m_cell_over  = ~0;
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
                    top_line.m_ends[0] = b0;
                    top_line.m_ends[1] = b1;
                    top_line.m_cell_under = ~0;
                    top_line.m_cell_over = cell_ix;
                }
                else {
                    // Edge matched previous edge, recycle edge.
                    SidedWallLine& bottom_line = current_lines.back();
                    bottom_line.m_cell_over = cell_ix;
                }
                boundary_line_index[side][2*m+0] = current_lines.size()-1u;

                // Top edge, always created
                current_lines.resize( current_lines.size() + 1 );
                SidedWallLine& top_line = current_lines.back();
                top_line.m_ends[0] = t0;
                top_line.m_ends[1] = t1;
                top_line.m_cell_under = cell_ix;
                top_line.m_cell_over = ~0;
                boundary_line_index[side][2*m+1] = current_lines.size()-1u;
            }
        }
    }


    // Step 2: Merge the lines from the two sides.
    wall_lines.clear();
    wall_lines.reserve( sided_wall_lines[0].size() + sided_wall_lines[1].size() );
    unsigned int i[2] = {0u, 0u };
    unsigned int cells_below[2] = { ~0u, ~0u };
    while( 1 ) {

        // check the current edge on each side to find the lexicographically smallest
        unsigned int smallest_p0   = ~0u;
        unsigned int smallest_p1   = ~0u;
        unsigned int overall_smallest_p1      = ~0u;   // the p1 of the largest edge
        unsigned int smallest_side = ~0u;
        for(unsigned int side=0; side<2; side++) {
            const unsigned int ii = i[side];
            const std::vector<SidedWallLine>& swls = sided_wall_lines[side];
            if( ii < swls.size() ) {
                const unsigned int p0 = swls[ii].m_ends[0];
                const unsigned int p1 = swls[ii].m_ends[1];
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
            LOGGER_INVARIANT( log, smallest_p0 == ~0u );
            LOGGER_INVARIANT( log, smallest_p1 == ~0u );
            break;
        }
        SidedWallLine& sl = sided_wall_lines[ smallest_side ][ i[smallest_side] ];


        unsigned int cells_above[2] = { cells_below[0], cells_below[1] };
        cells_above[ smallest_side ] = sl.m_cell_over;

        LOGGER_INVARIANT( log, cells_below[ smallest_side ] == sl.m_cell_under );


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
        }
        else {
            WallLine& l = wall_lines.back();
            l.m_side = l.m_side ^ (smallest_side + 1);
            //l.m_cutoff = smallest_p1;
            l.m_cell_over[0] = cells_above[0];
            l.m_cell_over[1] = cells_above[1];
            LOGGER_INVARIANT( log, l.m_side == 3 );
        }
        sl.m_maps_to_merged_ix = wall_lines.size()-1;

        i[ smallest_side ]++;
        cells_below[0] = cells_above[0];
        cells_below[1] = cells_above[1];
    }

#ifdef CHECK_INVARIANTS
    for( unsigned int side=0; side<2; side++) {
        for( unsigned int m=0; m<active_cell_count[side]; m++ ) {
            for(unsigned int i=0; i<2; i++ ) {
                SidedWallLine& sl = sided_wall_lines[side][ boundary_line_index[side][2*m+i] ];
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+0][2*m+i], sl.m_ends[0] );
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+1][2*m+i], sl.m_ends[1] );
            }
        }
    }
#endif
    // update sided indices to merged indices
    for( unsigned int side=0; side<2; side++ ) {
        for( unsigned int m = 0; m<2*active_cell_count[side]; m++ ) {
            LOGGER_INVARIANT( log, boundary_line_index[side][m] < sided_wall_lines[side].size() );
            const unsigned int sided_ix = boundary_line_index[ side ][ m ];
            const unsigned int merged_ix = sided_wall_lines[ side ][ sided_ix ].m_maps_to_merged_ix;
            boundary_line_index[ side ][ m ] = merged_ix;
        }
    }

#ifdef CHECK_INVARIANTS
    for( unsigned int side=0; side<2; side++) {
        for( unsigned int m=0; m<active_cell_count[side]; m++ ) {
            for(unsigned int i=0; i<2; i++ ) {
                WallLine& l = wall_lines[ boundary_line_index[side][ 2*m+i ] ];
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+0][2*m+i], l.m_ends[0] );
                LOGGER_INVARIANT_EQUAL( log, zcorn_ix[2*side+1][2*m+i], l.m_ends[1] );
            }
        }
    }
    for(unsigned int i=1; i<wall_lines.size(); i++ ) {
        LOGGER_INVARIANT_EQUAL( log, wall_lines[i-1].m_cell_over[0], wall_lines[i].m_cell_under[0] );
        LOGGER_INVARIANT_EQUAL( log, wall_lines[i-1].m_cell_over[1], wall_lines[i].m_cell_under[1] );
    }
#endif
}

template<typename Bridge>
void
CornerPointTessellator<Bridge>::intersectWallLines( Bridge&                       bridge,
                                                    std::vector<Intersection>&    wall_line_intersections,
                                                    std::vector<unsigned int>&    chains,
                                                    unsigned int*                 chain_offsets,
                                                    const std::vector<WallLine>&  wall_lines )
{
    Logger log = getLogger( "CPTessFactory.intersectWallLines" );
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
    const unsigned int isec_base_offset = wall_line_intersections.size();
#endif

    std::vector< std::list<unsigned int> > chains_tmp( wall_lines.size() );
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
                const unsigned int is_ix = wall_line_intersections.size();
                const unsigned int vtx_ix = segmentIntersection( bridge,
                                                                 lowest_at_p0.m_ends[0], lowest_at_p0.m_ends[1],
                                                                 larger_at_p0.m_ends[0], larger_at_p0.m_ends[1] );

                chains_tmp[lower].push_back( is_ix );
                chains_tmp[upper].push_front( is_ix );

                wall_line_intersections.resize( wall_line_intersections.size()+1 );
                Intersection& i = wall_line_intersections.back();
                i.m_vtx_ix = vtx_ix;
                i.m_upwrd_bndry_ix = lower;
                i.m_dnwrd_bndry_ix = upper;
                i.m_distance = distancePointLineSquared( bridge,
                                                         vtx_ix,
                                                         wall_lines.front().m_ends[0],
                                                         wall_lines.back().m_ends[0] );
                i.m_nxt_dnwrd_isec_ix = ~0u;
                i.m_nxt_upwrd_isec_ix = ~0u;
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

        LOGGER_INVARIANT( log, chain_offsets[b_ix] == ~0u );
        chain_offsets[b_ix] = chains.size();

        const std::list<unsigned int>& chain = chains_tmp[b_ix];
        if( !chain.empty() ) {

            for( auto it=chain.begin(); it!=chain.end(); ++it ) {
                chains.push_back( *it );
            }

            auto p = chain.begin();
            auto c = p;
            for( c++; c != chain.end(); c++ ) {
                if( b_ix == wall_line_intersections[*p].m_dnwrd_bndry_ix ) {
                    wall_line_intersections[ *p ].m_nxt_dnwrd_isec_ix = *c;
                }
                else if( b_ix == wall_line_intersections[*p].m_upwrd_bndry_ix ) {
                    wall_line_intersections[ *p ].m_nxt_upwrd_isec_ix = *c;
                }
                p = c;
            }
            if( b_ix == wall_line_intersections[*p].m_dnwrd_bndry_ix ) {
                wall_line_intersections[ *p ].m_nxt_dnwrd_isec_ix = end_flag | wall_lines[b_ix].m_ends[1];
            }
            else if( b_ix == wall_line_intersections[*p].m_upwrd_bndry_ix ) {
                wall_line_intersections[ *p ].m_nxt_upwrd_isec_ix = end_flag | wall_lines[b_ix].m_ends[1];
            }
        }
    }
    chain_offsets[ wall_lines.size() ] = chains.size();

#ifdef CHECK_INVARIANTS
    for( unsigned int l = 0; l < wall_lines.size(); l++ ) {
        const unsigned int a = chain_offsets[l];
        const unsigned int b = chain_offsets[l+1];
        for( unsigned int c=a; c<b; c++) {
            const unsigned int i = chains[c];
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


template class CornerPointTessellator<GridTessBridge>;
