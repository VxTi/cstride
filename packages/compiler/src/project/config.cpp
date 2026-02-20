#include "project/config.h"

#include <simdjson.h>


namespace stride::project
{
    namespace
    {
        std::optional<Mode> parse_mode(std::string_view mode_str)
        {
            if (mode_str == "COMPILE_NATIVE")
                return Mode::COMPILE_NATIVE;
            if (mode_str == "COMPILE_JIT")
                return Mode::COMPILE_JIT;

            return std::nullopt;
        }

        std::optional<Dependency> parse_dependency(
            simdjson::simdjson_result<simdjson::ondemand::value> dep_val)
        {
            Dependency dep;

            if (dep_val["name"].get_string().get(dep.name) ||
                dep_val["version"].get_string().get(dep.version) ||
                dep_val["path"].get_string().get(dep.path))
            {
                std::cerr << "Invalid dependency format\n";
                return std::nullopt;
            }

            return dep;
        }
    } // namespace

    std::optional<Config> read_config(const std::string& path)
    {
        simdjson::ondemand::parser parser;

        simdjson::padded_string json;
        if (const auto load_error = simdjson::padded_string::load(path).
            get(json))
        {
            std::cerr << "Failed to load config file: " << load_error << "\n";
            return std::nullopt;
        }

        simdjson::ondemand::document doc;
        if (auto parse_error = parser.iterate(json).get(doc))
        {
            std::cerr << "Failed to parse JSON: " << parse_error << "\n";
            return std::nullopt;
        }

        Config config;

        // Required fields
        if (doc["name"].get_string().get(config.name) ||
            doc["version"].get_string().get(config.version))
        {
            std::cerr << "Missing required fields: name or version\n";
            return std::nullopt;
        }

        config.main = "src/main.sr";
        doc["main"].get_string().get(config.main);

        config.buildPath = "./build/";
        doc["buildPath"].get_string().get(config.buildPath);

        config.target = "native";
        doc["target"].get_string().get(config.target);

        // Mode (default COMPILE_NATIVE)
        config.mode = Mode::COMPILE_NATIVE;
        if (auto mode_val = doc["mode"].get_string(); !mode_val.error())
        {
            const std::string_view mode_str = mode_val.value();
            const auto parsed_mode = parse_mode(mode_str);
            if (!parsed_mode)
            {
                std::cerr << "Invalid mode value\n";
                return std::nullopt;
            }
            config.mode = *parsed_mode;
        }

        // Dependencies
        if (auto deps = doc["dependencies"].get_array(); !deps.error())
        {
            for (auto dep : deps.value())
            {
                const auto parsed = parse_dependency(dep);
                if (dep.error() || !parsed.has_value())
                {
                    std::cerr << "Invalid dependency value\n";
                    return std::nullopt;
                }

                config.dependencies.push_back(parsed.value());
            }
        }

        return config;
    }
} // namespace stride::project
