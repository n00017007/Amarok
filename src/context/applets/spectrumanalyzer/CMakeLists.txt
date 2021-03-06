project(context-spectrum-analyzer)

set(spectrum_analyzer_SRCS SpectrumAnalyzerApplet.cpp AnalyzerGlWidget.cpp SplineInterpolation.cpp FastFourierTransformation.cpp)

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ../..
    ../../..
)

kde4_add_ui_files( spectrum_analyzer_SRCS spectrumAnalyzerSettings.ui )

kde4_add_plugin(amarok_context_applet_spectrum_analyzer ${spectrum_analyzer_SRCS})
if(APPLE)
   SET_TARGET_PROPERTIES(amarok_context_applet_spectrum_analyzer PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

target_link_libraries(amarok_context_applet_spectrum_analyzer amarokcore amaroklib ${KDE4_PLASMA_LIBS} ${KDE4_KIO_LIBS} ${QT_QTOPENGL_LIBRARY} ${OPENGL_LIBRARY})

install(TARGETS amarok_context_applet_spectrum_analyzer DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES amarok-context-applet-spectrum-analyzer.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES amarok-spectrum-analyzer-applet.svg DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/ )
install(FILES SpectrumAnalyzerAppletCustomStyle.css DESTINATION ${DATA_INSTALL_DIR}/amarok/data )
