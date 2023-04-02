include(${CMAKE_CURRENT_LIST_DIR}/TargetUtilities.cmake)

find_package(Threads)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

shuhai_get_target_property(protobuf_protoc_EXEC protobuf::protoc IMPORTED_LOCATION)
if(protobuf_protoc_EXEC)
    message(STATUS "Protobuf code generator found: ${protobuf_protoc_EXEC}")
else()
    message(FATAL_ERROR "Protobuf code generator not found.")
endif()

shuhai_get_target_property(grpc_cpp_plugin_EXEC gRPC::grpc_cpp_plugin IMPORTED_LOCATION)
if(grpc_cpp_plugin_EXEC)
    message(STATUS "gRPC cpp plugin for protoc found: ${grpc_cpp_plugin_EXEC}")
else()
    message(FATAL_ERROR "gRPC cpp plugin for protoc not found.")
endif()


# Add targets related to proto files, including proto code generator and the generated code library.
# The proto code generator generates *.pb.cc/h and *.grpc.pb.cc/h files corresponding to the specified proto files.
# The proto code library is a static library made up of the generated code.
#
# shuhai_grpc_add_proto_targets(
#   GENERATOR_TARGET target1
#   LIBRARY_TARGET target2
#   PROTOC_EXE protoc_executable_path
#   GRPC_PLUGIN_EXE grpc_plugin_excutable_path
#   OUTPUT_DIR output_dir
#   PROTO_FILES p1.proto p2.proto ...
#   PROTO_DIRS proto_include_dir1 proto_include_dir2 ...)
#
# GENERATOR_TARGET
#       Target name of the proto code generator.
#
# LIBRARY_TARGET
#       Target name of the proto code library.
#       A static library target is added if there isn't any target with same name exists; otherwise the generated code is
#   added to the existing target.
#       The generated proto code will be added to the target as sources.
#       The output directory will be added to the target as include directory.
#       The generator target (which specified by GENERATOR_TARGET option) will be added as dependency of the library target
#   if they are different targets.
#
# PROTOC_EXE
#       The protoc executable used to generates proto (*.pb.cc/h) code. Default to protobuf_protoc_EXEC if not specified.
#
# GRPC_PLUGIN_EXE
#       The grpc plugin executable of protoc used to generates grpc (*.grpc.pb.cc/h) code. Default to grpc_cpp_plugin_EXEC
#   if not specified.
#
# OUTPUT_DIR
#       Output directory of the generated code files.
#
# PROTO_FILES
#       A list of proto files used to generate code.
#
# PROTO_DIRS
#       Include directories for proto files. The directory containing the proto files is included by default, additional
#   directories can be added by this option.
function(shuhai_grpc_add_proto_targets)
    cmake_parse_arguments("ARG" # Variable prefix
            "" # Options
            "GENERATOR_TARGET;LIBRARY_TARGET;PROTOC_EXE;GRPC_PLUGIN_EXE;OUTPUT_DIR" # One-Value arguments
            "PROTO_FILES;PROTO_DIRS" # Multi-Value arguments
            ${ARGN})

    if(NOT ARG_GENERATOR_TARGET)
        message(FATAL_ERROR "Generator target name must be specified via option 'GENERATOR_TARGET'.")
    endif()
    if(NOT ARG_LIBRARY_TARGET)
        message(FATAL_ERROR "Library target name must be specified via option 'LIBRARY_TARGET'.")
    endif()

    set(PROTOC_EXE ${protobuf_protoc_EXEC})
    if(ARG_PROTOC_EXE)
        set(PROTOC_EXE ${ARG_PROTOC_EXE})
    endif()

    set(GRPC_PLUGIN_EXE ${grpc_cpp_plugin_EXEC})
    if(ARG_GRPC_PLUGIN_EXE)
        set(GRPC_PLUGIN_EXE ${ARG_GRPC_PLUGIN_EXE})
    endif()

    if(NOT ARG_PROTO_FILES)
        message(FATAL_ERROR "Proto file(s) is required.")
    endif()
    if(NOT ARG_OUTPUT_DIR)
        message(FATAL_ERROR "Output directory is required.")
    endif()
    make_directory(${ARG_OUTPUT_DIR})

    # proto directory (i.e. --proto_path option for protoc) includes the directory containing the specified proto files
    # by default.
    set(PROTO_DIRS "")
    foreach(PROTO_FILE ${ARG_PROTO_FILES})
        get_filename_component(DIR "${PROTO_FILE}" DIRECTORY)
        list(APPEND PROTO_DIRS ${DIR})
    endforeach()
    list(REMOVE_DUPLICATES PROTO_DIRS)

    foreach(PROTO_FILE ${ARG_PROTO_FILES})
        _shuhai_grpc_proto_add_generate_rule(GENERATED_FILES ${PROTOC_EXE} ${GRPC_PLUGIN_EXE} ${PROTO_FILE} "${PROTO_DIRS}" ${ARG_OUTPUT_DIR})
        list(APPEND GENERATED_PROTO_FILES ${GENERATED_FILES})
    endforeach()

    if(ARG_GENERATOR_TARGET)
        add_custom_target(${ARG_GENERATOR_TARGET} DEPENDS ${GENERATED_PROTO_FILES})
    endif()

    if(ARG_LIBRARY_TARGET)
        if(NOT TARGET ${ARG_LIBRARY_TARGET})
            add_library(${ARG_LIBRARY_TARGET} STATIC)
            #shuhai_add_alias_target_for_namespace(${ARG_LIBRARY_TARGET})
        endif()
        target_sources(${ARG_LIBRARY_TARGET}
                PUBLIC ${GENERATED_PROTO_FILES})
        target_include_directories(${ARG_LIBRARY_TARGET}
                PUBLIC ${ARG_OUTPUT_DIR}
                INTERFACE ${ARG_OUTPUT_DIR})
        target_link_libraries(${ARG_LIBRARY_TARGET}
                PUBLIC gRPC::grpc++
                INTERFACE gRPC::grpc++)
        if(NOT ${ARG_LIBRARY_TARGET} STREQUAL ${ARG_GENERATOR_TARGET})
            add_dependencies(${ARG_LIBRARY_TARGET} ${ARG_GENERATOR_TARGET})
        endif()
    endif()
endfunction()

function(_shuhai_grpc_proto_generate OUT_FILES)
    cmake_parse_arguments("ARG" # Variable prefix
            "" # Options
            "PROTOC_EXE;PLUGIN_EXE;PROTO_FILE;OUTPUT_DIR" # One-Value arguments
            "PROTO_DIRS" # Multi-Value arguments
            ${ARGN})

    if(NOT EXISTS ${ARG_PROTO_FILE})
        message(FATAL_ERROR "Proto file '${ARG_PROTO_FILE}' not found.")
    endif()

    set(PROTOC_EXE ${protobuf_protoc_EXEC})
    if(ARG_PROTOC_EXE)
        set(PROTOC_EXE ${ARG_PROTOC_EXE})
    endif()

    set(PLUGIN_EXE ${grpc_cpp_plugin_EXEC})
endfunction()

function(_shuhai_grpc_proto_add_generate_rule OUT_FILES PROTOC_EXE PLUGIN_EXE PROTO_FILE PROTO_DIRS OUTPUT_DIR)
    get_filename_component(PROTO_FILE ${PROTO_FILE} ABSOLUTE)
    _shuhai_grpc_make_proto_code_path(PROTO_HEADER_PATH "${PROTO_FILE}" "${OUTPUT_DIR}" ".pb.h")
    _shuhai_grpc_make_proto_code_path(PROTO_SOURCE_PATH "${PROTO_FILE}" "${OUTPUT_DIR}" ".pb.cc")
    _shuhai_grpc_make_proto_code_path(GRPC_HEADER_PATH "${PROTO_FILE}" "${OUTPUT_DIR}" ".grpc.pb.h")
    _shuhai_grpc_make_proto_code_path(GRPC_SOURCE_PATH "${PROTO_FILE}" "${OUTPUT_DIR}" ".grpc.pb.cc")
    add_custom_command(
            OUTPUT ${PROTO_HEADER_PATH} ${PROTO_SOURCE_PATH} ${GRPC_HEADER_PATH} ${GRPC_SOURCE_PATH}
            COMMAND ${PROTOC_EXE}
            --plugin=protoc-gen-grpc="${PLUGIN_EXE}"
            --grpc_out="${OUTPUT_DIR}"
            --cpp_out="${OUTPUT_DIR}"
            --proto_path="${PROTO_DIRS}"
            --experimental_allow_proto3_optional
            "${PROTO_FILE}"
            DEPENDS "${PROTO_FILE}"
            COMMENT "Generate protobuf and grpc protocol sources for \"${PROTO_FILE}\".")
    list(APPEND OUTPUT_FILES ${PROTO_HEADER_PATH} ${PROTO_SOURCE_PATH} ${GRPC_HEADER_PATH} ${GRPC_SOURCE_PATH})
    set(${OUT_FILES} ${OUTPUT_FILES} PARENT_SCOPE)
endfunction()

function(_shuhai_grpc_make_proto_code_path VAR PROTO_PATH OUTPUT_DIR EXTENSION)
    get_filename_component(FILENAME "${PROTO_PATH}" NAME_WE)
    set(${VAR} "${OUTPUT_DIR}/${FILENAME}${EXTENSION}" PARENT_SCOPE)
endfunction()

macro(_shuhai_setup_arg ARG)
endmacro()
