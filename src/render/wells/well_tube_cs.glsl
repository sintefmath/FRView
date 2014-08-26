#version 420 core




// -------------------- start of explicit inclusion of file 'common.glsl' --------------------
#pragma unroll
#pragma optionNV(unroll all)




#define PI 3.1415926535

#define REC_REF_LEVELS 5 // Max is 5. With 6 we get (2^6+1)*2 = 130 components in the patch-out struct, which has a limit of 120... Could probably pack stuff into vertx-out-structs...
#define REC_REF_FRAMES ((1<<REC_REF_LEVELS)-1)

#define P42_VEC2S ((1<<REC_REF_LEVELS)+1)
#define P42_VEC4S ((P42_VEC2S*2-1)/4+1) // If we reduce the number with 3, it doesn't crash on LEVELS 6. ( ((2^6+1)*2-1)/4+1-3 = 30, so 30 vec4s work, 31 not.) (!DEBUG doesn't help.)

#define TUBE_CIRCUMFERENCE_REFINEMENT 16
#define TUBE_AXIS_REFINEMENT 32
#define TUBE_REFINEMENT_S 4
#define TUBE_RADIUS 0.003



// The number of components for active per-patch output variables may not exceed GL_MAX_TESS_PATCH_COMPONENTS​. The minimum required limit is 120.
// Nvidia GeForce GTX 550 Ti: 120
// Note that REC_REF_LEVELS=5 combined with PACK_4_IN_2 gives (2^5+1)*2 = 66, and this works.
// However, LEVELS=6 gives (2^6+1)*2 = 130, which does not work. Seems to be the 120-limit. (No error occurs, but last 12 components are "lost". (130-12=118... Ok, may be that 120 works.)
// Without the packing: (2^5+1)*3 = 99, which should work, but doesn't. Don't know why. Is there also a hard limit on size of arrays? 2^5+1=33, 30 seems to be some sort of max...?!
// Think it should be possible to fix this by sending along the data split on the vertices instead, since they also can take a lot of data with them (128 components each?)

struct TUBE_PARAMETERS {
    // These vectors will be normalized
    vec4 rec_ref_e2[P42_VEC4S];
    vec3 color;
};




// 131023: Should probably be rewritten/optimized (for spped and numerical stability)
//         Check http://en.wikipedia.org/wiki/Cubic_Hermite_spline
//
// Return value: curve tangent
// Returned in output parameter c: position on curve

vec3 hermite_curve_eval(in vec3 p1, in vec3 m1, in vec3 p2, in vec3 m2,
                        in float u,
                        out vec3 c)
{
    float u0 = 2.0*u*u*u - 3.0*u*u + 1.0;       // h00(u)
    float u1 = u*u*u - 2.0*u*u + u;             // h10(u)
    float u2 = -2.0*u*u*u + 3.0*u*u;            // h01(u)
    float u3 = u*u*u - u*u;                 // h11(u)
    c = u0*p1 + u1*m1 + u2*p2 + u3*m2;          // h00(u)*p1 + h10(u)*m1 + h01(u)*p2 + h11(u)*m2

    float u0du = 6.0*u*u - 6.0*u;           // (dh00/du)(u)
    float u1du = 3.0*u*u - 4.0*u + 1.0;
    float u2du = -6.0*u*u + 6.0*u;
    float u3du = 3.0*u*u - 2.0*u;
    return u0du*p1 + u1du*m1 + u2du*p2 + u3du*m2; // the tangent
}

vec3 hermite_curve_tangent( in vec3 p1, in vec3 m1, in vec3 p2, in vec3 m2,
                            in float u )
{
    float u0du = 6.0*u*u - 6.0*u;           // (dh00/du)(u)
    float u1du = 3.0*u*u - 4.0*u + 1.0;
    float u2du = -6.0*u*u + 6.0*u;
    float u3du = 3.0*u*u - 2.0*u;
    return u0du*p1 + u1du*m1 + u2du*p2 + u3du*m2; // the tangent
}




// Returning unit vector
// Vil denne funke for v ikke normalisert?!?! Muligens, siden alle dot products er mellom v og unit vector... men litt teit. Sikkert dumt å ikke normalisere.
//hjalp ikke...
vec3 most_perpendicular_axis(in vec3 v0)
{
    vec3 v = normalize(v0);

    vec3 x_axis = vec3(1.0, 0.0, 0.0);
    vec3 y_axis = vec3(0.0, 1.0, 0.0);
    vec3 z_axis = vec3(0.0, 0.0, 1.0);

    vec3 ax = x_axis;
    float cosangle = abs(dot(v, ax));

    // Is the y_axis a better choice?
    if ( abs(dot(v, y_axis)) < cosangle ) {
        ax = y_axis;
        cosangle = abs(dot(v, ax));
    }

    // Is the z_axis a better choice?
    if ( abs(dot(v, z_axis)) < cosangle ) {
        ax = z_axis;
    }

    return ax;
}




// Evaluating the efc (exterior frame curve) in a given point.
// Also putting the test on whether or not to use the "one-piece" or "two-piece" version here, to make the
// calling code simpler.

void eval_efc( in vec3 p1, in vec3 m1,
               in vec3 p_middle, in vec3 m_middle,
               in vec3 p2, in vec3 m2,
               in float u,
               in float u_middle,
               in bool one_piece,
               out vec3 efc_pos, out vec3 efc_tangent )
{
    if (one_piece) {
        efc_tangent = hermite_curve_eval( p1, m1, p2, m2, u, efc_pos );
    } else {
        if ( u < u_middle ) {
            float u2 = u/u_middle;
            efc_tangent = hermite_curve_eval( p1, m1, p_middle, m_middle, u2, efc_pos );
        } else {
            float u2 = (u-u_middle)/(1.0-u_middle);
            efc_tangent = hermite_curve_eval( p_middle, m_middle, p2, m2, u2, efc_pos );
        }
    }
}




// SLERP
//
// u==0 -> v1
// u==1 -> v2

// Really strange (?) thing here: It seems that the normalization of params.rec_ref_e2[...] doesn't carry over to the
// eval shader from the control. At least not enough for the argument to acos to be <= 1.0, i.e., it fails with strange
// effects.

// This one does its own normalizing even though that may be unnecessary in some or all of the calls to it.

vec3 slerp( in vec3 v1, in vec3 v2, in float u )
{

//    return (1.0-u)*v1 + u*v2; // Slerp without the s... for testing

    if ( abs(dot(normalize(v1), normalize(v2))) >= 1.0 ) { // For trapping division by zero below
        return v1; // Should be equal to v2 when angle is zero
    } else {
        float subtended_angle_omega = acos( dot(normalize(v1), normalize(v2)) );
        float a = sin(      u *subtended_angle_omega )/sin(subtended_angle_omega);
        float b = sin( (1.0-u)*subtended_angle_omega )/sin(subtended_angle_omega);
        return b*v1 + a*v2;
    }
}




// Input vectors must be normalized by caller.

vec3 find_middle_pseudonormal( in vec3 e1_new, in vec3 e2_below, in vec3 e2_above,
                               in int lvl // If we are splitting [0, 1], lvl==0, on next level, lvl==1, and so on. Will be < s and < REC_REF_LEVELS when
                                          // called from TCS. When called from TES, will be equal to min(s, REC_REF_LEVELS).
                               // in bool called_from_ES = false // not in use
                             )
{
    // We split the interval in half, and compute *_new from *_bot and *_top values.

    // For vectors close to each other, we would like to use something in between, and we choose the average. For
    // vector far from each other (the extreme being two vectors cancelling each other) the average will not make much
    // sense, so instead we use something perpendicular to both the tangent and "roughly the normal-direction".
    // Hopefully, this will give a good "twist distribution".

    float cos_angle = dot(e2_above, e2_below); // Remember that cos(-x) = cos(x), and we get -1 <= cos_angle <= 1.

    vec3 e2_new;

    // Using the difference between e2_above and e2_below. Must first check if they point in opposite directions, in
    // which case the difference makes less sense.
    if ( cos_angle < -cos(radians(1.0)) ) { // Doesn't seem to be triggered often?! (Never?!)
        e2_new = most_perpendicular_axis( e2_below );
    } else {

        if ( cos_angle < cos(radians(30.0)) ) { // <=> angle > 30 deg
            e2_new = cross(e1_new, e2_below-e2_above); // Note the subtraction.
            // Hmm... Sometimes e2_above-e2_below is much better. But is it possible to determine which one is best while
            // we are here? We choose the one giving a result more similar to the average direction than otherwise.
            // (If they cancel exactly, the average vanishes, but that should have been taken care of already.)
            
            // Note that further below, we check for e2_new being parallel to e1_new. What we have not done is to check if
            // the e2_new just calculated makes sense, i.e., that e2_below-e2_above is not parallel to e1_new!
            float cos_angle2 = dot( normalize(e2_below-e2_above), e1_new );
            if ( abs(cos_angle2) > cos(radians(1.0)) ) {
                e2_new = most_perpendicular_axis( e1_new );
            }

            // If e2_above x e2_below more or less point in the same direction as e1_new, we go for the e2_above-e2_below
            // choice.
            // 140728: Hva er dette?? Blir dette en test på om e2_below-e2_above er nærmere e2_below+e2_above eller motsatt??
            if ( dot(e1_new, cross(e2_below, e2_above)) < 0.0 ) { // No need to normalize when comparing to zero
                e2_new = -e2_new;
            }
        } else {
            // Relatively similar directions, the middle should be a good choice. Note that SLERP is equal for u=0.5.
            e2_new = 0.5*(e2_below+e2_above);
        }
        
    }

    // Must now test if this guess (most of the time the average of the below and above e3s) is more or less parallel to e1
    // 131215: Hmm... If the stuff above works as intended, we should not really get into this situation. Disabling it.
    // 131218: Was that correct? Why can this not happen? Putting it back in.
    // 140728: Again, don't think this is really needed, but must double-check.
    cos_angle = dot( normalize(e2_new), e1_new );
    if ( abs(cos_angle) > cos(radians(1.0)) ) {
        e2_new = most_perpendicular_axis( e1_new );
    }

    // Instead of returning cross(e1_new, e2_new), which would be a true binormal, we return e2_new directly. This is
    // not necessarily perpendicular to e1, so it is neither a normal nor binormal. But it is close to what we would
    // prefer as a normal.
    return e2_new; // Normalizing not needed (as long as we don't use this vector directly as normal, but let is pass through the "normalize-cross-cross-path"...)
}




int num_of_rec_ref_frames(in int s)
{
    return (1<<s)-1;
}




vec2 cart_to_sph(in vec3 v)
{
    // return vec2( acos(clamp(v.z, -1.0, 1.0)), atan(v.y, v.x) );
    float acos_z, atan2;
    if (v.z<=-1.0) {
        acos_z = PI;
    } else {
        if (v.z>=1.0) {
            acos_z = 0.0;
        } else {
            acos_z = acos(v.z);
        }
    }
    if (v.x==0.0) {
        if (v.y>0.0) {
            atan2 = 0.5*PI;
        } else {
            atan2 = -0.5*PI;
        }
    } else {
        atan2 = atan(v.y, v.x);
    }
    return vec2( acos_z, atan2 );
}





vec3 sph_to_cart(in vec2 v)
{
    return vec3( sin(v.x)*cos(v.y), sin(v.x)*sin(v.y), cos(v.x) );
}




// Returning ivec2( index of vec4, offset of first float )
ivec2 p42_index_and_offset( in int vec2_indx )
{
    return ivec2( vec2_indx >> 1, (vec2_indx & 1) << 1 );
}




// Adding this for increased readability of code using it

vec3 fetch_normal_from_list( in vec4 rec_ref_e2[P42_VEC4S], in int indx )
{
    vec2 e2;
    if ( (indx & 1) == 0 ) {
        e2 = rec_ref_e2[ indx >> 1 ].xy;
    } else {
        e2 = rec_ref_e2[ indx >> 1 ].zw;
    }
    return sph_to_cart(e2);
}

// n should be normalized by caller
void store_normal_in_list( inout vec4 rec_ref_e2[P42_VEC4S], in int indx, in vec3 n )
{
    if ( (indx & 1) == 0 ) {
        rec_ref_e2[ indx >> 1 ].xy = cart_to_sph( n );
    } else {
        rec_ref_e2[ indx >> 1 ].zw = cart_to_sph( n );
    }
}




void compute_frame_in_middle( inout vec4 rec_ref_e2[P42_VEC4S],
                              in vec3 p1, in vec3 m1, in vec3 p2, in vec3 m2, in float u,
                              in int indx_bot, in int indx_top,
                              in int indx_new, in int lvl ) // If we are splitting [0, 1], lvl==0, on next level, lvl==1, and so on. Will be < s and < REC_REF_LEVELS.
                                                            // This is not called from the TES. That one calls find_middle_pseudonormal() directly.
{
    const vec3 e1 = normalize( hermite_curve_tangent(p1, m1, p2, m2, u) );
    const vec3 e2_bot = fetch_normal_from_list( rec_ref_e2, indx_bot );
    const vec3 e2_top = fetch_normal_from_list( rec_ref_e2, indx_top );
    const vec3 pseudo_e2 = find_middle_pseudonormal( e1, e2_bot, e2_top, lvl );
    // The frame the way it will be computed in the eval shader. We need this for the angle-testing (and
    // other computations?) for the deeper levels of the tree, to be computed below.
    const vec3 e3 = normalize(cross(e1, pseudo_e2)); // e3 normalized and orthogonal to e1, ...
    vec3 e2 = cross(e3, e1);
    store_normal_in_list( rec_ref_e2, indx_new, e2 ); // ..., so e3 x e1 will not need normalizing.
}
// -------------------- end of explicit inclusion of file 'common.glsl' --------------------




in VC
{
    vec3 position;
    vec3 tangent;
    vec3 normal;
    vec3 color;
} vert_in[];


out VF_post_control_shader
{
    vec3 pos;
} vert_out[];


patch out struct TUBE_PARAMETERS params;


layout (vertices=4) out;




void main()
{
    vert_out[ gl_InvocationID ].pos    = vert_in[ gl_InvocationID ].position;


    // Note the ordering: Adaption of "original ordering" to "FRView's ordering".
    const vec3 p1 = vert_in[1].position;
    const vec3 p2 = vert_in[2].position;
    vec3 p3 = vert_in[0].position;
    vec3 p4 = vert_in[3].position;
    if ( p3 == p1 ) {
        p3 = p1 - (p2-p1);
    }
    if ( p4 == p2 ) {
        p4 = p2 + (p2-p1);
    }


    // Conventions: - Index 1 for bottom, 2 for top, where applicable (!)
    //              - e1 for tangent, e2 for normal (in the direction of the curvature) and e3 for binormal
    //              - Right-hand systems
    //
    // "top"
    //
    //      p4  o   ^
    //          |   |
    //  u=1 p2  o   m2  ^
    //          |       |
    //  u=0 p1  o       m1
    //          |
    //      p3  o
    //
    // "bottom"

    vec3 m1            = 0.5*(p2-p3);
    vec3 m2            = 0.5*(p4-p1);

    // ----------------------------------------- Finding frames for the ends of the segment ------------------------------------------------------
    vec3 e2_top, e2_bottom; // Will be normalized
    {
        vec3 binormal_top, binormal_bottom;
        // p2-p1 is top-point - bottom-point ("secant")
        // m2: "Next p2-p1", i.e., p4-p2, i.e., spine tangent at top ("tangent") (Note: Averaged over two segments!)
        // Note that since m2 = "m1 of segment above us" = avg(p4-p2, p2-p1), we can test on parallelism of p4-p2 and p2-p1 both here, and in "the other segment".
        float cosangle = dot(p4-p2, p2-p1) / (length(p4-p2)*length(p2-p1));
        if ( cosangle > cos(radians(1.0)) ) {
            // p2-p1 and m2 (*and* p1-p3 and m1 of above segment!) are parallel, we must pick some other binormal.
            binormal_top = most_perpendicular_axis( m2 ); // NB! Using this ensures the same result for the adjoining segment
        } else {
            binormal_top = cross(p2-p1, m2);
        }
        e2_top = normalize( cross(binormal_top, m2) ); // Approximation to normalized Frenet normal (should be equal to e2_bottom of adjacent segment)

        cosangle = dot(p2-p1, p1-p3) / (length(p2-p1)*length(p1-p3));
        if ( cosangle > cos(radians(1.0)) ) {
            // Again, the two vectors are parallel.
            binormal_bottom = most_perpendicular_axis(m1); // NB! Using this ensures the same result for the adjoining segment
        } else {
            binormal_bottom = cross(m1, p2-p1);
        }
        e2_bottom = normalize( cross(binormal_bottom, m1) ); // Approximation to normalized Frenet normal (should be equal to e2_top of adjacent segment)

    }

    // Since we cannot read from the params struct, we need temporary variables:
    vec4 rec_ref_e2[P42_VEC4S];

    // Splitting the interval with a rolled-out traversing in the follwoing manner. Values shown are parameters for interval splitting.
    //
    //        [0, 1]                        Indices: 0, intervals
    //          |
    //         1/2              Level 1     intervals/2
    //       /     \
    //     1/4     3/4                2     1*intervals/4, 3*intervals/4
    //     / \     /  \
    //  1/8  3/8  5/8  7/8            3     1*intervals/8, 3*intervals/8, 5*intervals/8, 7*intervals/8
    // ...

    const int levels = min(TUBE_REFINEMENT_S, REC_REF_LEVELS);

    rec_ref_e2[0].xy                         = cart_to_sph(e2_bottom);
    const ivec2 p42_last = p42_index_and_offset( 1<<levels );
    rec_ref_e2[ p42_last.x ][ p42_last.y   ] = cart_to_sph(e2_top).x;
    rec_ref_e2[ p42_last.x ][ p42_last.y+1 ] = cart_to_sph(e2_top).y;

    for (int i=0; i<levels-1; i++) {
        const int splits = 1<<i;
        for (int j=0; j<splits; j++) {
            // The optimizer didn't see these by itself.
            const int tmp1 = ( 1<<(levels-i) ) * j;
            const int tmp2 =   1<<(levels-i);
            compute_frame_in_middle( rec_ref_e2, p1, m1, p2, m2, (j*2.0+1.0)/(2.0*splits), tmp1, tmp1 + tmp2, tmp1 + tmp2/2, i );
        }
    }

    // So far all invocations have done the same work. Now, the last level, we split it between all four invocations.
    // This way, a lot of work (half of the splittings) is repeated by all invocations, but the last splitting loop
    // will be divided equally among all of them. So, if the first half of the work runs in parallel, and does not
    // cause any hold-ups due to the stealing of resources from somewhere else, total new time should be 0.5 + 0.25 =
    // 0.75 of the old version. For a given case, s=5, fps went from 1159 to 1433 (1159 is 81% of new, or 25% more fps)
    // so this is not bad. Seems that a lot of the total time is actually spent in the TCS, then?! Could be interesting
    // to split the work even better, and also avoid redundant computations maybe, if possible.

    // (Will only work if levels>=3, and should only be done for the last splitting loop, since we don't share results between invocations.)
    if ( levels > 2 ) {
        // The remaining thing to do is the "splitting loop" for i==levels-1.
        const int i = levels - 1 ;
        const int splits = 1<<i;
        const int j0 =  gl_InvocationID   *(1<<(i-2));
        const int j1 = (gl_InvocationID+1)*(1<<(i-2));
        for (int j=j0; j<j1; j++) {
            const int tmp1 = ( 1<<(levels-i) ) * j;
            const int tmp2 =   1<<(levels-i);
            compute_frame_in_middle( rec_ref_e2, p1, m1, p2, m2, (j*2.0+1.0)/(2.0*splits), tmp1, tmp1 + tmp2, tmp1 + tmp2/2, i );
        }
        // Finally, we transfer the variables to the write-only-struct.
        // Note that we cannot split the whole P42_VEC4S-array in four, since all computed stuff (by all invocations) may actually reside in the first part of it!
        const int num_of_vec4s = 1<<i;
        const int i0 =  gl_InvocationID   *num_of_vec4s/4;
        const int i1 = (gl_InvocationID+1)*num_of_vec4s/4;
        // Again we must be careful about the 2-in-1-packing. We have a multiple of 4 + 1 values to copy, so some
        // element (first or last) has to be treated specially. We do this with the last element, which resides in the
        // first two components of a vec4 on its own.
        if ( gl_InvocationID == 0 ) {
            params.rec_ref_e2[num_of_vec4s] = rec_ref_e2[num_of_vec4s];
        }
        for (int i=i0; i<i1; i++) {
            params.rec_ref_e2[i] = rec_ref_e2[i];
        }
    } else {
        // Fallback to single-invocation version

        // The remaining thing to do is the "splitting loop" for i==levels-1.
        const int i = levels - 1 ;
        const int splits = 1<<i;
        for (int j=0; j<splits; j++) {
            // The optimizer didn't see these by itself.
            const int tmp1 = ( 1<<(levels-i) ) * j;
            const int tmp2 =   1<<(levels-i);
            compute_frame_in_middle( rec_ref_e2, p1, m1, p2, m2, (j*2.0+1.0)/(2.0*splits), tmp1, tmp1 + tmp2, tmp1 + tmp2/2, i );
        }
        if ( gl_InvocationID == 0 ) {
            // Finally, we transfer the variables to the write-only-struct.
            for (int i=0; i<P42_VEC4S; i++) {
                params.rec_ref_e2[i] = rec_ref_e2[i];
            }
        }
    }



    // No need for synchronizing, since we are not going to use results from other invocations here.



    if ( gl_InvocationID == 0 ) {

#if 1
        // Around the tube:
        gl_TessLevelOuter[0] = TUBE_CIRCUMFERENCE_REFINEMENT;
        gl_TessLevelOuter[2] = TUBE_CIRCUMFERENCE_REFINEMENT;
        gl_TessLevelInner[1] = TUBE_CIRCUMFERENCE_REFINEMENT;

        // Along the segment axis/spine:
        gl_TessLevelOuter[1] = TUBE_AXIS_REFINEMENT;
        gl_TessLevelOuter[3] = TUBE_AXIS_REFINEMENT;
        gl_TessLevelInner[0] = TUBE_AXIS_REFINEMENT;
#else
        // From Chris' shader:
        gl_TessLevelInner[ 0 ] = 40;
        gl_TessLevelInner[ 1 ] = 15;
        gl_TessLevelOuter[ 0 ] = 15;
        gl_TessLevelOuter[ 1 ] = 40;
        gl_TessLevelOuter[ 2 ] = 15;
        gl_TessLevelOuter[ 3 ] = 40;
#endif

        // In order to mimick the color selection of the old shader:
        params.color = vert_in[1].color;

    }

}
