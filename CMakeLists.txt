cmake_minimum_required(VERSION 3.16)
project(kate_gpg_plugin VERSION 1.0)

set(KF_DEP_VERSION "${QT_MAJOR_VERSION}")
find_package(ECM ${KF_DEP_VERSION} QUIET REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt${QT_MAJOR_VERSION}Widgets CONFIG REQUIRED)
find_package(Gpgmepp REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(KF${QT_MAJOR_VERSION}TextEditor ${KF_DEP_VERSION} QUIET REQUIRED)

find_package(KF${QT_MAJOR_VERSION}
    REQUIRED COMPONENTS
        CoreAddons # Core addons on top of QtCore
        I18n # For localization
        TextEditor # The editor component
)

# This line defines the actual target
if (QT_MAJOR_VERSION EQUAL 6)
    kcoreaddons_add_plugin(kate_gpg_plugin
        INSTALL_NAMESPACE "kf6/ktexteditor")
else ()
    kcoreaddons_add_plugin(kate_gpg_plugin
        INSTALL_NAMESPACE "ktexteditor")
endif ()

target_include_directories(kate_gpg_plugin PRIVATE include)

target_sources(
  kate_gpg_plugin
  PRIVATE
  include/kate_gpg_plugin.hpp
  include/GPGKeyDetails.hpp
  include/GPGMeWrapper.hpp
  src/kate_gpg_plugin.cpp
  src/GPGKeyDetails.cpp
  src/GPGMeWrapper.cpp
  kate_gpg_plugin.json
)

add_compile_options(-O3 -Wall -Wextra -Wpedantic -Wno-dev)

# This makes the plugin translatable
target_compile_definitions(kate_gpg_plugin PRIVATE TRANSLATION_DOMAIN="kate_gpg_plugin")

target_link_libraries(kate_gpg_plugin
    PRIVATE
    KF${QT_MAJOR_VERSION}::CoreAddons KF${QT_MAJOR_VERSION}::I18n KF${QT_MAJOR_VERSION}::TextEditor
    gpgmepp
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
