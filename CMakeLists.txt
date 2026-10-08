cmake_minimum_required(VERSION 3.16)

project(plasma-tuner VERSION 0.2.0)

set(QT_MIN_VERSION "6.7.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(KDEClangFormat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Core
    Multimedia
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    Config
)

find_package(Plasma REQUIRED)

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
    add_subdirectory(autotests)
endif()

# "cmake --build build --target clang-format" formats the code in the KDE style.
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h autotests/*.cpp autotests/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
