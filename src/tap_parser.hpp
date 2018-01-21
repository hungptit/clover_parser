#pragma once

#include <string>
#include <vector>

// XML parser
#include "pugixml.hpp"

// cereal headers that will be used to serialize/deserialize data to different
// formats.
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/archives/xml.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

namespace tap {
    struct TestFailure {
        std::string type;
        std::string message;
        std::string data;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("type", type), cereal::make_nvp("message", message),
               cereal::make_nvp("data", data));
        }
    };

    struct TestCase {
        std::string name;
        std::vector<TestFailure> failures;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("name", name), cereal::make_nvp("failures", failures));
        }
    };

    struct TestSuite {
        bool failures;
        unsigned int errors;
        unsigned int tests;
        std::string name;
        std::vector<TestCase> testcases;
        template <typename Archive> void serialize(Archive &ar) {
            ar(cereal::make_nvp("failures", failures), cereal::make_nvp("errors", errors),
               cereal::make_nvp("tests", tests), cereal::make_nvp("name", name),
               cereal::make_nvp("testcases", testcases));
        }
    };

    class Parser {
      public:
        using TestResults = std::vector<TestSuite>;
        TestResults operator()(const std::string &xmlfile) {
            TestResults results;

            // Read the XML file
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file(xmlfile.c_str());
			if (!result) {
				throw std::runtime_error("Cannot parse " + xmlfile);
			}
			
            // Check that the given file has TAP information
            auto root_node = doc.child("testsuites");
            if (!root_node) {
                throw std::runtime_error(xmlfile + " is an invalid XML TAP file!");
            }

            // Parse test results
            for (auto node = root_node.child("testsuite"); node;
                 node = node.next_sibling("testsuite")) {
                results.emplace_back(parse_testsuite(node));
            }

            return results;
        }

      private:
        TestFailure parse_failure(const pugi::xml_node root_node) {
            TestFailure failure;
            failure.type = root_node.attribute("type").value();
            failure.message = root_node.attribute("message").value();
            failure.data = root_node.first_child().value();
            return failure;
        }

        TestCase parse_testcase(const pugi::xml_node root_node) {
            TestCase testcase;
            testcase.name = root_node.attribute("name").value();
            for (auto node = root_node.child("failure"); node;
                 node = node.next_sibling("failure")) {
                testcase.failures.emplace_back(parse_failure(node));
            }
            return testcase;
        }

        TestSuite parse_testsuite(const pugi::xml_node root_node) {
            TestSuite testsuite;
            testsuite.failures = root_node.attribute("failures").as_int();
            testsuite.errors = root_node.attribute("errors").as_uint();
            testsuite.tests = root_node.attribute("tests").as_uint();
            testsuite.name = root_node.attribute("name").value();
            for (auto node = root_node.child("testcase"); node;
                 node = node.next_sibling("testcase")) {
                testsuite.testcases.emplace_back(parse_testcase(node));
            }
            return testsuite;
        }
    };
} // namespace tap
