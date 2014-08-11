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
 * along with the FRView.  If not, see http://www.gnu.org/licenses/.
 */

in VG {
    uvec4 meta;
} in_gs[];


layout(binding=1)   uniform usamplerBuffer  normal_ix;
layout(binding=2)   uniform usamplerBuffer  vertex_ix;
#ifndef POSITIONS_DECLARED
layout(binding=3)   uniform samplerBuffer   positions;
#endif

void
main()
{
    uint selected_cell = in_gs[0].meta.x;
    bool flip          = in_gs[0].meta.y == 1;
    uint o             = in_gs[0].meta.z;
    uint N             = in_gs[0].meta.w;
    uint convex_bit = bitfieldExtract( selected_cell, 30, 1);

#if MAX_OUT == 1
    uvec3 vtx_ix = uvec3( texelFetch( vertex_ix, int(o+0) ).r,
                          texelFetch( vertex_ix, int(o+1) ).r,
                          texelFetch( vertex_ix, int(o+2) ).r );
    uvec3 nrm_ix = uvec3( texelFetch( normal_ix, int(o+0) ).r,
                          texelFetch( normal_ix, int(o+1) ).r,
                          texelFetch( normal_ix, int(o+2) ).r );
    if( flip ) {
        uvec3 bit = bitfieldExtract( nrm_ix, 30, 1 );
        nrm_ix = bitfieldInsert( nrm_ix, bit, 31, 1 );
    }
    emit_triangle( selected_cell,
                   nrm_ix,
                   vtx_ix );
#else


    uint vtx_ix[MAX_OUT+2];
    uint nrm_ix[MAX_OUT+2];
    for(int i=0; i<N; i++) {
        vtx_ix[i] = texelFetch( vertex_ix, int(o+i) ).r;
        uint t = texelFetch( normal_ix, int(o+i) ).r;
        if( flip ) {    // Flip edge masks if needed
            uint bit = bitfieldExtract( t, 30, 1 );
            t = bitfieldInsert( t, bit, 31, 1 );
        }
        nrm_ix[i] = t;

    }
    if( false && convex_bit != 0 ) {
        for( int i=2; i<N; i++ ) {
            emit_triangle( selected_cell,
                           uvec3( nrm_ix[ 0 ],
                                  nrm_ix[ i-1 ],
                                  nrm_ix[ i ] ),
                           uvec3( vtx_ix[0],
                                  vtx_ix[ i-1 ],
                                  vtx_ix[ i ] ) );
        }
        return;
    }
    else {

        // If we have more than three vertices, we need geometric positions
        uint s = 0;
#if MAX_OUT > 1
        if( N > 3 ) {
            vec3 pos[MAX_OUT+2];
            for(int i=0; i<N; i++) {
                pos[i] = texelFetch( positions, int(vtx_ix[i]) ).rgb;
            }

            // If we have more than four vertices, do elaborate triangulation
            if( N > 4 ) {
                // compute approximate normal and barycenter
                // Newell's method (GPU gems III)
                vec3 n = vec3(0.f);
                for(int i=0; i<N; i++) {
                    vec3 q = pos[i];
                    vec3 r = pos[(i+1)%N];
                    n += vec3( (q.y-r.y)*(q.z+r.z),
                               (q.z-r.z)*(q.x+r.x),
                               (q.x-r.x)*(q.y+r.y) );
                }
                vec3 na = abs(n);
                vec3 m;
                if( (na.x < na.y) && (na.x < na.z) ) {
                    m = vec3( 1.f, 0.f, 0.f );
                }
                else if( na.y < na.z ) {
                    m = vec3( 0.f, 1.f, 0.f );
                }
                else {
                    m = vec3( 0.f, 0.f, 1.f );
                }
                vec3 u = normalize( m - (dot(m,n)/dot(n,n))*n );
                vec3 v = normalize( cross( n,u ) );
                // project to plane
                vec2 pos2[MAX_OUT+2];
                for( int i=0; i<N; i++ ) {
                    pos2[i] = vec2( dot(u, pos[i]), dot(v, pos[i]) );
                }

                while( N > 4 ) {
                    uvec3 ear = uvec3(0,1,2);
                    // Search backwards so we minimize the amount data shift
                    for(uint i=N; i>0; i--) {
                        uvec3 j = uvec3( i-1, i%N, (i+1)%N );
                        vec2 a = pos2[ j.x ];
                        vec2 b = pos2[ j.y ];
                        vec2 c = pos2[ j.z ];
                        vec2 ab = b-a;
                        vec2 ac = c-a;
                        float det = (ab.x*ac.y) - (ac.x*ab.y);
                        // Is current triangle a convex corner?
                        if( det > 1e-6 ) {
                            bool success = true;
                            // Is current triangle void of other points?
                            for(uint k=3; k<N; k++) {
                                uint l = (j.x+k)%N;
                                vec2 p = pos2[ l ];

                                if( ((p.x-a.x)*ab.y) - (ab.x*(p.y-a.y)) > 0.f ) {   // ab
                                    if( ((p.x-c.x)*ac.y) - (ac.x*(p.y-c.y)) < 0.f ) {   // ca
                                        vec2 bc = c-b;
                                        if( ((p.x-b.x)*bc.y) - (bc.x*(p.y-b.y)) > 0.f ) {   // bc
                                            success = false;
                                            break;
                                        }
                                    }
                                }
                            }
                            if( success ) {
                                ear = j;
                                break;
                            }
                        }
                    }
                    // emit ear
                    emit_triangle( selected_cell,
                                   uvec3( nrm_ix[ ear.x ],
                                          nrm_ix[ ear.y ],
                                          bitfieldInsert( nrm_ix[ ear.z ], 0u, 31, 1 ) ),
                                   uvec3( vtx_ix[ ear.x ],
                                          vtx_ix[ ear.y ],
                                          vtx_ix[ ear.z ] ) );

                    // Decrease polygon count and shift data
                    N--;
                    nrm_ix[ ear.x ] = bitfieldInsert( nrm_ix[ ear.x ], 0u, 31, 1 );
                    for( uint k=ear.y; k<N; k++ ) {
                        nrm_ix[k] = nrm_ix[k+1];
                        vtx_ix[k] = vtx_ix[k+1];
                        pos2[k] = pos2[k+1];
                    }
                }
            }
            // triangulate quad
            vec3 ab = pos[1]-pos[0];
            vec3 bc = pos[2]-pos[1];
            vec3 cd = pos[3]-pos[2];
            vec3 da = pos[0]-pos[3];
            float c0 = dot( cross( ab, bc ), cross( cd, da ) );
            float c1 = dot( cross( bc, cd ), cross( da, ab ) );
            if( c1 > c0 ) {
                s = 1;
            }
            // emit first triangle of quad
            emit_triangle( selected_cell,
                           uvec3( nrm_ix[ 2+s ],
                                  nrm_ix[ (3+s)%4 ],
                                  bitfieldInsert( nrm_ix[ 0+s ], 0u, 31, 1 ) ),
                           uvec3( vtx_ix[ 2+s ],
                                  vtx_ix[ (3+s)%4 ],
                                  vtx_ix[ 0+s ] ) );

            // clear edge flags for last triangle
            nrm_ix[ 2+s ] = bitfieldInsert( nrm_ix[ 2+s ], 0u, 31, 1 );
        }
#endif

        // Emit last triangle (or single triangle if poly was a triangle)
        emit_triangle( selected_cell,
                       uvec3( nrm_ix[ 0+s ],
                              nrm_ix[ 1+s ],
                              nrm_ix[ 2+s ] ),
                       uvec3( vtx_ix[ 0+s ],
                              vtx_ix[ 1+s ],
                              vtx_ix[ 2+s ] ) );
    }
#endif
}

