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

#include <algorithm>
#include "utils/Logger.hpp"
#include "dataset/VtkXmlParser.hpp"

namespace {

float dummy_vertices[3*4] = {
  0.f, 0.f, 0.f,
  1.f, 0.f, 0.f,
  0.f, 1.f, 0.f,
  0.f, 0.f, 1.f
};

int dummy_tetras[4*1] = {
    0, 1, 2, 3
};

}


void
VtkXmlParser::parse(std::vector<float>&         vertices,
                     std::vector<int> &tetrahedra,
                     const std::string &filename)
{
    vertices.resize( sizeof(dummy_vertices)/sizeof(float) );
    std::copy_n( &dummy_vertices[0], vertices.size(), vertices.begin() );
    
    tetrahedra.resize( sizeof(dummy_tetras)/sizeof(int) );
    std::copy_n( &dummy_tetras[0], tetrahedra.size(), tetrahedra.begin() );
}


