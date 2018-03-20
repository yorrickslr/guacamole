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

/* scenegraph overview for "main_scenegraph"
  /
  └ screen
    └ camera
  └ transform
    └ sphere_geometry
    └ monkey_geometry
    └ teapot_geometry
  └ point_light
  └ spot_light
  └ spot_light2
*/

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
                          "sphere_geometry",
                          "../data/objects/sphere.obj" )
                      );
  sphere_geometry->scale(0.2);
  sphere_geometry->translate(0.6,0.0,0.0);
  auto monkey_geometry(loader.create_geometry_from_file(
                          "sphere_geometry",
                          "../data/objects/monkey.obj" )
                      );
  monkey_geometry->scale(0.2);
  auto teapot_geometry(loader.create_geometry_from_file(
                          "sphere_geometry",
                          "../data/objects/teapot.obj" )
                      );
  teapot_geometry->scale(0.15);
  teapot_geometry->translate(-0.6,0.0,0.0);
  graph.add_node("/transform", sphere_geometry);
  graph.add_node("/transform", monkey_geometry);
  graph.add_node("/transform", teapot_geometry);

  /* LIGHTS */

  // add a light node and attach to to scenegraph
  auto point_light = graph.add_node<gua::node::LightNode>("/", "point_light");
  // set the type of the light (Point, Spot or Sun); data ist the configuration
  point_light->data.set_type(gua::node::LightNode::Type::POINT);
  // set light color
  point_light->data.color = gua::utils::Color3f(1.0f,0.0f,0.0f);
  // set light brightness
  point_light->data.brightness = 110.0f;
  // enable shadows
  point_light->data.set_enable_shadows(true);
  // set the position
  point_light->scale(15.f);
  point_light->translate(-3.0,2.0,-2.0);

  // add a spot light
  auto spot_light = graph.add_node<gua::node::LightNode>("/", "spot_light");
  spot_light->data.set_type(gua::node::LightNode::Type::SPOT);
  // set light color
  spot_light->data.set_color(gua::utils::Color3f(0.3f,0.2f,1.0f));
  // set light brightness
  spot_light->data.set_brightness(100.0f);
  // set light falloff
  spot_light->data.set_falloff(2.0f);
  // set softness
  spot_light->data.set_softness(2.0f);
  // enable shadows
  spot_light->data.set_enable_shadows(true);
  // set the position
  spot_light->scale(1.0f);
  spot_light->translate(0.0,0.0,1.0);

  // add a spot light
  auto spot_light2 = graph.add_node<gua::node::LightNode>("/", "spot_light2");
  spot_light2->data.set_type(gua::node::LightNode::Type::SPOT);
  // set light color
  spot_light2->data.set_color(gua::utils::Color3f(1.0f,0.8f,0.2f));
  // set light brightness
  spot_light2->data.set_brightness(20.0f);
  // set light falloff
  spot_light2->data.set_falloff(0.5f);
  // set softness
  spot_light2->data.set_softness(0.5f);
  // enable shadows
  spot_light2->data.set_enable_shadows(true);
  // set the position
  spot_light2->scale(1.5f);
  spot_light2->rotate(30.0, gua::math::vec3(1.0,0.0,0.0));
  spot_light2->translate(0.6,-1.0,1.0);

  auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
  screen->data.set_size(gua::math::vec2(1.28f, 0.72f));  // real world size of screen
  screen->translate(0, 0, 1.0);

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
  window->open();

  gua::Renderer renderer;

  // application loop
  gua::events::MainLoop loop;
  gua::events::Ticker ticker(loop, 1.0 / 500.0);

  // log fps and handle close events
  size_t ctr{};
  ticker.on_tick.connect([&]() {
    // use a timer for an ongoing transformation
	  auto time = gua::Timer::get_now();
	  auto trans = std::cos(time)*0.01;

    /* Optional Light Transformations */
    //// rotate point light around origin while moving up and down
    // point_light->rotate(time*0.0000000002, gua::math::vec3(0.0,1.0,0.0));
    // point_light->translate(0.0,trans, 0.0)

    //// rotate spotlight around own y-axis
    // spot_light->translate(0.0,0.0,-1.0);    
    // spot_light->rotate(time*0.0000000002, gua::math::vec3(0.0,1.0,0.0));
    // spot_light->translate(0.0,0.0,1.0); 

    ////move spot light from side to side
    // spot_light->translate(trans*0.2,0.0,0.0);

    // rotate spot light 2 around own x axis
    // spot_light2->translate(-0.6,1.0,-1.0);
    // spot_light2->rotate(time*0.0000000002, gua::math::vec3(1.0,0.0,0.0));
    // spot_light2->translate(0.6,-1.0,1.0);
    

    // log fps every 150th tick
    if (ctr++ % 150 == 0) {
      // change light color every 150th tick 
      float r = float(std::rand()) / float(RAND_MAX);
      float g = float(std::rand()) / float(RAND_MAX);
      float b = float(std::rand()) / float(RAND_MAX);
      point_light->data.color = gua::utils::Color3f(r,g,b);
      gua::Logger::LOG_WARNING
        << "Frame time: " << 1000.f / window->get_rendering_fps()
        << " ms, fps: " << window->get_rendering_fps() << std::endl;
    }
	
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
