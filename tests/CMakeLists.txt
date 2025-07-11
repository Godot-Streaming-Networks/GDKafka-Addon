# CMakeLists.txt for the test directory

# Ensure this directory is part of a parent project
if (NOT PROJECT_NAME)
    message(FATAL_ERROR "This CMakeLists.txt must be included in a parent project.")
endif()

# Get all subdirectories in the current directory
file(GLOB SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Filter only directories
set(DIRECTORY_LIST "")
foreach(SUBDIR ${SUBDIRS})
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR})
        list(APPEND DIRECTORY_LIST ${SUBDIR})
    endif()
endforeach()

# Print the directories
message(STATUS "Found directories in ${CMAKE_CURRENT_SOURCE_DIR}: ${DIRECTORY_LIST}")

# Create a CTest target for each directory
foreach(DIR ${DIRECTORY_LIST})
    # Define the test target name
    set(TEST_TARGET_NAME test_${DIR})

    # Collect all *.cpp files in the directory
    file(GLOB_RECURSE TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/**/*.cpp)
    if (NOT TEST_SOURCES)
        message(WARNING "No test sources found in ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}. Skipping.")
        continue()
    endif()

    # Add the test target
    add_executable(${TEST_TARGET_NAME} ${TEST_SOURCES})

    # Link the test target with any necessary libraries (adjust as needed)
    target_link_libraries(${TEST_TARGET_NAME} PRIVATE KafkaLib)

    # Add the test to CTest
    add_test(NAME ${TEST_TARGET_NAME} COMMAND ${TEST_TARGET_NAME})

    # Set properties for the test target
    set_target_properties(${TEST_TARGET_NAME} PROPERTIES 
        FOLDER "Tests"
        
    )
    # Set the output path for all configurations
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
        set_target_properties(${TEST_TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/.build/bin/${CMAKE_SYSTEM_NAME}/${OUTPUTCONFIG}"
        )
    endforeach()
endforeach()