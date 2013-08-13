#version 420
/******************************************************************************
 *
 *  Author(s): Christopher Dyken <christopher.dyken@sintef.no>
 *
 *
 *  Copyright (C) 2012 by SINTEF.  All rights reserved.
 *
 ******************************************************************************/

/** Main func for the surface compact paths */
layout(points, invocations=1) in;
layout(points, max_vertices=MAX_OUT) out;

in VG {
    uvec4 meta;
} in_gs[];

layout(location=0)  out uvec4               cell;
layout(location=1)  out uvec3               indices;
                    uniform bool            flip_faces;
layout(binding=0)   uniform usamplerBuffer  normal_ix;
layout(binding=1)   uniform usamplerBuffer  vertex_ix;
layout(binding=2)   uniform samplerBuffer   positions;

void
main()
{
    uint selected_cell = in_gs[0].meta.x;
    bool flip          = in_gs[0].meta.y == 1;
    uint o             = in_gs[0].meta.z;
    uint N             = in_gs[0].meta.w;

    if(true) {

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
        if( N < 4 ) {
            cell = uvec4( selected_cell,
                          nrm_ix[ 0 ],
                          nrm_ix[ 1 ],
                          nrm_ix[ 2 ] );
            indices = uvec3( vtx_ix[ 0 ],
                             vtx_ix[ 1 ],
                         vtx_ix[ 2 ] );
            EmitVertex();
            return;
        }

        vec3 pos[MAX_OUT+2];
        for(int i=0; i<N; i++) {
            pos[i] = texelFetch( positions, int(vtx_ix[i]) ).rgb;
        }

        if( N > 4 ) {
            // compute approximate normal and barycenter
            // Newell's method (GPU gems III)
            vec3 n = vec3(0.f);
            vec3 b = vec3(0.f);
            for(int i=0; i<N; i++) {
                vec3 q = pos[i];
                vec3 r = pos[(i+1)%N];
                b += pos[i];
                n += vec3( (q.y-r.y)*(q.z+r.z),
                           (q.z-r.z)*(q.x+r.x),
                           (q.x-r.x)*(q.y+r.y) );
            }
            b = (1.f/N)*b;
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
                vec3 t = pos[i]-b;
                pos2[i] = vec2( dot(u, t), dot(v, t) );
            }


            while( N > 4 ) {
                uvec3 ear = uvec3(0,1,2);
                for(uint i=0; i<N; i++) {
                    uvec3 j = uvec3( (i+N-1)%N, i, (i+1)%N );
                    vec2 a = pos2[ j.x ];
                    vec2 b = pos2[ j.y ];
                    vec2 c = pos2[ j.z ];
                    vec2 ab = b-a;
                    vec2 ac = c-a;
                    float det = (ab.x*ac.y) - (ac.x*ab.y);
                    if( det > 1e-6 ) {  // convex corner
                        bool success = true;
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
                cell = uvec4( selected_cell,
                              nrm_ix[ ear.x ],
                        nrm_ix[ ear.y ],
                        //                          nrm_ix[ ear.z ] & 0x0fffffff );
                        bitfieldInsert( nrm_ix[ ear.z ], 0u, 31, 1 ) );
                indices = uvec3( vtx_ix[ ear.x ],
                        vtx_ix[ ear.y ],
                        vtx_ix[ ear.z ] );
                EmitVertex();

                N--;
                nrm_ix[ ear.x ] = bitfieldInsert( nrm_ix[ ear.x ], 0u, 31, 1 );
                for( uint k=ear.y; k<N; k++ ) {
                    nrm_ix[k] = nrm_ix[k+1];
                    vtx_ix[k] = vtx_ix[k+1];
                    pos2[k] = pos2[k+1];
                }
            }
        }
        vec3 ab = pos[1]-pos[0];
        vec3 bc = pos[2]-pos[1];
        vec3 cd = pos[3]-pos[2];
        vec3 da = pos[0]-pos[3];
        float c0 = dot( cross( ab, bc ), cross( cd, da ) );
        float c1 = dot( cross( bc, cd ), cross( da, ab ) );
        uint s = 0;
        if( c1 > c0 ) {
            s = 1;
        }
        cell = uvec4( selected_cell,
                      nrm_ix[ 0+s ],
                nrm_ix[ 1+s ],
                bitfieldInsert( nrm_ix[ 2+s ], 0u, 31, 1 ) );
        indices = uvec3( vtx_ix[ 0+s ],
                vtx_ix[ 1+s ],
                vtx_ix[ 2+s ] );
        EmitVertex();

        cell = uvec4( selected_cell,
                      nrm_ix[ 2+s ],
                nrm_ix[ (3+s)%4 ],
                bitfieldInsert( nrm_ix[ 0+s ], 0u, 31, 1 ) );
        indices = uvec3( vtx_ix[ 2+s ],
                vtx_ix[ (3+s)%4 ],
                vtx_ix[ 0+s ] );
        EmitVertex();
        return;
    }
}

