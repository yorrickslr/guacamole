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

// class header
#include <gua/renderer/StencilPass.hpp>

#include <gua/renderer/TriMeshRessource.hpp>
#include <gua/renderer/StencilRenderer.hpp>
#include <gua/renderer/Pipeline.hpp>
#include <gua/utils/Logger.hpp>
#include <gua/databases/GeometryDatabase.hpp>
#include <gua/databases/MaterialShaderDatabase.hpp>
#include <gua/databases/Resources.hpp>
#include <gua/node/TriMeshNode.hpp>

namespace gua
{
////////////////////////////////////////////////////////////////////////////////

StencilPassDescription::StencilPassDescription() : PipelinePassDescription()
{
    vertex_shader_ = "shaders/tri_mesh_shader_stencil.vert";
    fragment_shader_ = "shaders/tri_mesh_shader_stencil.frag";
    private_.name_ = "StencilPass";

    private_.needs_color_buffer_as_input_ = false;
    private_.writes_only_color_buffer_ = true;
    private_.enable_for_shadows_ = false;
    private_.rendermode_ = RenderMode::Callback;

    private_.depth_stencil_state_desc_ = boost::make_optional(scm::gl::depth_stencil_state_desc(
        false, false, scm::gl::COMPARISON_LESS, true, 1, 0xFF, scm::gl::stencil_ops(scm::gl::COMPARISON_ALWAYS, scm::gl::STENCIL_KEEP, scm::gl::STENCIL_KEEP, scm::gl::STENCIL_REPLACE)));
}

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<PipelinePassDescription> StencilPassDescription::make_copy() const { return std::make_shared<StencilPassDescription>(*this); }

////////////////////////////////////////////////////////////////////////////////

PipelinePass StencilPassDescription::make_pass(RenderContext const& ctx, SubstitutionMap& substitution_map)
{
    auto renderer = std::make_shared<StencilRenderer>();
    renderer->create_state_objects(ctx);

    private_.process_ = [renderer](PipelinePass& pass, PipelinePassDescription const& desc, Pipeline& pipe) {
        pipe.current_viewstate().target->clear(pipe.get_context(), 1.f, 0);
        pipe.get_context().render_context->set_depth_stencil_state(pass.depth_stencil_state(), 1);
        renderer->render(pipe, pass.shader());
    };

    PipelinePass pass{*this, ctx, substitution_map};
    return pass;
}

} // namespace gua
