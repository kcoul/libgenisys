add_library(wavio STATIC
            wavio.cpp
            wavreader.c
            wavwriter.c
            )
target_compile_features(wavio PUBLIC cxx_std_17)
target_include_directories(wavio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})