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

#ifndef GUA_LOG_TO_LIN_SUB_RENDERER_HPP
#define GUA_LOG_TO_LIN_SUB_RENDERER_HPP

#include <gua/renderer/Pipeline.hpp>
#include <gua/renderer/PLodSubRenderer.hpp>

namespace gua
{
class PLodSubRenderer;

class GUA_LOD_DLL LogToLinSubRenderer : public PLodSubRenderer
{
  public:
    LogToLinSubRenderer();

    virtual void create_gpu_resources(gua::RenderContext const& ctx, scm::math::vec2ui const& render_target_dims, gua::plod_shared_resources& shared_resources) override;

    virtual void render_sub_pass(Pipeline& pipe,
                                 PipelinePassDescription const& desc,
                                 gua::plod_shared_resources& shared_resources,
                                 std::vector<node::Node*>& sorted_models,
                                 std::unordered_map<node::PLodNode*, std::unordered_set<lamure::node_t>>& nodes_in_frustum_per_model,
                                 lamure::context_t context_id,
                                 lamure::view_t lamure_view_id) override;

  private: // shader related auxiliary methods
    virtual void _load_shaders();

  private:
    scm::gl::depth_stencil_state_ptr no_depth_test_with_writing_depth_stencil_state_;
    scm::gl::quad_geometry_ptr fullscreen_quad_;

    scm::gl::sampler_state_ptr nearest_sampler_state_;
};
} // namespace gua

#endif // GUA_NORMAL_SUB_RENDERER_HPP