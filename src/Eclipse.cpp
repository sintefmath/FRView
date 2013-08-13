#include <stdexcept>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <ctype.h>
#include <cstring>
#include "Logger.hpp"
#include "Eclipse.hpp"

namespace Eclipse {
    using std::string;
    using std::vector;
    using std::list;


const string&
keywordString( Keyword keyword )
{
    switch( keyword ) {
#define CHECK( A, B ) case B: static const string A = #A; return A; break
#define CHUCK( A, B, C ) case B: static const string A = C; return A; break
    CHECK( DIMENS  , KEYWORD_DIMENS );
    CHECK( RADIAL  , KEYWORD_RADIAL );
    CHECK( BOXORIG , KEYWORD_BOXORIG );
    CHECK( MAPUNITS, KEYWORD_MAPUNITS );
    CHECK( MAPAXES , KEYWORD_MAPAXES );
    CHECK( GRIDUNIT, KEYWORD_GRIDUNIT );
    CHECK( COORD  ,  KEYWORD_COORD );
    CHECK( COORDSYS, KEYWORD_COORDSYS );
    CHECK( ZCORN,    KEYWORD_ZCORN );
    CHECK( ACTNUM,   KEYWORD_ACTNUM );
    CHECK( CORSNUM,  KEYWORD_CORSNUM );
    CHECK( HOSTNUM,  KEYWORD_HOSTNUM );

    CHECK( CORNERS , KEYWORD_CORNERS );
    CHECK( FILEHEAD, KEYWORD_FILEHEAD );
    CHECK( GRIDHEAD, KEYWORD_GRIDHEAD );
    CHECK( ENDGRID,  KEYWORD_ENDGRID );
    CHECK( SEQNUM  , KEYWORD_SEQNUM );
    CHECK( INTEHEAD, KEYWORD_INTEHEAD );
    CHECK( LOGIHEAD, KEYWORD_LOGIHEAD );
    CHECK( DOUBHEAD, KEYWORD_DOUBHEAD );
    CHECK( LGRNAMES, KEYWORD_LGRNAMES );
    CHECK( LGR     , KEYWORD_LGR );
    CHECK( LGRHEADI, KEYWORD_LGRHEADI );
    CHECK( LGRHEADQ, KEYWORD_LGRHEADQ );
    CHECK( LGRHEADD, KEYWORD_LGRHEADD );

    CHECK( IGRP,     KEYWORD_IGRP );
    CHECK( ISEG,     KEYWORD_ISEG );
    CHECK( IWEL,     KEYWORD_IWEL );
    CHECK( ZWEL,     KEYWORD_ZWEL );
    CHECK( ICON,     KEYWORD_ICON );
    CHECK( HIDDEN,   KEYWORD_HIDDEN );
    CHECK( ZTRACER,  KEYWORD_ZTRACER );
    CHECK( LGRJOIN,  KEYWORD_LGRJOIN );
    CHECK( ENDLGR,   KEYWORD_ENDLGR );

    CHECK( STARTSOL, KEYWORD_STARTSOL );
    CHUCK( PRESSURE, KEYWORD_PRESSURE,   "pressure" );
    CHUCK( SWAT,     KEYWORD_SWAT,       "water saturation" );
    CHUCK( SGAS,     KEYWORD_SGAS,       "gas saturation" );
    CHUCK( SOIL,     KEYWORD_SOIL,       "oil saturation" );
    CHUCK( RS,       KEYWORD_RS,         "gas-oil ratio" );
    CHUCK( RV,       KEYWORD_RV,         "oil-gas ratio" );
    CHUCK( OWC,      KEYWORD_OWC,        "oil-water contact" );
    CHUCK( OGC,      KEYWORD_OGC,        "oil-gas contact" );
    CHUCK( GWC,      KEYWORD_GWC,        "gas-water contact" );
    CHUCK( OILAPI,   KEYWORD_OILAPI,     "oil API values" );
    CHUCK( FIPOIL,   KEYWORD_FIPOIL,     "oil fluid-in-place" );
    CHUCK( FIPGAS,   KEYWORD_FIPGAS,     "Gas fluid-in-place" );
    CHUCK( FIPWAT,   KEYWORD_FIPWAT,     "Water fluid-in-place" );
    CHUCK( OIL_POTN, KEYWORD_OIL_POTN,   "Oil potential" );
    CHUCK( GAS_POTN, KEYWORD_GAS_POTN,   "Gas potential" );
    CHUCK( WAT_POTN, KEYWORD_WAT_POTN,   "Water potential" );
    CHUCK( POLYMER,  KEYWORD_POLYMER,    "Polymer concentrations" );
    CHUCK( PADS,     KEYWORD_PADS,       "Adsorbed polymer concentrations" );
    CHUCK( XMF,      KEYWORD_XMF,        "Liquid mole fractions" );
    CHUCK( YMF,      KEYWORD_YMF,        "Vapor mole fractions" );
    CHUCK( ZMF,      KEYWORD_ZMF,        "Total mole fractions" );
    CHUCK( SSOL,     KEYWORD_SSOL,       "Solvent saturation" );
    CHUCK( PBUB,     KEYWORD_PBUB,       "Bubble point pressure" );
    CHUCK( PDEW,     KEYWORD_PDEW,       "Dew-point pressure" );
    CHUCK( SURFACT,  KEYWORD_SURFACT,    "Surface interactions" );
    CHUCK( SURFADS,  KEYWORD_SURFADS,    "Adsorbed surfactant concentrations" );
    CHUCK( SURFMAX,  KEYWORD_SURFMAX,    "Maximum surfactant concentrations" );
    CHUCK( SURFCNM,  KEYWORD_SURFCNM,    "Surfactant capillary numbers" );
    CHUCK( GGI,      KEYWORD_GGI,        "GI injected gas ratio" );
    CHUCK( WAT_PRES, KEYWORD_WAT_PRES,   "Water phase pressure" );
    CHUCK( GAS_PRES, KEYWORD_GAS_PRES,   "Gas phase pressure" );
    CHUCK( OIL_VISC, KEYWORD_OIL_VISC,   "Oil viscosity" );
    CHUCK( WAT_VISC, KEYWORD_WAT_VISC,   "Water viscosity" );
    CHUCK( GAS_VISC, KEYWORD_GAS_VISC,   "Gas viscosity" );
    CHUCK( OIL_DEN,  KEYWORD_OIL_DEN,    "Oil density" );
    CHUCK( WAT_DEN,  KEYWORD_WAT_DEN,    "Water density" );
    CHUCK( GAS_DEN,  KEYWORD_GAS_DEN,    "Gas density" );
    CHUCK( DRAINAGE, KEYWORD_DRAINAGE,   "Drainage region numbers" );
    CHUCK( FLOOILIP, KEYWORD_FLOOILIP, "Oil phase flow in positive I direction" );
    CHUCK( FLOOILJP, KEYWORD_FLOOILJP, "Oil phase flow in positive J direction" );
    CHUCK( FLOOILKP, KEYWORD_FLOOILKP, "Oil phase flow in positive K direction" );
    CHUCK( FLOGASIP, KEYWORD_FLOGASIP, "Gas phase flow in positive I direction" );
    CHUCK( FLOGASJP, KEYWORD_FLOGASJP, "Gas phase flow in positive J direction" );
    CHUCK( FLOGASKP, KEYWORD_FLOGASKP, "Gas phase flow in positive K direction" );
    CHUCK( FLOWATIP, KEYWORD_FLOWATIP, "Water phase flow in positive I direction" );
    CHUCK( FLOWATJP, KEYWORD_FLOWATJP, "Water phase flow in positive J direction" );
    CHUCK( FLOWATKP, KEYWORD_FLOWATKP, "Water phase flow in positive K direction" );
    CHECK( ENDSOL, KEYWORD_ENDSOL );
#undef CHUCK
#undef CHECK
    case KEYWORD_UNKNOWN:
        static const string UNKNOWN = "<unknown>";
        return UNKNOWN;
        break;
    }
    static const string ERROR    = "<error>";
    return ERROR;
}

const string&
typeString( DataType type )
{
    static const string INTEGER = "INTEGER";
    static const string FLOAT   = "FLOAT";
    static const string BOOL    = "BOOL";
    static const string DOUBLE  = "DOUBLE";
    static const string STRING  = "STRING";
    static const string MESSAGE = "MESSAGE";
    static const string ERROR   = "<error>";
    switch( type ) {
    case TYPE_INTEGER: return INTEGER; break;
    case TYPE_FLOAT:   return FLOAT;   break;
    case TYPE_BOOL:    return BOOL;    break;
    case TYPE_DOUBLE:  return DOUBLE;  break;
    case TYPE_STRING:  return STRING;  break;
    case TYPE_MESSAGE: return MESSAGE; break;
    }
    return ERROR;
}

Keyword
keyword( const char* char8 )
{
#define CHECK( A, B ) else if( strncmp(A, char8, 8) == 0 ) return B
    switch( char8[0] ) {


    case 'A':
        if( false ){ }
        CHECK( "ACTNUM  ", KEYWORD_ACTNUM );
        break;
    case 'B':
        if( false ){ }
        CHECK( "BOXORIG ", KEYWORD_BOXORIG );
        break;
    case 'C':
        if( false ){ }
        CHECK( "COORD   ", KEYWORD_COORD );
        CHECK( "COORDSYS", KEYWORD_COORDSYS );
        CHECK( "CORNERS ", KEYWORD_CORNERS );
        CHECK( "CORSNUM ", KEYWORD_CORSNUM );
        break;
    case 'D':
        if( false ){ }
        CHECK( "DIMENS  ", KEYWORD_DIMENS );
        CHECK( "DOUBHEAD", KEYWORD_DOUBHEAD );
        CHECK( "DRAINAGE", KEYWORD_DRAINAGE );
        break;
    case 'E':
        if( false ){ }
        CHECK( "ENDGRID ", KEYWORD_ENDGRID );
        CHECK( "ENDSOL  ", KEYWORD_ENDSOL );
        CHECK( "ENDLGR  ", KEYWORD_ENDLGR );
        break;
    case 'F':
        if( false ){ }
        CHECK( "FILEHEAD", KEYWORD_FILEHEAD );
        CHECK( "FIPGAS  ", KEYWORD_FIPGAS );
        CHECK( "FIPOIL  ", KEYWORD_FIPOIL );
        CHECK( "FIPWAT  ", KEYWORD_FIPWAT );
        CHECK( "FLOGASI+", KEYWORD_FLOGASIP );
        CHECK( "FLOGASJ+", KEYWORD_FLOGASJP );
        CHECK( "FLOGASK+", KEYWORD_FLOGASKP );
        CHECK( "FLOOILI+", KEYWORD_FLOOILIP );
        CHECK( "FLOOILJ+", KEYWORD_FLOOILJP );
        CHECK( "FLOOILK+", KEYWORD_FLOOILKP );
        CHECK( "FLOWATI+", KEYWORD_FLOWATIP );
        CHECK( "FLOWATJ+", KEYWORD_FLOWATJP );
        CHECK( "FLOWATK+", KEYWORD_FLOWATKP );
        break;
    case 'G':
        if( false ){ }
        CHECK( "GAS_DEN ", KEYWORD_GAS_DEN );
        CHECK( "GAS-DEN ", KEYWORD_GAS_DEN );
        CHECK( "GAS_POTN", KEYWORD_GAS_POTN );
        CHECK( "GAS-POTN", KEYWORD_GAS_POTN );
        CHECK( "GAS_PRES", KEYWORD_GAS_PRES );
        CHECK( "GAS-PRES", KEYWORD_GAS_PRES );
        CHECK( "GAS_VISC", KEYWORD_GAS_VISC );
        CHECK( "GAS-VISC", KEYWORD_GAS_VISC );
        CHECK( "GGI     ", KEYWORD_GGI );
        CHECK( "GRIDHEAD", KEYWORD_GRIDHEAD );
        CHECK( "GRIDUNIT", KEYWORD_GRIDUNIT );
        CHECK( "GWC     ", KEYWORD_GWC );
        break;
    case 'H':
        if( false ){ }
        CHECK( "HOSTNUM ", KEYWORD_HOSTNUM );
        CHECK( "HIDDEN  ", KEYWORD_HIDDEN );
        break;
    case 'I':
        if( false ){ }
        CHECK( "INTEHEAD", KEYWORD_INTEHEAD );
        CHECK( "ICON    ", KEYWORD_ICON );
        CHECK( "IGRP    ", KEYWORD_IGRP );
        CHECK( "ISEG    ", KEYWORD_ISEG );
        CHECK( "IWEL    ", KEYWORD_IWEL );
        break;
    case 'L':
        if( false ){ }
        CHECK( "LGRHEADD", KEYWORD_LGRHEADD );
        CHECK( "LGRHEADI", KEYWORD_LGRHEADI );
        CHECK( "LGRHEADQ", KEYWORD_LGRHEADQ );
        CHECK( "LGR     ", KEYWORD_LGR );
        CHECK( "LGRNAMES", KEYWORD_LGRNAMES );
        CHECK( "LOGIHEAD", KEYWORD_LOGIHEAD );
        CHECK( "LGRJOIN ", KEYWORD_LGRJOIN );
        break;
    case 'M':
        if( false ){ }
        CHECK( "MAPAXES ", KEYWORD_MAPAXES );
        CHECK( "MAPUNITS", KEYWORD_MAPUNITS );
        break;
    case 'O':
        if( false ){ }
        CHECK( "OGC     ", KEYWORD_OGC );
        CHECK( "OILAPI  ", KEYWORD_OILAPI );
        CHECK( "OIL_DEN ", KEYWORD_OIL_DEN );
        CHECK( "OIL-DEN ", KEYWORD_OIL_DEN );
        CHECK( "OIL_POTN", KEYWORD_OIL_POTN );
        CHECK( "OIL-POTN", KEYWORD_OIL_POTN );
        CHECK( "OIL_VISC", KEYWORD_OIL_VISC );
        CHECK( "OIL-VISC", KEYWORD_OIL_VISC );
        CHECK( "OWC     ", KEYWORD_OWC );
        break;
    case 'P':
        if( false ){ }
        CHECK( "PADS    ", KEYWORD_PADS );
        CHECK( "PBUB    ", KEYWORD_PBUB );
        CHECK( "PDEW    ", KEYWORD_PDEW);
        CHECK( "POLYMER ", KEYWORD_POLYMER );
        CHECK( "PRESSURE", KEYWORD_PRESSURE);
        break;
    case 'R':
        if( false ){ }
        CHECK( "RADIAL  ", KEYWORD_RADIAL );
        CHECK( "RS      ", KEYWORD_RS );
        CHECK( "RV      ", KEYWORD_RV );
        break;
    case 'S':
        if( false ){ }
        CHECK( "SEQNUM  ", KEYWORD_SEQNUM );
        CHECK( "SGAS    ", KEYWORD_SGAS );
        CHECK( "SOIL    ", KEYWORD_SOIL );
        CHECK( "SSOL    ", KEYWORD_SSOL);
        CHECK( "STARTSOL", KEYWORD_STARTSOL );
        CHECK( "SURFACT ", KEYWORD_SURFACT );
        CHECK( "SURFADS ", KEYWORD_SURFADS );
        CHECK( "SURFCNM ", KEYWORD_SURFCNM );
        CHECK( "SURFMAX ", KEYWORD_SURFMAX );
        CHECK( "SWAT    ", KEYWORD_SWAT );
        break;
    case 'V':
        if( false ){ }
        CHECK( "VGAS    ", KEYWORD_GAS_VISC );
        CHECK( "VOIL    ", KEYWORD_OIL_VISC );
        CHECK( "VWAT    ", KEYWORD_WAT_VISC );
        break;
    case 'W':
        if( false ){ }
        CHECK( "WAT_DEN ", KEYWORD_WAT_DEN);
        CHECK( "WAT-DEN ", KEYWORD_WAT_DEN);
        CHECK( "WAT_POTN", KEYWORD_WAT_POTN );
        CHECK( "WAT-POTN", KEYWORD_WAT_POTN );
        CHECK( "WAT_PRES", KEYWORD_WAT_PRES );
        CHECK( "WAT-PRES", KEYWORD_WAT_PRES );
        CHECK( "WAT_VISC", KEYWORD_WAT_VISC );
        CHECK( "WAT-VISC", KEYWORD_WAT_VISC );
        break;
    case 'X':
        if( false ){ }
        CHECK( "XMF     ", KEYWORD_XMF );
        break;
    case 'Y':
        if( false ){ }
        CHECK( "YMF     ", KEYWORD_YMF );
        break;
    case 'Z':
        if( false ){ }
        CHECK( "ZCORN   ", KEYWORD_ZCORN );
        CHECK( "ZMF     ", KEYWORD_ZMF );
        CHECK( "ZTRACER ", KEYWORD_ZTRACER );
        CHECK( "ZWEL    ", KEYWORD_ZWEL );
        break;
    }
#undef CHECK
    return KEYWORD_UNKNOWN;
}


} // of namespace Eclipse
