#include "GridTessBridge.hpp"
#include "CornerPointTessellatorSimple.hpp"


template<typename Bridge>
void
CornerPointTessellatorSimple<Bridge>::triangulate( Bridge&                                    bridge,
                                                   const unsigned int                         nx,
                                                   const unsigned int                         ny,
                                                   const unsigned int                         nz,
                                                   const unsigned int                         nr,
                                                   const std::vector<typename Bridge::REAL>&  coord,
                                                   const std::vector<typename Bridge::REAL>&  zcorn,
                                                   const std::vector<int>&                    actnum )
{
    for( unsigned int jz=0; jz<nz; jz++ ) {
        for( unsigned int jy=0; jy<ny; jy++) {
            for( unsigned int jx=0; jx<nx; jx++ ) {
                size_t gix = jx + nx*(jy + jz*ny);


                if( actnum[ gix ] != 0 ) {
                    unsigned int base_ix = bridge.vertexCount();

                    unsigned int cix = bridge.cellCount();
                    bridge.addCell( gix,
                                    base_ix + 0,
                                    base_ix + 1,
                                    base_ix + 2,
                                    base_ix + 3,
                                    base_ix + 4,
                                    base_ix + 5,
                                    base_ix + 6,
                                    base_ix + 7 );


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

                                bridge.addVertex( b*x1 + a*x2, b*y1 + a*y2, z );
                            }
                        }
                    }

/*
                    bridge.addTriangle( base_ix + 0, base_ix + 2, base_ix + 3, ~0u, cix );
                    bridge.addTriangle( base_ix + 3, base_ix + 1, base_ix + 0, ~0u, cix );

                    bridge.addTriangle( base_ix + 7, base_ix + 5, base_ix + 1, ~0u, cix );
                    bridge.addTriangle( base_ix + 1, base_ix + 3, base_ix + 7, ~0u, cix );

                    bridge.addTriangle( base_ix + 4, base_ix + 5, base_ix + 7, ~0u, cix );
                    bridge.addTriangle( base_ix + 7, base_ix + 6, base_ix + 4, ~0u, cix );

                    bridge.addTriangle( base_ix + 4, base_ix + 6, base_ix + 0, ~0u, cix );
                    bridge.addTriangle( base_ix + 2, base_ix + 0, base_ix + 6, ~0u, cix );

                    bridge.addTriangle( base_ix + 0, base_ix + 1, base_ix + 5, ~0u, cix );
                    bridge.addTriangle( base_ix + 5, base_ix + 4, base_ix + 0, ~0u, cix );

                    bridge.addTriangle( base_ix + 7, base_ix + 3, base_ix + 2, ~0u, cix );
                    bridge.addTriangle( base_ix + 2, base_ix + 6, base_ix + 7, ~0u, cix );
*/
                }
            }
        }
    }
}

template class CornerPointTessellatorSimple<GridTessBridge>;
