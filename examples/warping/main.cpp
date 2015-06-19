/******************************************************************************
 * guacamole - delicious VR                                                   *
 *                                                                            *
 * Copyright: (c) 2011-2013 Bauhaus-Universität Weimar                        *
 * Contact:   felix.lauer@uni-weimar.de / simon.schneegans@uni-weimar.de      *
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
#include "Navigator.hpp"
#include <gua/renderer/TexturedScreenSpaceQuadPass.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/renderer/ToneMappingPass.hpp>
#include <gua/renderer/DebugViewPass.hpp>
#include <gua/utils/Trackball.hpp>

#include <scm/input/tracking/art_dtrack.h>
#include <scm/input/tracking/target.h>

#include <gua/gui.hpp>

#define COUNT 6

bool depth_test             = true;
bool backface_culling       = true;
bool manipulation_navigator = true;
bool manipulation_camera    = false;
bool manipulation_object    = false;
bool latency_reduction      = true;

bool power_wall = true;

gua::math::mat4 current_tracking_matrix(gua::math::mat4::identity());

gua::math::mat4 current_warping_matrix_center(gua::math::mat4::identity());
gua::math::mat4 current_warping_matrix_right(gua::math::mat4::identity());
gua::math::mat4 current_warping_matrix_left(gua::math::mat4::identity());

// forward mouse interaction to trackball
void mouse_button (gua::utils::Trackball& trackball, int mousebutton, int action, int mods) {
  gua::utils::Trackball::button_type button;
  gua::utils::Trackball::state_type state;

  switch (mousebutton) {
    case 0: button = gua::utils::Trackball::left; break;
    case 2: button = gua::utils::Trackball::middle; break;
    case 1: button = gua::utils::Trackball::right; break;
  };

  switch (action) {
    case 0: state = gua::utils::Trackball::released; break;
    case 1: state = gua::utils::Trackball::pressed; break;
  };

  trackball.mouse(button, state, trackball.posx(), trackball.posy());
}

void show_backfaces(std::shared_ptr<gua::node::Node> const& node) {
  auto casted(std::dynamic_pointer_cast<gua::node::TriMeshNode>(node));
  if (casted) {
    casted->get_material()->set_show_back_faces(!backface_culling);
  }

  for (auto& child: node->get_children()) {
    show_backfaces(child);
  }
}

int main(int argc, char** argv) {
  bool fullscreen = (argc == 2);

  auto resolution = gua::math::vec2ui(1920, 1080);
  // auto resolution = gua::math::vec2ui(1600, 900);
  // auto resolution = gua::math::vec2ui(1280, 768);

  if (power_wall) {
    fullscreen = true;
    resolution = gua::math::vec2ui(1920, 1200);
  }

  // add mouse interaction
  gua::utils::Trackball object_trackball(0.01, 0.002, 0, 0.2);
  Navigator nav;
  Navigator warp_nav;
  nav.set_transform(scm::math::make_translation(0.f, 0.f, 3.f));

  // initialize guacamole
  gua::init(argc, argv);

  gua::SceneGraph graph("main_scenegraph");

  // ---------------------------------------------------------------------------
  // ---------------------------- setup scene ----------------------------------
  // ---------------------------------------------------------------------------

  gua::TriMeshLoader loader;
  auto transform = graph.add_node<gua::node::TransformNode>("/", "transform");
  transform->get_tags().add_tag("scene");

  auto light = graph.add_node<gua::node::LightNode>("/", "light");
  light->data.set_type(gua::node::LightNode::Type::SUN);
  light->data.set_brightness(4.f);
  light->data.set_shadow_cascaded_splits({0.1f, 0.6f, 1.5f, 6.f});
  light->data.set_shadow_near_clipping_in_sun_direction(10.0f);
  light->data.set_shadow_far_clipping_in_sun_direction(10.0f);
  light->data.set_max_shadow_dist(20.0f);
  light->data.set_shadow_offset(0.0002f);
  light->data.set_enable_shadows(true);
  light->data.set_shadow_map_size(2048);
  light->rotate(-95, 1, 0.5, 0);

  // many oilrigs scene --------------------------------------------------------
  auto scene_root = graph.add_node<gua::node::TransformNode>("/transform", "many_oilrigs");
  scene_root->rotate(-90, 1, 0, 0);
  auto add_oilrig = [&](int x, int y, int c, std::string const& parent) {
    auto t = graph.add_node<gua::node::TransformNode>(parent, "t");
    t->translate((x - c*0.5 + 0.5)/1.5, (y - c*0.5 + 0.8)/2, 0);
    auto teapot(loader.create_geometry_from_file("teapot", "/opt/3d_models/OIL_RIG_GUACAMOLE/oilrig.obj",
      gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
      gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
      gua::TriMeshLoader::NORMALIZE_SCALE));
    t->add_child(teapot);

  };

  for (int x(0); x<COUNT; ++x) {
    for (int y(0); y<COUNT; ++y) {
      add_oilrig(x, y, COUNT, "/transform/many_oilrigs");
    }
  }

  // one oilrig ----------------------------------------------------------------
  scene_root = graph.add_node<gua::node::TransformNode>("/transform", "one_oilrig");
  scene_root->scale(3);
  scene_root->rotate(-90, 1, 0, 0);
  add_oilrig(0, 0, 1, "/transform/one_oilrig");

  // textured quads scene ------------------------------------------------------
  scene_root = graph.add_node<gua::node::TransformNode>("/transform", "textured_quads");
  for (int x(0); x<10; ++x) {
    auto node = graph.add_node<gua::node::TexturedQuadNode>("/transform/textured_quads", "node" + std::to_string(x));
    node->translate(0, 0, -x);
    node->data.set_size(gua::math::vec2(1.92f*2, 1.08f*2));
  }

  // teapot --------------------------------------------------------------------
  scene_root = graph.add_node<gua::node::TransformNode>("/transform", "teapot");
  auto teapot(loader.create_geometry_from_file("teapot", "data/objects/teapot.obj",
    gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
    gua::TriMeshLoader::NORMALIZE_SCALE));
  scene_root->add_child(teapot);

  // spheres -------------------------------------------------------------------
  scene_root = graph.add_node<gua::node::TransformNode>("/transform", "sphere");
  auto sphere(loader.create_geometry_from_file("sphere", "data/objects/sphere.obj",
    gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
    gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
    gua::TriMeshLoader::NORMALIZE_SCALE));
  scene_root->add_child(sphere);

  // sponza --------------------------------------------------------------------
  scene_root = graph.add_node<gua::node::TransformNode>("/transform", "sponza");
  scene_root->scale(20);
  auto sponza(loader.create_geometry_from_file("sponza", "/opt/3d_models/Sponza/sponza.obj",
    gua::TriMeshLoader::OPTIMIZE_GEOMETRY | gua::TriMeshLoader::NORMALIZE_POSITION |
    gua::TriMeshLoader::LOAD_MATERIALS | gua::TriMeshLoader::OPTIMIZE_MATERIALS |
    gua::TriMeshLoader::NORMALIZE_SCALE));
  scene_root->add_child(sponza);


  show_backfaces(transform);

  auto set_scene = [&](std::string const& name) {
    graph["/transform/many_oilrigs"]->get_tags().add_tag("invisible");
    graph["/transform/sponza"]->get_tags().add_tag("invisible");
    graph["/transform/one_oilrig"]->get_tags().add_tag("invisible");
    graph["/transform/textured_quads"]->get_tags().add_tag("invisible");
    graph["/transform/teapot"]->get_tags().add_tag("invisible");
    graph["/transform/sphere"]->get_tags().add_tag("invisible");

    if (name == "set_scene_many_oilrigs")
      graph["/transform/many_oilrigs"]->get_tags().remove_tag("invisible");
    if (name == "set_scene_one_oilrig")
      graph["/transform/one_oilrig"]->get_tags().remove_tag("invisible");
    if (name == "set_scene_sponza")
      graph["/transform/sponza"]->get_tags().remove_tag("invisible");
    if (name == "set_scene_textured_quads")
      graph["/transform/textured_quads"]->get_tags().remove_tag("invisible");
    if (name == "set_scene_teapot")
      graph["/transform/teapot"]->get_tags().remove_tag("invisible");
    if (name == "set_scene_sphere")
      graph["/transform/sphere"]->get_tags().remove_tag("invisible");
  };

  set_scene("set_scene_one_oilrig");

  // ---------------------------------------------------------------------------
  // ------------------------ setup rendering pipelines ------------------------
  // ---------------------------------------------------------------------------
  auto navigation = graph.add_node<gua::node::TransformNode>("/", "navigation");

  // slow client ---------------------------------------------------------------
  auto slow_screen = graph.add_node<gua::node::ScreenNode>("/navigation", "slow_screen");
  auto slow_cam = graph.add_node<gua::node::CameraNode>("/navigation", "slow_cam");
  slow_screen->data.set_size(gua::math::vec2(3, 2));
  slow_screen->translate(0, 1.8, -1.7);
  slow_cam->translate(0, 1.8, 1.7);
  slow_cam->config.set_resolution(resolution);
  slow_cam->config.set_screen_path("/navigation/slow_screen");
  slow_cam->config.set_scene_graph_name("main_scenegraph");
  slow_cam->config.mask().blacklist.add_tag("invisible");
  slow_cam->config.set_near_clip(0.1f);

  // fast client ---------------------------------------------------------------
  auto warp_navigation = graph.add_node<gua::node::TransformNode>("/navigation", "warp");

  auto fast_screen = graph.add_node<gua::node::ScreenNode>("/navigation/warp", "fast_screen");
  auto fast_cam = graph.add_node<gua::node::CameraNode>("/navigation/warp", "fast_cam");
  fast_screen->data.set_size(gua::math::vec2(3, 2));
  fast_screen->translate(0, 1.8, -1.7);
  fast_cam->translate(0, 1.8, 1.7);
  fast_cam->config.set_resolution(resolution);
  fast_cam->config.set_screen_path("/navigation/warp/fast_screen");
  fast_cam->config.set_scene_graph_name("main_scenegraph");
  fast_cam->config.set_far_clip(slow_cam->config.get_far_clip()*1.5);
  fast_cam->config.set_near_clip(0.1f);

  auto tex_quad_pass(std::make_shared<gua::TexturedQuadPassDescription>());
  auto light_pass(std::make_shared<gua::LightVisibilityPassDescription>());
  auto warp_pass(std::make_shared<gua::WarpPassDescription>());
  auto grid_pass(std::make_shared<gua::GenerateWarpGridPassDescription>());
  auto render_grid_pass(std::make_shared<gua::RenderWarpGridPassDescription>());
  auto trimesh_pass(std::make_shared<gua::TriMeshPassDescription>());
  auto res_pass(std::make_shared<gua::ResolvePassDescription>());
  res_pass->background_mode(gua::ResolvePassDescription::BackgroundMode::SKYMAP_TEXTURE).
            background_texture("/opt/guacamole/resources/skymaps/cycles_island.jpg").
            background_color(gua::utils::Color3f(0, 0, 0)).
            ssao_enable(true).
            environment_lighting(gua::utils::Color3f(0.05, 0.1, 0.2)).
            horizon_fade(0.2f).
            ssao_intensity(2.f).
            ssao_radius(5.f);

  slow_cam->config.set_output_window_name("window");

  auto warp_pipe = std::make_shared<gua::PipelineDescription>();
  warp_pipe->add_pass(trimesh_pass);
  warp_pipe->add_pass(tex_quad_pass);
  warp_pipe->add_pass(light_pass);
  warp_pipe->add_pass(res_pass);
  warp_pipe->add_pass(grid_pass);
  warp_pipe->add_pass(warp_pass);
  warp_pipe->add_pass(render_grid_pass);
  warp_pipe->add_pass(std::make_shared<gua::TexturedScreenSpaceQuadPassDescription>());
  warp_pipe->set_enable_abuffer(true);
  slow_cam->set_pipeline_description(warp_pipe);

  auto normal_pipe = std::make_shared<gua::PipelineDescription>();
  normal_pipe->add_pass(trimesh_pass);
  normal_pipe->add_pass(tex_quad_pass);
  normal_pipe->add_pass(light_pass);
  normal_pipe->add_pass(res_pass);
  normal_pipe->add_pass(std::make_shared<gua::TexturedScreenSpaceQuadPassDescription>());
  normal_pipe->set_enable_abuffer(true);

  // ---------------------------------------------------------------------------
  // ----------------------------- setup gui -----------------------------------
  // ---------------------------------------------------------------------------
  auto window = std::make_shared<gua::GlfwWindow>();

  auto gui = std::make_shared<gua::GuiResource>();
  auto gui_quad = std::make_shared<gua::node::TexturedScreenSpaceQuadNode>("gui_quad");

  auto stats = std::make_shared<gua::GuiResource>();
  auto stats_quad = std::make_shared<gua::node::TexturedScreenSpaceQuadNode>("stats_quad");

  auto mouse = std::make_shared<gua::GuiResource>();
  auto mouse_quad = std::make_shared<gua::node::TexturedScreenSpaceQuadNode>("mouse_quad");
  mouse->init("mouse", "asset://gua/data/gui/mouse.html", gua::math::vec2ui(50, 50));
  mouse_quad->data.texture() = "mouse";
  mouse_quad->data.size() = gua::math::vec2ui(50, 50);
  mouse_quad->data.anchor() = gua::math::vec2(-1.f, -1.f);

  gua::Interface::instance()->on_cursor_change.connect([&](gua::Cursor pointer){
    mouse->call_javascript("set_active", pointer == gua::Cursor::HAND);
    return true;
  });

  // right side gui ----------------------------------------------------------
  gui->init("gui", "asset://gua/data/gui/gui.html", gua::math::vec2ui(330, 750));

  gui->on_loaded.connect([&]() {
    gui->add_javascript_getter("get_depth_layers", [&](){ return std::to_string(warp_pass->max_layers());});
    gui->add_javascript_getter("get_split_threshold", [&](){ return gua::string_utils::to_string(grid_pass->split_threshold());});
    gui->add_javascript_getter("get_cell_size", [&](){ return gua::string_utils::to_string(std::log2(grid_pass->cell_size()));});
    gui->add_javascript_getter("get_depth_test", [&](){ return std::to_string(depth_test);});
    gui->add_javascript_getter("get_backface_culling", [&](){ return std::to_string(backface_culling);});
    gui->add_javascript_getter("get_latency_reduction", [&](){ return std::to_string(latency_reduction);});
    gui->add_javascript_getter("get_background", [&](){ return std::to_string(res_pass->background_mode() == gua::ResolvePassDescription::BackgroundMode::SKYMAP_TEXTURE);});
    gui->add_javascript_getter("get_show_warp_grid", [&](){ return std::to_string(render_grid_pass->show_warp_grid());});
    gui->add_javascript_getter("get_debug_cell_colors", [&](){ return std::to_string(warp_pass->debug_cell_colors());});
    gui->add_javascript_getter("get_debug_cell_gap", [&](){ return std::to_string(warp_pass->debug_cell_gap());});
    gui->add_javascript_getter("get_adaptive_abuffer", [&](){ return std::to_string(trimesh_pass->adaptive_abuffer());});

    gui->add_javascript_callback("set_depth_layers");
    gui->add_javascript_callback("set_split_threshold");
    gui->add_javascript_callback("set_cell_size");
    gui->add_javascript_callback("set_depth_test");
    gui->add_javascript_callback("set_backface_culling");
    gui->add_javascript_callback("set_background");
    gui->add_javascript_callback("set_latency_reduction");
    gui->add_javascript_callback("set_gbuffer_type_none");
    gui->add_javascript_callback("set_gbuffer_type_points");
    gui->add_javascript_callback("set_gbuffer_type_scaled_points");
    gui->add_javascript_callback("set_gbuffer_type_quads_screen_aligned");
    gui->add_javascript_callback("set_gbuffer_type_quads_normal_aligned");
    gui->add_javascript_callback("set_gbuffer_type_quads_depth_aligned");
    gui->add_javascript_callback("set_gbuffer_type_grid_depth_theshold");
    gui->add_javascript_callback("set_gbuffer_type_grid_surface_estimation");
    gui->add_javascript_callback("set_gbuffer_type_grid_advanced_surface_estimation");
    gui->add_javascript_callback("set_gbuffer_type_grid_non_uniform_surface_estimation");
    gui->add_javascript_callback("set_abuffer_type_none");
    gui->add_javascript_callback("set_abuffer_type_points");
    gui->add_javascript_callback("set_abuffer_type_quads");
    gui->add_javascript_callback("set_abuffer_type_scaled_points");
    gui->add_javascript_callback("set_adaptive_abuffer");
    gui->add_javascript_callback("set_scene_one_oilrig");
    gui->add_javascript_callback("set_scene_many_oilrigs");
    gui->add_javascript_callback("set_scene_sponza");
    gui->add_javascript_callback("set_scene_teapot");
    gui->add_javascript_callback("set_scene_sphere");
    gui->add_javascript_callback("set_scene_textured_quads");
    gui->add_javascript_callback("set_manipulation_navigator");
    gui->add_javascript_callback("set_manipulation_camera");
    gui->add_javascript_callback("set_manipulation_object");
    gui->add_javascript_callback("set_show_warp_grid");
    gui->add_javascript_callback("set_debug_cell_colors");
    gui->add_javascript_callback("set_debug_cell_gap");
    gui->add_javascript_callback("set_bg_tex");
    gui->add_javascript_callback("set_view_mono_warped");
    gui->add_javascript_callback("set_view_stereo_warped");
    gui->add_javascript_callback("set_view_mono");
    gui->add_javascript_callback("set_view_stereo");

    gui->add_javascript_callback("reset_view");
    gui->add_javascript_callback("reset_object");

    gui->call_javascript("init");
  });

  gui->on_javascript_callback.connect([&](std::string const& callback, std::vector<std::string> const& params) {
    if (callback == "set_depth_layers") {
      std::stringstream str(params[0]);
      int depth_layers;
      str >> depth_layers;
      warp_pass->max_layers(depth_layers);
    } else if (callback == "set_view_mono_warped") {
      slow_cam->set_pipeline_description(warp_pipe);
      slow_cam->config.set_enable_stereo(false);
      slow_cam->config.set_eye_dist(0.07f);
      window->config.set_stereo_mode(gua::StereoMode::MONO);
    } else if (callback == "set_view_stereo_warped") {
      slow_cam->set_pipeline_description(warp_pipe);
      slow_cam->config.set_enable_stereo(true);
      slow_cam->config.set_eye_dist(0.f);
      trimesh_pass->set_enable_for_right_eye(false);
      tex_quad_pass->set_enable_for_right_eye(false);
      light_pass->set_enable_for_right_eye(false);
      res_pass->set_enable_for_right_eye(false);
      grid_pass->set_enable_for_right_eye(false);
      render_grid_pass->set_enable_for_right_eye(false);
      window->config.set_stereo_mode(power_wall ? gua::StereoMode::SIDE_BY_SIDE : gua::StereoMode::ANAGLYPH_RED_CYAN);
    } else if (callback == "set_view_mono") {
      slow_cam->set_pipeline_description(normal_pipe);
      slow_cam->config.set_enable_stereo(false);
      window->config.set_stereo_mode(gua::StereoMode::MONO);
    } else if (callback == "set_view_stereo") {
      slow_cam->set_pipeline_description(normal_pipe);
      slow_cam->config.set_enable_stereo(true);
      slow_cam->config.set_eye_dist(0.07f);
      trimesh_pass->set_enable_for_right_eye(true);
      tex_quad_pass->set_enable_for_right_eye(true);
      light_pass->set_enable_for_right_eye(true);
      res_pass->set_enable_for_right_eye(true);
      grid_pass->set_enable_for_right_eye(true);
      render_grid_pass->set_enable_for_right_eye(true);
      window->config.set_stereo_mode(power_wall ? gua::StereoMode::SIDE_BY_SIDE : gua::StereoMode::ANAGLYPH_RED_CYAN);
    } else if (callback == "set_split_threshold") {
      std::stringstream str(params[0]);
      float split_threshold;
      str >> split_threshold;
      grid_pass->split_threshold(split_threshold);
    } else if (callback == "set_bg_tex") {
      res_pass->background_texture(params[0]);
    } else if (callback == "set_cell_size") {
      std::stringstream str(params[0]);
      int cell_size;
      str >> cell_size;
      grid_pass->cell_size(std::pow(2, cell_size));
    } else if (callback == "set_depth_test") {
      std::stringstream str(params[0]);
      str >> depth_test;
      warp_pass->depth_test(depth_test);
    } else if (callback == "set_backface_culling") {
      std::stringstream str(params[0]);
      str >> backface_culling;
      show_backfaces(transform);
    } else if (callback == "set_latency_reduction") {
      std::stringstream str(params[0]);
      str >> latency_reduction;
    } else if (callback == "set_background") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      res_pass->background_mode(checked ? gua::ResolvePassDescription::BackgroundMode::SKYMAP_TEXTURE : gua::ResolvePassDescription::BackgroundMode::COLOR);
    } else if (callback == "set_show_warp_grid") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      render_grid_pass->show_warp_grid(checked);
    } else if (callback == "set_debug_cell_colors") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      warp_pass->debug_cell_colors(checked);
    } else if (callback == "set_debug_cell_gap") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      warp_pass->debug_cell_gap(checked);
    } else if (callback == "set_adaptive_abuffer") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      trimesh_pass->adaptive_abuffer(checked);
    } else if (callback == "reset_view") {
      warp_nav.reset();
    } else if (callback == "reset_object") {
      object_trackball.reset();
    } else if (callback == "set_gbuffer_type_points"
             | callback == "set_gbuffer_type_scaled_points"
             | callback == "set_gbuffer_type_quads_screen_aligned"
             | callback == "set_gbuffer_type_quads_normal_aligned"
             | callback == "set_gbuffer_type_quads_depth_aligned"
             | callback == "set_gbuffer_type_grid_depth_theshold"
             | callback == "set_gbuffer_type_grid_surface_estimation"
             | callback == "set_gbuffer_type_grid_advanced_surface_estimation"
             | callback == "set_gbuffer_type_grid_non_uniform_surface_estimation"
             | callback == "set_gbuffer_type_none") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      if (checked) {

        gua::WarpPassDescription::GBufferWarpMode mode(gua::WarpPassDescription::GBUFFER_NONE);

        if (callback == "set_gbuffer_type_points")
          mode = gua::WarpPassDescription::GBUFFER_POINTS;
        if (callback == "set_gbuffer_type_scaled_points")
          mode = gua::WarpPassDescription::GBUFFER_SCALED_POINTS;
        if (callback == "set_gbuffer_type_quads_screen_aligned")
          mode = gua::WarpPassDescription::GBUFFER_QUADS_SCREEN_ALIGNED;
        if (callback == "set_gbuffer_type_quads_normal_aligned")
          mode = gua::WarpPassDescription::GBUFFER_QUADS_NORMAL_ALIGNED;
        if (callback == "set_gbuffer_type_quads_depth_aligned")
          mode = gua::WarpPassDescription::GBUFFER_QUADS_DEPTH_ALIGNED;
        if (callback == "set_gbuffer_type_grid_depth_theshold")
          mode = gua::WarpPassDescription::GBUFFER_GRID_DEPTH_THRESHOLD;
        if (callback == "set_gbuffer_type_grid_surface_estimation")
          mode = gua::WarpPassDescription::GBUFFER_GRID_SURFACE_ESTIMATION;
        if (callback == "set_gbuffer_type_grid_advanced_surface_estimation")
          mode = gua::WarpPassDescription::GBUFFER_GRID_ADVANCED_SURFACE_ESTIMATION;
        if (callback == "set_gbuffer_type_grid_non_uniform_surface_estimation")
          mode = gua::WarpPassDescription::GBUFFER_GRID_NON_UNIFORM_SURFACE_ESTIMATION;
        if (callback == "set_gbuffer_type_none")
          mode = gua::WarpPassDescription::GBUFFER_NONE;

        warp_pass->gbuffer_warp_mode(mode);
        grid_pass->mode(mode);
        render_grid_pass->mode(mode);
      }
    } else if (callback == "set_abuffer_type_points"
             | callback == "set_abuffer_type_quads"
             | callback == "set_abuffer_type_scaled_points"
             | callback == "set_abuffer_type_none") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      if (checked) {
        if (callback == "set_abuffer_type_points")        warp_pass->abuffer_warp_mode(gua::WarpPassDescription::ABUFFER_POINTS);
        if (callback == "set_abuffer_type_quads")         warp_pass->abuffer_warp_mode(gua::WarpPassDescription::ABUFFER_QUADS);
        if (callback == "set_abuffer_type_scaled_points") warp_pass->abuffer_warp_mode(gua::WarpPassDescription::ABUFFER_SCALED_POINTS);
        if (callback == "set_abuffer_type_none")          warp_pass->abuffer_warp_mode(gua::WarpPassDescription::ABUFFER_NONE);
      }
    } else if (callback == "set_manipulation_object"
            || callback == "set_manipulation_camera"
            || callback == "set_manipulation_navigator") {
      std::stringstream str(params[0]);
      bool checked;
      str >> checked;
      if (checked) {
        manipulation_object = false;
        manipulation_camera = false;
        manipulation_navigator = false;

        if (callback == "set_manipulation_camera") manipulation_camera = true;
        if (callback == "set_manipulation_object") manipulation_object = true;
        if (callback == "set_manipulation_navigator") manipulation_navigator = true;
      }
    } else if (callback == "set_scene_one_oilrig" ||
               callback == "set_scene_many_oilrigs" ||
               callback == "set_scene_sponza" ||
               callback == "set_scene_teapot" ||
               callback == "set_scene_sphere" ||
               callback == "set_scene_textured_quads") {
      set_scene(callback);
    }
  });

  gui_quad->data.texture() = "gui";
  gui_quad->data.size() = gua::math::vec2ui(330, 750);
  gui_quad->data.anchor() = gua::math::vec2(1.f, 1.f);

  graph.add_node("/", gui_quad);

  // bottom gui --------------------------------------------------------------
  stats->init("stats", "asset://gua/data/gui/statistics.html", gua::math::vec2ui(1210, 30));

  stats->on_loaded.connect([&]() {
    stats->call_javascript("init");
  });

  stats_quad->data.texture() = "stats";
  stats_quad->data.size() = gua::math::vec2ui(1210, 30);
  stats_quad->data.anchor() = gua::math::vec2(0.f, -1.f);

  graph.add_node("/", stats_quad);
  graph.add_node("/", mouse_quad);


  // ---------------------------------------------------------------------------
  // ----------------------------- setup windows -------------------------------
  // ---------------------------------------------------------------------------

  window->config.set_fullscreen_mode(fullscreen);
  window->cursor_mode(gua::GlfwWindow::CursorMode::HIDDEN);
  window->on_resize.connect([&](gua::math::vec2ui const& new_size) {
    if (!power_wall) {
      resolution = new_size;
      window->config.set_resolution(new_size);
      slow_cam->config.set_resolution(new_size);
      slow_screen->data.set_size(gua::math::vec2(1.08*2 * new_size.x / new_size.y, 1.08*2));
      fast_screen->data.set_size(gua::math::vec2(1.08*2 * new_size.x / new_size.y, 1.08*2));
    }
  });
  window->on_button_press.connect([&](int key, int action, int mods) {
    gui->inject_mouse_button(gua::Button(key), action, mods);

    nav.set_mouse_button(key, action);
    warp_nav.set_mouse_button(key, action);

  });
  window->on_key_press.connect([&](int key, int scancode, int action, int mods) {
    if (manipulation_navigator) {
      nav.set_key_press(static_cast<gua::Key>(key), action);
    } else if (manipulation_camera) {
      warp_nav.set_key_press(static_cast<gua::Key>(key), action);
    }
    gui->inject_keyboard_event(gua::Key(key), scancode, action, mods);
    if (key == 72 && action == 1) {
      // hide gui
      if (gui_quad->get_tags().has_tag("invisible")) {
        gui_quad->get_tags().remove_tag("invisible");
        stats_quad->get_tags().remove_tag("invisible");
      } else {
        gui_quad->get_tags().add_tag("invisible");
        stats_quad->get_tags().add_tag("invisible");
      }
    }
  });
  window->on_move_cursor.connect([&](gua::math::vec2 const& pos) {
    mouse_quad->data.offset() = pos + gua::math::vec2i(-5, -45);
    gua::math::vec2 hit_pos;
    if (gui_quad->pixel_to_texcoords(pos, resolution, hit_pos)) {
      gui->inject_mouse_position_relative(hit_pos);
    } else {
      if (manipulation_object) {
        object_trackball.motion(pos.x, pos.y);
      } else if (manipulation_navigator) {
        nav.set_mouse_position(gua::math::vec2i(pos));
      } else {
        warp_nav.set_mouse_position(gua::math::vec2i(pos));
      }
    }
  });
  window->on_button_press.connect(std::bind(mouse_button, std::ref(object_trackball), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  if (power_wall) {
    window->config.set_size(gua::math::vec2ui(1920*2, 1200));
    window->config.set_right_position(gua::math::vec2ui(1920, 0));
    window->config.set_right_resolution(gua::math::vec2ui(1780, 1185));
    window->config.set_left_position(gua::math::vec2ui(140, 0));
    window->config.set_left_resolution(gua::math::vec2ui(1780, 1185));
  } else {
    window->config.set_size(resolution);
    window->config.set_resolution(resolution);
  }
  window->config.set_enable_vsync(false);
  gua::WindowDatabase::instance()->add("window", window);
  window->open();

  // tracking ------------------------------------------------------------------
  warp_pass->supply_warp_matrix([&](gua::CameraMode mode){
    if (latency_reduction) {
      fast_cam->set_transform(current_tracking_matrix);

      gua::Frustum frustum = fast_cam->get_rendering_frustum(graph, mode);
      gua::math::mat4 t(frustum.get_projection() * frustum.get_view());
      if (mode == gua::CameraMode::CENTER) {
        current_warping_matrix_center = t;
      } else if (mode == gua::CameraMode::LEFT) {
        current_warping_matrix_left = t;
      } else {
        current_warping_matrix_right = t;
      }
    } 

    if (mode == gua::CameraMode::CENTER) {
      return gua::math::mat4f(current_warping_matrix_center);
    } else if (mode == gua::CameraMode::LEFT) {
      return gua::math::mat4f(current_warping_matrix_left);
    } else {
      return gua::math::mat4f(current_warping_matrix_right);
    }
  });

  std::thread tracking_thread([&]() {
    if (power_wall) {

      scm::inp::tracker::target_container targets;
      targets.insert(scm::inp::tracker::target_container::value_type(5, scm::inp::target(5)));
      
      scm::inp::art_dtrack* dtrack(new scm::inp::art_dtrack(5000));
      if (!dtrack->initialize()) {
        std::cerr << std::endl << "Tracking System Fault" << std::endl;
        return;
      }
      while (true) {
        dtrack->update(targets);
        auto t = targets.find(5)->second.transform();
        t[12] /= 1000.f; t[13] /= 1000.f; t[14] /= 1000.f;
        current_tracking_matrix = t;
      }
    }
  });

  window->on_start_frame.connect([&](){
    if (!latency_reduction) {
      fast_cam->set_transform(current_tracking_matrix);
      gua::Frustum frustum = fast_cam->get_rendering_frustum(graph, gua::CameraMode::CENTER);
      current_warping_matrix_center = frustum.get_projection() * frustum.get_view();
      frustum = fast_cam->get_rendering_frustum(graph, gua::CameraMode::LEFT);
      current_warping_matrix_left = frustum.get_projection() * frustum.get_view();
      frustum = fast_cam->get_rendering_frustum(graph, gua::CameraMode::RIGHT);
      current_warping_matrix_right = frustum.get_projection() * frustum.get_view();
    }
  });

  // render setup --------------------------------------------------------------
  gua::Renderer renderer;

  // application loop
  gua::events::MainLoop loop;
  gua::events::Ticker ticker(loop, 1.0/500.0);

  int ctr=0;

  ticker.on_tick.connect([&]() {

    gua::Interface::instance()->update();

    slow_cam->set_transform(current_tracking_matrix);

    nav.update();
    warp_nav.update();

    navigation->set_transform(gua::math::mat4(nav.get_transform()));
    warp_navigation->set_transform(gua::math::mat4(warp_nav.get_transform()));

    gua::Frustum frustum = fast_cam->get_rendering_frustum(graph, gua::CameraMode::CENTER);

    gua::math::mat4 modelmatrix = scm::math::make_translation(gua::math::float_t(object_trackball.shiftx()),
                                                              gua::math::float_t(object_trackball.shifty()),
                                                              gua::math::float_t(object_trackball.distance())) * gua::math::mat4(object_trackball.rotation());

    transform->set_transform(modelmatrix);


    if (ctr++ % 100 == 0) {
      double trimesh_time(0);
      double gbuffer_warp_time(0);
      double abuffer_warp_time(0);
      double grid_time(0);
      int gbuffer_primitives(0);
      int abuffer_primitives(0);

      for (auto const& result: window->get_context()->time_query_results) {
        if (result.first.find("GPU") != std::string::npos) {
          if (result.first.find("Trimesh") != std::string::npos) trimesh_time += result.second;
          if (result.first.find("Resolve") != std::string::npos) trimesh_time += result.second;
          if (result.first.find("WarpPass GBuffer") != std::string::npos) gbuffer_warp_time += result.second;
          if (result.first.find("WarpPass ABuffer") != std::string::npos) abuffer_warp_time += result.second;
          if (result.first.find("WarpGridGenerator") != std::string::npos) grid_time += result.second;
        }
      }

      for (auto const& result: window->get_context()->primitive_query_results) {
        if (result.first.find("WarpPass GBuffer") != std::string::npos) gbuffer_primitives += result.second.first;
        if (result.first.find("WarpPass ABuffer") != std::string::npos) abuffer_primitives += result.second.first;
      }

      stats->call_javascript("set_stats", 1000.f / window->get_rendering_fps(),
                           window->get_rendering_fps(), trimesh_time, grid_time,
                           gbuffer_warp_time, abuffer_warp_time, gbuffer_primitives,
                           abuffer_primitives);
    }

    window->process_events();
    if (window->should_close()) {
      renderer.stop();
      window->close();
      loop.stop();
    } else {
      renderer.queue_draw({&graph});
    }
  });

  loop.start();

  return 0;
}
