#pragma once
#include <fstream>
#include <string>

namespace stride
{
    typedef struct
    {
        const std::string& path;
        const std::string& source;
    } SourceFile;


     std::shared_ptr<SourceFile> read_file(const std::string& path);
}
