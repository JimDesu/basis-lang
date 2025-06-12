#include "CompileOptions.h"

#include <functional>
#include <unordered_map>
#include <vector>

namespace basis {
    using CompileOptionsSetter = std::function<void(CompileOptions&, std::string&)>;
    const std::unordered_map<std::string, CompileOptionsSetter> options_map = {
             {"-file", [](CompileOptions& o, std::string& arg) { o.filename = arg; }}
    };
    bool CompileOptions::readCompileOptions(std::vector<std::string>& arguments) {
        if ( arguments.empty() ) return false;
        auto size = arguments.size();
        if ( size % 2 != 0 ) return false;
        try {
            for ( int i = 0; i < size; i += 2 ) {
                options_map.at(arguments[i])(*this, arguments[i + 1]);
            }
        } catch ( std::exception& e ) {
            return false;
        }
        if (filename.empty()) return false;
        return true;
    }
} // basis