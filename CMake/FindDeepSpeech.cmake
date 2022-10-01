include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

CPMAddPackage(
        NAME DeepSpeech
        GITHUB_REPOSITORY kcoul/DeepSpeech
        GIT_TAG origin/master)



#LARGE FILES NOT CONTAINED IN REPO BUT AVAILABLE FROM 0.9.3 Release Page:
#https://github.com/mozilla/DeepSpeech/releases/tag/v0.9.3
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.pbmm")
    file(DOWNLOAD
            https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.pbmm
            ${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.pbmm
            SHOW_PROGRESS)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.tflite)
    file(DOWNLOAD
            https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.tflite
            ${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.tflite
            SHOW_PROGRESS)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.scorer)
    file(DOWNLOAD
            https://github.com/mozilla/DeepSpeech/releases/download/v0.9.3/deepspeech-0.9.3-models.scorer
            ${CMAKE_CURRENT_SOURCE_DIR}/LFS/deepspeech-0.9.3-models.scorer
            SHOW_PROGRESS)
endif()