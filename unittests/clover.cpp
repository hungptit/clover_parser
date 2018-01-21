#include <iostream>

#include "fmt/format.h"
#include "pugixml.hpp"

#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/archives/xml.hpp"
#include "clover_parser.hpp"
#include "utilities.hpp"

int main(int argc, char *argv[]) {

    if (argc == 1) return EXIT_SUCCESS;

    for (auto idx = 1; idx < argc; ++idx) {
        std::string data_file(argv[idx]);
        coverage::CloverParser parser;
        auto results = parser(data_file);
        utilities::print<cereal::JSONOutputArchive>(results);
        utilities::print<cereal::XMLOutputArchive>(results);
        utilities::print<cereal::BinaryOutputArchive>(results);
        utilities::print<cereal::PortableBinaryOutputArchive>(results);
        utilities::print<cereal::JSONOutputArchive>(results, true);
    }

    return EXIT_SUCCESS;
}
