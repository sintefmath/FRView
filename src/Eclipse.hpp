#pragma once
#include <string>
#include <vector>
#include <list>
#include <boost/utility.hpp>
#include <boost/variant.hpp>
#include <unordered_map>

namespace Eclipse {

enum DataType {
    TYPE_INTEGER,    // 4-byte integer
    TYPE_FLOAT,       // 4-byte floating point
    TYPE_BOOL,    // 4-byte logical
    TYPE_DOUBLE,     // 8-byte floating point
    TYPE_STRING,
    TYPE_MESSAGE     // 0-byte message ?
};

enum Keyword {
    KEYWORD_DIMENS,
    KEYWORD_RADIAL,
    KEYWORD_BOXORIG,
    KEYWORD_MAPUNITS,
    KEYWORD_MAPAXES,
    KEYWORD_GRIDUNIT,
    KEYWORD_COORD,
    KEYWORD_COORDSYS,
    KEYWORD_ZCORN,
    KEYWORD_ACTNUM,
    KEYWORD_CORSNUM,
    KEYWORD_HOSTNUM,

    KEYWORD_CORNERS,
    KEYWORD_FILEHEAD,
    KEYWORD_GRIDHEAD,
    KEYWORD_ENDGRID,
    KEYWORD_SEQNUM,
    KEYWORD_INTEHEAD,
    KEYWORD_LOGIHEAD,
    KEYWORD_DOUBHEAD,
    KEYWORD_LGRNAMES,
    KEYWORD_LGR,
    KEYWORD_LGRHEADI,
    KEYWORD_LGRHEADQ,
    KEYWORD_LGRHEADD,

    KEYWORD_IGRP,
    KEYWORD_ISEG,
    KEYWORD_IWEL,
    KEYWORD_ZWEL,
    KEYWORD_ICON,
    KEYWORD_HIDDEN,
    KEYWORD_ZTRACER,
    KEYWORD_LGRJOIN,
    KEYWORD_ENDLGR,

    KEYWORD_STARTSOL,
    KEYWORD_PRESSURE,   // pressure
    KEYWORD_SWAT,       // water saturation
    KEYWORD_SGAS,       // gas saturation
    KEYWORD_SOIL,       // oil saturation
    KEYWORD_RS,         // gas-oil ratio
    KEYWORD_RV,         // oil-gas ratio
    KEYWORD_OWC,        // oil-water contact
    KEYWORD_OGC,        // oil-gas contact
    KEYWORD_GWC,        // gas-water contact
    KEYWORD_OILAPI,     // oil API values
    KEYWORD_FIPOIL,     // oil fluid-in-place
    KEYWORD_FIPGAS,     // Gas fluid-in-place
    KEYWORD_FIPWAT,     // Water fluid-in-place
    KEYWORD_OIL_POTN,   // Oil potential
    KEYWORD_GAS_POTN,   // Gas potential
    KEYWORD_WAT_POTN,   // Water potential
    KEYWORD_POLYMER,    // Polymer concentrations
    KEYWORD_PADS,       // Adsorbed polymer concentrations
    KEYWORD_XMF,        // Liquid mole fractions
    KEYWORD_YMF,        // Vapor mole fractions
    KEYWORD_ZMF,        // Total mole fractions
    KEYWORD_SSOL,       // Solvent saturation
    KEYWORD_PBUB,       // Bubble point pressure
    KEYWORD_PDEW,       // Dew-point pressure
    KEYWORD_SURFACT,    // Surface interactions
    KEYWORD_SURFADS,    // Adsorbed surfactant concentrations
    KEYWORD_SURFMAX,    // Maximum surfactant concentrations
    KEYWORD_SURFCNM,    // Surfactant capillary numbers
    KEYWORD_GGI,        // GI injected gas ratio
    KEYWORD_WAT_PRES,   // Water phase pressure
    KEYWORD_GAS_PRES,   // Gas phase pressure
    KEYWORD_OIL_VISC,   // Oil viscosity
    KEYWORD_WAT_VISC,   // Water viscosity
    KEYWORD_GAS_VISC,   // Gas viscosity
    KEYWORD_OIL_DEN,    // Oil density
    KEYWORD_WAT_DEN,    // Water density
    KEYWORD_GAS_DEN,    // Gas density
    KEYWORD_DRAINAGE,   // Drainage region numbers
    KEYWORD_FLOOILIP,   // Oil phase flow in positive I direction.
    KEYWORD_FLOOILJP,   // Oil phase flow in positive J direction.
    KEYWORD_FLOOILKP,   // Oil phase flow in positive K direction.
    KEYWORD_FLOGASIP,   // Gas phase flow in positive I direction.
    KEYWORD_FLOGASJP,   // Gas phase flow in positive J direction.
    KEYWORD_FLOGASKP,   // Gas phase flow in positive K direction.
    KEYWORD_FLOWATIP,   // Water phase flow in positive I direction.
    KEYWORD_FLOWATJP,   // Water phase flow in positive J direction.
    KEYWORD_FLOWATKP,   // Water phase flow in positive K direction.
    KEYWORD_ENDSOL,

    KEYWORD_UNKNOWN
};

enum Penetration
{
    PENETRATION_X,
    PENETRATION_Y,
    PENETRATION_Z,
    PENETRATION_FRACTURED_X,
    PENETRATION_FRACTURED_Y
};

enum WellType
{
    WELL_PRODUCER,
    WELL_OIL_INJECTION,
    WELL_WATER_INJECTION,
    WELL_GAS_INJECTION
};



typedef boost::variant<int,double,std::string> PropertyItem;
typedef std::unordered_map<std::string,PropertyItem> Properties;

struct Block {
    Keyword         m_keyword;
    DataType        m_datatype;
    unsigned int    m_typesize;
    unsigned int    m_count;
    unsigned int    m_record_size;
    unsigned int    m_records;
    size_t          m_offset;
    size_t          m_size;
    char            m_keyword_string[8];
};

struct Completion {
    int        m_i;
    int        m_j;
    int        m_k;
    int        m_segment;
    int        m_connection_index;
    bool       m_open;
    Penetration m_penetration;
};

struct Well {
    int                m_head_i;
    int                m_head_j;
    int                m_head_k;
    int                m_group;
    WellType                    m_type;
    bool                        m_open;
    int                m_segmented_well_id;
    //unsigned int        m_completion_connections;
    std::vector<Completion>     m_completions;
    std::string                 m_name;
};

struct Solution {
    std::string         m_name;
    Block               m_location;
};

struct ReportStep {
    unsigned int        m_sequence_number;
    Properties          m_properties;
    std::list<Solution> m_solutions;
    std::vector<Well>   m_wells;
};


/** Given 8 chars of ASCII, return keyword enum. */
Keyword
keyword( const char* char8 );


/** Given a keyword enum, return a textual description. */
const std::string&
keywordString( Keyword keyword );

/** Given a data type enum, return a textual description. */
const std::string&
typeString( DataType type );



} // of namespace Eclipse
