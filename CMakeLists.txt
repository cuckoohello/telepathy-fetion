project(telepathy-fetion)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)
set(TARNAME telepathy-fetion)

# program name, version etc

set(PACKAGE_VERSION "1.0.0")
set(PACKAGE_NAME "telepathy-fetion")
set(PACKAGE_TARNAME "${TARNAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://code.google.com/p/ofetion/issues/list")

find_package(PkgConfig REQUIRED)

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif(NOT DEFINED BIN_INSTALL_DIR)

if(NOT DEFINED DATA_INSTALL_DIR)
    set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/${PACKAGE_TARNAME}")
endif(NOT DEFINED DATA_INSTALL_DIR)

if(NOT DEFINED CERTS_INSTALL_DIR)
    set(CERTS_INSTALL_DIR "${DATA_INSTALL_DIR}/ca-certs")
endif(NOT DEFINED CERTS_INSTALL_DIR)

IF(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(IS_ARM  TRUE)
ENDIF(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")

add_definitions(-DPACKAGE_VERSION="${PACKAGE_VERSION}")
add_definitions(-DDATA_DIR="${DATA_INSTALL_DIR}")
add_definitions(-DPIXMAPS_DIR="${DATA_INSTALL_DIR}/pixmaps/")
add_definitions(-DCERTS_DIR="${DATA_INSTALL_DIR}/ca-certs/")
add_definitions(-DCCERT_DIR="${DATA_INSTALL_DIR}/c-cert")

if(DEFINED IS_ARM)
    add_definitions(-DCONFIG_ARM)
endif(DEFINED IS_ARM)

# im build requirements
if(NOT DEFINED IS_ARM)
    pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
    pkg_check_modules(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)
endif(NOT DEFINED IS_ARM)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)
pkg_check_modules(GMODULE2 REQUIRED gmodule-2.0)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
pkg_check_modules(TELEPATHY REQUIRED telepathy-glib)

##########################################################################

add_definitions(-g -Wall)

##########################################################################

include_directories(
    ${GTK2_INCLUDE_DIRS}
    ${GDKPIXBUF_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    ${GOBJECT2_INCLUDE_DIRS}
    ${GMODULE2_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
    ${TELEPATHY_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/plugin
    )

# source list of fetion module
set(telepathy_fetion_SRC
    conn-aliasing.c
    conn-avatars.c
    connection.c
    connection-manager.c
    conn-presence.c
    im-channel.c
    im-factory.c
    main.c
    protocol.c
    roster.c
    lib/account.c
    lib/config.c
    lib/connect.c
    lib/eventloop.c
    lib/logs.c
    lib/module.c
    lib/network.c
    lib/util.c
    lib/xmlnode.c
    lib/blist.c
    lib/conv.c
    plugin/fetion.c 
    plugin/fx_buddy.c
    plugin/fx_group.c  
    plugin/fx_msg.c 
    plugin/fx_trans.c
    plugin/fx_account.c
    plugin/fx_config.c
    plugin/fx_login.c
    plugin/fx_sip.c
    plugin/fx_util.c
    )

if(NOT DEFINED IS_ARM)
    set(telepathy_fetion_confirm_SRC
        lib/confirm.c
        )
endif(NOT DEFINED IS_ARM)

if(DEFINED IS_ARM)
    set(telepathy_fetion_confirm_SRC
        lib/confirm_arm.c
        )
endif(DEFINED IS_ARM)


add_executable(telepathy-fetion ${telepathy_fetion_SRC} ${telepathy_fetion_confirm_SRC})

target_link_libraries(telepathy-fetion
    ${GOBJECT2_LIBRARIES}
    ${GMODULE2_LIBRARIES}
    ${GTK2_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GDKPIXBUF_LIBRARIES}
    ${GTHREAD2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${TELEPATHY_LIBRARIES})

# install program module
install(TARGETS telepathy-fetion RUNTIME DESTINATION "/lib/telepathy-glib/")

file(GLOB FETION_SERVICE "share/dbus-1/*")
install(DIRECTORY ${FETION_SERVICE} DESTINATION "/usr/share/dbus-1/")

file(GLOB FETION_MANAGER "share/telepathy/*")
install(DIRECTORY ${FETION_MANAGER} DESTINATION "/usr/share/telepathy/")

if(DEFINED IS_ARM)
    file(GLOB FETION_THEME_ICONS "share/themes/*")
    install(DIRECTORY ${FETION_THEME_ICONS} DESTINATION "/usr/share/themes/")
    file(GLOB FETION_ACCOUNTS_FILES "share/accounts/*")
    install(DIRECTORY ${FETION_ACCOUNTS_FILES} DESTINATION "/usr/share/accounts/")
    file(GLOB FETION_BIN_FILES "share/bin/*")
    install(PROGRAMS ${FETION_BIN_FILES} DESTINATION "${DATA_INSTALL_DIR}/bin/")
endif(DEFINED IS_ARM)

# install icons pixmaps
file(GLOB HYBRID_ICONS_PIXMAPS "share/pixmaps/icons/*.png")
install(FILES ${HYBRID_ICONS_PIXMAPS} DESTINATION "${DATA_INSTALL_DIR}/pixmaps/icons/")

#install ca certs files
file(GLOB HYBRID_CA_CERTS "share/ca-certs/*")
install(FILES ${HYBRID_CA_CERTS} DESTINATION "${DATA_INSTALL_DIR}/ca-certs/")

#include client certs files
file(GLOB HYBRID_CLIENT_CERT "share/c-cert/*")
install(FILES ${HYBRID_CLIENT_CERT} DESTINATION "${DATA_INSTALL_DIR}/c-cert/")

# install fetion resource xml files
file(GLOB FETION_RESOURCE_XML "share/protocols/fetion/*.xml")
install(FILES ${FETION_RESOURCE_XML} DESTINATION "${DATA_INSTALL_DIR}/protocols/fetion/")
add_definitions(-DFETION_RES_DIR="${DATA_INSTALL_DIR}/protocols/fetion/")
