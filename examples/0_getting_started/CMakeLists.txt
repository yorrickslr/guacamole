# guacamole examples
IF (GUACAMOLE_EXAMPLES)

  # input requires GLFW3
  IF (${GUACAMOLE_GLFW3})
    add_subdirectory(0_window)
  ENDIF (${GUACAMOLE_GLFW3})

ENDIF (GUACAMOLE_EXAMPLES)
