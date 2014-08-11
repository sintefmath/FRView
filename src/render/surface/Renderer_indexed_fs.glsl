#version 420
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


in GO {
    flat bool    two_sided;
    flat vec4    color;
#ifdef DO_PAINT
    flat vec3    boundary_a;
    flat vec3    boundary_b;
    flat vec3    boundary_c;
#endif
    smooth vec3  normal;
    smooth vec3  obj_pos;
} in_f;

uniform vec4   edge_color;
uniform float  line_width = 0.5; // actually half the width of the line.
uniform bool            solid_pass;

/** Analytical anti-aliasing: determine the line coverage of a pixel
  *
  * For simplicity, we assume that the pixel is circular, and has unit area,
  * that is, R = sqrt( 1/PI ). We find the distance from the pixel center to the
  * line. Then given a line width, we determine the area of the intersection
  * between this circle and the thick line.
  */
float
lineCover( const in vec2 p, const in vec3 line, const float width )
{
    // Pixel radius.
    float R = sqrt( 1.f/3.1415926535897932384626433832795f );
    
    // Distance between pixel center and line, always positive.
    float d = abs( dot(p, line.xy ) + line.z );

    // In: smallest distance, may be negative.
    float ei = d - width;
    if( ei >= R ) {
        // triangle interior, fast path
        return 0.0f;
    }
    else {
        float ai = abs(ei);
        ai = R*R*acos( min( 1.f, ai/R) ) - ai*sqrt( max( 0.f, R*R - ai*ai) );
        ei = ei > 0.f ? ai : 1.f - ai;
        
        // Out: greatest distance, always positive
        float eo = d + width;
        eo = R*R*acos( min( 1.f, eo/R) ) - eo*sqrt( max( 0.f, R*R - eo*eo) );
        
        return ei - eo;
    }
}

vec4 colorize()
{
    vec3 v = normalize( vec3( 0.f, 0.f, 1.5f ) - in_f.obj_pos ); // towards eye
    vec3 l = normalize( vec3( 0.f, 1.5f, 1.5f ) - in_f.obj_pos );  // towards light
    vec3 h = normalize( v + l );                  // half-vector
    vec3 n = normalize( in_f.normal );
    if( in_f.two_sided && !gl_FrontFacing ) {
        n = -n;
    }
    
    float d = max( 0.3f, dot(n,l) );
    float s = pow( max( 0.f, dot(n,h) ), 50.f );

    vec4 color = in_f.color;
    color.rgb = clamp(
#ifdef SHADING_DIFFUSE_COMPONENT
                        d*color.rgb
#else
                        color.rgb
#endif
#ifdef SHADING_SPECULAR_COMPONENT
                        + vec3(color.a*s)
#endif
                        , vec3(0.f), vec3(1.f) );

#ifdef DO_PAINT
    if( edge_color.w > 0.f ) {
#if 1
        float ca = lineCover( gl_FragCoord.xy, in_f.boundary_a, line_width );
        float cb = lineCover( gl_FragCoord.xy, in_f.boundary_b, line_width );
        float cc = lineCover( gl_FragCoord.xy, in_f.boundary_c, line_width );

        float c = edge_color.w*max( ca, max( cb, cc ) );
        if( c > 0.f ) {
            color.rgb = c*edge_color.rgb + (1.f-c)*color.rgb;
            color.a   = color.a + c - color.a*c;
        }
#else
        float line = min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_a )),
                         min(abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_b )),
                             abs(dot( vec3( gl_FragCoord.xy, 1.f ), in_f.boundary_c )) ) );

        if( line < line_width ) {
            float alpha = edge_color.w *max(0.0, 1.0-(1.0/line_width)*line);
            color += vec4( alpha*edge_color.rgb, alpha );
            //color += vec4(edge_color*max(0.f, 1.0-line ));
        }
#endif
    }
#endif
    return color;
}
