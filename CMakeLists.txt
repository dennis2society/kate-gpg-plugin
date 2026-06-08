cmake_minimum_required(VERSION 3.16)
project(kategpgplugin VERSION 1.0)

# Default to Qt6
if(NOT QT_MAJOR_VERSION)
    set(QT_MAJOR_VERSION 6)
endif()

set(KF_DEP_VERSION "${QT_MAJOR_VERSION}")
find_package(ECM ${KF_DEP_VERSION} QUIET REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# KDEInstallDirs must be included before any find_package() call that pulls in
# GNUInstallDirs (e.g. find_package(Qt6 ...)), otherwise CMAKE_INSTALL_BINDIR
# is set by GNUInstallDirs first and KDE_INSTALL_BINDIR inherits that value,
# which produces a CMake warning.
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)

find_package(Qt${QT_MAJOR_VERSION}Widgets CONFIG REQUIRED)
find_package(Gpgmepp REQUIRED)


# use C++20 like KF6 itself
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(KF${QT_MAJOR_VERSION}TextEditor ${KF_DEP_VERSION} QUIET REQUIRED)

find_package(KF${QT_MAJOR_VERSION}
    REQUIRED COMPONENTS
        CoreAddons # Core addons on top of QtCore
        I18n # For localization
        TextEditor # The editor component
)

# This line defines the actual target
if (QT_MAJOR_VERSION EQUAL 6)
    kcoreaddons_add_plugin(kategpgplugin
        INSTALL_NAMESPACE "kf6/ktexteditor")
else ()
    kcoreaddons_add_plugin(kategpgplugin
        INSTALL_NAMESPACE "ktexteditor")
endif ()

target_include_directories(kategpgplugin PRIVATE)

target_sources(
  kategpgplugin
  PRIVATE
  kategpgplugin.hpp
  gpgkeydetails.hpp
  gpgmeppwrapper.hpp
  kategpgplugin.cpp
  gpgkeydetails.cpp
  gpgmeppwrapper.cpp
  kategpgplugin.json
)

add_compile_options(-O3 -Wall -Wextra -Wpedantic -Wno-dev -Wunused-includes -Wunused-parameter -Werror)

# This makes the plugin translatable
target_compile_definitions(kategpgplugin PRIVATE TRANSLATION_DOMAIN="kategpgplugin")

target_link_libraries(kategpgplugin
    PRIVATE
    KF${QT_MAJOR_VERSION}::CoreAddons KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::TextEditor
    gpgmepp
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
