#include "project/config.h"
#include <simdjson.h>

using namespace stride::project;
using namespace simdjson;

std::unique_ptr<Config> stride::project::read_config(const std::string& path)
{
    ondemand::parser parser;
    const padded_string json = padded_string::load(path);
    ondemand::document content = parser.iterate(json);


}
