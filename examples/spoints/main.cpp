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
#include <gua/spoints.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/renderer/TexturedQuadPass.hpp>
#include <gua/renderer/ToneMappingPass.hpp>
#include <gua/utils/Trackball.hpp>
#include <gua/renderer/DebugViewPass.hpp>

void set_window_default(std::shared_ptr<gua::WindowBase> const& window, gua::math::vec2ui const& res)
{
    window->config.set_size(res);
    window->config.set_resolution(res);
    window->config.set_enable_vsync(true);
    window->config.set_stereo_mode(gua::StereoMode::ANAGLYPH_RED_CYAN);
}

// forward mouse interaction to trackball
void mouse_button(gua::utils::Trackball& trackball, int mousebutton, int action, int mods)
{
    gua::utils::Trackball::button_type button;
    gua::utils::Trackball::state_type state;

    switch(mousebutton)
    {
    case 0:
        button = gua::utils::Trackball::left;
        break;
    case 2:
        button = gua::utils::Trackball::middle;
        break;
    case 1:
        button = gua::utils::Trackball::right;
        break;
    };

    switch(action)
    {
    case 0:
        state = gua::utils::Trackball::released;
        break;
    case 1:
        state = gua::utils::Trackball::pressed;
        break;
    };

    trackball.mouse(button, state, trackball.posx(), trackball.posy());
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cout << "ERROR: please provide a *.sr file containing at least a 'serverport' attribute!" << std::endl;
    }
    std::string spoints_resource_file_string(argv[1]);

    char* argv_tmp[] = {"./example-spoints", NULL};
    int argc_tmp = sizeof(argv_tmp) / sizeof(char*) - 1;
    ;
    // initialize guacamole
    gua::init(argc_tmp, argv_tmp);

    // setup scene
    gua::SceneGraph graph("main_scenegraph");

    gua::SPointsLoader vloader;
    auto transform = graph.add_node<gua::node::TransformNode>("/", "transform");
    auto spoints_geode(vloader.create_geometry_from_file(std::string("spoints_geode"), spoints_resource_file_string.c_str(), nullptr, 8u) );
    graph.add_node("/transform", spoints_geode);


    spoints_geode->rotate(180.0, 0.0, 1.0, 0.0);
    spoints_geode->translate(0.0, -1.0, -20.0);

    transform->translate(0.0f, 0.0f, -10.0f);


    auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
    screen->data.set_size(gua::math::vec2(1.218f, 1.218f));
    screen->translate(0, 0, 1.0);

    auto screen2 = graph.add_node<gua::node::ScreenNode>("/", "screen2");
    screen2->data.set_size(gua::math::vec2(1.218f, 1.218f));
    screen2->translate(5.75, 1.5, 1.0);

    auto screen3 = graph.add_node<gua::node::ScreenNode>("/", "screen3");
    screen3->data.set_size(gua::math::vec2(1.218f, 1.218f));
    screen3->translate(0, 0, 1.0);

    auto screen4 = graph.add_node<gua::node::ScreenNode>("/", "screen4");
    screen4->data.set_size(gua::math::vec2(1.218f, 1.218f));
    screen4->translate(0, 0, 1.0);

    // add mouse interaction
    gua::utils::Trackball trackball(0.01, 0.002, 0.2);

    // setup rendering pipeline and window
    auto resolution = gua::math::vec2ui(800, 800);
    //auto resolution = gua::math::vec2ui(3840/2, 2160/2);
    //auto resolution = gua::math::vec2ui(4096, 2160);

    auto pipe = std::make_shared<gua::PipelineDescription>();
    pipe->add_pass(std::make_shared<gua::SPointsPassDescription>());
    pipe->add_pass(std::make_shared<gua::TriMeshPassDescription>());

    pipe->add_pass(std::make_shared<gua::TexturedQuadPassDescription>());
    pipe->add_pass(std::make_shared<gua::LightVisibilityPassDescription>());
    pipe->add_pass(std::make_shared<gua::ResolvePassDescription>());
    // pipe->add_pass(std::make_shared<gua::DebugViewPassDescription>());


    auto camera = graph.add_node<gua::node::CameraNode>("/screen", "cam");
    camera->translate(0, 0, 2.0);
    camera->config.set_resolution(resolution);
    camera->config.set_screen_path("/screen");
    camera->config.set_scene_graph_name("main_scenegraph");
    camera->config.set_output_window_name("window1");
    camera->config.set_enable_stereo(false);
    //camera->config.set_enable_stereo(true);
    camera->set_pipeline_description(pipe);





    //auto add_window = [&](std::string const& window_name, std::shared_ptr<gua::node::CameraNode> const& cam_node) {
    auto window = std::make_shared<gua::GlfwWindow>();
    gua::WindowDatabase::instance()->add("window1", window);
    camera->config.set_output_window_name("window1");

    window->config.set_size(resolution);
    window->config.set_resolution(resolution);
    window->config.set_stereo_mode(gua::StereoMode::MONO);
    window->config.set_enable_vsync(false);

    window->on_move_cursor.connect([&](gua::math::vec2 const& pos) { trackball.motion(pos.x, pos.y); });
    window->on_button_press.connect(std::bind(mouse_button, std::ref(trackball), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


    window->open();
 

  
    gua::Renderer renderer;

    // application loop
    gua::events::MainLoop loop;
    gua::events::Ticker ticker(loop, 1.0 / 10000.0);

    unsigned framecount = 0;

    ticker.on_tick.connect([&]() {
        // apply trackball matrix to object
        gua::math::mat4 modelmatrix = scm::math::make_translation(gua::math::float_t(trackball.shiftx()), gua::math::float_t(trackball.shifty()), gua::math::float_t(trackball.distance())) *
                                      gua::math::mat4(trackball.rotation());

        transform->set_transform(modelmatrix);

        // window->process_events();
        if(false /* window->should_close()*/)
        {
            renderer.stop();
            // window->close();
            loop.stop();
        }
        else
        {
            renderer.queue_draw({&graph});

            // if(50 == framecount) {
            //  add_window("window2", camera2);
            //}
            /*
            if(100 == framecount) {
              add_window("window3", camera3);
            }
            if(150 == framecount) {
              add_window("window4", camera4);
            }*/

            //std::cout << window_handle->get_rendering_fps() << "\n";
            ++framecount;
        }
    });

    loop.start();

    return 0;
}
