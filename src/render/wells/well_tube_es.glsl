#version 420 core
#extension GL_EXT_gpu_shader4 : enable


uniform mat4    projection;
uniform mat4    modelview;

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
    float u0 = 2.0*u*u*u - 3.0*u*u + 1.0;	// h00(u)
    float u1 = u*u*u - 2.0*u*u + u;     	// h10(u)
    float u2 = -2.0*u*u*u + 3.0*u*u;		// h01(u)
    float u3 = u*u*u - u*u;                 // h11(u)
    c = u0*p1 + u1*m1 + u2*p2 + u3*m2;		// h00(u)*p1 + h10(u)*m1 + h01(u)*p2 + h11(u)*m2

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




// Should only be used in the control shader in the end, but it is useful to have for debugging purposes in the eval shader as well.

vec3 find_middle_pseudonormal_w_normalized_input( in vec3 e1_new, in vec3 e2_below, in vec3 e2_above,
                                                  in int lvl, // If we are splitting [0, 1], lvl==0, on next level, lvl==1, and so on. Will be < s and < REC_REF_LEVELS when
                                                              // called from TCS. When called from TES, will be equal to min(s, REC_REF_LEVELS).
                                                  in bool called_from_TES = false )
{
    // We split the interval in half, and compute *_new from *_bot and *_top values.

    // For vectors close to each other, we would like to use something in between, and we choose the average. For
    // vector far from each other (the extreme being two vectors cancelling each other) the average will not make much
    // sense, so instead we use something perpendicular to both the tangent and "roughly the normal-direction".
    // Hopefully, this will give a good "twist distribution".

    float cos_angle = dot(e2_above, e2_below); // Remember that cos(-x) = cos(x), and we get -1 <= cos_angle <= 1.

    // case 0: with 120 deg threshold we get into "cancelling" branch

    vec3 e2_new;

    if ( cos_angle < cos(radians(30.0)) ) { // <=> angle > 30 deg

        e2_new = cross(e1_new, e2_below-e2_above); // Note the subtraction!

        // Hmm... Sometimes e2_above-e2_below is much better. But is it possible to determine which one is best while
        // we are here? Yes, we choose the one giving a result more similar to the average direction than otherwise.
        // (If they cancel exactly, the average vanishes, but then the choice doesn't matter so mush either!)
        //
        // If e2_above x e2_below more or less point in the same direction as e1_new, we go for the e2_above-e2_below
        // choice.

        if ( dot(e1_new, cross(e2_below, e2_above)) < 0.0 ) { // No need to normalize when comparing to zero
            e2_new = -e2_new; // This gives cross(e1_new, e2_above-e2_below))
        } // (Faster than mult with sign(...).)

    } else {

        // Relatively similar directions, the middle should be a good choice. Note that SLERP is equal for u=0.5.

        e2_new = 0.5*(e2_below+e2_above);
        // e2_new = slerp( e2_below, e2_above, 0.5 );

    }

    // Must now test if this guess (most of the time the average of the below and above e3s) is more or less parallel to e1
    // 131215: Hmm... If the stuff above works as intended, we should not really get into this situation. Disabling it.
    // 131218: Was that correct? Why can this not happen? Putting it back in.
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
    return vec2( acos(v.z), atan(v.y, v.x) );
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




void compute_frame_in_middle( inout vec4 rec_ref_e2[P42_VEC4S],
                              in vec3 p1, in vec3 m1, in vec3 p2, in vec3 m2, in float u,
                              in int indx_bot, in int indx_top,
                              in int indx_new, in int lvl ) // If we are splitting [0, 1], lvl==0, on next level, lvl==1, and so on. Will be < s and < REC_REF_LEVELS.
                                                            // This is not called from the TES. That one calls find_middle_pseudonormal_w_normalized_input directly.
{
    const vec3 e1 = normalize( hermite_curve_tangent(p1, m1, p2, m2, u) );
    vec2 e2_bot, e2_top;
    if ( (indx_bot & 1) == 0 ) {
        e2_bot = rec_ref_e2[ indx_bot >> 1 ].xy;
    } else {
        e2_bot = rec_ref_e2[ indx_bot >> 1 ].zw;
    }
    if ( (indx_top & 1) == 0 ) {
        e2_top = rec_ref_e2[ indx_top >> 1 ].xy;
    } else {
        e2_top = rec_ref_e2[ indx_top >> 1 ].zw;
    }

    const vec3 pseudo_e2 = find_middle_pseudonormal_w_normalized_input( e1, sph_to_cart(e2_bot), sph_to_cart(e2_top), lvl );
    // The frame the way it will be computed in the eval shader. We need this for the angle-testing (and
    // other computations?) for the deeper levels of the tree, to be computed below.
    const vec3 e3 = normalize(cross(e1, pseudo_e2));

    if ( (indx_new & 1) == 0 ) {
        rec_ref_e2[ indx_new >> 1 ].xy = cart_to_sph(cross(e3, e1));
    } else {
        rec_ref_e2[ indx_new >> 1 ].zw = cart_to_sph(cross(e3, e1));
    }
}


in VF_post_control_shader
{
    vec3 pos;
} vert_in[];


out EF {
    vec2 param;
    vec3 normal;
    vec3 color;
} vert_out;


patch in TUBE_PARAMETERS params;


layout(quads, equal_spacing, ccw) in;




void main()
{
    vert_out.color = vec3(0.0, 1.0, 0.0);
    float u=gl_TessCoord.x, v=gl_TessCoord.y;
    //vert_out.u = u+gl_PrimitiveID;
    vert_out.param = vec2(u+gl_PrimitiveID, v); // Addition of gl_PrimitiveID gives a "global" parameter for the whole tube.

    // In order to mimick the color selection of the old shader:
    vert_out.color = params.color;

    // Note the ordering: Adaption of "original ordering" to "FRView's ordering".
    const vec3 p1 = vert_in[1].pos;
    const vec3 p2 = vert_in[2].pos;
    vec3 p3 = vert_in[0].pos;
    vec3 p4 = vert_in[3].pos;
    if ( p3 == p1 ) {
        p3 = p1 - (p2-p1);
    }
    if ( p4 == p2 ) {
        p4 = p2 + (p2-p1);
    }

    float tube_r = TUBE_RADIUS;
    float al     = v*2.0*PI;
    vec3 m1      = 0.5*(p2-p3);
    vec3 m2      = 0.5*(p4-p1);


    // --------------------------- c and e1 will become spine point and tangent, respectively -------------------------------------
    vec3 c;
    vec3 e1 = normalize( hermite_curve_eval(p1, m1, p2, m2, u, c) );    // Result in c


    // ----------------------------------------- "recursive-algo"-test -----------------------------------------------
    // (Old names and comments are outdated... recursion removed...)
    vec3 e2, e3;
    {
        const int levels = min(TUBE_REFINEMENT_S, REC_REF_LEVELS);
        const int intervals = 1<<levels;

        // Now jumping straight to the frames for which u is between.
        int indx = int(floor(u*intervals));
        if (indx==intervals)
            indx = intervals - 1;
        float uu = u*intervals - indx;
        // Using the end point frames to first make pseudo_e2, then e3, then e2.

        // Now we have the frame below in rec_ref_e?[indx] and the one above in rec_ref_e?[indx_top], and uu in [0, 1] for this interval.
        // We already have the exact e1.
                                                    // 0, 1, 2, 3, 4,  5, ... indx
        int float_indx = 2*indx;                    // 0, 2, 4, 6, 8, 10, ...
        int vec4_indx = float_indx/4;               // 0, 0, 1, 1, 2,  2, ...
        int vec4_offs = float_indx - 4*vec4_indx;   // 0, 2, 0, 2, 0,  2, ...
        vec2 e2_bot=vec2(0.0), e2_top=vec2(0.0);
        switch (vec4_offs) {
            case 0: e2_bot = params.rec_ref_e2[vec4_indx].xy;
                    e2_top = params.rec_ref_e2[vec4_indx].zw;
                    break;
            case 2: e2_bot = params.rec_ref_e2[vec4_indx  ].zw;
                    e2_top = params.rec_ref_e2[vec4_indx+1].xy;
                    break;
        }
        // ------------------------------------------------------------------------------------------------
//#ifdef TOGGLE_10
#if 0
        {
            // test 140101
            // Benchmark w/torus on bombadil:   s=3         s=4         s=5
            //                                  s=2 + T10   s=3 + T10   s=4 + T10
            //                              -----------------------------------------
            //                                  1500        1388        1304
            //                                  1421 (-5%)  1400 (+1%)  1208 (-7%)
            const float u_middle = (indx+0.5)/float(intervals);
            const vec3 e1_middle = normalize( hermite_curve_tangent(p1, m1, p2, m2, u_middle) );
            const vec3 pseudo_e2_middle = find_middle_pseudonormal_w_normalized_input( e1_middle, sph_to_cart(e2_bot), sph_to_cart(e2_top), min(s, REC_REF_LEVELS), true );
            const vec3 e3_middle = normalize( cross(e1_middle, pseudo_e2_middle) );
            const vec2 e2_middle = cart_to_sph( cross(e3_middle, e1_middle) );
            if ( uu < 0.5 ) {
                e2_top = e2_middle;
                uu = 2.0*uu;
            } else {
                e2_bot = e2_middle;
                uu = 2.0*(uu-0.5);
            }
        }
#endif
//#endif
        // ------------------------------------------------------------------------------------------------
        e2 = slerp( sph_to_cart(e2_bot), sph_to_cart(e2_top), uu );

        // Remember that this e2, even if the slerp end-points are normals, is not necessary a normal itself. (Length should be one, though.)
        // Maybe (hopefully) a good approximation, but still, nothing more.
        // 131209: And with the new bugfix: It is not a normal, just a preferred direction for a normal.
        e3 = normalize(cross(e1, e2)); // In the plane perpendicular to e1, so it is a binormal for som unknown normal, which is now to be computed:
        e2 = cross(e3, e1);
    }


    vec3 pos = c + tube_r*cos(al)*e2 + tube_r*sin(al)*e3;


    // Using object space, so now must convert to eye space.
    const vec4 es_pos = modelview * vec4( pos, 1.0 );
    gl_Position = projection * es_pos;

    // Is this the proper way to do it? Except that NM should be a uniform... (fps decrease compared to using mat3(MV) is just 1%...)
    const vec3 nrm = normalize( pos - c );
    mat3 NM = transpose(inverse(mat3(modelview)));
    vert_out.normal = NM*nrm;
}
