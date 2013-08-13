/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2009 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

bool
CPTessFactory::triangulate( std::vector<unsigned int>&  debug_edges0,
                            std::vector<unsigned int>&  debug_edges1,
                            std::vector<float>&         vertices,
                            std::vector<unsigned int>&  edge_info,
                            std::vector<unsigned int>&  edges,
                            std::vector<unsigned int>&  triangle_info,
                            std::vector<unsigned int>&  triangles,
                            std::vector<unsigned int>&  cell_index,
                            std::vector<unsigned int>&  cell_corner,
                            const std::vector<float>&   coord,
                            const std::vector<float>&   zcorn,
                            const std::vector<int>&     actnum,
                            const std::vector<int>&     cell_map,
                            const unsigned int          nx,
                            const unsigned int          ny,
                            const unsigned int          nz )
{


    Logger log = getLogger( "CPTessFactory.triangulate" );

    static const unsigned int chain_offset_stride = 4*nz+1;

    // Chains along j-direction, i.e., from p(i,j-1) to p(i,j); nx+1 entries
    std::vector<unsigned int> pi0jm1_d01_chains;
    std::vector<unsigned int> pi0jm1_d01_chain_offsets( (nx+1)*chain_offset_stride, ~0u );

    // Chains along i-direction, i.e., from p(i-1,j) to p(i,j); nx entries.
    std::vector<unsigned int> pi0jm1_d10_chains;
    std::vector<unsigned int> pi0jm1_d10_chain_offsets( nx*chain_offset_stride, ~0u );
    std::vector<unsigned int> pi0j0_d10_chains;
    std::vector<unsigned int> pi0j0_d10_chain_offsets( nx*chain_offset_stride, ~0u );

    std::vector<unsigned int> jm1_active_cell_list( (nx+2)*(nz) );
    std::vector<unsigned int> jm1_active_cell_count( (nx+2), 0u );
    std::vector<unsigned int> jm0_active_cell_list( (nx+2)*(nz) );
    std::vector<unsigned int> jm0_active_cell_count( (nx+2), 0u );

    // These are of size [nx][4][2*nz]
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


        // Iterate over all i's.
        jm0_active_cell_count[0] = 0u;
        for( unsigned int i=0; i<nx+1; i++ ) {

            // Find active cells in column
            if( i < nx && j < ny ) {
                if(!findActiveCellsInColumn( jm0_active_cell_list.data() + nz*(i+1),
                                             jm0_active_cell_count[i+1],
                                             actnum.data() + i + nx*j,
                                             nx*ny,
                                             nz ) )
                {
                    LOGGER_FATAL( log, "Failed to determine active cells in column c(" << i << ", " << j << ")" );
                    return NULL;
                }
            }
            else {
                jm0_active_cell_count[ i+1 ] = 0u;
            }

            const unsigned int vertex_pillar_start = vertices.size()/4;
            std::vector<unsigned int> adjacent_cells;
            if( !uniquePillarVertices( vertices,
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
                                       jm0_active_cell_count[ i+1 ] ) )
            {
                LOGGER_FATAL( log, "Failed to determine unique vertices on pillar p(" << i << ", " << j << ")" );
                return NULL;
            }

            pillarEdges( edge_info,
                         edges,
                         vertex_pillar_start,
                         adjacent_cells,
                         cell_map.data() + (i-1) + nx*(j-1),
                         cell_map.data() + (i  ) + nx*(j-1),
                         cell_map.data() + (i-1) + nx*(j  ),
                         cell_map.data() + (i  ) + nx*(j  ),
                         nx*ny );

            if( 0 < j ) {
                // Stitch pillar wall between pillars p(i,j-1) and p(i,j), i.e. along j
                //
                //            ----- p(i,j-1) ----> i
                //                     |
                //       c(i-1,j-1)    |   c(i,j-1)
                //                     |
                //            ----- p(i,j  ) ----
                //                     |
                //                     V j
                //
                std::vector<WallLine> wall_lines;
                std::vector<Intersection> wall_line_intersections;
                if( !extractWallLines( wall_lines,
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
                                       nx*ny ) ) {
                    LOGGER_FATAL( log, "Failed to extract lines on wall between pillars p(" << i << ", " << (j-1) << ") and p(" << i << ", " << j << ")");
                    return NULL;
                }
                if( !intersectWallLines( vertices,
                                         wall_line_intersections,
                                         pi0jm1_d01_chains,
                                         pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i,
                                         wall_lines ) ) {
                    LOGGER_FATAL( log, "Failed to intersect wall lines between pillars  p(" << i << ", " << (j-1) << ") and p(" << i << ", " << j << ")");
                    return NULL;
                }
#if 1
                bool success;
                success = wallEdges( edge_info,
                                     edges,
                                     wall_lines,
                                     wall_line_intersections,
                                     pi0jm1_d01_chains,
                                     pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i );

                if( wall_line_intersections.empty() ) {
                    // no intersections, we can use a simpler stitching scheme
                    success = stitchPillarsNoIntersections( edge_info,
                                                            edges,
                                                            triangle_info,
                                                            triangles,
                                                            wall_lines );
                }
                else {
                     success = stitchPillarsHandleIntersections( triangle_info,
                                                                triangles,
                                                                wall_lines,
                                                                wall_line_intersections,
                                                                pi0jm1_d01_chains,
                                                                pi0jm1_d01_chain_offsets.data() + chain_offset_stride*i );
                }
                if( !success ) {
                    LOGGER_FATAL( log, "Failed to stitch between  pillars p(" << i << ", " << (j-1) << ") and p(" << i << ", " << j << ")");
                }
#endif


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
                std::vector<WallLine> wall_lines;
                std::vector<Intersection> wall_line_intersections;

               if( !extractWallLines( wall_lines,
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
                                      nx*ny ) ) {
                   LOGGER_FATAL( log, "Failed to extract lines on wall between pillars p(" << (i-1) << ", " << j << ") and p(" << i << ", " << j << ")");
                   return NULL;
                }
                if( !intersectWallLines( vertices,
                                         wall_line_intersections,
                                         pi0j0_d10_chains,
                                         pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1),
                                         wall_lines ) ) {
                    LOGGER_FATAL( log, "Failed to intersect wall lines between pillars  p(" << i << ", " << (j-1) << ") and p(" << i << ", " << j << ")");
                    return NULL;
                }


#if 1
                bool success;
                success = wallEdges( edge_info,
                                     edges,
                                     wall_lines,
                                     wall_line_intersections,
                                     pi0j0_d10_chains,
                                     pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1) );

                if( wall_line_intersections.empty() ) {
                    // no intersections, we can use a simpler stitching scheme
                    success = stitchPillarsNoIntersections( edge_info,
                                                            edges,
                                                            triangle_info,
                                                            triangles,
                                                            wall_lines );
                }
                else {
                     success = stitchPillarsHandleIntersections( triangle_info,
                                                                triangles,
                                                                wall_lines,
                                                                wall_line_intersections,
                                                                pi0j0_d10_chains,
                                                                pi0j0_d10_chain_offsets.data() + chain_offset_stride*(i-1) );
                }
                if( !success ) {
                    LOGGER_FATAL( log, "Failed to stitch between  pillars p(" << i << ", " << (j-1) << ") and p(" << i << ", " << j << ")");
                }
#endif

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

                if(!stitchTopBottom( debug_edges0,
                                     triangle_info,
                                     triangles,
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
                                     nx*ny ) ) {
                    LOGGER_FATAL( log, "Failed to stitch top and bottom for cell c(" << (i-1) << ", " << (j-1) << ")" );
                    return NULL;
                }

                // process cell column c(i-1,j-1)
                for( unsigned int m=0; m < jm1_active_cell_count[ i ]; m++ ) {
                    const unsigned int k = jm1_active_cell_list[ (nz*i) + m];
                    const size_t gix = i-1 + nx*((j-1) + k*ny);
                    unsigned int cix = cell_map[gix];
                    cell_index[ cix ] = gix;

                    const unsigned int i000 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O00 ) + m ) + 0 ];
                    const unsigned int i100 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O10 ) + m ) + 0 ];
                    const unsigned int i010 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O01 ) + m ) + 0 ];
                    const unsigned int i110 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O11 ) + m ) + 0 ];
                    const unsigned int i001 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O00 ) + m ) + 1 ];
                    const unsigned int i101 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O10 ) + m ) + 1 ];
                    const unsigned int i011 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O01 ) + m ) + 1 ];
                    const unsigned int i111 = jm1_zcorn_ix[ 2*(nz*( 4*(i-1) + CELL_CORNER_O11 ) + m ) + 1 ];



                    cell_corner[ 8*cix + 0 ] = i000;
                    cell_corner[ 8*cix + 1 ] = i100;
                    cell_corner[ 8*cix + 2 ] = i010;
                    cell_corner[ 8*cix + 3 ] = i110;
                    cell_corner[ 8*cix + 4 ] = i001;
                    cell_corner[ 8*cix + 5 ] = i101;
                    cell_corner[ 8*cix + 6 ] = i011;
                    cell_corner[ 8*cix + 7 ] = i111;


                }

            }
        }

        pi0jm1_d10_chains.swap( pi0j0_d10_chains );
        pi0jm1_d10_chain_offsets.swap( pi0j0_d10_chain_offsets );

        jm1_zcorn_ix.swap( jm0_zcorn_ix );
        jm0_active_cell_list.swap( jm1_active_cell_list );
        jm0_active_cell_count.swap( jm1_active_cell_count );

        jm0_wall_line_ix.swap( jm1_wall_line_ix );

    }

#if 0
    for( unsigned int jz=0; jz<nz; jz++ ) {
        for( unsigned int jy=0; jy<ny; jy++) {
            for( unsigned int jx=0; jx<nx; jx++ ) {
                size_t gix = jx + nx*(jy + jz*ny);

                if( actnum[ gix ] != 0 ) {
                    unsigned int cix = cell_map[gix];
                    cell_index[ cix ] = gix;
                    unsigned int base_ix = vertices.size()/4;

                    for(int iz=0; iz<2; iz++) {
                        for(int iy=0; iy<2; iy++) {
                            for(int ix=0; ix<2; ix++) {
                                int cix = 6*( jx+ix  +
                                              (nx+1)*(jy+iy) +
                                              (nx+1)*(ny+1)*(0) );

                                float x1 = coord[ cix + 0 ];
                                float y1 = coord[ cix + 1 ];
                                float z1 = coord[ cix + 2 ];
                                float x2 = coord[ cix + 3 ];
                                float y2 = coord[ cix + 4 ];
                                float z2 = coord[ cix + 5 ];

                                int zix =             2*jx+ix   +
                                        (2*nx)*( 2*jy+iy ) +
                                        (2*nx*2*ny)*( 2*jz+iz );
                                float z = zcorn[ zix ];

                                float a = (z-z1)/(z2-z1);
                                float b = 1.f-a;


                                const float px = b*x1 + a*x2;
                                const float py = b*y1 + a*y2;
                                const float pz = z;

                                vertices.push_back( px );
                                vertices.push_back( py );
                                vertices.push_back( pz );
                                vertices.push_back( 1.f );
                            }
                        }
                    }

                    debug_edges0.push_back( base_ix + 0 );
                    debug_edges0.push_back( base_ix + 4 );

                    debug_edges0.push_back( base_ix + 1 );
                    debug_edges0.push_back( base_ix + 5 );

                    debug_edges0.push_back( base_ix + 2 );
                    debug_edges0.push_back( base_ix + 6 );

                    debug_edges0.push_back( base_ix + 3 );
                    debug_edges0.push_back( base_ix + 7 );


                    debug_edges0.push_back( base_ix + 0 );
                    debug_edges0.push_back( base_ix + 1 );

                    debug_edges0.push_back( base_ix + 1 );
                    debug_edges0.push_back( base_ix + 3 );

                    debug_edges0.push_back( base_ix + 3 );
                    debug_edges0.push_back( base_ix + 2 );

                    debug_edges0.push_back( base_ix + 2 );
                    debug_edges0.push_back( base_ix + 0 );

                    debug_edges0.push_back( base_ix + 0 + 4 );
                    debug_edges0.push_back( base_ix + 1 + 4 );

                    debug_edges0.push_back( base_ix + 1 + 4 );
                    debug_edges0.push_back( base_ix + 3 + 4 );

                    debug_edges0.push_back( base_ix + 3 + 4 );
                    debug_edges0.push_back( base_ix + 2 + 4 );

                    debug_edges0.push_back( base_ix + 2 + 4  );
                    debug_edges0.push_back( base_ix + 0 + 4 );

                }
            }
        }
    }
#endif

    return true;
}




static
bool
triangulate( std::vector<unsigned int>&  debug_edges0,
             std::vector<unsigned int>&  debug_edges1,
             std::vector<float>&         vertices,
             std::vector<unsigned int>&  edge_info,
             std::vector<unsigned int>&  edges,
             std::vector<unsigned int>&  triangle_info,
             std::vector<unsigned int>&  triangles,
             std::vector<unsigned int>&  cell_index,
             std::vector<unsigned int>&  cell_corner,
             const std::vector<float>&   coord,
             const std::vector<float>&   zcorn,
             const std::vector<int>&     actnum,
             const std::vector<int>&     cell_map,
             const unsigned int          nx,
             const unsigned int          ny,
             const unsigned int          nz );



#if 0
/*
if(1) {
    const unsigned int c00_i = i-1;             const unsigned int c00_j = j-1;
    const unsigned int c10_i = i;               const unsigned int c10_j = j-1;
    const unsigned int c01_i = i-1;             const unsigned int c01_j = j;
    const unsigned int c11_i = i;               const unsigned int c11_j = j;
    const unsigned int z00_i = 2u*c00_i + 1u;   const unsigned int z00_j = 2u*c00_j + 1u;
    const unsigned int z10_i = 2u*c10_i;        const unsigned int z10_j = 2u*c10_j + 1u;
    const unsigned int z01_i = 2u*c01_i + 1u;   const unsigned int z01_j = 2u*c01_j;
    const unsigned int z11_i = 2u*c11_i;        const unsigned int z11_j = 2u*c11_j;
    if(!uniquePillarVerticesOld( vertices,
                              m_0_0 ? zcorn_full_ix.data() +  z00_i + 2u*nx*z00_j : NULL,
                              m_1_0 ? zcorn_full_ix.data() +  z10_i + 2u*nx*z10_j : NULL,
                              m_0_1 ? zcorn_full_ix.data() +  z01_i + 2u*nx*z01_j : NULL,
                              m_1_1 ? zcorn_full_ix.data() +  z11_i + 2u*nx*z11_j : NULL,
                              4*nx*ny,
                              m_0_0 ? zcorn.data()    +  z00_i + 2u*nx*z00_j : NULL,
                              m_1_0 ? zcorn.data()    +  z10_i + 2u*nx*z10_j : NULL,
                              m_0_1 ? zcorn.data()    +  z01_i + 2u*nx*z01_j : NULL,
                              m_1_1 ? zcorn.data()    +  z11_i + 2u*nx*z11_j : NULL,
                              m_0_0 ? actnum.data()   +  c00_i + nx*c00_j    : NULL,
                              m_1_0 ? actnum.data()   +  c10_i + nx*c10_j    : NULL,
                              m_0_1 ? actnum.data()   +  c01_i + nx*c01_j    : NULL,
                              m_1_1 ? actnum.data()   +  c11_i + nx*c11_j    : NULL,
                              nx*ny,
                              coord.data() + 6u*((nx+1u)*j + i),
                              nz ) )
    {
        LOGGER_ERROR( log, "at pillar("<<i<<","<<j<<"): failed to determine unique vertices" );
        return NULL;
    }
}
*/
/** Find the unique vertices used by 2-4 cells along a single pillar.
  *
  *   +--+--+-> i
  *   |00|01|
  *   +--*--+
  *   |10|11|
  *   +--+--+
  *   V
  *   j
  */
bool
CPTessFactory::uniquePillarVerticesOld( std::vector<float>&  vertices,
                                     unsigned int*        zcornix_00,
                                     unsigned int*        zcornix_01,
                                     unsigned int*        zcornix_10,
                                     unsigned int*        zcornix_11,
                                     const unsigned int   zcornix_stride,
                                     const float* const   zcorn_00,
                                     const float* const   zcorn_01,
                                     const float* const   zcorn_10,
                                     const float* const   zcorn_11,
                                     const int* const     actnum_00,
                                     const int* const     actnum_01,
                                     const int* const     actnum_10,
                                     const int* const     actnum_11,
                                     const unsigned int   stride,
                                     const float*         coord,
                                     const unsigned int   nz )
{
    unsigned int* zcornix[4] = { zcornix_00, zcornix_01, zcornix_10, zcornix_11 };
    const float* zcorn[4]    = { zcorn_00, zcorn_01, zcorn_10, zcorn_11 };
    const int* actnum[4]     = { actnum_00, actnum_01, actnum_10, actnum_11 };


    struct UniqVtxHelper
    {
        UniqVtxHelper() {}
        UniqVtxHelper( const float value, const unsigned int index )
            : m_value( value ),
              m_index( index )
        {}
        float           m_value;
        unsigned int    m_index;
    };

    Logger log = getLogger( "CPTessFactory.uniquePillarVertices" );


    const float x1 = coord[ 0 ];
    const float y1 = coord[ 1 ];
    const float z1 = coord[ 2 ];
    const float x2 = coord[ 3 ];
    const float y2 = coord[ 4 ];
    const float z2 = coord[ 5 ];

    std::vector<UniqVtxHelper> helper;
    helper.reserve( 4*2*nz );

    bool increasing = false;
    bool decreasing = false;


    for( unsigned int k=0; k<4; k++ ) {
        if( actnum[k] != NULL ) {
            size_t o = helper.size();
            for( unsigned int i=0; i<nz; i++ ) {
                if( (actnum[k])[stride*i] != 0 ) {
                    float a = (zcorn[k])[4u*stride*(2*i+0)];
                    float b = (zcorn[k])[4u*stride*(2*i+1)];
                    helper.push_back( UniqVtxHelper( a, ( ((2*i+0)<<2) | k ) ) );
                    helper.push_back( UniqVtxHelper( b, ( ((2*i+1)<<2) | k ) ) );
                }
            }
            // we always push two values onto helper, so it is never odd
            if( o < helper.size() ) {
                if( helper[o].m_value < helper.back().m_value ) {
                    increasing = true;
                }
                if( helper.back().m_value < helper[o].m_value ) {
                    decreasing = true;
                }
            }
        }
    }
    if( helper.empty() ) {
        return true;
    }
    if( increasing && decreasing ) {
        LOGGER_ERROR( log, "zcorn of cells around pillar are both increasing and decreasing" );
        return false;
    }

    if( increasing ) {
        std::sort( helper.begin(), helper.end(),
                   []( const UniqVtxHelper& a, const UniqVtxHelper& b ) {
                       return a.m_value < b.m_value;
                   } );
        for( auto jt = helper.begin(); jt != helper.end();  ) {
            const float val = jt->m_value;
            const float threshold = val + epsilon;
            bool searched = false;
            // search forward until value epsilon away from first value
            for( ; jt != helper.end() && (jt->m_value <= threshold); ++jt ) {
                searched = true;
                // and set this
                unsigned int k = jt->m_index & 0x3;
                unsigned int i = jt->m_index >> 2;
                (zcornix[k])[ zcornix_stride*i ] = vertices.size()/4;
            }
            LOGGER_INVARIANT( log, searched == true );

            const float a = (val-z1)/(z2-z1);
            const float b = 1.f - a;
            vertices.push_back( b*x1 + a*x2 );
            vertices.push_back( b*y1 + a*y2 );
            vertices.push_back( val );
            vertices.push_back( 1.f );
        }
    }
    else {
        std::sort( helper.begin(), helper.end(),
                   []( const UniqVtxHelper& a, const UniqVtxHelper& b ) {
                       return b.m_value < a.m_value;
                   } );
        for( auto jt = helper.begin(); jt != helper.end();  ) {
            const float val = jt->m_value;
            const float threshold = val - epsilon;
            bool searched = false;
            for( ; jt != helper.end() && (jt->m_value >= threshold); ++jt ) {
                searched = true;
                unsigned int k = jt->m_index & 0x3;
                unsigned int i = jt->m_index >> 2;
                (zcornix[k])[ zcornix_stride*i ] = vertices.size()/4;
            }
            LOGGER_INVARIANT( log, searched == true );
            const float a = (val-z1)/(z2-z1);
            const float b = 1.f - a;
            vertices.push_back( b*x1 + a*x2 );
            vertices.push_back( b*y1 + a*y2 );
            vertices.push_back( val );
            vertices.push_back( 1.f );
        }
    }

#ifdef DEBUG
    // sanity check
    for( unsigned int k=0; k<4; k++ ) {
        if( actnum[k] != NULL ) {
            unsigned int last = 0u;
            for( unsigned int i=0; i<nz; i++ ) {
                if( actnum[k][stride*i] != 0 ) {
                    for( unsigned int j=0; j<2; j++ ) {
                        unsigned int ix = zcornix[k][ zcornix_stride*(2*i+j) ];
                        LOGGER_INVARIANT( log, last <= ix );
                        last = ix;
                    }
                }
            }
        }
    }
#endif
    return true;
}
#endif
            void
            CPTessFactory::expandZCornIX( unsigned int*              zcorn_full_ix,
                                          const unsigned int         stride,
                                          const unsigned int* const  zcorn_ix,
                                          const unsigned int* const  active_cell_list,
                                          const unsigned int         active_cell_count )
            {
                for( unsigned int m=0; m<active_cell_count; m++) {
                    const unsigned int k = active_cell_list[m];
                    zcorn_full_ix[ 4*stride*(2*k+0) ] = zcorn_ix[ 2*m + 0 ];
                    zcorn_full_ix[ 4*stride*(2*k+1) ] = zcorn_ix[ 2*m + 1 ];
                }
            }

#if 0
 // old unused code
bool
CPTessFactory::boundaryLines( std::vector<unsigned int>&  debug_edges,
                              std::vector<WallLine>&  boundaries,
                              const unsigned int*         (&zcornixp)[4],
                              const unsigned int          zcorn_stride,
                              const int*                  (&cell_map)[2],
                              const unsigned int          cell_map_stride,
                              const int*                  (&actnum)[2],
                              const unsigned int          actnum_stride,
                              const unsigned int          nz )
{
    Logger log = getLogger( "CPTessFactory.boundaryLines" );

    struct SidedBoundaryLine {
        unsigned int    m_ends[2];
        unsigned int    m_side;
        unsigned int    m_tag;
        unsigned int    m_cell_over;
        unsigned int    m_cell_under;
    };


    // Extract all boundaries, that is, the separation lines on a cell stack
    // side between two cells.
    std::vector<SidedBoundaryLine> all_boundaries;
    all_boundaries.reserve( 2*2*nz );

    for( unsigned int side=0; side<2; side++ ) {
        if( actnum[side] != NULL ) {

            size_t o = all_boundaries.size();
            bool first = true;
            for( unsigned int k=0; k<nz; k++) {
                if( actnum[side][ actnum_stride*k] != 0 ) {
                    const unsigned int b0 = zcornixp[2*side+0][ zcorn_stride*(2*k + 0 )];
                    const unsigned int b1 = zcornixp[2*side+1][ zcorn_stride*(2*k + 0 )];
                    const unsigned int t0 = zcornixp[2*side+0][ zcorn_stride*(2*k + 1 )];
                    const unsigned int t1 = zcornixp[2*side+1][ zcorn_stride*(2*k + 1 )];

                    if( b0 == t0 && b1 == t1 ) {
                        // degenerate face
                        continue;
                    }
                    const int cix = cell_map[side][ cell_map_stride*k ];

                    if( first || (all_boundaries.back().m_ends[0] != b0 || all_boundaries.back().m_ends[1] != b1 ) ) {
                        // is first edge or previous edge didn't match, create new
                        all_boundaries.resize( all_boundaries.size()+1 );
                        SidedBoundaryLine& lb = all_boundaries.back();
                        lb.m_ends[0] = b0;
                        lb.m_ends[1] = b1;
                        lb.m_side = side;
                        lb.m_tag = 0;
                        lb.m_cell_under = ~0;
                        lb.m_cell_over =  cix;
                    }
                    else {
                        SidedBoundaryLine& lb = all_boundaries.back();
                        LOGGER_INVARIANT( log, lb.m_tag == 1 );
                        lb.m_tag = 0;
                        lb.m_cell_over =  cix;
                    }

                    // add top edge
                    all_boundaries.resize( all_boundaries.size()+1 );
                    SidedBoundaryLine& ub = all_boundaries.back();
                    ub.m_ends[0] = t0;
                    ub.m_ends[1] = t1;
                    ub.m_side = side;
                    ub.m_tag = 1;
                    ub.m_cell_under = cix;
                    ub.m_cell_over = ~0;
                    first = false;
                }
            }

#ifdef DEBUG
            // sanity check
            for( unsigned int i = o; i+1 < all_boundaries.size(); i++ ) {
                const SidedBoundaryLine& lb = all_boundaries[i];
                const SidedBoundaryLine& ub = all_boundaries[i+1];
                LOGGER_INVARIANT( log, !((ub.m_ends[0] < lb.m_ends[0] ) || (ub.m_ends[1] < lb.m_ends[1] ) ) );
            }
#endif

        }
    }
    if( all_boundaries.empty() ) {
        // no edges
        return true;
    }

//    for(auto it=)

    // -------------------------------------------------------------------------
    // Sort boundaries lexicographically. After this, identical edges should lie
    // adjacent. And since edges have been produces from top to bottom, and the
    // sort is stable, identical edges should have order preserved.
    std::stable_sort( all_boundaries.begin(), all_boundaries.end(),
        [](const SidedBoundaryLine& a, const SidedBoundaryLine& b ) {
            if( a.m_ends[0] < b.m_ends[0] ) {
                return true;
            }
            else if( a.m_ends[0] == b.m_ends[0] ) {
                return a.m_ends[1] < b.m_ends[1];
            }
            else {
                return false;
            }
        } );

    // Extract unique boundaries
#ifdef DEBUG
    for( size_t i=0; i+1<all_boundaries.size(); i++) {
        if( (all_boundaries[i+1].m_ends[0] < all_boundaries[i].m_ends[0] ) ||
                ( (all_boundaries[i+1].m_ends[0] == all_boundaries[i].m_ends[0] ) &&
                  (all_boundaries[i+1].m_ends[1] < all_boundaries[i].m_ends[1] ) ) )
        {
            LOGGER_FATAL( log, "Invariant broken at " << __FILE__ << "@" << __LINE__ );
            return false;
        }
    }
#endif


    // march from bottom to top
    boundaries.reserve( all_boundaries.size() );
    unsigned int cells_below[2] = { ~0u, ~0u };


    for( auto it = all_boundaries.begin(); it != all_boundaries.end(); ++it ) {
        unsigned int cells_above[2] = { cells_below[0], cells_below[1] };
        cells_above[ it->m_side ] = it->m_cell_over;
        LOGGER_INVARIANT( log, cells_below[ it->m_side ] == it->m_cell_under );



        bool copy = true;
        if( it != all_boundaries.begin() ) {
            WallLine& p = boundaries.back();
            SidedBoundaryLine& c = *it;
            if( p.m_ends[0]==c.m_ends[0] && p.m_ends[1]==c.m_ends[1] ) {
                copy = false;

                // Merge. Since we already have cleaned up the edges along each
                // of the two sides of the faces, we have this invariant.

                boundaries.back().m_side = boundaries.back().m_side ^ (it->m_side+1);
                if( boundaries.back().m_side != 3u ) {
                    LOGGER_FATAL( log, "assertion failed at " << __FILE__ << '@' << __LINE__ );
                }

            }
        }

        if( copy ) {
            boundaries.resize( boundaries.size() + 1 );
            WallLine& l = boundaries.back();
            l.m_ends[0] = it->m_ends[0];
            l.m_ends[1] = it->m_ends[1];
            l.m_side = it->m_side + 1;
            l.m_cell_over[0] = cells_above[0];
            l.m_cell_over[1] = cells_above[1];
            l.m_cell_under[0] = cells_below[0];
            l.m_cell_under[1] = cells_below[1];
        }
        else {
            WallLine& l = boundaries.back();
            l.m_cell_over[0] = cells_above[0];
            l.m_cell_over[1] = cells_above[1];
            //l.m_cell_under[0] = cells_below[0];
            //l.m_cell_under[1] = cells_below[1];
        }


        cells_below[0] = cells_above[0];
        cells_below[1] = cells_above[1];
    }

    for(unsigned int i=1; i<boundaries.size(); i++ ) {
        LOGGER_INVARIANT( log, boundaries[i-1].m_cell_over[0] == boundaries[i].m_cell_under[0] );
        LOGGER_INVARIANT( log, boundaries[i-1].m_cell_over[1] == boundaries[i].m_cell_under[1] );
    }
    return true;
}

    bool
    CPTessFactory::insertIntersections( std::vector<unsigned int>&        problem_edges,
                                        std::vector<float>&               vertices,
                                        std::vector< Intersection >&      intersections,
                                        std::vector<unsigned int>&        chains,
                                        std::vector<unsigned int>&        chain_offsets,
                                        const std::vector<WallLine>&  boundaries )
    {
        Logger log = getLogger( "CPTessFactory.insertIntersections" );
        if( boundaries.size() < 1 ) {
            return true;
        }
        std::vector< std::list<unsigned int> > chains_dep( boundaries.size() );

        // Get the two end points of pillar 0 (we add vertices here so vertices.data
        // might change due to reallocation, i.e., we need a copy)
        float p0min[3], p0max[3];
        for( unsigned int i=0; i<3; i++) {
            p0min[i] = vertices[ 4*boundaries.front().m_ends[0] + i];
            p0max[i] = vertices[ 4*boundaries.back().m_ends[0]  + i];
        }

        // TODO: Here we can add an early exit by creating an array of minimum
        // index at pillar 1 for each boundary.

        intersections.clear();
        chains_dep.clear();
        chains_dep.resize( boundaries.size() );
        for( size_t lower = 0; lower < boundaries.size(); lower++ ) {
            const WallLine& lb = boundaries[lower];

            // scan over edges that are above at pillar 0
            for( size_t upper = lower+1; upper < boundaries.size(); upper++ ) {
                const WallLine& ub = boundaries[upper];

                // detect upper edges that are lower at pillar 1 -> intersection
                if( boundaries[upper].m_ends[1] < boundaries[lower].m_ends[1] ) {

                    // We have an intersection. Since boundaries have been compacted
                    // on each side of the cell column wall, the boundaries should
                    // be from different sides.

                    LOGGER_INVARIANT( log, (boundaries[ upper ].m_side ^ boundaries[ lower ].m_side) == 3 );
                    const unsigned int is_ix = intersections.size();
                    const unsigned int vtx_ix = vertices.size()/4u;


                    // find intersection
                    vertices.resize( vertices.size() + 4u );
                    if( !approximateSegmentsIntersection( vertices.data() + 4*vtx_ix,
                                                          vertices.data() + 4*lb.m_ends[0], vertices.data() + 4*lb.m_ends[1],
                                                          vertices.data() + 4*ub.m_ends[0], vertices.data() + 4*ub.m_ends[1] ) )
                    {
                        problem_edges.push_back( lb.m_ends[0] );
                        problem_edges.push_back( lb.m_ends[1] );
                        problem_edges.push_back( ub.m_ends[0] );
                        problem_edges.push_back( ub.m_ends[1] );
    //                    LOGGER_FATAL( log, "Failed to intersect lines:" );
    //                    LOGGER_FATAL( log, "  lower line " << lower << " = [" << lb.m_ends[0] << ", " << lb.m_ends[1] << "] " );
    //                    LOGGER_FATAL( log, "  upper line " << upper << " = [" << ub.m_ends[0] << ", " << ub.m_ends[1] << "] " );
    //                    return false;
                    }

                    chains_dep[lower].push_back( is_ix );
                    chains_dep[upper].push_front( is_ix );

                    intersections.resize( intersections.size()+1 );
                    Intersection& i = intersections.back();
                    i.m_vtx_ix = vtx_ix;
                    i.m_upwrd_bndry_ix = lower;
                    i.m_dnwrd_bndry_ix = upper;
                    i.m_distance = distancePointLineSquared( vertices.data() + 4*vtx_ix, p0min, p0max );
                    i.m_nxt_dnwrd_isec_ix = ~0u;
                    i.m_nxt_upwrd_isec_ix = ~0u;
                }
            }
        }


        for(size_t b_ix=0; b_ix<boundaries.size(); b_ix++) {
            const std::list<unsigned int>& chain = chains_dep[b_ix];
            if( !chain.empty() ) {
                auto p = chain.begin();
                auto c = p;
                for( c++; c != chain.end(); c++ ) {
                    if( b_ix == intersections[*p].m_dnwrd_bndry_ix ) {
                        intersections[ *p ].m_nxt_dnwrd_isec_ix = *c;
                    }
                    else if( b_ix == intersections[*p].m_upwrd_bndry_ix ) {
                        //next_l[ *p ] = *c;
                        intersections[ *p ].m_nxt_upwrd_isec_ix = *c;
                    }
                    p = c;
                }
                if( b_ix == intersections[*p].m_dnwrd_bndry_ix ) {
                    //next_u[*p] = end_flag | boundaries[b].m_ends[1];
                    intersections[ *p ].m_nxt_dnwrd_isec_ix = end_flag | boundaries[b_ix].m_ends[1];
                }
                else if( b_ix == intersections[*p].m_upwrd_bndry_ix ) {
                    //next_l[*p] = end_flag | boundaries[b].m_ends[1];
                    intersections[ *p ].m_nxt_upwrd_isec_ix = end_flag | boundaries[b_ix].m_ends[1];
                }
            }
        }

        // populate chains and chain_offsets.

        chains.clear();
        chains.reserve( 2u*intersections.size() );
        chain_offsets.resize( boundaries.size() + 1u );
        for( unsigned int i=0; i<boundaries.size(); i++ ) {
            chain_offsets[i] = chains.size();
            for( auto it=chains_dep[i].begin(); it!=chains_dep[i].end(); ++it ) {
                chains.push_back( *it );
            }
        }
        chain_offsets.back() = chains.size();


    #ifdef VERBOSE_DEBUG
        for( size_t i=0; i<intersections.size(); i++ ) {
            LOGGER_DEBUG( log, "i["<<i<<"]=" << intersections[i].m_vtx_ix
                          << ", lower_line=" << intersections[i].m_lower_line
                          << ", upper_line=" << intersections[i].m_upper_line
                          << ", next_u=" << (intersections[i].m_next_u&end_mask) << ((intersections[i].m_next_u&end_flag)?"E":"")
                          << ", next_l=" << (intersections[i].m_next_l&end_mask) << ((intersections[i].m_next_l&end_flag)?"E":"")
                          << ", distance=" << intersections[i].m_distance);
        }
    #endif

        return true;
    }



#endif
    static
    bool
    uniquePillarVerticesOld( std::vector<float>&  vertices,
                          unsigned int*        zcornix_00,
                          unsigned int*        zcornix_01,
                          unsigned int*        zcornix_10,
                          unsigned int*        zcornix_11,
                          const unsigned int   zcornix_stride,
                          const float* const   zcorn_00,
                          const float* const   zcorn_01,
                          const float* const   zcorn_10,
                          const float* const   zcorn_11,
                          const int* const     actnum_00,
                          const int* const     actnum_01,
                          const int* const     actnum_10,
                          const int* const     actnum_11,
                          const unsigned int   stride,
                          const float*         coord,
                          const unsigned int   nz );

    static
    bool
    simpleTriangulate( std::vector<float>&         vertices,
                       std::vector<unsigned int>&  triangle_info,
                       std::vector<unsigned int>&  triangles,
                       std::vector<unsigned int>&  cell_index,
                       std::vector<unsigned int>&  cell_corner,
                       const std::vector<float>&   coord,
                       const std::vector<float>&   zcorn,
                       const std::vector<int>&     actnum,
                       const std::vector<int>&     cell_map,
                       const unsigned int          nx,
                       const unsigned int          ny,
                       const unsigned int          nz );
