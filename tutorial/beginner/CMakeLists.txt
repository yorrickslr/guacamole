# guacamole tutorial

# input requires GLFW3
IF (${GUACAMOLE_GLFW3})
  add_subdirectory(0_window)
  add_subdirectory(1_load_trimesh)
  add_subdirectory(2_transformations)
  add_subdirectory(3_lights)
ENDIF (${GUACAMOLE_GLFW3})
