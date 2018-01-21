PROJECT(TOOLS)
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")
set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3p/")
message("ROOT_DIR: ${ROOT_DIR}")

set(CMAKE_INSTALL_PREFIX "$ENV{HOME}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Read the git tags to determine the project version
include(GetGitVersion)
get_git_version(GIT_VERSION)

# Tell the user what versions we are using
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" VERSION ${GIT_VERSION})
message("-- Version: ${VERSION}")

# The version of the libraries
set(GENERIC_LIB_VERSION ${VERSION})
string(SUBSTRING ${VERSION} 0 1 GENERIC_LIB_SOVERSION)

include(CheckCXXCompilerFlag)
include(AddCXXCompilerFlag)
include(CXXFeatureCheck)

# Use libc++  provided by llvm
# set(CMAKE_CXX_COMPILER "clang++")
# add_cxx_compiler_flag(-stdlib=libc++)

# set(CMAKE_BUILD_TYPE Debug)

set (CMAKE_BUILD_TYPE Release)
add_cxx_compiler_flag(-O3)
add_cxx_compiler_flag(-march=native)

add_cxx_compiler_flag(-std=c++14)
add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-flto)

# add_cxx_compiler_flag(-Wextra)

# add_cxx_compiler_flag(-Wshadow)
# add_cxx_compiler_flag(-Werror RELEASE)
# add_cxx_compiler_flag(-Werror RELWITHDEBINFO)
# add_cxx_compiler_flag(-Werror MINSIZEREL)

add_cxx_compiler_flag(-pedantic)
add_cxx_compiler_flag(-pedantic-errors)

# add_cxx_compiler_flag(-Wshorten-64-to-32)
# add_cxx_compiler_flag(-Wfloat-equal)
# add_cxx_compiler_flag(-Wint-to-void-pointer-cast)

# add_cxx_compiler_flag(-Wshorten-64-to-32)
# add_cxx_compiler_flag(-Wfloat-equal)
add_cxx_compiler_flag(-Wint-to-void-pointer-cast)

add_cxx_compiler_flag(-fstrict-aliasing)
if (HAVE_CXX_FLAG_FSTRICT_ALIASING)
  add_cxx_compiler_flag(-Wstrict-aliasing)
endif()
# add_cxx_compiler_flag(-Wthread-safety)
if (HAVE_WTHREAD_SAFETY)
  add_definitions(-DHAVE_WTHREAD_SAFETY)
  cxx_feature_check(THREAD_SAFETY_ATTRIBUTES)
endif()

add_cxx_compiler_flag(-DPUGIXML_HEADER_ONLY)
add_cxx_compiler_flag(-DFMT_HEADER_ONLY)
add_cxx_compiler_flag(-DCEREAL_RAPIDJSON_HAS_CXX11_RVALUE_REFS)
# add_cxx_compiler_flag(-flto)

ENABLE_TESTING()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Libraries used by rocksdb
set(LIB_ROCKSDB "${EXTERNAL_DIR}/3p/rocksdb/librocksdb.a")
set(LIB_ZLIB "${EXTERNAL_DIR}/lib/libz.a")
set(LIB_LZ4 "${EXTERNAL_DIR}/lib/liblz4.a")
set(LIB_BZ2 "${EXTERNAL_DIR}/lib/libbz2.a")
set(LIB_SNAPPY "${EXTERNAL_DIR}/lib/libsnappy.a")
set(LIB_CELERO "${EXTERNAL_DIR}/lib/libcelero.a")
# set(LIB_JEMALLOC "${EXTERNAL_DIR}/lib/libjemalloc.a")
set(LIB_JEMALLOC "-ljemalloc")

# This option make sure that we use the local boost version. Note that if the
# system boost is installed then CMake might use that boost version.
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_NO_SYSTEM_PATHS FALSE)
if (Boost_NO_SYSTEM_PATHS)
  set(BOOST_ROOT "${EXTERNAL_DIR}")
  set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
  set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/lib")
endif (Boost_NO_SYSTEM_PATHS)
# message(${BOOST_ROOT})
find_package(Boost REQUIRED date_time system filesystem iostreams)
set(LIB_PROGRAM_OPTIONS "${EXTERNAL_DIR}/lib/libboost_program_options.a")

# Include folder
include_directories ("${EXTERNAL_DIR}/include/")

# Clover headers
include_directories ("${ROOT_DIR}/src")

message("root_dir: ${EXTERNAL_DIR}")
message("include_dir: ${EXTERNAL_DIR}/include")
message("src_dir: ${EXTERNAL_DIR}/src")

set(COMMAND_SRC_FILES clover tap)
foreach (src_file ${COMMAND_SRC_FILES})
  ADD_EXECUTABLE(${src_file} ${src_file}.cpp)
  TARGET_LINK_LIBRARIES(${src_file} -lpthread)
endforeach (src_file)



