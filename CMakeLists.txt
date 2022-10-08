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
add_library(GenisysDynamic SHARED)
target_link_libraries(GenisysDynamic PUBLIC libGenisysObjects)
add_library(GenisysStatic STATIC)
target_link_libraries(GenisysStatic PUBLIC libGenisysObjects)
target_compile_features(libGenisysObjects PUBLIC cxx_std_14)

target_include_directories(libGenisysObjects PUBLIC
		"${DeepSpeech_SOURCE_DIR}/native_client"
		libgenisys-dsp
		include
		)

set(PBMM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.pbmm")
set(TFLITE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.tflite")
set(SCORER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.scorer")

target_compile_definitions(libGenisysObjects PUBLIC
		JUCE_WEB_BROWSER=0
		JUCE_USE_CURL=0
		JUCE_DISABLE_JUCE_VERSION_PRINTING=1
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

add_library(RNNoise STATIC IMPORTED)
if (APPLE)
	set_property(TARGET RNNoise PROPERTY
			IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/macOS/librnnoise.a")
elseif(WIN32)
	set_property(TARGET RNNoise PROPERTY
			IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/Windows/rnnoise.lib")
elseif(UNIX AND NOT APPLE)
	set_property(TARGET RNNoise PROPERTY
		IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/Linux/librnnoise.a")
endif()

target_link_libraries(libGenisysObjects
		PRIVATE
		DeepSpeechPrecompiled
		wavio
		libGenisysDSP
		RNNoise
		)

if (APPLE)
	target_link_libraries(libGenisysObjects
			PRIVATE
			"-framework Accelerate"
			)
endif()

add_subdirectory(libgenisys-dsp)
add_subdirectory(Modules)
add_subdirectory(TestBench)
add_subdirectory(tools)

if(APPLE)
	set(LIBRARY_FILE "${CMAKE_CURRENT_BINARY_DIR}/libGenisysDynamic.dylib")
	set(LIBRARY_FILE_NAME "libGenisysDynamic.dylib")
	add_custom_command(TARGET GenisysDynamic POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
			"${LIBRARY_FILE}"
			"${CMAKE_CURRENT_BINARY_DIR}/TestBench/libGenisysTestBench_artefacts/$<CONFIG>/${LIBRARY_FILE_NAME}"
			COMMENT "Copying library to output directory")
endif()
