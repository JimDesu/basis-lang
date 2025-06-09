#include "../compiler.h"

#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> arguments;
    for ( int i = 0; i < argc; i++ ) {
        arguments.emplace_back(argv[i]);
    }
    return compile(arguments);
}
