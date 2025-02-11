# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "03")
set(RELEASE_SERVICE_VERSION_MICRO "70")

# The RELEASE_SERVICE_VERSION is used by Gpg4win to add the Gpg4win version
if (NOT RELEASE_SERVICE_VERSION)
    set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
endif()
if(RELEASE_SERVICE_VERSION_MICRO LESS 10)
    set(KDE_APPLICATIONS_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}0${RELEASE_SERVICE_VERSION_MICRO}")
else()
    set(KDE_APPLICATIONS_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")
endif()

set(KLEOPATRA_VERSION_MAJOR "3")
set(KLEOPATRA_VERSION_MINOR "1")
set(KLEOPATRA_VERSION_MICRO "26")

set(kleopatra_version "${KLEOPATRA_VERSION_MAJOR}.${KLEOPATRA_VERSION_MINOR}.${KLEOPATRA_VERSION_MICRO}.${KDE_APPLICATIONS_COMPACT_VERSION}")
# The following is for Windows
set(kleopatra_version_win "${KLEOPATRA_VERSION_MAJOR}.${KLEOPATRA_VERSION_MINOR}.${KLEOPATRA_VERSION_MICRO}")
set(kleopatra_fileversion_win "${KLEOPATRA_VERSION_MAJOR},${KLEOPATRA_VERSION_MINOR},${KLEOPATRA_VERSION_MICRO},0")
if (NOT KLEOPATRA_DISTRIBUTION_TEXT)
    # This is only used on Windows for the file attributes of Kleopatra
    set(KLEOPATRA_DISTRIBUTION_TEXT "KDE")
endif()

project(kleopatra VERSION ${kleopatra_version})

option(DISABLE_KWATCHGNUPG "Don't build the kwatchgnupg tool [default=OFF]" OFF)

# Standalone build. Find / include everything necessary.
set(KF5_MIN_VERSION "5.101.0")
set(KIDENTITYMANAGEMENT_VERSION "5.22.40")
set(KMAILTRANSPORT_VERSION "5.22.40")
set(KMIME_VERSION "5.22.40")
set(LIBKLEO_VERSION "5.22.44")
set(QT_REQUIRED_VERSION "5.15.2")
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
endif()
set(GPGME_REQUIRED_VERSION "1.16.0")
set(LIBASSUAN_REQUIRED_VERSION "2.4.2")
set(GPG_ERROR_REQUIRED_VERSION "1.36")

if (WIN32)
  set(KF5_WANT_VERSION "5.70.0")
  set(KMIME_WANT_VERSION "5.12.0")
else ()
  set(KF5_WANT_VERSION ${KF5_MIN_VERSION})
  set(KMIME_WANT_VERSION ${KMIME_VERSION})
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_WANT_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)
include(GenerateExportHeader)
include(ECMGenerateHeaders)
include(FeatureSummary)
include(CheckFunctionExists)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(KDEClangFormat)

# Find KF5 packages
find_package(KF5WidgetsAddons ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5CoreAddons ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5I18n ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5ItemModels ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5XmlGui ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5DocTools ${KF5_WANT_VERSION} CONFIG)
find_package(KF5Crash ${KF5_WANT_VERSION} REQUIRED)

set_package_properties(KF5DocTools PROPERTIES
    DESCRIPTION "Documentation tools"
    PURPOSE "Required to generate Kleopatra documentation."
    TYPE OPTIONAL)

# Optional packages
if (WIN32)
  # Only a replacement available for Windows so this
  # is required on other platforms.
  find_package(KF5DBusAddons ${KF5_WANT_VERSION} CONFIG)
  set_package_properties(KF5DBusAddons PROPERTIES DESCRIPTION "Support library to work with DBus"
                         PURPOSE "DBus session integration"
                         URL "https://inqlude.org/libraries/kdbusaddons.html"
                         TYPE OPTIONAL)
else()
  find_package(KF5DBusAddons ${KF5_WANT_VERSION} CONFIG REQUIRED)
  set(_kleopatra_dbusaddons_libs KF5::DBusAddons)
endif()

set(HAVE_QDBUS ${Qt${QT_MAJOR_VERSION}DBus_FOUND})

find_package(Gpgmepp ${GPGME_REQUIRED_VERSION} CONFIG REQUIRED)
if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(QGpgmeQt6 ${GPGME_REQUIRED_VERSION} CONFIG REQUIRED)
else()
    find_package(QGpgme ${GPGME_REQUIRED_VERSION} CONFIG REQUIRED)
endif()
if (QGpgme_VERSION VERSION_GREATER_EQUAL "1.17.0")
    set(QGPGME_SUPPORTS_CHANGING_EXPIRATION_OF_COMPLETE_KEY 1)
    set(QGPGME_CRYPTOCONFIGENTRY_HAS_DEFAULT_VALUE 1)
    set(QGPGME_SUPPORTS_WKDLOOKUP 1)
    set(QGPGME_SUPPORTS_IMPORT_WITH_FILTER 1)
    set(QGPGME_SUPPORTS_IMPORT_WITH_KEY_ORIGIN 1)
    set(QGPGME_SUPPORTS_SECRET_KEY_EXPORT 1)
    set(QGPGME_SUPPORTS_SECRET_SUBKEY_EXPORT 1)
    set(QGPGME_SUPPORTS_RECEIVING_KEYS_BY_KEY_ID 1)
endif()
if (QGpgme_VERSION VERSION_GREATER_EQUAL "1.18.0")
    set(QGPGME_SUPPORTS_KEY_REVOCATION 1)
    set(QGPGME_SUPPORTS_KEY_REFRESH 1)
    set(QGPGME_SUPPORTS_SET_FILENAME 1)
    set(QGPGME_SUPPORTS_SET_PRIMARY_UID 1)
endif()
if (Gpgmepp_VERSION VERSION_GREATER_EQUAL "1.18.1")
    set(GPGMEPP_SUPPORTS_SET_CURVE 1)
endif()

# Kdepimlibs packages
find_package(KF5Libkleo ${LIBKLEO_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_WANT_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${KIDENTITYMANAGEMENT_VERSION} CONFIG)
find_package(KF5MailTransport ${KMAILTRANSPORT_VERSION} CONFIG)
find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_VERSION} CONFIG)

find_package(Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Test Network PrintSupport)

find_package(LibAssuan ${LIBASSUAN_REQUIRED_VERSION} REQUIRED)
set_package_properties(LibAssuan PROPERTIES
  TYPE REQUIRED
  PURPOSE "Needed for Kleopatra to act as the GnuPG UI Server"
)
find_package(LibGpgError ${GPG_ERROR_REQUIRED_VERSION} REQUIRED)
set_package_properties(LibGpgError PROPERTIES
  TYPE REQUIRED
)

set(kleopatra_release FALSE)

if(NOT kleopatra_release)
    find_package(Git)
    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE rc
                        ERROR_QUIET)
        if(rc EQUAL 0)
            execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --oneline --format=%h ${CMAKE_CURRENT_SOURCE_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE Kleopatra_WC_REVISION)
            string(REGEX REPLACE "\n" "" Kleopatra_WC_REVISION "${Kleopatra_WC_REVISION}")

            execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --oneline --format=%cI ${CMAKE_CURRENT_SOURCE_DIR}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE Kleopatra_WC_LAST_CHANGED_DATE)
            string(REGEX REPLACE "^([0-9]+)-([0-9]+)-([0-9]+)T([0-9]+):([0-9]+):([0-9]+).*$" "\\1\\2\\3T\\4\\5\\6"
                   Kleopatra_WC_LAST_CHANGED_DATE "${Kleopatra_WC_LAST_CHANGED_DATE}")

            set(kleopatra_version "${kleopatra_version}+git${Kleopatra_WC_LAST_CHANGED_DATE}~${Kleopatra_WC_REVISION}")
        endif()
    endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version-kleopatra.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/version-kleopatra.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-kleopatra.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kleopatra.h)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)
# On Windows, we need to use stuff deprecated since Qt 5.11, e.g. from QDesktopWidget
ecm_set_disabled_deprecation_versions(QT 5.10.0  KF 5.102.0)
else ()
ecm_set_disabled_deprecation_versions(QT 5.15.0  KF 5.102.0)
endif ()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces -Wno-parentheses -Wno-ignored-qualifiers")
endif()
if(MINGW)
    # we do not care about different signedness of passed pointer arguments
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-pointer-sign>)
endif()

add_definitions(-DQT_NO_EMIT)
remove_definitions(-DQT_NO_FOREACH)

# Disable the use of QStringBuilder for operator+ to prevent crashes when
# returning the result of concatenating string temporaries in lambdas. We do
# this for example in some std::transform expressions.
# This is a known issue: https://bugreports.qt.io/browse/QTBUG-47066
# Alternatively, one would always have to remember to force the lambdas to
# return a QString instead of QStringBuilder, but that's just too easy to
# forget and, unfortunately, the compiler doesn't issue a warning if one forgets
# this. So, it's just too dangerous.
# One can still use QStringBuilder explicitly with the operator% if necessary.
remove_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
remove_definitions(-DQT_USE_QSTRINGBUILDER)

kde_enable_exceptions()
option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if (USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
endif()


add_subdirectory(pics)
add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
    add_subdirectory(autotests)
endif()

ecm_qt_install_logging_categories(
        EXPORT KLEOPATRA
        FILE kleopatra.categories
        DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
        )

ki18n_install(po)
if(KF5DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
