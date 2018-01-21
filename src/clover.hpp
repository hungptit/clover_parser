#pragma once

#include <string>
#include <vector>

#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#include "fmt/format.h"

// TODO: Save data into SQLite database.
// TODO: We might need to sort the coverage data so we can reduce the access time.
// TODO: Write coverage data into database.
// TODO: Create a report writer.
// TODO: Figure out how to trade off between performance and storage? May be it is not a big
// deal.
// TODO: Allow users to get the code coverage for a given set of files and folders.
// TODO: Allow users to reduce their test suites using code coverage information.

namespace clover {
    enum class CoverageType { STMT = 0, METHOD = 1, COND = 2 };

    template <typename T = size_t> struct Line {
        using index_type = T;
        index_type file_id;
        unsigned int num;
    };

    template <typename index_type>
    bool operator==(const Line<index_type> first, const Line<index_type> second) {
        return std::tie(first.file_id, first.num) == std::tie(second.file_id, second.num);
    }

    template <typename index_type>
    bool operator>(const Line<index_type> first, const Line<index_type> second) {
        return std::tie(first.file_id, first.num) > std::tie(second.file_id, second.num);
    }

    template <typename index_type>
    bool operator<(const Line<index_type> first, const Line<index_type> second) {
        return std::tie(first.file_id, first.num) < std::tie(second.file_id, second.num);
    }

    // A structure which hold test information.
    struct Test {
        std::string file;
        std::string name;
    };

    bool operator==(const Test &first, const Test &second) {
        return std::tie(first.file, first.name) == std::tie(second.file, second.name);
    }

    bool operator>(const Test &first, const Test &second) {
        return std::tie(first.file, first.name) > std::tie(second.file, second.name);
    }

    bool operator<(const Test &first, const Test &second) {
        return std::tie(first.file, first.name) < std::tie(second.file, second.name);
    }

    // This data structure hold the coverage information of a source line.
    template <typename T = unsigned int> struct CoverageInfo {
        using value_type = T;
        CoverageType type;
        value_type count;
        value_type truecount;
        value_type falsecount;

        CoverageInfo() : type(CoverageType::STMT), count(0), truecount(0), falsecount(0) {}
    };

    // This structure hold the map between a test file and a source line.
    template <typename T1, typename T2> struct LineCoverage {
        using index_type = T1;
        using value_type = T2;
        index_type test_id; // A test file id
        index_type line_id; // A source line id.
        CoverageInfo<value_type> info;
        template <typename Archive> void serialize(Archive &ar) {
            ar(test_id, line_id, info.type, info.count, info.truecount, info.falsecount);
        }
    };

    // Operators for LineCoverage.
    template <typename index_type, typename value_type>
    bool operator==(const LineCoverage<index_type, value_type> first,
                    const LineCoverage<index_type, value_type> second) {
        return std::tie(first.test_id, first.line_id) ==
               std::tie(second.test_id, second.line_id);
    }

    template <typename index_type, typename value_type>
    bool operator>(const LineCoverage<index_type, value_type> first,
                   const LineCoverage<index_type, value_type> second) {
        return std::tie(first.test_id, first.line_id) >
               std::tie(second.test_id, second.line_id);
    }

    template <typename index_type, typename value_type>
    bool operator<(const LineCoverage<index_type, value_type> first,
                   const LineCoverage<index_type, value_type> second) {
        return std::tie(first.test_id, first.line_id) <
               std::tie(second.test_id, second.line_id);
    }

    // A simple class which allows users to import and query code coverage information.
    template <typename T1, typename T2> class Database {
      public:
        using index_type = T1;
        using value_type = T2;
        bool parse(const index_type test_id, const char *xmlfile) {
            // Read the XML file
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file(xmlfile);
            if (!result) {
                return false; // Invalid XML file.
            }

            // Skip this a given file does not have any coverage information.
            auto root_node = doc.child("coverage");
            if (!root_node.attribute("clover")) {
                return false; // This is not a clover XML file.
            }

            // Only care about the line coverage information. We do not care
            // about project, package, and metrics because we can always
            // construct this information from line coverage data.
            for (auto project_node = root_node.child("project"); project_node;
                 project_node = project_node.next_sibling("project")) {
                for (auto package_node = project_node.child("package"); package_node;
                     package_node = package_node.next_sibling("package")) {
                    for (auto file_node = package_node.child("file"); file_node;
                         file_node = file_node.next_sibling("file")) {
                        parse_file_node(test_id, file_node);
                    }
                }
            }

            return true;
        }

        // Return the index of a test point.
        index_type get_test_index(const Test val) {
            auto it = test2idx.find(val);
            if (it != test2idx.end()) {
                return it->second;
            }

            // Update tests and test2idx
            const auto pos = tests.size();
            tests.push_back(val);
            test2idx[val] = pos;
            return pos;
        }

        // Return the index of a source file.
        index_type get_file_index(std::string &&apath) {
            auto it = file2idx.find(apath);
            if (it == file2idx.end()) {
                index_type pos = source_files.size();
                file2idx[apath] = pos;
                source_files.emplace_back(apath);
                return pos;
            }
            return it->second;
        }

        void info() {
			fmt::print("Number of source tests: {}\n", tests.size());
            fmt::print("Number of source files: {}\n", source_files.size());
            std::for_each(source_files.cbegin(), source_files.cend(),
                          [](auto item) { fmt::print("{}\n", item); });
            fmt::print("Number of source lines: {}\n", lines.size());
            fmt::print("Number of coverage item: {}\n", data.size());
        }

        void print(const std::string &msg) {
            fmt::print("{}\n", msg);
            for (const auto item : data) {
                auto const aline = lines[item.line_id];
                if (item.info.type == CoverageType::STMT) {
                    fmt::print("test {0} -> {1}:{2}, type: stmt, and count = {3}\n",
                               tests[item.test_id].file, source_files[aline.file_id], aline.num,
                               item.info.count);
                } else if (item.info.type == CoverageType::METHOD) {
                    fmt::print("test {0} -> {1}:{2}, type: method, and count = {3}\n",
                               tests[item.test_id].file, source_files[aline.file_id], aline.num,
                               item.info.count);
                } else {
                    fmt::print(
                        "test {0} -> {1}:{2}, type: {3}, falsecount: {4}, and truecount: {5}\n",
                        tests[item.test_id].file, source_files[aline.file_id], aline.num,
                        item.info.truecount, item.info.falsecount);
                }
            }
        }

      private:
        // This hold a list of source files.
        std::vector<std::string> source_files;

        // This hold a list of tests which might be a file or just of test point which be long
        // to a file.
        std::vector<Test> tests;
        std::vector<Line<index_type>> lines;
        std::vector<LineCoverage<index_type, value_type>> data;

        // A map from a file name to the index in the files table.
        std::unordered_map<std::string, index_type> file2idx;

        // A map from a line objecy to the index in the lines table
        std::unordered_map<Line<index_type>, index_type> line2idx;

        // A map from a test object to the index in the test table.
        std::unordered_map<Test, index_type> test2idx;

        // Get a coverage type string.
        const char *get_type_string(const CoverageType type) {
            if (type == CoverageType::STMT) {
                return "stmt";
            }

            if (type == CoverageType::METHOD) {
                return "method";
            }

            if (type == CoverageType::COND) {
                return "cond";
            }

            return "unknown";
        }

        // Parse file node
        void parse_file_node(index_type test_id, const pugi::xml_node root_node) {
            for (auto node = root_node.child("line"); node; node = node.next_sibling("line")) {
                auto source_id = get_file_index(root_node.attribute("path").value());
                parse_line_node(test_id, source_id, node);
            }
        }

        // Methods
        index_type get_line_idx(Line<index_type> &&aline) {
            auto it = line2idx.find(aline);
            if (it != line2idx.end()) {
                return it->second;
            };

            // Push a given line into lines table and update the map.
            const index_type pos = lines.size();
            line2idx[aline] = pos;
            lines.push_back(aline);
            return pos;
        }

        void add_coverage_info(LineCoverage<index_type, value_type> &&info) {
            data.emplace_back(info); // TODO: How to avoid duplicated info?
        }

        void parse_line_node(const index_type test_id, const index_type source_id,
                             const pugi::xml_node line_node) {
            const unsigned int linenum = line_node.attribute("num").as_uint();
            size_t line_idx = get_line_idx({source_id, linenum});

            // Update the coverage data. We will kkip this line if it does not
            // have any coverage information.
            CoverageInfo<value_type> info;
            const std::string type(line_node.attribute("type").value());
            if (type == "stmt") {
                info.type = CoverageType::STMT;
                info.count = line_node.attribute("count").as_uint();
                if (!info.count) {
                    return;
                }
            } else if (type == "method") {
                info.type = CoverageType::METHOD;
                info.count = line_node.attribute("count").as_uint();
                if (!info.count) {
                    return;
                }
            } else if (type == "cond") {
                info.type = CoverageType::COND;
                info.truecount = line_node.attribute("truecount").as_uint();
                info.falsecount = line_node.attribute("falsecount").as_uint();
                if (!info.truecount || !info.falsecount) {
                    return;
                }
            } else {
                assert("Unexpected coverage type");
            }

            // Add coverage data.
            add_coverage_info({test_id, line_idx, info});
        }
    };

    // This data structure hold the map from a test to source lines. Note that
    // source lines can be from different files.
    template <typename T> struct TestInfo {
        using index_type = T;
        index_type test_id;
        std::vector<index_type> source_lines;

		TestInfo() : test_id(), source_lines(){};
    };

	// TODO: Given a set of TestInfo find a smaller set that can have the same
	// coverage i.e cover the same set of source lines. This is set covering problem.
}

namespace std {
    template <typename index_type> struct hash<clover::Line<index_type>> {
        using result_type = std::size_t;
        result_type operator()(const clover::Line<index_type> &value) const {
            result_type const h1(std::hash<index_type>()(value.file_id));
            result_type const h2(std::hash<index_type>()(value.num));
            return h1 ^ (h2 << 4);
        }
    };

    template <> struct hash<clover::Test> {
        using result_type = std::size_t;
        result_type operator()(const clover::Test &value) const {
            result_type const h1(std::hash<decltype(value.file)>()(value.file));
            result_type const h2(std::hash<decltype(value.name)>()(value.name));
            return h1 ^ (h2 << 4);
        }
    };

    template <typename index_type, typename value_type>
    struct hash<clover::LineCoverage<index_type, value_type>> {
        using result_type = std::size_t;
        result_type
        operator()(const clover::LineCoverage<index_type, value_type> &value) const {
            result_type const h1(std::hash<index_type>()(value.test_id));
            result_type const h2(std::hash<index_type>()(value.line_id));
            return h1 ^ (h2 << 4);
        }
    };
}
