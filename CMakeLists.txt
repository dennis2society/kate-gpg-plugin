cmake_minimum_required(VERSION 3.16)
project(kate_gpg_plugin VERSION 1.0)

find_package(ECM ${KF5_DEP_VERSION} QUIET REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt${QT_MAJOR_VERSION}Widgets CONFIG REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(KF${QT_MAJOR_VERSION}TextEditor ${KF5_DEP_VERSION} QUIET REQUIRED)

#set(KF5_DEP_VERSION "${QT_MAJOR_VERSION}")
find_package(KF${QT_MAJOR_VERSION}
    REQUIRED COMPONENTS
        CoreAddons # Core addons on top of QtCore
        I18n # For localization
        TextEditor # The editor component
)

# This line defines the actual target
if (QT_MAJOR_VERSION EQUAL 5)
    kcoreaddons_add_plugin(kate_gpg_plugin
        INSTALL_NAMESPACE "ktexteditor")
else ()
    kcoreaddons_add_plugin(kate_gpg_plugin
        INSTALL_NAMESPACE "kf6/ktexteditor")
endif ()

target_sources(
  kate_gpg_plugin
  PRIVATE
  kate_gpg_plugin.hpp
  kate_gpg_plugin.cpp
  GPGKeyDetails.hpp
  GPGKeyDetails.cpp
  GPGMeWrapper.hpp
  GPGMeWrapper.cpp
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
