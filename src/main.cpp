#include "args.hpp"

#include <iostream>

int main(int argc, char** argv) {
    const auto parsed = mtfind::parse_args(argc, argv);
    if (!parsed) {
        std::cerr << mtfind::usage() << '\n';
        return 1;
    }

    return 0;
}