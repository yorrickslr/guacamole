# guacamole tutorial

# input requires GLFW3
IF (${GUACAMOLE_GLFW3})
  add_subdirectory(0_window)
  add_subdirectory(1_load_model)
ENDIF (${GUACAMOLE_GLFW3})
