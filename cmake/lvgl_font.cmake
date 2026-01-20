# LVGL Font Conversion CMake Function
# Converts font files to C source files using lv_font_conv

# Function to convert a font file to LVGL C source
# Usage:
#   lvgl_add_font(
#       FONT_FILE path/to/font.ttf
#       SIZE 14
#       [OUTPUT_NAME custom_name]
#       [BPP 1]
#       [FORMAT lvgl]
#       [RANGE 0x20-0x7F,0x00C0-0x1EF9]
#       [SYMBOLS "symbol1,symbol2"]
#       [NO_COMPRESS]
#       [NO_PREFILTER]
#       [LCD]
#       [LCD_V]
#   )
function(lvgl_add_font)
    set(options NO_COMPRESS NO_PREFILTER LCD LCD_V)
    set(oneValueArgs FONT_FILE SIZE OUTPUT_NAME BPP FORMAT RANGE SYMBOLS)
    set(multiValueArgs)
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT ARG_FONT_FILE)
        message(FATAL_ERROR "FONT_FILE is required")
    endif()
    
    if(NOT ARG_SIZE)
        message(FATAL_ERROR "SIZE is required")
    endif()
    
    # Get absolute path to font file
    get_filename_component(FONT_ABS "${ARG_FONT_FILE}" ABSOLUTE)
    
    if(NOT EXISTS "${FONT_ABS}")
        message(FATAL_ERROR "Font file not found: ${FONT_ABS}")
    endif()
    
    # Determine output name
    if(ARG_OUTPUT_NAME)
        set(OUTPUT_BASE "${ARG_OUTPUT_NAME}")
    else()
        get_filename_component(FONT_NAME "${ARG_FONT_FILE}" NAME_WE)
        string(TOLOWER "${FONT_NAME}" FONT_NAME_LOWER)
        string(REPLACE "-" "_" FONT_NAME_LOWER "${FONT_NAME_LOWER}")
        string(REPLACE "." "_" FONT_NAME_LOWER "${FONT_NAME_LOWER}")
        set(OUTPUT_BASE "${FONT_NAME_LOWER}_${ARG_SIZE}")
    endif()
    
    # Output C file in build directory
    set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated_fonts/${OUTPUT_BASE}.c")
    
    # Check if lv_font_conv is available
    find_program(LV_FONT_CONV lv_font_conv)
    if(NOT LV_FONT_CONV)
        message(FATAL_ERROR "lv_font_conv not found. Install it with: npm install -g lv_font_conv")
    endif()
    
    # Build command arguments
    set(CMD_ARGS)
    list(APPEND CMD_ARGS "--font" "${FONT_ABS}")
    list(APPEND CMD_ARGS "--size" "${ARG_SIZE}")
    
    # BPP (bits per pixel)
    if(ARG_BPP)
        list(APPEND CMD_ARGS "--bpp" "${ARG_BPP}")
    else()
        list(APPEND CMD_ARGS "--bpp" "1")
    endif()
    
    # Format
    if(ARG_FORMAT)
        list(APPEND CMD_ARGS "--format" "${ARG_FORMAT}")
    else()
        list(APPEND CMD_ARGS "--format" "lvgl")
    endif()
    
    # Unicode range
    if(ARG_RANGE)
        list(APPEND CMD_ARGS "--range" "${ARG_RANGE}")
    else()
        list(APPEND CMD_ARGS "--range" "0x20-0x7F")
    endif()
    
    # Symbols
    if(ARG_SYMBOLS)
        list(APPEND CMD_ARGS "--symbols" "${ARG_SYMBOLS}")
    endif()
    
    # Optional flags
    if(ARG_NO_COMPRESS)
        list(APPEND CMD_ARGS "--no-compress")
    endif()
    
    if(ARG_NO_PREFILTER)
        list(APPEND CMD_ARGS "--no-prefilter")
    endif()
    
    if(ARG_LCD)
        list(APPEND CMD_ARGS "--lcd")
    endif()
    
    if(ARG_LCD_V)
        list(APPEND CMD_ARGS "--lcd-v")
    endif()
    
    # Output file
    list(APPEND CMD_ARGS "--output" "${OUTPUT_FILE}")
    
    # Create output directory
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated_fonts")
    
    # Add custom command to generate C source
    add_custom_command(
        OUTPUT "${OUTPUT_FILE}"
        COMMAND ${LV_FONT_CONV} ${CMD_ARGS}
        DEPENDS "${FONT_ABS}"
        COMMENT "Converting font ${ARG_FONT_FILE} (size ${ARG_SIZE}) to LVGL C source"
        VERBATIM
    )
    
    # Add generated file to parent scope sources
    set(LVGL_FONT_SOURCES ${LVGL_FONT_SOURCES} "${OUTPUT_FILE}" PARENT_SCOPE)
    
    message(STATUS "LVGL Font: ${ARG_FONT_FILE} (size ${ARG_SIZE}) -> ${OUTPUT_FILE}")
endfunction()

# Batch conversion function for multiple fonts with same settings
# Usage:
#   lvgl_add_fonts(
#       FONTS font1.ttf font2.ttf
#       SIZE 14
#       [BPP 1]
#       [FORMAT lvgl]
#       [RANGE 0x20-0x7F]
#       [NO_COMPRESS]
#   )
function(lvgl_add_fonts)
    set(options NO_COMPRESS NO_PREFILTER LCD LCD_V)
    set(oneValueArgs SIZE BPP FORMAT RANGE SYMBOLS)
    set(multiValueArgs FONTS)
    
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT ARG_FONTS)
        message(FATAL_ERROR "FONTS list is required")
    endif()
    
    if(NOT ARG_SIZE)
        message(FATAL_ERROR "SIZE is required")
    endif()
    
    foreach(FONT ${ARG_FONTS})
        set(CALL_ARGS FONT_FILE ${FONT} SIZE ${ARG_SIZE})
        
        if(ARG_BPP)
            list(APPEND CALL_ARGS BPP ${ARG_BPP})
        endif()
        
        if(ARG_FORMAT)
            list(APPEND CALL_ARGS FORMAT ${ARG_FORMAT})
        endif()
        
        if(ARG_RANGE)
            list(APPEND CALL_ARGS RANGE ${ARG_RANGE})
        endif()
        
        if(ARG_SYMBOLS)
            list(APPEND CALL_ARGS SYMBOLS ${ARG_SYMBOLS})
        endif()
        
        if(ARG_NO_COMPRESS)
            list(APPEND CALL_ARGS NO_COMPRESS)
        endif()
        
        if(ARG_NO_PREFILTER)
            list(APPEND CALL_ARGS NO_PREFILTER)
        endif()
        
        if(ARG_LCD)
            list(APPEND CALL_ARGS LCD)
        endif()
        
        if(ARG_LCD_V)
            list(APPEND CALL_ARGS LCD_V)
        endif()
        
        lvgl_add_font(${CALL_ARGS})
    endforeach()
    
    # Propagate sources to parent scope
    set(LVGL_FONT_SOURCES ${LVGL_FONT_SOURCES} PARENT_SCOPE)
endfunction()
