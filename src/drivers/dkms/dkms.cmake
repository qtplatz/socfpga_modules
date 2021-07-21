
configure_file (
  "${DKMS}/postinst.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dkms/postinst"
  )

configure_file (
  "${DKMS}/prerm.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dkms/prerm"
  )

configure_file (
  "${DKMS}/Makefile.in"
  "${CMAKE_CURRENT_BINARY_DIR}/dkms/Makefile"
  @ONLY
  )

configure_file (
  "${DKMS}/dkms.conf"
  "${CMAKE_CURRENT_BINARY_DIR}/dkms/dkms.conf"
  @ONLY
  )

install( FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dkms/Makefile
  ${CMAKE_CURRENT_BINARY_DIR}/dkms/dkms.conf
  ${SRCS}
  DESTINATION /usr/src/${MODNAME}-${PACKAGE_VERSION} COMPONENT "${DKMS_PACKAGE_PREFIX}${MODNAME}" )

#------ /etc/init.d/initrc-script ----------
configure_file (
  "${DKMS}/init.d-generic.sh"
  "${CMAKE_CURRENT_BINARY_DIR}/etc/init.d/${MODNAME}"
  @ONLY
  )

install( FILES
  "${CMAKE_CURRENT_BINARY_DIR}/etc/init.d/${MODNAME}" DESTINATION "/etc/init.d" COMPONENT "${DKMS_PACKAGE_PREFIX}${MODNAME}"
  )

# --- Validation ---
unset( __dkms CACHE )
find_path( __dkms NAMES postinst prerm Makefile PATHS ${CMAKE_CURRENT_BINARY_DIR}/dkms REQUIRED )

set ( DKMS_PACKAGES ${DKMS_PACKAGES} ${MODNAME} PARENT_SCOPE )
