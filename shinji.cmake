include_guard()

# ------------------------------------------------------------------------------------------------
# CMRC
# Reference: https://github.com/vector-of-bool/cmrc/blob/master/CMakeRC.cmake
# ------------------------------------------------------------------------------------------------

if (DEFINED SHINJI_GENERATE_EMBEDDABLE_CPP_FILE)
    # Took from:
    # https://github.com/vector-of-bool/cmrc/blob/master/CMakeRC.cmake

    # NAMESPACE
    # INPUT_FILE
    # OUTPUT_FILE
    # SYMBOL

    # Read in the digits
    file(READ "${INPUT_FILE}" bytes HEX)
    # Format each pair into a character literal. Heuristics seem to favor doing
    # the conversion in groups of five for fastest conversion
    string(REGEX REPLACE "(..)(..)(..)(..)(..)" "'\\\\x\\1','\\\\x\\2','\\\\x\\3','\\\\x\\4','\\\\x\\5'," chars "${bytes}")
    # Since we did this in groups, we have some leftovers to clean up
    string(LENGTH "${bytes}" n_bytes2)
    math(EXPR n_bytes "${n_bytes2} / 2")
    math(EXPR remainder "${n_bytes} % 5") # <-- '5' is the grouping count from above
    set(cleanup_re "$")
    set(cleanup_sub )
    while(remainder)
        set(cleanup_re "(..)${cleanup_re}")
        set(cleanup_sub "'\\\\x\\${remainder}',${cleanup_sub}")
        math(EXPR remainder "${remainder} - 1")
    endwhile()
    if(NOT cleanup_re STREQUAL "$")
        string(REGEX REPLACE "${cleanup_re}" "${cleanup_sub}" chars "${chars}")
    endif()
    string(CONFIGURE [[
namespace ${NAMESPACE}::shinji
{
    extern const char ${SYMBOL_NAME}[] = { @chars@ 0 };
    extern const size_t ${SYMBOL_NAME}_size = @n_bytes@;
}
]] code)
    file(WRITE "${OUTPUT_FILE}" "${code}")
    # Exit from the script. Nothing else needs to be processed
    return()
endif()

# ------------------------------------------------------------------------------------------------
# Initialization
# ------------------------------------------------------------------------------------------------

set(SHINJI_HOME ${CMAKE_CURRENT_LIST_DIR})

set(SHINJI_HPP_TEMPLATE ${SHINJI_HOME}/shinji.hpp.in)
set(SHINJI_CMAKE ${SHINJI_HOME}/shinji.cmake)

find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc)  # Because it's CMake 3.19 (otherwise find_package(Vulkan))
find_program(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE NAMES glslangValidator)

# ------------------------------------------------------------------------------------------------
# Functions
# ------------------------------------------------------------------------------------------------


function (_shinji_assert_glslc)
    if (Vulkan_GLSLC_EXECUTABLE-NOTFOUND)
        message(FATAL_ERROR "Couldn't find glslc")
    endif()
endfunction()


function (_shinji_assert_glslangValidator)
    if (Vulkan_GLSLANG_VALIDATOR_EXECUTABLE-NOTFOUND)
        message(FATAL_ERROR "Couldn't find glslangValidator")
    endif()
endfunction()


function (_shinji_symbol_name SYMBOL_NAME_VAR INPUT_STRING)
    string(MD5 SYMBOL_NAME ${INPUT_STRING})
    string(SUBSTRING ${SYMBOL_NAME} 0 31 SYMBOL_NAME)
    set(SYMBOL_NAME "_${SYMBOL_NAME}")

    set(${SYMBOL_NAME_VAR} ${SYMBOL_NAME} PARENT_SCOPE)
endfunction()


function (_shinji_fix_src_path PATH OUT_PATH_VAR)
    cmake_parse_arguments(ARG "CHECK" "" "" "${ARGN}")

    if (NOT IS_ABSOLUTE ${PATH})
        set(PATH "${CMAKE_CURRENT_SOURCE_DIR}/${PATH}")
    endif()

    if (${ARG_CHECK} AND NOT EXISTS ${PATH})
        message(FATAL_ERROR "Path doesn't exist: ${PATH}")
    endif()

    set(${OUT_PATH_VAR} "${PATH}" PARENT_SCOPE)
endfunction()


function (_shinji_fix_dst_path PATH OUT_PATH_VAR)
    if (NOT IS_ABSOLUTE ${PATH})
        set(PATH "${CMAKE_CURRENT_BINARY_DIR}/${PATH}")
    endif()

    set(${OUT_PATH_VAR} "${PATH}" PARENT_SCOPE)
endfunction()


function (_shinji_format_log LOG LOG_VAR)
    set(${LOG_VAR} "[shinji] ${LOG}" PARENT_SCOPE)
endfunction()


function (_shinji_log TARGET MODE MSG)
    set(LOG "[shinji] (${TARGET}) ${MSG}")  # todo _shinji_format_log
    message(${MODE} "${LOG}")
endfunction()


function (_shinji_try_init TARGET LIB_NAME_VAR)
    set(LIB_NAME "${TARGET}-shinji-lib")
    if (NOT TARGET ${LIB_NAME})
        _shinji_log(${TARGET} STATUS "Initializing shinji library...")

        add_library(${LIB_NAME} INTERFACE)
        add_dependencies(${TARGET} ${LIB_NAME})
    endif()
    set(${LIB_NAME_VAR} ${LIB_NAME} PARENT_SCOPE)
endfunction()


function (shinji_validate_glsl TARGET SHADER)
    cmake_parse_arguments(ARG "" "OPTIONS" "" "${ARGN}")

    _shinji_assert_glslangValidator()

    _shinji_try_init(${TARGET} SHINJI_LIB)

    _shinji_fix_src_path(${SHADER} SHADER_PATH CHECK)

    set(LOG "Validating GLSL: ${SHADER}")
    if (DEFINED ARG_OPTIONS)
        set(LOG "${LOG} (${ARG_OPTIONS})")
    endif()
    _shinji_format_log("${LOG}" LOG)

    _shinji_symbol_name(SHADER_SYMBOL_NAME ${SHADER})

    set(VALIDATION_RULE "shinji-validate-glsl-${SHADER_SYMBOL_NAME}")
    add_custom_command(
            OUTPUT ${VALIDATION_RULE}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} ${SHADER_PATH} ${ARG_OPTIONS}
            MAIN_DEPENDENCY ${SHADER_PATH}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "${LOG}"
    )
    set_property(SOURCE ${VALIDATION_RULE} PROPERTY SYMBOLIC TRUE)

    target_sources(${SHINJI_LIB} PRIVATE ${VALIDATION_RULE})

    _shinji_log(${TARGET} STATUS "Validating GLSL: ${SHADER}")
endfunction()


function (shinji_compile_glsl_to_spirv TARGET GLSL_SHADER SPIRV_SHADER)
    cmake_parse_arguments(ARG "" "OPTIONS" "" "${ARGN}")

    _shinji_assert_glslc()

    _shinji_try_init(${TARGET} SHINJI_LIB)

    _shinji_fix_src_path(${GLSL_SHADER} GLSL_SHADER_PATH CHECK)
    _shinji_fix_dst_path(${SPIRV_SHADER} SPIRV_SHADER_PATH)

    get_filename_component(SPIRV_SHADER_DIR ${SPIRV_SHADER_PATH} DIRECTORY)
    file(MAKE_DIRECTORY ${SPIRV_SHADER_DIR})

    add_custom_command(
            OUTPUT ${SPIRV_SHADER}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${GLSL_SHADER_PATH} ${ARG_OPTIONS} -o ${SPIRV_SHADER_PATH}
            MAIN_DEPENDENCY ${GLSL_SHADER}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "[shinji] Compiling GLSL to SPIR-V: ${GLSL_SHADER} -> ${SPIRV_SHADER}"
    )

    target_sources(${TARGET} INTERFACE ${SPIRV_SHADER})

    _shinji_log(${TARGET} STATUS "Compiling GLSL to SPIR-V: ${GLSL_SHADER_PATH} -> ${SPIRV_SHADER_PATH}")
endfunction()


function (shinji_embed TARGET NAMESPACE)
    cmake_parse_arguments(ARG "" "" "" "${ARGN}")

    _shinji_try_init(${TARGET} SHINJI_LIB)

    set(SHINJI_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/_shinji)

    file(MAKE_DIRECTORY ${SHINJI_BIN_DIR})

    foreach (FILE ${ARG_UNPARSED_ARGUMENTS})
        _shinji_symbol_name(SYMBOL_NAME ${FILE})
        set(EMBEDDED_CPP_FILE ${SHINJI_BIN_DIR}/${SYMBOL_NAME}.cpp)

        add_custom_command(
                OUTPUT ${EMBEDDED_CPP_FILE}
                COMMAND ${CMAKE_COMMAND}
                    -DSHINJI_GENERATE_EMBEDDABLE_CPP_FILE=TRUE
                    -DNAMESPACE=${NAMESPACE}
                    -DSYMBOL_NAME=${SYMBOL_NAME}
                    -DINPUT_FILE=${FILE}
                    -DOUTPUT_FILE=${EMBEDDED_CPP_FILE}
                    -P ${SHINJI_CMAKE}
                MAIN_DEPENDENCY ${FILE}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "[shinji] Embedding file ${FILE} into ${EMBEDDED_CPP_FILE}"
        )

        target_sources(${SHINJI_LIB} PUBLIC ${EMBEDDED_CPP_FILE})

        set_property(
            TARGET ${TARGET}
            APPEND_STRING PROPERTY SHINJI_BUNDLED_RESOURCES_DECL_CODE
            "namespace ${NAMESPACE}::shinji { extern char const ${SYMBOL_NAME}[]; extern size_t const ${SYMBOL_NAME}_size; }\n"
        )

        set_property(
            TARGET ${TARGET}
            APPEND_STRING PROPERTY SHINJI_BUNDLED_RESOURCES_REGISTRATION_CODE
            "{\"${FILE}\", {${NAMESPACE}::shinji::${SYMBOL_NAME}, ${NAMESPACE}::shinji::${SYMBOL_NAME}_size}},\n"
        )

        _shinji_log(${TARGET} STATUS "Embedding file ${FILE} into ${EMBEDDED_CPP_FILE}")
    endforeach()
endfunction()


function (shinji_move TARGET SHADER)
endfunction()


function (shinji_finalize TARGET NAMESPACE)
    _shinji_log(${TARGET} STATUS "Finalizing...")

    _shinji_try_init(${TARGET} SHINJI_LIB)

    set(SHINJI_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/_shinji)
    set(SHINJI_HPP ${SHINJI_BIN_DIR}/shinji.hpp)

    get_property(
            SHINJI_BUNDLED_RESOURCES_DECL_CODE
            TARGET ${TARGET}
            PROPERTY SHINJI_BUNDLED_RESOURCES_DECL_CODE
    )

    get_property(
            SHINJI_BUNDLED_RESOURCES_REGISTRATION_CODE
            TARGET ${TARGET}
            PROPERTY SHINJI_BUNDLED_RESOURCES_REGISTRATION_CODE
    )

    string(CONFIGURE [=[
#pragma once

#include <unordered_map>
#include <string>
#include <stdexcept>
#include <fstream>

@SHINJI_BUNDLED_RESOURCES_DECL_CODE@

namespace @NAMESPACE@::shinji
{
    struct bundled_resource
    {
        char const* m_data;
        size_t m_size;
    };

    inline std::unordered_map<std::string, bundled_resource> s_bundled_resources{
@SHINJI_BUNDLED_RESOURCES_REGISTRATION_CODE@
    };

    inline bundled_resource load_resource_from_bundle(char const* resource)
    {
        if (s_bundled_resources.find(resource) != s_bundled_resources.end()) {
            return s_bundled_resources.at(resource);
        } else {
            throw std::runtime_error("Resource not bundled");
        }
    }

    inline void load_resource_from_file(char const* resource, std::vector<char>& buf)
    {
        std::ifstream f(resource, std::ios::binary | std::ios::ate);
        if (!f.is_open())
        {
            throw std::runtime_error("Resource file not found");
        }

        size_t buf_len = (size_t) f.tellg();
        buf.resize(buf_len);

        f.seekg(0);
        f.read(buf.data(), (std::streamsize) buf_len);
    }
}
]=] SHINJI_HPP_CONTENT @ONLY)
    file(GENERATE OUTPUT "${SHINJI_HPP}" CONTENT "${SHINJI_HPP_CONTENT}")

    target_sources(${SHINJI_LIB} PUBLIC ${SHINJI_HPP})

    target_include_directories(${TARGET} PRIVATE ${SHINJI_BIN_DIR})
    target_link_libraries(${TARGET} PRIVATE ${SHINJI_LIB})
endfunction()
