function(add_sce_font_ft_library target)
    set(fontDir ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
    add_library(${target} SHARED EXCLUDE_FROM_ALL
            ${fontDir}/Export.cpp
            ${fontDir}/src/Driver.cpp
    )
    target_include_directories(${target} PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(${target} PRIVATE freetype libc)
    set_target_properties(${target} PROPERTIES
            CXX_EXTENSIONS OFF
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
    )
    configure_windows_unwind(${target})
    if(BUILD_TESTING)
        add_test_executable(font_ft_engine_tests ${fontDir}/tests/FontFtEngine.cpp ${fontDir}/Export.cpp)
        target_include_directories(font_ft_engine_tests PRIVATE ${LIBS_INCLUDE_DIR})
        configure_windows_unwind(font_ft_engine_tests)
        add_test(NAME font_ft_engine COMMAND font_ft_engine_tests)
    endif()
endfunction()
