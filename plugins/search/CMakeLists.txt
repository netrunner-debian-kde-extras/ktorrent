set(ktsearchplugin_SRC 
	webview.cpp 
	localfilenetworkreply.cpp 
	buffernetworkreply.cpp 
	searchenginelist.cpp 
	searchprefpage.cpp 
	searchwidget.cpp 
	searchtoolbar.cpp 
	searchplugin.cpp 
	searchengine.cpp 
	opensearchdownloadjob.cpp
	searchactivity.cpp)

kde4_add_ui_files(ktsearchplugin_SRC searchpref.ui)
kde4_add_kcfg_files(ktsearchplugin_SRC searchpluginsettings.kcfgc)
kde4_add_plugin(ktsearchplugin ${ktsearchplugin_SRC})

target_link_libraries(
    ktsearchplugin
    ktcore
    ${LIBKTORRENT_LIBRARIES}
    ${QT_QTWEBKIT_LIBRARY}
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KDEWEBKIT_LIBS}
    )
install(TARGETS ktsearchplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES ktsearchplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install(FILES ktsearchpluginui.rc DESTINATION ${DATA_INSTALL_DIR}/ktorrent )

add_subdirectory(opensearch)
add_subdirectory(home)
