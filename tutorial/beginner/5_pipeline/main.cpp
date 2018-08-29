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
#include <gua/renderer/DebugViewPass.hpp>

/* scenegraph overview for "main_scenegraph"
  /
  └ screen
    └ camera
  └ transform
    └ sphere_geometry
    └ monkey_geometry
    └ teapot_geometry
  └ light_transform
    └ sun_light
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
  auto sphere_geometry(loader.create_geometry_from_file(
                          "sphere_geometry", "../data/objects/sphere.obj", 
                          gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
                          gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
                          gua::TriMeshLoader::NORMALIZE_SCALE )
                      );
  sphere_geometry->scale(0.4);
  sphere_geometry->translate(0.6,0.0,0.0);
  auto monkey_geometry(loader.create_geometry_from_file(
                          "monkey_geometry", "../data/objects/monkey.obj", 
                          gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
                          gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
                          gua::TriMeshLoader::NORMALIZE_SCALE )
                      );
  monkey_geometry->scale(0.5);
  auto teapot_geometry(loader.create_geometry_from_file(
                          "teapot_geometry", "../data/objects/teapot.obj", 
                          gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
                          gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
                          gua::TriMeshLoader::NORMALIZE_SCALE )
                      );
  teapot_geometry->scale(0.5);
  teapot_geometry->translate(-0.6,0.0,0.0);
  auto plane_geometry(loader.create_geometry_from_file(
                          "plane_geometry", "../data/objects/plane.obj", 
                          gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
                          gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
                          gua::TriMeshLoader::NORMALIZE_SCALE)
                      );
  plane_geometry->rotate(10.0f, gua::math::vec3(1.0f,0.0f,0.0f));
  plane_geometry->translate(0.0, -0.2, 0.0);
  // plane_geometry->scale(gua::math::vec3(2.0f,0.0f,0.0f));
  graph.add_node("/transform", sphere_geometry);
  graph.add_node("/transform", monkey_geometry);
  graph.add_node("/transform", teapot_geometry);
  graph.add_node("/transform", plane_geometry);

  /* LIGHTS */
  // create some transform nodes to append the different lights to
  // main light transform node
  auto light_transform = graph.add_node<gua::node::TransformNode>("/","light_transform");

  // add a sun light
  auto sun_light = graph.add_node<gua::node::LightNode>("/light_transform", "sun_light");
  sun_light->data.set_type(gua::node::LightNode::Type::SUN);
  sun_light->data.set_color(gua::utils::Color3f(1.5f, 1.2f, 1.f));
  sun_light->data.set_shadow_cascaded_splits({0.1f, 1.5, 5.f, 10.f});
  sun_light->data.set_shadow_near_clipping_in_sun_direction(100.0f);
  sun_light->data.set_shadow_far_clipping_in_sun_direction(100.0f);
  sun_light->data.set_max_shadow_dist(30.0f);
  sun_light->data.set_shadow_offset(0.0004f);
  sun_light->data.set_enable_shadows(true);
  sun_light->data.set_shadow_map_size(512);
  sun_light->rotate(-65, 1, 0, 0);
  sun_light->rotate(-100, 0, 1, 0);
  
  // sun_light_transform->rotate(90.0f, gua::math::vec3(-1.0f,1.0f,0.0f));
  

  auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
  screen->data.set_size(gua::math::vec2(1.28f, 0.72f));  // real world size of screen
  screen->translate(0, 0, 1.0);

  // resolution to be used for camera resolution and windows size
  auto resolution = gua::math::vec2ui(1280, 720);

  // initialize trackball for mouse interaction
  // trackball(zoom_factor, shift_factor, rotation_factor)
  gua::utils::Trackball trackball(0.01, 0.002, 0.2);

  // set up camera and connect to screen in scenegraph
  auto camera = graph.add_node<gua::node::CameraNode>("/screen", "cam");
  camera->translate(0, 0, 2.0);
  camera->config.set_resolution(resolution);
  camera->config.set_screen_path("/screen");
  camera->config.set_scene_graph_name("main_scenegraph");
  camera->config.set_output_window_name("main_window");

  // Pipeline Description
  // The pipeline description is stored in the camera and later used for
  // pipeline creation When creating a camera, a default pipeline description is
  // set up It includes several passes affecting the pipeline layout and
  // functionality defalut pipeline description includes: TrimeMeshPass,
  // LineStripPass, TexturedQuadPassDescription, LightVisibilityPassDescription, 
  // BBoxPassDescription, ResolvePassDescription, TexturedScreenSpaceQuadPassDescription
  // You can create an own pipeline description from scratch or configure the existing description.

  auto pipe_desc = camera->get_pipeline_description();
  // Let's set a skymap as background for the scene
  // Those settings have to be configured in the resolve pass
  auto res_pass = pipe_desc->get_resolve_pass();
  res_pass->background_mode(gua::ResolvePassDescription::BackgroundMode::SKYMAP_TEXTURE);
  res_pass->background_texture("../data/textures/sphericalskymap.jpg");
  res_pass->background_color(gua::utils::Color3f(0,0,0));
  res_pass->environment_lighting_texture("../data/textures/sphericalskymap.jpg");
  res_pass->environment_lighting(gua::utils::Color3f(0.4, 0.4, 0.5));
  res_pass->environment_lighting_mode(gua::ResolvePassDescription::EnvironmentLightingMode::AMBIENT_COLOR);
  res_pass->ssao_enable(true);
  res_pass->tone_mapping_method(gua::ResolvePassDescription::ToneMappingMethod::HEJL);
  res_pass->tone_mapping_exposure(1.5f);
  res_pass->horizon_fade(0.2f);
  res_pass->ssao_intensity(1.5f);
  res_pass->ssao_radius(2.f);
  res_pass->vignette_coverage(0.2f);
  res_pass->vignette_softness(1.f);
  res_pass->vignette_color(gua::math::vec4f(0.5, 0.1, 0.3, 1.0));


  // adding a pipeline pass /!\ don't forget to include the pass /!\ 
  // for example add a debug view pass. 
  // Debug View will add overviews of normals, depth, color and position of the scene.
  pipe_desc->add_pass(std::make_shared<gua::DebugViewPassDescription>());

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
  window->open();
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
  // add keyboard input
  window->on_key_press.connect([&](int key, int scancode, int action, int mods) { 
  });

  gua::Renderer renderer;

  // application loop
  gua::events::MainLoop loop;
  gua::events::Ticker ticker(loop, 1.0 / 500.0);

  // log fps and handle close events
  size_t ctr{};
  ticker.on_tick.connect([&]() {
    // use a timer for an ongoing transformation
	  auto time = gua::Timer::get_now()*0.0000000001;
	  auto trans = (std::cos(time*17000000000.0f)+1.0f)/2.0f;
    
    // sun_light_transform->rotate(time, gua::math::vec3(-1.0f,1.0f,0.0f));
    /* sun_light->data.set_color(gua::utils::Color3f(1.0f,trans,0.07f)); */
    // log fps every 150th tick
    if (ctr++ % 150 == 0) {
      // change light color every 150th tick 
      // float r = float(std::rand()) / float(RAND_MAX);
      // float g = float(std::rand()) / float(RAND_MAX);
      // float b = float(std::rand()) / float(RAND_MAX);
      // point_light->data.color = gua::utils::Color3f(r,g,b);
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
