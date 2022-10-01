#
# 1. CMake preamble / policy setup
#
cmake_minimum_required(VERSION 3.19)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

# See SWI-1661
# The policy uses the download time for timestamp, instead of the timestamp in the archive. This
# allows for proper rebuilds when a projects url changes
if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif ()

#
# 2. Project-wide setup
#
project(libGenisys)
enable_testing()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

#
# 3. Dependencies
#
include(CMake/CPM.cmake)
find_package(DeepSpeech)
find_package(JUCE)
find_package(Gin)

#
# 4. Main build targets
#
add_library(libGenisysObjects OBJECT src/LibGenisysAPI.cpp src/LibGenisysAPI.h src/LibGenisysImpl.cpp src/LibGenisysImpl.h)
add_library(libGenisysDynamic SHARED)
target_link_libraries(libGenisysDynamic PUBLIC libGenisysObjects)
add_library(libGenisysStatic STATIC)
target_link_libraries(libGenisysStatic PUBLIC libGenisysObjects)
target_compile_features(libGenisysObjects PUBLIC cxx_std_14)

target_include_directories(libGenisysObjects PUBLIC
		"${DeepSpeech_SOURCE_DIR}/native_client"
		)

set(PBMM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.pbmm")
set(TFLITE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.tflite")
set(SCORER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.scorer")

target_compile_definitions(libGenisysObjects PUBLIC
		JUCE_WEB_BROWSER=0
		JUCE_USE_CURL=0
		"PBMM_PATH=\"${PBMM_PATH}\""
		"TFLITE_PATH=\"${TFLITE_PATH}\""
		"SCORER_PATH=\"${SCORER_PATH}\"")

add_library(DeepSpeechPrecompiled SHARED IMPORTED)
if (APPLE)
	set_property(TARGET DeepSpeechPrecompiled PROPERTY
			IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/macOS/libdeepspeech.so")
elseif(WIN32)
	set_property(TARGET DeepSpeechPrecompiled PROPERTY
			IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/Windows/libdeepspeech.so")
	set_property(TARGET DeepSpeechPrecompiled PROPERTY
			IMPORTED_IMPLIB "${PROJECT_SOURCE_DIR}/libs/Windows/libdeepspeech.so.if.lib")
elseif(UNIX AND NOT APPLE)
	set_property(TARGET DeepSpeechPrecompiled PROPERTY
			IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/Linux/libdeepspeech.so")
endif()

target_link_libraries(libGenisysObjects
		PRIVATE
		juce::juce_dsp
		gin
		gin_dsp
		DeepSpeechPrecompiled
		wavio
		)

add_subdirectory(Modules)
add_subdirectory(TestBench)
add_subdirectory(tools)