# Add an option to compile & link against system GeoIP (off by default)
option(WITH_SYSTEM_GEOIP 
	"Link InfoWidget plugin against system GeoIP library and use system-wide default GeoIP Country database"
	OFF)

if (WITH_SYSTEM_GEOIP)
	find_path(GEOIP_INCLUDE_DIR NAMES GeoIP.h PATHS ${KDE4_INCLUDE_DIR}/ ${INCLUDE_INSTALL_DIR}/)
	find_library(GEOIP_LIBRARY NAMES GeoIP PATHS ${KDE4_LIB_DIR} ${LIB_INSTALL_DIR})
	
	if (NOT GEOIP_INCLUDE_DIR OR NOT GEOIP_LIBRARY)
		set(WITH_SYSTEM_GEOIP OFF CACHE BOOL
		  "GeoIP development files could not be found on this system. Forcing this option to OFF"
		  FORCE)
		message(SEND_ERROR
		  "GeoIP library development files could not be found on your system. Embedding a local copy of GeoIP instead")
	endif(NOT GEOIP_INCLUDE_DIR OR NOT GEOIP_LIBRARY)
endif(WITH_SYSTEM_GEOIP)

if (WITH_SYSTEM_GEOIP)
	message(STATUS "   Linking InfoWidget against system GeoIP library")
	add_definitions( -DUSE_SYSTEM_GEOIP )
	include_directories(GEOIP_INCLUDE_DIR)
	set(geoip_link ${GEOIP_LIBRARY})
else(WITH_SYSTEM_GEOIP)
	message(STATUS "   Compiling GeoIP support directly into InfoWidget plugin")
	set(geoip_src GeoIP.c)
endif(WITH_SYSTEM_GEOIP)    

set(ktinfowidgetplugin_SRC 
	infowidgetplugin.cpp 
	iwprefpage.cpp 
	monitor.cpp 
	availabilitychunkbar.cpp 
	downloadedchunkbar.cpp 
	statustab.cpp
	fileview.cpp
	peerview.cpp
	peerviewmodel.cpp
	chunkdownloadview.cpp
	chunkdownloadmodel.cpp
	trackerview.cpp
	trackermodel.cpp
	addtrackersdialog.cpp
	${geoip_src}
	geoipmanager.cpp
	flagdb.cpp
	iwfiletreemodel.cpp
	iwfilelistmodel.cpp
	webseedstab.cpp
	webseedsmodel.cpp)

kde4_add_ui_files(ktinfowidgetplugin_SRC iwprefpage.ui statustab.ui chunkdownloadview.ui trackerview.ui webseedstab.ui)
kde4_add_kcfg_files(ktinfowidgetplugin_SRC infowidgetpluginsettings.kcfgc)
kde4_add_plugin(ktinfowidgetplugin ${ktinfowidgetplugin_SRC})

target_link_libraries(
    ktinfowidgetplugin
    ktcore
    ${LIBKTORRENT_LIBRARIES}
    ${KDE4_KPARTS_LIBS}
    ${geoip_link}
    )
install(TARGETS ktinfowidgetplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES ktinfowidgetplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

