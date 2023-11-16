set (SOURCES
  gstimpactacquiresrc.c)
    
set (HEADERS
  gstimpactacquiresrc.h
  impactacquirepixelformat.h)

include_directories (AFTER
  ${GSTREAMER_INCLUDE_DIR}/..
  ${IMPACT_ACQUIRE_INCLUDE_DIR}
  )

set (libname gstimpactacquire)

add_library (${libname} MODULE
  ${SOURCES}
  ${HEADERS}
  )

target_sources(${libname} PRIVATE
  ${IMPACT_ACQUIRE_INCLUDE_DIR}/apps/Common/exampleHelper_C.c
  )

target_link_libraries (${libname}
  ${GLIB2_LIBRARIES}
  ${GOBJECT_LIBRARIES}
  ${GSTREAMER_LIBRARY}
  ${GSTREAMER_BASE_LIBRARY}
  ${GSTREAMER_VIDEO_LIBRARY}
  ${IMPACT_ACQUIRE_LIBRARIES}
  )

if(MSVC)
  target_compile_options(${libname} PRIVATE /W4)
else()
  target_compile_options(${libname} PRIVATE -Wall)
endif()

if (WIN32)
  install (FILES $<TARGET_PDB_FILE:${libname}> DESTINATION ${PDB_INSTALL_DIR} COMPONENT pdb OPTIONAL)
endif ()
install(TARGETS ${libname} LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR})
