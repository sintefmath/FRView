#pragma once
#include "PrintExporter.hpp"

class TikZExporter : public PrintExporter
{
public:
    TikZExporter( GridTess* tess );


private:

    void
    format( std::vector<char>& buffer );
};
