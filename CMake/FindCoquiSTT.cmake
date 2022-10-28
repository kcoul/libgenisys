include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

CPMAddPackage(
        NAME Coqui
        GITHUB_REPOSITORY kcoul/STT
        GIT_TAG origin/main)

#LARGE FILES NOT CONTAINED IN REPO BUT AVAILABLE FROM 0.9.3 Release Page:
#https://coqui.ai/english/coqui/v0.9.3

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LFS/model.tflite)
    message("Downloading Coqui tflite file to LFS folder")
    file(DOWNLOAD
            https://coqui.gateway.scarf.sh/english/coqui/v0.9.3/model.tflite
            ${CMAKE_CURRENT_SOURCE_DIR}/LFS/model.tflite
            SHOW_PROGRESS)
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LFS/coqui-stt-0.9.3-models.scorer)
    message("Downloading Coqui scorer file to LFS folder")
    file(DOWNLOAD
            https://coqui.gateway.scarf.sh/english/coqui/v0.9.3/coqui-stt-0.9.3-models.scorer
            ${CMAKE_CURRENT_SOURCE_DIR}/LFS/coqui-stt-0.9.3-models.scorer
            SHOW_PROGRESS)
endif()