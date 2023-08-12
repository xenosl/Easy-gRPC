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


# Add a custom target that generates code from proto files, and a static library target builds the generated code if the
# given target name does not refer to an existing target.
#
# shuhai_grpc_add_proto_targets(
#   CODEGEN_TARGET target1
#   LIBRARY_TARGET target2
#   [PROTOC_EXE protoc_executable_path]
#   [GRPC_PLUGIN_EXE grpc_plugin_excutable_path]
#   [INCLUDE_INSTALL_INTERFACE dir1 dir2 ...]
#   [fatal_warnings]
#   [print_free_field_numbers]
#   [enable_codegen_trace]
#   grpc_out output_dir
#   cpp_out output_dir
#   proto_path path
#   PROTO_FILES p1.proto p2.proto ...)
#
# CODEGEN_TARGET
#       Target name of the proto code generator.
#
# LIBRARY_TARGET
#       Target name of the proto code library.
#       A static library target is added if there isn't any target with same name exists; otherwise the generated code is
#   added to the existing target.
#       The generated proto code will be added to the target as sources.
#       The output directory will be added to the target as include directory.
#       The generator target (which specified by CODEGEN_TARGET option) will be added as dependency of the library target
#   if they are different targets.
#
# PROTOC_EXE
#       The protoc executable used to generates proto (*.pb.cc/h) code. Default to protobuf_protoc_EXEC if not specified.
#
# GRPC_PLUGIN_EXE
#       The grpc plugin executable of protoc used to generates grpc (*.grpc.pb.cc/h) code. Default to grpc_cpp_plugin_EXEC
#   if not specified.
#
# INCLUDE_INSTALL_INTERFACE
#       If set, the generator expression "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_INTERFACE}>" will be added to the include
#   directories of the LIBRARY_TARGET.
#
# OUTPUT_DIRECTORY
#       If the option is set, the option cpp_out and grpc_out is set to the value of it by default, otherwise cpp_out and
#   grpc_out is required to be set explicitly.
#
# Other options used by protoc.exe keep the same name as the program.
function(shuhai_grpc_add_proto_targets)
    cmake_parse_arguments("ARG" # Variable prefix
            "fatal_warnings;print_free_field_numbers;enable_codegen_trace" # Options
            "CODEGEN_TARGET;LIBRARY_TARGET;PROTOC_EXE;GRPC_PLUGIN_EXE;" # One-Value arguments
            "PROTO_FILES;proto_path;cpp_out;grpc_out;OUTPUT_DIRECTORY;INCLUDE_INSTALL_INTERFACE" # Multi-Value arguments
            ${ARGN})

    if(NOT ARG_CODEGEN_TARGET)
        message(FATAL_ERROR "Generator target name must be specified via option 'CODEGEN_TARGET'.")
    endif()
    if(NOT ARG_LIBRARY_TARGET)
        message(FATAL_ERROR "Library target name must be specified via option 'LIBRARY_TARGET'.")
    endif()

    _shuhai_grpc_setup_protoc_args()
    _shuhai_grpc_create_output_directories()
    _shuhai_grpc_make_protoc_command()

    # Generates codes immediately during CMake generate stage so that the generated codes are available for the library
    # target in the first place.
    execute_process(COMMAND ${PROTOC_COMMAND} COMMAND_ERROR_IS_FATAL ANY)

    _shuhai_grpc_extract_protoc_output_files(OUTPUT_FILES)

    if(ARG_CODEGEN_TARGET)
        add_custom_command(OUTPUT "${OUTPUT_FILES}" COMMAND ${PROTOC_COMMAND} DEPENDS "${OUTPUT_FILES}")
        add_custom_target(${ARG_CODEGEN_TARGET} DEPENDS ${OUTPUT_FILES})
    endif()

    if(ARG_LIBRARY_TARGET)
        if(NOT TARGET ${ARG_LIBRARY_TARGET})
            add_library(${ARG_LIBRARY_TARGET} STATIC)
        endif()

        target_sources(${ARG_LIBRARY_TARGET}
                PRIVATE ${OUTPUT_FILES})

        unset(INCLUDE_DIRECTORIES)
        list(APPEND INCLUDE_DIRECTORIES ${cpp_out} ${grpc_out})
        list(REMOVE_DUPLICATES INCLUDE_DIRECTORIES)
        if(ARG_INCLUDE_INSTALL_INTERFACE)
            list(APPEND INTERFACE_INCLUDE_DIRECTORIES $<BUILD_INTERFACE:${INCLUDE_DIRECTORIES}>)
            list(APPEND INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:${ARG_INCLUDE_INSTALL_INTERFACE}>)
        endif()
        target_include_directories(${ARG_LIBRARY_TARGET}
                PRIVATE ${INCLUDE_DIRECTORIES}
                INTERFACE ${INTERFACE_INCLUDE_DIRECTORIES})

        target_link_libraries(${ARG_LIBRARY_TARGET}
                PUBLIC gRPC::grpc++)

        if(NOT ${ARG_LIBRARY_TARGET} STREQUAL ${ARG_CODEGEN_TARGET})
            add_dependencies(${ARG_LIBRARY_TARGET} ${ARG_CODEGEN_TARGET})
        endif()
    endif()
endfunction()

# Execute the protoc.exe command line with given options as child process.
function(shuhai_grpc_execute_protoc)
    _shuhai_grpc_parse_protoc_args(${ARGN})
    _shuhai_grpc_setup_protoc_args()
    _shuhai_grpc_create_output_directories()
    _shuhai_grpc_make_protoc_command()

    message(STATUS "Add proto targets with commands:")
    foreach(LINE ${PROTOC_COMMAND})
        message(STATUS "  ${LINE}")
    endforeach()

    execute_process(COMMAND ${PROTOC_COMMAND} COMMAND_ERROR_IS_FATAL ANY)
endfunction()

# Add custom command for generating proto codes with given options.
function(shuhai_grpc_add_protoc_command OUTPUT_FILES)
    _shuhai_grpc_parse_protoc_args(${ARGN})
    _shuhai_grpc_setup_protoc_args()
    _shuhai_grpc_create_output_directories()
    _shuhai_grpc_make_protoc_command()
    _shuhai_grpc_extract_protoc_output_files(OUTPUT_FILES)
    add_custom_command(OUTPUT "${OUTPUT_FILES}" COMMAND ${PROTOC_COMMAND} DEPENDS "${OUTPUT_FILES}")
endfunction()

# Parse the macro arguments into protoc command-line arguments.
macro(_shuhai_grpc_parse_protoc_args)
    cmake_parse_arguments("ARG" # Variable prefix
            "fatal_warnings;print_free_field_numbers;enable_codegen_trace" # Options
            "PROTOC_EXE;GRPC_PLUGIN_EXE" # One-Value arguments
            "PROTO_FILES;proto_path;cpp_out;grpc_out;OUTPUT_DIRECTORY" # Multi-Value arguments
            ${ARGN})
endmacro()

# Setup protoc command-line arguments for later use.
# The macro assumes that the protoc arguments already parsed by _shuhai_grpc_parse_protoc_args in current scope.
macro(_shuhai_grpc_setup_protoc_args)
    set(PROTOC_EXE ${protobuf_protoc_EXEC})
    if(ARG_PROTOC_EXE)
        set(PROTOC_EXE ${ARG_PROTOC_EXE})
    endif()
    set(GRPC_PLUGIN_EXE ${grpc_cpp_plugin_EXEC})
    if(ARG_GRPC_PLUGIN_EXE)
        set(GRPC_PLUGIN_EXE ${ARG_GRPC_PLUGIN_EXE})
    endif()

    if(NOT ARG_PROTO_FILES)
        message(FATAL_ERROR "*.proto files not specified (by PROTO_FILES option).")
    endif()
    set(PROTO_FILES ${ARG_PROTO_FILES})

    if(ARG_fatal_warnings)
        set(fatal_warnings ${ARG_fatal_warnings})
    endif()
    if(ARG_print_free_field_numbers)
        set(print_free_field_numbers ${ARG_print_free_field_numbers})
    endif()
    if(enable_codegen_trace)
        set(enable_codegen_trace ${ARG_enable_codegen_trace})
    endif()

    # Set default output directory for cpp_out and grpc_out.
    if(ARG_OUTPUT_DIRECTORY)
        set(OUTPUT_DIRECTORY ${ARG_OUTPUT_DIRECTORY})
        set(cpp_out ${OUTPUT_DIRECTORY})
        set(grpc_out ${OUTPUT_DIRECTORY})
    endif()

    # Override cpp_out with explicit specified option if possible.
    if(ARG_cpp_out)
        set(cpp_out ${ARG_cpp_out})
    endif()
    if(NOT cpp_out)
        message(FATAL_ERROR "cpp_out or OUTPUT_DIRECTORY is required to specify the output directory for generated Protobuf codes.")
    endif()

    # Override grpc_out with explicit specified option if possible.
    if(ARG_grpc_out)
        set(grpc_out ${ARG_grpc_out})
    endif()
    if(NOT grpc_out)
        message(FATAL_ERROR "grpc_out or OUTPUT_DIRECTORY is required to specify the output directory for generated gRPC codes.")
    endif()

    if(ARG_proto_path)
        set(proto_path ${ARG_proto_path})
    else()
        # Take the containing directory of the first proto file as proto_path by default.
        list(GET PROTO_FILES 0 PROTO_FILE_0)
        cmake_path(GET PROTO_FILE_0 PARENT_PATH proto_path)
    endif()
endmacro()

# Create output directories for Protobuf and gRPC generated codes.
# The macro assumes that the protoc arguments already parsed by _shuhai_grpc_parse_protoc_args in current scope.
macro(_shuhai_grpc_create_output_directories)
    file(MAKE_DIRECTORY ${cpp_out})
    file(MAKE_DIRECTORY ${grpc_out})
endmacro()

# Store the full command into a list for later use in execute_process or add_custom_command.
# The macro assumes that the protoc arguments already parsed by _shuhai_grpc_parse_protoc_args in current scope.
macro(_shuhai_grpc_make_protoc_command)
    list(APPEND PROTOC_COMMAND "${PROTOC_EXE}")
    list(APPEND PROTOC_COMMAND "--plugin=protoc-gen-grpc=${GRPC_PLUGIN_EXE}")

    if(fatal_warnings)
        list(APPEND PROTOC_COMMAND "--fatal_warnings")
    endif()
    if(print_free_field_numbers)
        list(APPEND PROTOC_COMMAND "--print_free_field_numbers")
    endif()
    if(enable_codegen_trace)
        list(APPEND PROTOC_COMMAND "--enable_codegen_trace")
    endif()

    if(cpp_out)
        list(APPEND PROTOC_COMMAND "--cpp_out=${cpp_out}")
    endif()
    if(grpc_out)
        list(APPEND PROTOC_COMMAND "--grpc_out=${grpc_out}")
    endif()
    if(proto_path)
        list(APPEND PROTOC_COMMAND "--proto_path=${proto_path}")
    endif()
    list(APPEND PROTOC_COMMAND "${PROTO_FILES}")
endmacro()

# Extract output files' path from protoc arguments.
# The function assumes that the protoc arguments already parsed by _shuhai_grpc_parse_protoc_args in parent scope.
function(_shuhai_grpc_extract_protoc_output_files VAR)
    unset(OUTPUT_FILES)

    foreach(PROTO_FILE ${PROTO_FILES})
        cmake_path(RELATIVE_PATH PROTO_FILE BASE_DIRECTORY ${proto_path} OUTPUT_VARIABLE PROTO_FILE_SUB)
        cmake_path(REMOVE_EXTENSION PROTO_FILE_SUB)
        if(NOT PROTO_FILE_SUB)
            message(WARNING "'Proto path (which specified by 'proto_path' option) ${proto_path} does not encompass the provided proto file '${PROTO_FILE}'")
            continue()
        endif()

        foreach(EXT .pb.h .pb.cc .grpc.pb.h .grpc.pb.cc)
            list(APPEND OUTPUT_FILES ${cpp_out}/${PROTO_FILE_SUB}${EXT})
        endforeach()
    endforeach()

    set(${VAR} ${OUTPUT_FILES} PARENT_SCOPE)
endfunction()
