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

#include <sstream>
#include "utils/Logger.hpp"
#include "dataset/AbstractDataSource.hpp"
#include "job/SourceItem.hpp"

void
SourceItem::setName( const  std::vector<boost::shared_ptr<SourceItem> >& sources )
{
    std::string stem = m_source->name();
    if( stem.empty() ) {
        stem = "unnamed";
    }
    
    std::string candidate = stem;

    bool done = false;
    for(int instance=1; instance<1000 && !done; instance++) {

        done = true;
        for( size_t i=0; i<sources.size(); i++ ) {
            if( sources[i]->m_name == candidate ) {
                std::stringstream o;
                o << stem << " (" << (instance+1) << ")";
                candidate = o.str();
                done = false;
                break;
            }
        }
    }
    if( done ) {
        m_name = candidate;
    }
    else {
        Logger log = getLogger( "SourceItem.setName" );
        LOGGER_FATAL( log, "Failed to set name of source item '" << stem << "'." );
    }
    
}
    
    