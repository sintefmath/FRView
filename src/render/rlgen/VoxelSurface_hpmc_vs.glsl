#version 330
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


uniform mat4 mvp;

// Forward declaration of func provided by HPMC
void
extractVertex( out vec3 a, out vec3 b, out vec3 p, out vec3 n );

out vec3 out_pos;
out vec3 out_col;

uniform vec3            scale;
uniform vec3            shift;
uniform sampler3D       voxels;
#define VOXEL_SAMPLER_DEFINED

void
main()
{
    vec3 a, b, p, n;
    extractVertex( a, b, p, n );

    // Fetch color from voxel field. One end of the edge [a,b] on which the
    // intersection sits is inside the field (and has nonzero w).
    vec4 col = texture( voxels, a );
    if( col.w < 0.5 ) {
        col = texture( voxels, b );
    }

    // Since the field is binary, the normal vector has a limited set of
    // directions, and we encode this in the integer part of the position.
    vec3 an = abs( n );
    float s = 1.f/max( max( an.x, an.y ), an.z );   // taxicab-norm-scale
    vec3 ni = clamp( vec3(round(s*n)+vec3(2)), vec3(1.0), vec3(3.0));                       // shift zero to 2 (range should be [1,3])

    out_pos = vec3(clamp(scale*p+shift, vec3(0.0), vec3(0.9999)) + floor(ni));
    out_col = vec3( col );
}
