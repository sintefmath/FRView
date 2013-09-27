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

#include <fcntl.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include "utils/Logger.hpp"
#include "FooBarParser.hpp"
#include "render/GridField.hpp"

void
FooBarParser::parseGeometry( unsigned int&         nx,
                             unsigned int&         ny,
                             unsigned int&         nz,
                             std::vector<float>&   coord,
                             std::vector<float>&   zcorn,
                             std::vector<int>&     actnum,
                             const std::string&    filename )
{
    Logger log = getLogger( "FooBarParser.parseGeometry" );

    LOGGER_DEBUG( log, "Opening '" << filename << "'" );

    int fd = open( filename.c_str(), O_RDONLY );
    if( fd < 0 ) {
        throw std::runtime_error( "Error opening " + filename );
    }

    struct stat finfo;
    if( fstat( fd, &finfo ) != 0 ) {
        throw std::runtime_error( "Error stat'ing " + filename );
    }
    LOGGER_DEBUG( log, "file.size = " << finfo.st_size );

    if( finfo.st_size < 16 ) {
        throw std::runtime_error( "File smaller than header " + filename );
    }

    std::vector<int> header(4);
    if( read( fd, header.data(), sizeof(int)*4) < (int)sizeof(int)*4) {
        throw std::runtime_error( "Error reading header " + filename );
    }
    if( header[0] < 1 ) {
        throw std::runtime_error( "nx < 1" );
    }
    if( header[1] < 1 ) {
        throw std::runtime_error( "ny < 1" );
    }
    if( header[2] < 1 ) {
        throw std::runtime_error( "nz < 1" );
    }

    nx = header[0];
    ny = header[1];
    nz = header[2];
    LOGGER_DEBUG( log, "nx=" << nx << ", ny=" << ny << ", nz=" << nz );

    ssize_t bytes;
    std::vector<double> tmpd;
    std::vector<int> tmpi;

    // coord
    tmpd.resize( 6*(nx+1)*(ny+1) );
    bytes = read( fd, tmpd.data(), sizeof(double)*tmpd.size() );
    if( bytes < 0 || (size_t)bytes < sizeof(double)*tmpd.size() ) {
        throw std::runtime_error( "Premature end of file reading coord" );
    }
    coord.resize( tmpd.size() );
    std::copy( tmpd.begin(), tmpd.end(), coord.begin() );

    // zcorn
    tmpd.resize( 2*nx*2*ny*2*nz );
    bytes = read( fd, tmpd.data(), sizeof(double)*tmpd.size() );
    if( bytes < 0 || (size_t)bytes < sizeof(double)*tmpd.size() ) {
        throw std::runtime_error( "Premature end of file reading coord" );
    }
    zcorn.resize( tmpd.size() );
    std::copy( tmpd.begin(), tmpd.end(), zcorn.begin() );

    tmpd.clear();

    // actnum

    actnum.resize( nx*ny*nz );
    bytes = read( fd, actnum.data(), sizeof(int)*actnum.size() );
    if( bytes == 0 ) {
        std::fill( actnum.begin(), actnum.end(), 1 );
    }
    else if( bytes == (ssize_t)(sizeof(int)*actnum.size()) ) {
        // ok
    }
    else {
        throw std::runtime_error( "Error reading actnum" );
    }

}

void
FooBarParser::parseTxtGeometry( unsigned int&         nx,
                                unsigned int&         ny,
                                unsigned int&         nz,
                                std::vector<float>&   coord,
                                std::vector<float>&   zcorn,
                                std::vector<int>&     actnum,
                                const std::string&    filename )
{
    Logger log = getLogger( "FooBarParser.parseTxtGeometry" );

    std::ifstream in( filename.c_str() );
    if(!in.good()) {
        throw std::runtime_error( filename + ": failed to open" );
    }
    in >> nx >> ny >> nz;
    LOGGER_DEBUG( log, "nx=" << nx << ", ny=" << ny << ", nz=" << nz );
    if( nx < 1 ) {
        throw std::runtime_error( "nx < 1" );
    }
    if( ny < 1 ) {
        throw std::runtime_error( "ny < 1" );
    }
    if( nz < 1 ) {
        throw std::runtime_error( "nz < 1" );
    }

    coord.resize( 6*(nx+1)*(ny+1) );
    for(auto it=coord.begin(); it!=coord.end(); ++it ) {
        in >> *it;
    }
    LOGGER_DEBUG( log, "coord.size = " << coord.size() );

    zcorn.resize( 2*nx*2*ny*2*nz );
    for( auto it=zcorn.begin(); it!=zcorn.end(); ++it ) {
        in >> *it;
    }
    LOGGER_DEBUG( log, "zcorn.size = " << zcorn.size() );

    actnum.resize( nx*ny*nz );
    std::fill( actnum.begin(), actnum.end(), 1 );
}



GridField*
FooBarParser::parseField( GridTess* tess, const std::string& filename )
{
    throw std::runtime_error( "Need global to local map, codepath missing." );
    return NULL;
/*
    Logger log = getLogger( "FooBarParser.parseGeometry" );


    LOGGER_DEBUG( log, "Opening '" << filename << "'" );

    int fd = open( filename.c_str(), O_RDONLY );
    if( fd < 0 ) {
        LOGGER_ERROR( log, "Error opening '" << filename << "': " << strerror(errno)  );
        return NULL;
    }

    struct stat finfo;
    if( fstat( fd, &finfo ) != 0 ) {
        LOGGER_ERROR( log, "Error stat'ing' '" << filename << "': " << strerror(errno)  );
        return NULL;
    }
    LOGGER_DEBUG( log, "file.size = " << finfo.st_size );

    std::vector<double> body( finfo.st_size/sizeof(double) );
    if( read( fd, body.data(), sizeof(double)*body.size() ) < (ssize_t)(sizeof(double)*body.size()) ) {
        LOGGER_ERROR( log, filename << ": error reading body: " << strerror(errno) );
        return NULL;
    }

    //if( body.size() != tess->cells() ) {
    //    LOGGER_ERROR( log, filename << ": wrong number of elements, expected"
    //                  << tess->cells() << ", got " << body.size() );
    //    return NULL;
    //}

    std::vector<float> bodyf( tess->activeCells() );
    const std::vector<unsigned int>& map = tess->cellIndex();
    for(size_t i=0; i<map.size(); i++ ) {
        float value = std::log( body[ map[i] ] );
        bodyf[i] = value;
    }

    float min = bodyf[0];
    float max = bodyf[0];
    for( auto it=bodyf.begin(); it!=bodyf.end(); ++it ) {
        min = std::min( min, *it );
        max = std::max( max, *it );
    }
    LOGGER_DEBUG( log, "min = " << min << ", max = " << max );

    return new GridField( bodyf,
                          min,
                          max );

*/
}
