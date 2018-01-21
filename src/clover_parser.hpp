#pragma once

#include <string>
#include <vector>

#include "fmt/format.h"
#include "pugixml.hpp"

#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#include "data_structures.hpp"
#include "utilities.hpp"

// Use TBB concurent vector and hash table.
#include <unordered_map>

namespace coverage {


    class CloverParser {
      public:
        ProjectCoverage operator()(const std::string &data_file) {
            ProjectCoverage results;

            // Read the XML file
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file(data_file.c_str());
			if (!result) {
				throw std::runtime_error("Invalid xml file: " + data_file);
			}
			
            // Check that the given file has coverage information
            auto root_node = doc.child("coverage");
            if (!root_node.attribute("clover")) {
                throw std::runtime_error("Invalid clover code coverage xml file!");
            }

            // Parse project information
            parse_project_coverage(root_node.child("project"), results);

            // Return results
            return results;
        }

      private:
        LineCoverage parse_line_coverage(const pugi::xml_node line_node) {
            LineCoverage item;
            item.num = line_node.attribute("num").as_uint();
            const std::string type(line_node.attribute("type").value());
            if (type == "stmt") {
                item.type = CoverageType::STMT;
                item.count = line_node.attribute("count").as_uint();
            } else if (type == "method") {
                item.type = CoverageType::METHOD;
                item.count = line_node.attribute("count").as_uint();
            } else if (type == "cond") {
                item.type = CoverageType::COND;
                item.truecount = line_node.attribute("truecount").as_uint();
                item.falsecount = line_node.attribute("falsecount").as_uint();
            } else {
                assert("Unexpected coverage type");
            }
            return item;
        }

        ClassCoverage parse_class_coverage(const pugi::xml_node node) {
            ClassCoverage item;
            item.name = node.attribute("name").value();
            // auto metrics = parse_class_metrics(node.child("metrics"));
            return item;
        }

        FileCoverage parse_file_coverage(const pugi::xml_node root_node) {
            FileCoverage item;
            item.path = root_node.attribute("path").value();
            item.name = root_node.attribute("name").value();
            // auto metrics = parse_file_metrics(root_node.child("metrics"));
            for (auto node = root_node.child("class"); node;
                 node = node.next_sibling("class")) {
                item.classes.emplace_back(parse_class_coverage(node));
            }
            for (auto node = root_node.child("line"); node; node = node.next_sibling("line")) {
                item.lines.emplace_back(parse_line_coverage(node));
            }
            return item;
        }

        PackageCoverage parse_package_coverage(const pugi::xml_node root_node) {
            PackageCoverage item;
            item.name = root_node.attribute("name").value();
            // auto metrics = parse_package_metrics(root_node.child("metrics"));
            for (auto node = root_node.child("file"); node; node = node.next_sibling("file")) {
                item.files.emplace_back(parse_file_coverage(node));
            }
            return item;
        }

        void parse_project_coverage(const pugi::xml_node root_node, ProjectCoverage &results) {

            results.timestamp = root_node.attribute("timestamp").value();
            results.name = root_node.attribute("name").value();
            // auto metrics = parse_project_metrics(root_node.child("metrics"));

            // Parse package information
            for (auto node = root_node.child("package"); node;
                 node = node.next_sibling("package")) {
                results.packages.emplace_back(parse_package_coverage(node));
            }
        }

        ProjectMetrics parse_project_metrics(const pugi::xml_node node) {
            ProjectMetrics results;
            results.packages = node.attribute("packages").as_uint();
            results.metrics = parse_package_metrics(node);
            return results;
        }

        PackageMetrics parse_package_metrics(const pugi::xml_node node) {
            PackageMetrics results;
            results.files = node.attribute("files").as_uint();
            results.metrics = parse_file_metrics(node);
            return results;
        }

        FileMetrics parse_file_metrics(const pugi::xml_node node) {
            FileMetrics results;
            results.classes = node.attribute("classes").as_uint();
            results.metrics = parse_class_metrics(node);
            return results;
        }

        ClassMetrics parse_class_metrics(const pugi::xml_node node) {
            ClassMetrics metrics;
            metrics.elements = node.attribute("elements").as_uint();
            metrics.coveredelements = node.attribute("coveredelements").as_uint();
            metrics.statements = node.attribute("statements").as_uint();
            metrics.coveredstatements = node.attribute("coveredstatements").as_uint();
            metrics.conditionals = node.attribute("conditionals").as_uint();
            metrics.coveredconditionals = node.attribute("coveredconditionals").as_uint();
            metrics.methods = node.attribute("methods").as_uint();
            metrics.coveredmethods = node.attribute("coveredmethods").as_uint();
            metrics.loc = node.attribute("loc").as_uint();
            metrics.ncloc = node.attribute("ncloc").as_uint();
            metrics.complexity = node.attribute("complexity").as_uint();
            return metrics;
        }
    };

    // Compute files coverage information.
    FileMetrics compute_file_metrics(const FileCoverage &data) {
        FileMetrics results;

        for (LineCoverage line : data.lines) {
            if (line.type == CoverageType::STMT) {
                results.metrics.statements++;
                results.metrics.coveredstatements += line.count > 0;
            } else if (line.type == CoverageType::METHOD) {
                results.metrics.methods++;
                results.metrics.coveredmethods += line.count > 0;
            } else if (line.type == CoverageType::COND) {
                results.metrics.conditionals += 2;
                results.metrics.coveredconditionals += (line.truecount > 0);
            } else {
                throw(std::runtime_error("Unexpected coverage type"));
            }
        }

        // Update elements and coveredelements.
        results.metrics.elements =
            results.metrics.statements + results.metrics.methods + results.metrics.conditionals;
        results.metrics.coveredelements = results.metrics.coveredstatements +
                                          results.metrics.coveredmethods +
                                          results.metrics.coveredconditionals;

        return results;
    }

    template <typename OArchive = cereal::JSONOutputArchive>
    void print_file_coverage_info(const ProjectCoverage &data, const std::string &apath) {
        for (PackageCoverage pkg : data.packages) {
            for (FileCoverage afile : pkg.files) {
                if (afile.path == apath) {
                    utilities::print<OArchive>(compute_file_metrics(afile), true);
                }
            }
        }
    }

} // namespace coverage
