# Generated from qtnetworkauth.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtNetworkAuth
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Network Auth Libraries" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Network) # special case
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Widgets) # special case

if(NOT TARGET Qt::Network)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Network\" is not met.")
    return()
endif()

if(NOT QT_FEATURE_http)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_http\" is not met.")
    return()
endif()
qt_build_repo()
