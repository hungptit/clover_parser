#pragma once

#include "fmt/format.h"

// System files for open, read, and close.
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

namespace utilities {
    template <typename OArchive, typename T> void print(T &&data, const bool info = false) {
        std::stringstream output;
        {
            OArchive oar(output);
            oar(cereal::make_nvp("test_results", data));
        }

        if (info) {
            fmt::print("{}\n", output.str());
        } else {
            fmt::print("Use memory: {}\n", output.str().size());
        }
    }
} // namespace utilities
