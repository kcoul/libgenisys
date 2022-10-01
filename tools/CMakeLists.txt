if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.3.0")
    # Tools won't build due to incomplete C++17 support
else()
    add_subdirectory(wavio)
endif()
