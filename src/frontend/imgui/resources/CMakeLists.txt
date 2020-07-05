include(JSONParser)

file(GLOB RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/files/*.png ${CMAKE_SOURCE_DIR}/files/*.ttf)

add_custom_command(
        OUTPUT resourcedata.c
        COMMAND resourcer ${CMAKE_CURRENT_SOURCE_DIR}/files/ ${CMAKE_CURRENT_BINARY_DIR}/resourcedata.c
        DEPENDS resourcer ${RESOURCE_FILES}
)

#ADD_CUSTOM_TARGET(resourcedata DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/resourcedata.c)
add_library(resourcedata STATIC ${CMAKE_CURRENT_BINARY_DIR}/resourcedata.c)

function(convert_font file symbol output)
    get_filename_component(full_path ${file} ABSOLUTE)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${output}
            COMMAND binary_to_compressed_c "-base85" "${full_path}" "${symbol}" > ${CMAKE_CURRENT_BINARY_DIR}/${output}
            DEPENDS binary_to_compressed_c
            COMMENT "Regenerating ${output}")
    add_custom_target(${symbol}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${output}
            COMMENT "Checking for need to rebuild ${output}")
endfunction()

function (number_to_hex number output)
    set (chars "0123456789abcdef")
    set (hex "")
    foreach (i RANGE 3)
        math (EXPR nibble "${number} & 15")
        string (SUBSTRING "${chars}" "${nibble}" 1 nibble_hex)
        set(hex "${nibble_hex}${hex}")
        math (EXPR number "${number} >> 4")
    endforeach ()
    set ("${output}" "${hex}" PARENT_SCOPE)
endfunction ()

# parse feather.json
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Feather.json feather_json)
sbeParseJson(feather_icons feather_json)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/feather_icons.h "#pragma once\n")
set(first YES)
set(min_char NO)
set(max_char NO)
foreach(var ${feather_icons})
    string(REGEX REPLACE "feather_icons\\." "ICON_FTH_" label ${var})
    string(REGEX REPLACE "-" "_" label "${label}")
    string(TOUPPER "${label}" label)
    number_to_hex(${${var}} hexval)
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/feather_icons.h "#define ${label} u8\"\\u${hexval}\"\n")
    if(first)
        set(min_char ${${var}})
        set(max_char ${${var}})
        set(first NO)
    else()
        if(min_char GREATER ${${var}})
            set(min_char ${${var}})
        endif()
        if(max_char LESS ${${var}})
            set(max_char ${${var}})
        endif()
    endif()
endforeach()
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/feather_icons.h "\n#define ICON_MIN_FTH ${min_char}\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/feather_icons.h "#define ICON_MAX_FTH ${max_char}\n")

convert_font(files/Feather.ttf FeatherIcons feather_icons.cpp)
convert_font(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui/misc/fonts/Roboto-Medium.ttf RobotoMediumFont roboto_medium.cpp)
