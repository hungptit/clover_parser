#pragma once

#include <string>
#include <vector>

#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

namespace coverage {
    enum class CoverageType { STMT, METHOD, COND };

    using index_type = size_t;

    struct Line {
        index_type source_id;
        unsigned int num;
    };

    bool operator==(const Line first, const Line second) {
        return std::tie(first.source_id, first.num) == std::tie(second.source_id, second.num);
    }

    bool operator>(const Line first, const Line second) {
        return std::tie(first.source_id, first.num) > std::tie(second.source_id, second.num);
    }

    bool operator<(const Line first, const Line second) {
        return std::tie(first.source_id, first.num) < std::tie(second.source_id, second.num);
    }

    struct CoverageInfo {
        CoverageType type;
        unsigned int count;
        unsigned int falsecount;
        unsigned int truecount;
    };

    struct LineCoverageData {
        index_type source_id; // A test file id
        index_type line_id;   // A source line id.
        CoverageInfo info;
        template <typename Archive> void serialize(Archive &ar) {
            ar(source_id, line_id, info.type, info.count, info.truecount, info.falsecount);
        }
    };

    struct ClassMetrics {
        int elements;
        int coveredelements;
        int statements;
        int coveredstatements;
        int conditionals;
        int coveredconditionals;
        int methods;
        int coveredmethods;
        int complexity;
        int loc;
        int ncloc;

        ClassMetrics() noexcept = default;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("elements", elements),
               cereal::make_nvp("coveredelements", coveredelements),
               cereal::make_nvp("statements", statements),
               cereal::make_nvp("coveredstatements", coveredstatements),
               cereal::make_nvp("conditionals", conditionals),
               cereal::make_nvp("coveredconditionals", coveredconditionals),
               cereal::make_nvp("methods", methods),
               cereal::make_nvp("coveredmethods", coveredmethods), cereal::make_nvp("loc", loc),
               cereal::make_nvp("ncloc", ncloc), cereal::make_nvp("complexity", complexity));
        }
    };

    struct FileMetrics {
        int classes;
        ClassMetrics metrics;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("classes", classes), cereal::make_nvp("metrics", metrics));
        }
        FileMetrics() noexcept : classes(), metrics() {}
    };

    struct PackageMetrics {
        int files;
        FileMetrics metrics;
        PackageMetrics() noexcept : files(), metrics() {}
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("files", files), cereal::make_nvp("metrics", metrics));
        }
    };

    struct ProjectMetrics {
        int packages;
        PackageMetrics metrics;

        ProjectMetrics() noexcept : packages(), metrics(){};
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("packages", packages), cereal::make_nvp("metrics", metrics));
        }
    };

    struct LineCoverage {
        unsigned int num;
        unsigned int count;
        CoverageType type;
        unsigned int truecount;
        unsigned int falsecount;

        LineCoverage() noexcept : num(), count(), type(), truecount(), falsecount() {}
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("num", num), cereal::make_nvp("count", count),
               cereal::make_nvp("type", type), cereal::make_nvp("truecount", truecount),
               cereal::make_nvp("falsecount", falsecount));
        }
    };

    struct ClassCoverage {
        std::string name;
        ClassCoverage() noexcept = default;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("name", name));
        }
    };

    struct FileCoverage {
        std::string path;
        std::string name;
        std::vector<ClassCoverage> classes;
        std::vector<LineCoverage> lines;

        FileCoverage() noexcept = default;

        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("path", path), cereal::make_nvp("name", name),
               cereal::make_nvp("classes", classes), cereal::make_nvp("lines", lines));
        }
    };

    struct PackageCoverage {
        std::string name;
        std::vector<FileCoverage> files;

        PackageCoverage() noexcept = default;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("name", name), cereal::make_nvp("files", files));
        }
    };

    struct ProjectCoverage {
        std::string timestamp;
        std::string name;
        std::vector<PackageCoverage> packages;
        ProjectCoverage() noexcept = default;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("timestamp", timestamp), cereal::make_nvp("name", name),
               cereal::make_nvp("packages", packages));
        }
    };
} // namespace coverage

namespace std {}
