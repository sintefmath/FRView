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
 * along with the FRView.  If not, see <http://www.gnu.org/licenses/>.
 */


in GO {
    flat vec4    color;
#ifdef DO_PAINT
    flat vec3    boundary_a;
    flat vec3    boundary_b;
    flat vec3    boundary_c;
#endif
    smooth vec3  normal;
    smooth vec3  obj_pos;
} in_f;

uniform vec4            edge_color;
uniform float line_width = 1.0;

// area of circular segment:
// acos(1-(x+1)) - (1-(x+1))*sqrt(2*(x+1)-(x+1)*(x+1))
// approximate differences of this using a linear function
float
lineCover( const in vec2 p, const in vec3 line, const float w )
{
    float h = abs(dot( p, line.xy ) + line.z) + 1.0;
    float h0 = h + w;
    float h1 = h - w;


    return clamp( h + w, 0.0, 1.0 ) - clamp( h - w, 0.0, 1.0 );
}

vec4 colorize()
{
    vec3 v = normalize( vec3( 0.f, 0.f, 1.5f ) - in_f.obj_pos ); // towards eye
    vec3 l = normalize( vec3( 0.f, 1.5f, 1.5f ) - in_f.obj_pos );  // towards light
    vec3 h = normalize( v + l );                  // half-vector
    vec3 n = normalize( in_f.normal );
    float d = max( 0.3f, dot(n,l) );
    float s = pow( max( 0.f, dot(n,h) ), 50.f );

    vec4 color = in_f.color;
    color.rgb = clamp( d*color.rgb /* + vec3(s)*/, vec3(0.f), vec3(1.f) );

#ifdef DO_PAINT
    if( edge_color.w > 0.f ) {
#if 1
        float ca = lineCover( gl_FragCoord.xy, in_f.boundary_a, line_width );
        float cb = lineCover( gl_FragCoord.xy, in_f.boundary_b, line_width );
        float cc = lineCover( gl_FragCoord.xy, in_f.boundary_c, line_width );

        float c = max( ca, max( cb, cc ) );
        if( c > 0.f ) {
            float alpha = edge_color.w *c;

            color.rgb = mix( color.rgb, edge_color.rgb, alpha );
            color.a += alpha;
            //  color = mix( vec4(alpha*edge_color.rgb, alpha), color, 0.1*alpha );

//            color += vec4( alpha*edge_color.rgb, alpha );
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
