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

#include "utils/Logger.hpp"
#include "utils/Path.hpp"

namespace {
    const std::string package = "utils.Path";
}

namespace utils {
namespace Path {

void
split( std::string& path,
       std::string& stem,
       std::string& suffix,
       const std::string& filename )
{
    Logger log = getLogger( package + ".split" );
    
    std::string rest;

    size_t dot = filename.find_last_of( '.' );
    if( dot == std::string::npos ) {
        suffix = "";
        rest = filename;
    }
    else {
        suffix = filename.substr( dot );
        rest = filename.substr( 0, dot );
    }
    
    size_t slash = filename.find_last_of( '/' );
    if( slash == std::string::npos ) {
        path = "";
        stem = rest;
    }
    else {
        path = rest.substr( 0, slash+1 );
        stem = rest.substr( slash+1 );
    }
    
    LOGGER_DEBUG( log,
                  "filename='" << filename << "', " <<
                  "path='" << path << "', " <<
                  "stem='" << stem << "', " <<
                  "suffix='" << suffix << "'.")
    
}




} // of namespace Path
} // of namespace utils
