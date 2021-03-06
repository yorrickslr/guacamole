/******************************************************************************
 * guacamole - delicious VR                                                   *
 *                                                                            *
 * Copyright: (c) 2011-2018 Bauhaus-Universität Weimar                        *
 * Contact:   yorrick.paolo.sieler-morzuch@uni.weimar.de                      *
 *            joachim.billert@uni-weimar.de                                   *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify it    *
 * under the terms of the GNU General Public License as published by the Free *
 * Software Foundation, either version 3 of the License, or (at your option)  *
 * any later version.                                                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program. If not, see <http://www.gnu.org/licenses/>.             *
 *                                                                            *
 ******************************************************************************/

#include <functional>

#include <gua/guacamole.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/utils/Trackball.hpp>

/* scenegraph overview for "main_scenegraph"
  /
  └ screen
    └ camera
  └ transform
    └ teapot_geometry
*/

/*  to use the mouse input for trackball navigation, 
    we need a callback to forward the interaction to the trackball */
void mouse_button(gua::utils::Trackball& trackball, int mousebutton, int action, int mods) {
  gua::utils::Trackball::button_type button;
  gua::utils::Trackball::state_type state;

  switch (mousebutton) {
    // left mb for rotation
    case 0: button = gua::utils::Trackball::left; break;
    // middle mb for translation (shift on x/y plane) on screen pane
    case 2: button = gua::utils::Trackball::middle; break;
    // right mb for zoom or shift on z axis
    case 1: button = gua::utils::Trackball::right; break;
  };

  switch (action) {
    case 0: state = gua::utils::Trackball::released; break;
    case 1: state = gua::utils::Trackball::pressed; break;
  };
  // forward button and state along with the mouse position to the trackball class
  trackball.mouse(button, state, trackball.posx(), trackball.posy());
}

int main(int argc, char** argv) {
  // initialize guacamole
  gua::init(argc, argv);

  // initialize scenegraph
  gua::SceneGraph graph("main_scenegraph");

  // initialize trimeshloader for model loading
  gua::TriMeshLoader loader;

  // create a transform node
  // which will be attached to the scenegraph
  auto transform = graph.add_node<gua::node::TransformNode>("/", "transform");
  // the model will be attached to the transform node
  auto teapot_geometry(loader.create_geometry_from_file(
                          "teapot_geometry", "../data/objects/teapot.obj", 
                          gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
                          gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
                          gua::TriMeshLoader::NORMALIZE_SCALE )
                      );
  // teapot_geometry->scale(0.15);
  teapot_geometry->translate(-0.6,0.0,0.0);
  graph.add_node("/transform", teapot_geometry);


  auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
  screen->data.set_size(gua::math::vec2(1.28f, 0.72f));  // real world size of screen
  screen->translate(0, 0, 1.0);

  // initialize trackball for mouse interaction
  // trackball(zoom_factor, shift_factor, rotation_factor)
  gua::utils::Trackball trackball(0.01, 0.002, 0.2);

  // initialize navigator node for key input and keyboard navigation


  // resolution to be used for camera resolution and windows size
  auto resolution = gua::math::vec2ui(1280, 720);

  // set up camera and connect to screen in scenegraph
  auto camera = graph.add_node<gua::node::CameraNode>("/screen", "cam");
  camera->translate(0, 0, 2.0);
  camera->config.set_resolution(resolution);
  camera->config.set_screen_path("/screen");
  camera->config.set_scene_graph_name("main_scenegraph");
  camera->config.set_output_window_name("main_window");

  // set up window
  auto window = std::make_shared<gua::GlfwWindow>();
  gua::WindowDatabase::instance()->add("main_window", window);
  window->config.set_enable_vsync(false);
  window->config.set_size(resolution);
  window->config.set_resolution(resolution);
  window->config.set_stereo_mode(gua::StereoMode::MONO);
  window->on_resize.connect([&](gua::math::vec2ui const& new_size) {
    window->config.set_resolution(new_size);
    camera->config.set_resolution(new_size);
    screen->data.set_size(
        gua::math::vec2(0.001 * new_size.x, 0.001 * new_size.y));
  });
  // connect mouse movement in window to trackball motion
  window->on_move_cursor.connect(
      [&](gua::math::vec2 const& pos) { trackball.motion(pos.x, pos.y); });
  // connect mouse input to our mouse button callback.
  // the placeholders will be replaced by the mousebutton, 
  // action and mods arguments of the callback funktion
  window->on_button_press.connect(
      std::bind(mouse_button, std::ref(trackball), std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  // add a simple keyboard movement;
  // as we need the camera, lambda function is good way to go
  window->on_key_press.connect([&](int key, int scancode, int action, int mods) {
    if(action == 0) return;

    switch(key) {
      case 'W':
        camera->translate(0.0,0.0,0.5);
        break;
      case 'S':
        camera->translate(0.0,0.0,-0.5);
        break;
      case 'A':
        camera->translate(0.5,0.0,0.0);
        break;
      case 'D':
        camera->translate(-0.5,0.0,0.0);
        break;
    }
  }); 
  window->open();

  gua::Renderer renderer;

  // application loop
  gua::events::MainLoop loop;
  gua::events::Ticker ticker(loop, 1.0 / 500.0);

  // log fps and handle close events
  size_t ctr{};
  ticker.on_tick.connect([&]() {
    // log fps every 150th tick
    if (ctr++ % 150 == 0) {
      gua::Logger::LOG_WARNING
        << "Frame time: " << 1000.f / window->get_rendering_fps()
        << " ms, fps: " << window->get_rendering_fps() << std::endl;
    }
    // apply trackball matrix to the object
    gua::math::mat4 modelmatrix =
        scm::math::make_translation(trackball.shiftx(), trackball.shifty(),
                                    trackball.distance()) *
        gua::math::mat4(trackball.rotation());

    transform->set_transform(modelmatrix);
	
    window->process_events();
    if (window->should_close()) {
      // stop rendering and close the window
      renderer.stop();
      window->close();
      loop.stop();
    } else {
      // draw our scenegrapgh
      renderer.queue_draw({&graph});
    }
  });

  loop.start();

  return 0;
}
