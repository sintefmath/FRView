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


layout(location=0)  in      vec3 bbmin;
layout(location=1)  in      vec3 bbmax;
layout(location=2)  in      uint cell;

out GI {
    vec3 bbmin;
    vec3 bbmax;
    uint cell;
} gi;

void main()
{
    gi.bbmin = vec3(2.f,2.f,1.f)*bbmin - vec3(1.f,1.f,0.f);
    gi.bbmax = vec3(2.f,2.f,1.f)*bbmax - vec3(1.f,1.f,0.f);
//    gi.bbmin = vec3(2.f,1.f,1.f)*bbmin - vec3(1.f,1.f,0.f);
//    gi.bbmax = vec3(2.f,1.f,1.f)*bbmax - vec3(1.f,1.f,0.f);
    gi.cell  = cell;

}
