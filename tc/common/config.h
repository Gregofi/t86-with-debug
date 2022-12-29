#pragma once

#include <unordered_map>

#include "helpers.h"

namespace tiny {

    /** The configuration object

     */
    class Config {
    public:

        static constexpr char const * const VERBOSE = " verbose";
        static constexpr char const * const MODE = " mode";
        static constexpr char const * const INPUT = " input";

        void parse(int argc, char * argv[]) {
            for (int i = 1; i < argc; ++i) {
                std::string arg{argv[i]};
                if (arg == "--verbose" || arg == "-v") {
                    setOption(VERBOSE, "true");
                } else if (arg == "--interactive" || arg == "-i" || arg == "--repl") {
                    setOption(MODE, "interactive");
                } else if (arg == "--compile" || arg == "-c") {
                    setOption(MODE, "compile");
                    interactive_ = false;
                // user specified option
                } else if (arg[0] == '-') {
                    std::string argName{arg};
                    std::string argValue;
                    auto split = argName.find('=');
                    if (split != std::string::npos) {
                        argValue = argName.substr(split + 1);
                        argName = argName.substr(0, split);
                    }
                    setOption(argName, argValue);
                // does not start with -, is a file to open
                } else {
                    setOption(INPUT, argv[i]);
                }
            }
#if (!defined NDEBUG)
            setDefaultIfMissing(VERBOSE, "true");
#endif
        }

        bool interactive() const {
            auto i = args_.find(MODE);
            if (i == args_.end())
                return false;
            else
                return i->second == "interactive";
        }

        bool verbose() const {
            auto i = args_.find(VERBOSE);
            if (i == args_.end())
                return false;
            return i->second == "true";
        }

        std::string const & input() {
            auto i = args_.find(INPUT);
            if (i == args_.end())
                throw std::runtime_error("Input not specified");
            return i->second;
        }

        std::string const & get(std::string const & name) const {
            auto i = args_.find(name);
            if (i == args_.end())
                throw std::runtime_error(STR("Configuration value " << name << " not provided"));
            return i->second;
        }

        bool setDefaultIfMissing(std::string const & name, std::string const & value) {
            if (args_.find(name) == args_.end()) {
                args_.insert(std::make_pair(name, value));
                return true;
            } else {
                return false;
            }
        }

    private:

        void setOption(std::string const & name, std::string const & value) {
            if (args_.find(name) != args_.end())
                throw std::runtime_error(STR("Argument " << name << " already defined"));
            args_.insert(std::make_pair(name, value));
        }

        bool interactive_;
        bool verbose_;
        std::string input_;
        std::unordered_map<std::string, std::string> args_;

    };

    /** The configuration singleton.
     */
    extern Config config;

}