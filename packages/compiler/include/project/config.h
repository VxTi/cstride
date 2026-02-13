#pragma once

#include <memory>
#include <string>

namespace stride::project
{
    struct Config
    {

    };

    std::unique_ptr<Config> read_config(const std::string& path);
}