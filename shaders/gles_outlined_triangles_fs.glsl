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

#ifdef GL_ES
precision highp float;
#endif
varying vec3 barycentric;
void
main()
{
    float d = min( barycentric.x, min( barycentric.y, barycentric.z) );
    float l = max(0.0, 1.0 - 5.0*d);
    gl_FragColor = vec4( 0.2+l, 0.2+l, 0.5+l, 1.0 );
}
