include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

CPMAddPackage(
        NAME Gin
        GITHUB_REPOSITORY FigBug/Gin
        GIT_TAG origin/master)
		
juce_add_modules(${Gin_SOURCE_DIR}/modules/gin)
juce_add_modules(${Gin_SOURCE_DIR}/modules/gin_dsp)