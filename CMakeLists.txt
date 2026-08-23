cmake_minimum_required(VERSION 3.24)
project(Quickary VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(QUICKARY_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(QUICKARY_ENABLE_PINYIN "Enable optional cpp-pinyin matching when the package is available" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

if(QUICKARY_ENABLE_PINYIN)
    find_package(cpp-pinyin CONFIG QUIET)
endif()

qt_add_executable(Quickary
    src/main.cpp
    src/core/SearchTypes.h
    src/core/AppSettings.h
    src/core/AppSettings.cpp
    src/core/ConfigStore.h
    src/core/ConfigStore.cpp
    src/core/QueryParser.h
    src/core/QueryParser.cpp
    src/core/Ranker.h
    src/core/Ranker.cpp
    src/core/UsageStore.h
    src/core/UsageStore.cpp
    src/core/SearchCoordinator.h
    src/core/SearchCoordinator.cpp
    src/providers/ISearchProvider.h
    src/providers/EverythingProvider.h
    src/providers/EverythingProvider.cpp
    src/providers/AppProvider.h
    src/providers/AppProvider.cpp
    src/providers/CommandProvider.h
    src/providers/CommandProvider.cpp
    src/providers/WebProvider.h
    src/providers/WebProvider.cpp
    src/providers/FavoritesProvider.h
    src/providers/FavoritesProvider.cpp
    src/providers/RecentProvider.h
    src/providers/RecentProvider.cpp
    src/platform/HotkeyManager.h
    src/platform/HotkeyManager.cpp
    src/platform/ShellActions.h
    src/platform/ShellActions.cpp
    src/platform/ShellPreviewHost.h
    src/platform/ShellPreviewHost.cpp
    src/platform/DialogNavigator.h
    src/platform/DialogNavigator.cpp
    src/ui/SearchResultModel.h
    src/ui/SearchResultModel.cpp
    src/ui/ResultFilterProxy.h
    src/ui/ResultFilterProxy.cpp
    src/ui/ResultDelegate.h
    src/ui/ResultDelegate.cpp
    src/ui/PreviewPane.h
    src/ui/PreviewPane.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/LauncherWindow.h
    src/ui/LauncherWindow.cpp
)

set_target_properties(Quickary PROPERTIES WIN32_EXECUTABLE TRUE)

target_link_libraries(Quickary PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent
)

if(TARGET cpp-pinyin::cpp-pinyin)
    target_link_libraries(Quickary PRIVATE cpp-pinyin::cpp-pinyin)
    target_compile_definitions(Quickary PRIVATE QUICKARY_HAVE_CPP_PINYIN=1)

    # cpp-pinyin needs its runtime dictionary. vcpkg installs it under share/cpp-pinyin/dict.
    # Users may also point QUICKARY_CPP_PINYIN_DICT_DIR at a source/install dictionary.
    set(QUICKARY_CPP_PINYIN_DICT_DIR "" CACHE PATH "Path containing cpp-pinyin mandarin/ and cantonese/ dictionaries")
    if(NOT QUICKARY_CPP_PINYIN_DICT_DIR AND DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_quickary_vcpkg_dict "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict")
        if(EXISTS "${_quickary_vcpkg_dict}/mandarin")
            set(QUICKARY_CPP_PINYIN_DICT_DIR "${_quickary_vcpkg_dict}")
        endif()
    endif()
    if(QUICKARY_CPP_PINYIN_DICT_DIR AND EXISTS "${QUICKARY_CPP_PINYIN_DICT_DIR}/mandarin")
        add_custom_command(TARGET Quickary POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${QUICKARY_CPP_PINYIN_DICT_DIR}" "$<TARGET_FILE_DIR:Quickary>/dict")
    else()
        message(STATUS "cpp-pinyin found, but runtime dictionary was not auto-located; set QUICKARY_CPP_PINYIN_DICT_DIR to enable pinyin at runtime")
    endif()
endif()

if(WIN32)
    target_compile_definitions(Quickary PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(Quickary PRIVATE shell32 shlwapi ole32 oleaut32 user32 advapi32)
endif()

if(QUICKARY_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(Quickary PRIVATE /W4 /permissive- /EHsc /utf-8)
    else()
        target_compile_options(Quickary PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

install(TARGETS Quickary RUNTIME DESTINATION .)
