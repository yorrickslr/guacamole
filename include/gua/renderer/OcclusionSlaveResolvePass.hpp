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

#ifndef GUA_OCCLUSION_SLAVE_RESOLVE_PASS_HPP
#define GUA_OCCLUSION_SLAVE_RESOLVE_PASS_HPP

#include <gua/renderer/PipelinePass.hpp>

#include <memory>

namespace gua {

class Pipeline;

class GUA_DLL OcclusionSlaveResolvePassDescription : public PipelinePassDescription {
 public:


  OcclusionSlaveResolvePassDescription();


  void apply_post_render_action(RenderContext const& ctx, gua::Pipeline* pipe) const override;



  std::shared_ptr<PipelinePassDescription> make_copy() const override;
  friend class Pipeline;

 protected:
  PipelinePass make_pass(RenderContext const&, SubstitutionMap&) override;
  mutable std::size_t last_rendered_view_id = std::numeric_limits<std::size_t>::max();
  mutable std::size_t last_rendered_side = 0;
};
}

#endif  // GUA_OCCLUSION_SLAVE_RESOLVE_PASS_HPP
