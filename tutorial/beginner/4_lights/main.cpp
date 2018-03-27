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
  └ light_transform
    case:1
    └ point_light_transform
      └ point_light0
      └ point_light1
      └ point_light2
    case:2
    └ spot_light_transform
      └ spot_light0
      └ spot_light1
      └ spot_light2  
    case:3
    └ sun_light_transform
      └ sun_light
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
  auto plane_geometry(loader.create_geometry_from_file(
                          "plane_geometry",
                          "../data/objects/plane.obj")
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
  // one transform node for every type
  auto point_light_transform = std::make_shared<gua::node::TransformNode>();
  auto spot_light_transform = std::make_shared<gua::node::TransformNode>();
  auto sun_light_transform = std::make_shared<gua::node::TransformNode>();

  // create some point lights and configure them
  auto point_light0 = point_light_transform->add_child<gua::node::LightNode>("point_light0");
  point_light0->data.set_type(gua::node::LightNode::Type::POINT);
  point_light0->data.set_brightness(40.0f);
  point_light0->data.set_falloff(2.0f);
  point_light0->data.set_softness(1.5f);
  point_light0->data.set_enable_shadows(true);
  // float r = float(std::rand()) / float(RAND_MAX);
  // float g = float(std::rand()) / float(RAND_MAX);
  // float b = float(std::rand()) / float(RAND_MAX);
  // point_light0->data.set_color(gua::utils::Color3f(0.5f+r,g,b));
  point_light0->data.set_color(gua::utils::Color3f(1.0f,0.0f,0.0f));
  point_light0->scale(10.0f);
  point_light0->translate(2.0f,2.0f,2.0f);


  auto point_light1 = point_light_transform->add_child<gua::node::LightNode>("point_light1");
  point_light1->data.set_type(gua::node::LightNode::Type::POINT);
  point_light1->data.set_brightness(25.0f);
  point_light1->data.set_falloff(0.5f);
  point_light1->data.set_softness(2.0f);
  point_light1->data.set_enable_shadows(true);
  // r = float(std::rand()) / float(RAND_MAX);
  // g = float(std::rand()) / float(RAND_MAX);
  // b = float(std::rand()) / float(RAND_MAX);
  // point_light1->data.set_color(gua::utils::Color3f(r,0.5f+g,b));
  point_light1->data.set_color(gua::utils::Color3f(0.0f,1.0f,0.0f));
  point_light1->scale(4.0f);
  point_light1->translate(-1.5f,-2.0f,1.0f);

  auto point_light2 = point_light_transform->add_child<gua::node::LightNode>("point_light2");
  point_light2->data.set_type(gua::node::LightNode::Type::POINT);
  point_light2->data.set_brightness(110.0f);
  point_light2->data.set_falloff(1.0f);
  point_light2->data.set_softness(1.0f);
  point_light2->data.set_enable_shadows(true);
  // r = float(std::rand()) / float(RAND_MAX);
  // g = float(std::rand()) / float(RAND_MAX);
  // b = float(std::rand()) / float(RAND_MAX);
  // point_light2->data.set_color(gua::utils::Color3f(r,g,0.5f+b));
  point_light2->data.set_color(gua::utils::Color3f(0.0f,0.0f,1.0f));
  point_light2->scale(10.0f);
  point_light2->translate(-0.5f,5.0f,0.0f);

  // add the point lights to the scenegraph, you can later switch the light via keyboard input
  auto temp = graph.add_node<gua::node::TransformNode>(light_transform, point_light_transform);

  // create some spot lights and configure them
  auto spot_light0 = spot_light_transform->add_child<gua::node::LightNode>("spot_light0");
  spot_light0->data.set_type(gua::node::LightNode::Type::SPOT);
  spot_light0->data.set_brightness(50.0f);
  spot_light0->data.set_falloff(2.0f);
  spot_light0->data.set_softness(1.5f);
  spot_light0->data.set_enable_shadows(true);
  // r = float(std::rand()) / float(RAND_MAX);
  // g = float(std::rand()) / float(RAND_MAX);
  // b = float(std::rand()) / float(RAND_MAX);
  // spot_light0->data.set_color(gua::utils::Color3f(0.5f+r,g,b));
  spot_light0->data.set_color(gua::utils::Color3f(1.0f,0.0f,0.0f));
  spot_light0->scale(1.5f);
  spot_light0->rotate(45.0f, gua::math::vec3(0.0f,1.0f,0.0f));
  spot_light0->translate(1.0f,0.0f,1.0f);


  auto spot_light1 = spot_light_transform->add_child<gua::node::LightNode>("spot_light1");
  spot_light1->data.set_type(gua::node::LightNode::Type::SPOT);
  spot_light1->data.set_brightness(35.0f);
  spot_light1->data.set_falloff(0.5f);
  spot_light1->data.set_softness(2.0f);
  spot_light1->data.set_enable_shadows(true);
  // r = float(std::rand()) / float(RAND_MAX);
  // g = float(std::rand()) / float(RAND_MAX);
  // b = float(std::rand()) / float(RAND_MAX);
  // spot_light1->data.set_color(gua::utils::Color3f(r,0.5f+g,b));
  spot_light1->data.set_color(gua::utils::Color3f(0.0f,0.6f,0.2f));
  spot_light1->scale(2.0f);
  spot_light1->rotate(63.43f, gua::math::vec3(1.0f,0.0f,0.0f));  
  spot_light1->translate(-0.6f,-1.f,0.5f);

  auto spot_light2 = spot_light_transform->add_child<gua::node::LightNode>("spot_light2");
  spot_light2->data.set_type(gua::node::LightNode::Type::SPOT);
  spot_light2->data.set_brightness(120.0f);
  spot_light2->data.set_falloff(0.8f);
  spot_light2->data.set_softness(1.0f);
  spot_light2->data.set_enable_shadows(true);
  // r = float(std::rand()) / float(RAND_MAX);
  // g = float(std::rand()) / float(RAND_MAX);
  // b = float(std::rand()) / float(RAND_MAX);
  // spot_light2->data.set_color(gua::utils::Color3f(r,g,0.5f+b));
  spot_light2->data.set_color(gua::utils::Color3f(0.0f,0.0f,1.0f));
  spot_light2->scale(5.0f);
  spot_light2->rotate(-90.0f, gua::math::vec3(1.0f,0.0f,0.0f));
  spot_light2->translate(0.0f,5.0f,0.0f);

  // add a sun light
  auto sun_light = sun_light_transform->add_child<gua::node::LightNode>("sun_light");
  sun_light->data.set_type(gua::node::LightNode::Type::SUN);
  sun_light->data.set_brightness(3.0f);
  // sun_light->data.set_falloff(2.0f);
  // sun_light->data.set_softness(1.5f);
  sun_light->data.set_enable_shadows(true);
  sun_light->data.set_shadow_map_size(1024);
  // r = float(std::rand()) / float(RAND_MAX);
  // g = float(std::rand()) / float(RAND_MAX);
  // b = float(std::rand()) / float(RAND_MAX);
  // sun_light->data.set_color(gua::utils::Color3f(0.5f+r,g,b));
  sun_light->data.set_color(gua::utils::Color3f(1.0f,0.7f,0.07f));
  sun_light->scale(1.0f);
  sun_light->translate(70.0f,70.0f,1.0f);

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
  // add keyboard input
  window->on_key_press.connect([&](int key, int scancode, int action, int mods) {
    if(action == 0) return;

    switch(key) {
      case '1':
        std::cout << "SWITCHED TO POINT LIGHTS" << std::endl;
        light_transform->clear_children();
        graph.add_node<gua::node::TransformNode>(light_transform, point_light_transform);
        break;
      case '2':
        std::cout << "SWITCHED TO SPOT LIGHTS" << std::endl;
        light_transform->clear_children();
        graph.add_node<gua::node::TransformNode>(light_transform, spot_light_transform);
        break;
      case '3':
        std::cout << "SWITCHED TO SUN LIGHT" << std::endl;
        light_transform->clear_children();
        graph.add_node<gua::node::TransformNode>(light_transform, sun_light_transform);
        break;
    }
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
    
    sun_light_transform->rotate(time, gua::math::vec3(-1.0f,1.0f,0.0f));
    sun_light->data.set_color(gua::utils::Color3f(1.0f,trans,0.07f));
    // log fps every 150th tick
    if (ctr++ % 150 == 0) {
      // change light color every 150th tick 
      // float r = float(std::rand()) / float(RAND_MAX);
      // float g = float(std::rand()) / float(RAND_MAX);
      // float b = float(std::rand()) / float(RAND_MAX);
      // point_light->data.color = gua::utils::Color3f(r,g,b);
      gua::Logger::LOG_WARNING
        << "Frame time: " << 1000.f / window->get_rendering_fps()
        << " ms, fps: " << window->get_rendering_fps() 
        << "\nCOS(TIME): " << trans << std::endl;
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
