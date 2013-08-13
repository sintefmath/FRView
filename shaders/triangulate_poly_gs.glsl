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
layout(binding=1)   uniform usamplerBuffer  normal_ix;
layout(binding=2)   uniform usamplerBuffer  vertex_ix;
layout(binding=3)   uniform samplerBuffer   positions;

void
main()
{
    uint selected_cell = in_gs[0].meta.x;
    bool flip          = in_gs[0].meta.y == 1;
    uint o             = in_gs[0].meta.z;
    uint N             = in_gs[0].meta.w;
    uint convex_bit = bitfieldExtract( selected_cell, 30, 1);
#if 0
    //if( convex_bit == 1 ) {
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
        cell = uvec4( selected_cell, nrm_ix );
        indices = vtx_ix;
        EmitVertex();

        for(uint i=3; i<N; i++ ) {
            vtx_ix.y = vtx_ix.z;
            vtx_ix.z = texelFetch( vertex_ix, int(o+i) ).r;
            nrm_ix.y = nrm_ix.z;
            nrm_ix.z = texelFetch( normal_ix, int(o+i) ).r;
            if( flip ) {
                nrm_ix.z = bitfieldInsert( nrm_ix.z, bitfieldExtract( nrm_ix.z, 30, 1 ), 31, 1 );
            }
            cell = uvec4( selected_cell, nrm_ix );
            indices = vtx_ix;
            EmitVertex();
        }
    /*}
    else {
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

        // compute approximate normal and barycenter
        // Newell's method (GPU gems III)
        vec3 n = vec3(0.f);
        for(int i=0; i<N; i++) {
            vec3 q = texelFetch( positions, int(vtx_ix[i]) ).rgb;
            vec3 r = texelFetch( positions, int(vtx_ix[ (i+1)%N ]) ).rgb;
            n += vec3( (q.y-r.y)*(q.z+r.z),
                       (q.z-r.z)*(q.x+r.x),
                       (q.x-r.x)*(q.y+r.y) );
        }
        vec3 na = abs(n);
        bool ax = (na.x < na.y) && (na.x < na.z);
        bool ay = !ax && na.y < na.z;
        bool az = !ax && !ay;
        vec3 m = vec3( ax, ay, az );

        vec3 u = normalize( m - (dot(m,n)/dot(n,n))*n );
        vec3 v = normalize( cross( n,u ) );
        // project to plane
        vec2 pos2[MAX_OUT+2];
        for( int i=0; i<N; i++ ) {
            vec3 p = texelFetch( positions, int(vtx_ix[i]) ).rgb;
            pos2[i] = vec2( dot(u, p), dot(v, p) );
        }

        while( N > 3 ) {
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
            cell = uvec4( selected_cell,
                          nrm_ix[ ear.x ],
                    nrm_ix[ ear.y ],
                    bitfieldInsert( nrm_ix[ ear.z ], 0u, 31, 1 ) );
            indices = uvec3( vtx_ix[ ear.x ],
                    vtx_ix[ ear.y ],
                    vtx_ix[ ear.z ] );
            EmitVertex();

            // Decrease polygon count and shift data
            N--;
            nrm_ix[ ear.x ] = bitfieldInsert( nrm_ix[ ear.x ], 0u, 31, 1 );
            for( uint k=ear.y; k<N; k++ ) {
                nrm_ix[k] = nrm_ix[k+1];
                vtx_ix[k] = vtx_ix[k+1];
                pos2[k] = pos2[k+1];
            }
        }
        cell = uvec4( selected_cell,
                       nrm_ix[ 0 ],
                       nrm_ix[ 1 ],
                    nrm_ix[ 2 ] );
        indices = uvec3( vtx_ix[ 0 ],
                        vtx_ix[ 1 ],
                        vtx_ix[ 2 ] );
        EmitVertex();
    }*/
#else
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
    cell = uvec4( selected_cell, nrm_ix );
    indices = vtx_ix;
    EmitVertex();
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
            cell = uvec4( selected_cell,
                          nrm_ix[ 0 ],
                          nrm_ix[ i-1 ],
                          nrm_ix[ i ] );
            indices = uvec3( vtx_ix[0],
                             vtx_ix[ i-1 ],
                             vtx_ix[ i ] );
            EmitVertex();
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
                    cell = uvec4( selected_cell,
                                  nrm_ix[ ear.x ],
                            nrm_ix[ ear.y ],
                            bitfieldInsert( nrm_ix[ ear.z ], 0u, 31, 1 ) );
                    indices = uvec3( vtx_ix[ ear.x ],
                            vtx_ix[ ear.y ],
                            vtx_ix[ ear.z ] );
                    EmitVertex();

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
            cell = uvec4( selected_cell,
                          nrm_ix[ 2+s ],
                    nrm_ix[ (3+s)%4 ],
                    bitfieldInsert( nrm_ix[ 0+s ], 0u, 31, 1 ) );
            indices = uvec3( vtx_ix[ 2+s ],
                    vtx_ix[ (3+s)%4 ],
                    vtx_ix[ 0+s ] );
            EmitVertex();

            // clear edge flags for last triangle
            nrm_ix[ 2+s ] = bitfieldInsert( nrm_ix[ 2+s ], 0u, 31, 1 );
        }
#endif

        // Emit last triangle (or single triangle if poly was a triangle)
        cell = uvec4( selected_cell,
                      nrm_ix[ 0+s ],
                nrm_ix[ 1+s ],
                nrm_ix[ 2+s ] );
        indices = uvec3( vtx_ix[ 0+s ],
                vtx_ix[ 1+s ],
                vtx_ix[ 2+s ] );
        EmitVertex();
    }
#endif
#endif
}

