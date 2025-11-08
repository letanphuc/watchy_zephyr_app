# LVGL Image Conversion CMake Function
# Converts image files to C source files using LVGL's LVGLImage.py script

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Function to convert an image file to LVGL C source
# Usage:
#   lvgl_add_image(
#       IMAGE_FILE path/to/image.png
#       [OUTPUT_NAME custom_name]
#       [COLOR_FORMAT RGB565]
#       [COMPRESS NONE|RLE|LZ4]
#       [PREMULTIPLY]
#       [RGB565_DITHER]
#   )
function(lvgl_add_image)
    set(options PREMULTIPLY RGB565_DITHER)
    set(oneValueArgs IMAGE_FILE OUTPUT_NAME COLOR_FORMAT COMPRESS)
    set(multiValueArgs)
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT ARG_IMAGE_FILE)
        message(FATAL_ERROR "IMAGE_FILE is required")
    endif()
    
    # Get absolute path to image file
    get_filename_component(IMAGE_ABS "${ARG_IMAGE_FILE}" ABSOLUTE)
    
    if(NOT EXISTS "${IMAGE_ABS}")
        message(FATAL_ERROR "Image file not found: ${IMAGE_ABS}")
    endif()
    
    # Determine output name
    if(ARG_OUTPUT_NAME)
        set(OUTPUT_BASE "${ARG_OUTPUT_NAME}")
    else()
        get_filename_component(OUTPUT_BASE "${ARG_IMAGE_FILE}" NAME_WE)
        string(REPLACE "-" "_" OUTPUT_BASE "${OUTPUT_BASE}")
        string(REPLACE "." "_" OUTPUT_BASE "${OUTPUT_BASE}")
    endif()
    
    # Output C file in build directory
    set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated_images/${OUTPUT_BASE}.c")
    
    # Path to LVGLImage.py script
    set(LVGL_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../modules/lib/gui/lvgl/scripts/LVGLImage.py")
    
    if(NOT EXISTS "${LVGL_SCRIPT}")
        message(FATAL_ERROR "LVGLImage.py script not found at: ${LVGL_SCRIPT}")
    endif()
    
    # Build command arguments
    set(CMD_ARGS "${LVGL_SCRIPT}")
    list(APPEND CMD_ARGS "--ofmt" "C")
    list(APPEND CMD_ARGS "-o" "${CMAKE_CURRENT_BINARY_DIR}/generated_images")
    
    # Color format
    if(ARG_COLOR_FORMAT)
        list(APPEND CMD_ARGS "--cf" "${ARG_COLOR_FORMAT}")
    else()
        list(APPEND CMD_ARGS "--cf" "RGB565")
    endif()
    
    # Compression
    if(ARG_COMPRESS)
        list(APPEND CMD_ARGS "--compress" "${ARG_COMPRESS}")
    else()
        list(APPEND CMD_ARGS "--compress" "NONE")
    endif()
    
    # Optional flags
    if(ARG_PREMULTIPLY)
        list(APPEND CMD_ARGS "--premultiply")
    endif()
    
    if(ARG_RGB565_DITHER)
        list(APPEND CMD_ARGS "--rgb565dither")
    endif()
    
    # Output name
    if(ARG_OUTPUT_NAME)
        list(APPEND CMD_ARGS "--name" "${OUTPUT_BASE}")
    endif()
    
    # Input file (must be last)
    list(APPEND CMD_ARGS "${IMAGE_ABS}")
    
    # Create output directory
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated_images")
    
    # Add custom command to generate C source
    add_custom_command(
        OUTPUT "${OUTPUT_FILE}"
        COMMAND ${Python3_EXECUTABLE} ${CMD_ARGS}
        DEPENDS "${IMAGE_ABS}" "${LVGL_SCRIPT}"
        COMMENT "Converting image ${ARG_IMAGE_FILE} to LVGL C source"
        VERBATIM
    )
    
    # Add generated file to parent scope sources
    set(LVGL_IMAGE_SOURCES ${LVGL_IMAGE_SOURCES} "${OUTPUT_FILE}" PARENT_SCOPE)
    
    message(STATUS "LVGL Image: ${ARG_IMAGE_FILE} -> ${OUTPUT_FILE}")
endfunction()

# Batch conversion function for multiple images
# Usage:
#   lvgl_add_images(
#       IMAGES image1.png image2.png image3.png
#       [COLOR_FORMAT RGB565]
#       [COMPRESS NONE]
#       [PREMULTIPLY]
#       [RGB565_DITHER]
#   )
function(lvgl_add_images)
    set(options PREMULTIPLY RGB565_DITHER)
    set(oneValueArgs COLOR_FORMAT COMPRESS)
    set(multiValueArgs IMAGES)
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT ARG_IMAGES)
        message(FATAL_ERROR "IMAGES list is required")
    endif()
    
    foreach(IMAGE ${ARG_IMAGES})
        set(CALL_ARGS IMAGE_FILE ${IMAGE})
        
        if(ARG_COLOR_FORMAT)
            list(APPEND CALL_ARGS COLOR_FORMAT ${ARG_COLOR_FORMAT})
        endif()
        
        if(ARG_COMPRESS)
            list(APPEND CALL_ARGS COMPRESS ${ARG_COMPRESS})
        endif()
        
        if(ARG_PREMULTIPLY)
            list(APPEND CALL_ARGS PREMULTIPLY)
        endif()
        
        if(ARG_RGB565_DITHER)
            list(APPEND CALL_ARGS RGB565_DITHER)
        endif()
        
        lvgl_add_image(${CALL_ARGS})
    endforeach()
    
    # Propagate sources to parent scope
    set(LVGL_IMAGE_SOURCES ${LVGL_IMAGE_SOURCES} PARENT_SCOPE)
endfunction()
