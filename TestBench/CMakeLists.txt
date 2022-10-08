project(libGenisysTestBench VERSION 0.0.1)

set(TargetName ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

#helper function to add all images from a given folder:
function(add_images_from_directory target imagesSubFolder)
    set (ImagesDir ${CMAKE_CURRENT_LIST_DIR}/${imagesSubFolder})
    file(GLOB_RECURSE images
            "${ImagesDir}/*.jpg"
            "${ImagesDir}/*.png"
            "${ImagesDir}/*.jpeg")

    if(NOT images STREQUAL "")
        set (ImagesTarget "${target}-Images")
        juce_add_binary_data(${ImagesTarget} SOURCES ${images})
        target_link_libraries(${target} PRIVATE ${ImagesTarget})
    endif()
endfunction()

juce_add_gui_app(${TargetName} PRODUCT_NAME "libGenisysTestBench"
        COMPANY_NAME="MusicLink Consulting Ltd."
        PRODUCT_NAME="libGenisys Test Bench"
        VERSION="0.0.1"
        BUNDLE_ID="com.libgenisys.testbench"
        MICROPHONE_PERMISSION_ENABLED=TRUE
        MICROPHONE_PERMISSION_TEXT="libGenisysTestBench requires microphone access"
        BLUETOOTH_PERMISSION_ENABLED=TRUE
        BLUETOOTH_PERMISSION_TEXT="libGenisysTestBench requires bluetooth access"
        ICON_SMALL "${CMAKE_CURRENT_LIST_DIR}/Images/geniSYS.png"
        ICON_BIG "${CMAKE_CURRENT_LIST_DIR}/Images/geniSYS.png")

#add_images_from_directory needs to be below juce_add_gui_app
add_images_from_directory(${TargetName} Images)

#==============
#TARGET_SOURCES
#==============

set (SRC_LIST
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainWindow.cpp)

target_sources(${TargetName} PRIVATE
        ${SRC_LIST})

target_include_directories(${TargetName} PRIVATE ${CMAKE_SOURCE_DIR}/src)

#==========================
#TARGET_COMPILE_DEFINITIONS
#==========================

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>"
        JUCE_DISPLAY_SPLASH_SCREEN=0)

add_library(LibGenisys SHARED IMPORTED)
if(APPLE)
    set_property(TARGET LibGenisys PROPERTY
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/cmake-build-debug/libGenisysDynamic.dylib")
elseif(WIN32)
    set_property(TARGET LibGenisys PROPERTY
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/cmake-build-debug/GenisysDynamic.dll")
    set_property(TARGET LibGenisys PROPERTY
            IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/cmake-build-debug/GenisysDynamic.lib")

    #Copy DLLs to TestBench build directory
    set(LIBRARY_FILE "${CMAKE_SOURCE_DIR}/cmake-build-debug/GenisysDynamic.dll")
    set(LIBRARY_FILE_NAME "GenisysDynamic.dll")
    add_custom_command(TARGET ${TargetName} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            "${LIBRARY_FILE}"
            "${CMAKE_CURRENT_BINARY_DIR}/libGenisysTestBench_artefacts/$<CONFIG>/${LIBRARY_FILE_NAME}"
            COMMENT "Copying library to output directory")

    set(DS_LIBRARY_FILE "${CMAKE_SOURCE_DIR}/libs/Windows/libDeepSpeech.so")
    set(DS_LIBRARY_FILE_NAME "libDeepSpeech.so")
    add_custom_command(TARGET ${TargetName} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
            "${DS_LIBRARY_FILE}"
            "${CMAKE_CURRENT_BINARY_DIR}/libGenisysTestBench_artefacts/$<CONFIG>/${DS_LIBRARY_FILE_NAME}"
            COMMENT "Copying libDeepSpeech to output directory")

elseif(UNIX AND NOT APPLE)
    set_property(TARGET LibGenisys PROPERTY
            IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/cmake-build-debug/libGenisysDynamic.so")
endif()

target_compile_definitions(${TargetName} PUBLIC
        JUCE_DISABLE_JUCE_VERSION_PRINTING=1)

#=====================
#TARGET_LINK_LIBRARIES
#=====================

target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        gin
        gin_dsp
        hack_audio_gui
        ff_meters
        LibGenisys)