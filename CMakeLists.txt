cmake_minimum_required(VERSION 3.23)
project(
    Godot-Kafka
    VERSION 0.1.0
    LANGUAGES CXX
)

# Output System Information
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "CMake system name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMake system processor: ${CMAKE_SYSTEM_PROCESSOR}")

# Output Compiler Information for both C++ and C
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Compiler ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "C Compiler Version: ${CMAKE_C_COMPILER_VERSION}")


option(BUILD_TESTS "Build the tests" ON)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(
        # Enable MTd (Multi-threaded Debug) for Debug builds
        $<$<CONFIG:Debug>:/MDd>
        # Enable MT (Multi-threaded) for Release builds
        $<$<NOT:$<CONFIG:Debug>>:/MDd>
    )
endif()

# Use folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set CMake Predefined Targets Folders
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "External/CMake")

# Output the binaries to the bin folder
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/.build/bin/${CMAKE_SYSTEM_NAME}/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/.build/bin/${CMAKE_SYSTEM_NAME}/${OUTPUTCONFIG}")
endforeach()

# Prefix all the binaries with "lib"
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
if(WIN32)
    set(LIB_TYPE "dll")
elseif(APPLE)
    set(LIB_TYPE "dylib")
elseif(UNIX)
    set(LIB_TYPE "so")
endif()
set(CMAKE_SHARED_LIBRARY_SUFFIX ".${CMAKE_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.${LIB_TYPE}") # Example: libGDKafka.Linux.x86_64.so for a Linux x86_64 system, or libGDKafka.Windows.AMD64.dll for a Windows AMD64 system.

# Glob all sources files.
file(GLOB_RECURSE GODOT_SOURCES "project/src/*.cpp" "project/src/**/*.cpp")
file(GLOB_RECURSE GODOT_HEADERS "project/includes/*.hpp" "project/includes/*.h" "project/includes/**/*.hpp" "project/includes/**/*.h") 
file(GLOB_RECURSE LIB_SOURCES "lib/src/*.cpp" "lib/src/**/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "lib/includes/*.hpp" "lib/includes/*.h" "lib/includes/**/*.hpp" "lib/includes/**/*.h")

# Set Preprocessor Definitions
# Define preprocessor definitions based on build configuration using generator expressions
add_compile_definitions(
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:_NDEBUG>
)

# Compiler -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include OpenSSL
set(OPENSSL_USE_STATIC_LIBS TRUE CACHE BOOL "Use static OpenSSL libraries" FORCE)
find_package(OpenSSL REQUIRED)
set(system_library_extension "lib")

if(UNIX)
    set(system_library_extension "a")
endif()

find_library(CRYPTO_LIB libcrypto.${system_library_extension} REQUIRED)
find_library(SSL_LIB libssl.${system_library_extension} REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_libraries(OpenSSL::Crypto)
endif()

if(CRYPTO_LIB)
    message(STATUS "Found libcrypto: ${CRYPTO_LIB}")
    link_libraries(${CRYPTO_LIB})
endif()

if(SSL_LIB)
    message(STATUS "Found libssl: ${SSL_LIB}")
    link_libraries(${SSL_LIB})
endif()

add_library(KafkaLib STATIC
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)
target_include_directories(KafkaLib PUBLIC lib/includes)

add_library(GDKafka SHARED
    ${GODOT_SOURCES}
    ${GODOT_HEADERS}
)
target_include_directories(GDKafka PUBLIC
    project/includes
)

# Include Godot::cpp
set(GODOTCPP_DEBUG_CRT ON CACHE BOOL "Compile with MSVC's debug CRT (/MDd)" FORCE)
add_subdirectory(extern/godot-cpp)
target_link_libraries(GDKafka PRIVATE godot::cpp)
target_link_libraries(GDKafka PRIVATE KafkaLib)
set_target_properties(godot-cpp PROPERTIES FOLDER "External/Godot")

# Include librdkafka
set(RDKAFKA_BUILD_EXAMPLES OFF CACHE BOOL "Disable building examples" FORCE)
set(RDKAFKA_BUILD_TESTS OFF CACHE BOOL "Disable building examples" FORCE)
set(RDKAFKA_BUILD_STATIC ON CACHE BOOL "Disable building examples" FORCE)
add_subdirectory(extern/librdkafka)
target_link_libraries(KafkaLib PUBLIC rdkafka) # rdkafka is the C library and the core.
target_link_libraries(KafkaLib PUBLIC rdkafka++) # rdkafka++ is a C++ wrapper around librdkafka
set_target_properties(rdkafka PROPERTIES FOLDER "External/rdkafka")
set_target_properties(rdkafka++ PROPERTIES FOLDER "External/rdkafka")


# Load Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Tests are enabled")
else()
    message(STATUS "Tests are disabled")
endif()