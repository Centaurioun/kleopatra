include_directories(${kleopatra_SOURCE_DIR}/src)


set(_kcm_kleopatra_libkleopatraclient_extra_SRCS
  smimevalidationconfigurationwidget.cpp
  smimevalidationconfigurationpage.cpp
  cryptooperationsconfigwidget.cpp
  cryptooperationsconfigpage.cpp
  smimevalidationconfigurationwidget.h
  smimevalidationconfigurationpage.h
  cryptooperationsconfigwidget.h
  cryptooperationsconfigpage.h
  smartcardconfigpage.cpp
  smartcardconfigpage.h
)

ki18n_wrap_ui(_kcm_kleopatra_libkleopatraclient_extra_SRCS
  smimevalidationconfigurationwidget.ui
)

kconfig_add_kcfg_files(_kcm_kleopatra_libkleopatraclient_extra_SRCS
  ${kleopatra_SOURCE_DIR}/src/kcfg/smimevalidationpreferences.kcfgc
)

set(_kcm_kleopatra_libkleopatraclient_extra_LIBS kleopatraclientgui)


set(kcm_kleopatra_PART_SRCS
  dirservconfigpage.cpp
  appearanceconfigpage.cpp
  appearanceconfigwidget.cpp
  gnupgsystemconfigurationpage.cpp
  dirservconfigpage.h
  appearanceconfigpage.h
  appearanceconfigwidget.h
  gnupgsystemconfigurationpage.h
  labelledwidget.cpp
  labelledwidget.cpp
  ${kleopatra_BINARY_DIR}/src/kleopatra_debug.cpp
  ${_kcm_kleopatra_libkleopatraclient_extra_SRCS}
)


ki18n_wrap_ui(kcm_kleopatra_PART_SRCS
  appearanceconfigwidget.ui
  smimevalidationconfigurationwidget.ui
)

kconfig_add_kcfg_files(kcm_kleopatra_PART_SRCS
  ${kleopatra_SOURCE_DIR}/src/kcfg/tooltippreferences.kcfgc
  ${kleopatra_SOURCE_DIR}/src/kcfg/emailoperationspreferences.kcfgc
  ${kleopatra_SOURCE_DIR}/src/kcfg/fileoperationspreferences.kcfgc
  ${kleopatra_SOURCE_DIR}/src/kcfg/tagspreferences.kcfgc
  ${kleopatra_SOURCE_DIR}/src/kcfg/settings.kcfgc
)

add_library(kcm_kleopatra_static STATIC ${kcm_kleopatra_PART_SRCS})
set_property(TARGET kcm_kleopatra_static PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(kcm_kleopatra_static
  KF5::Libkleo
  KF5::IconThemes
  KF5::I18n
  KF5::WidgetsAddons
  KF5::ConfigWidgets
  ${_kcm_kleopatra_extra_libs}
  ${_kleopatra_dbusaddons_libs}
  ${_kcm_kleopatra_libkleopatraclient_extra_LIBS}
)

if (NOT WIN32)
  add_library(kleopatra_config_gnupgsystem MODULE kcm_gnupgsystemconfigurationpage.cpp)
  target_link_libraries(kleopatra_config_gnupgsystem kcm_kleopatra_static KF5::CoreAddons)

  install(TARGETS kleopatra_config_gnupgsystem DESTINATION ${KDE_INSTALL_PLUGINDIR}/pim${QT_MAJOR_VERSION}/kcms/kleopatra)
endif()
