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

#ifndef GUA_SPOINTS_SYNC_HPP
#define GUA_SPOINTS_SYNC_HPP

#include <gua/spoints/platform.hpp>

#include <memory>
#include <queue>

#include <gua/math/math.hpp>
#include <gua/utils.hpp>

#include <fstream>
#include <iostream>

namespace gua
{
class GUA_SPOINTS_DLL SPointsSync
{
  public:
    SPointsSync();
    ~SPointsSync(){};

    void set_sync_length(int new_sync_length);
    int get_sync_length();
    scm::math::mat<double, 4u, 4u> get_synchronized(scm::math::mat<double, 4u, 4u> const& matrix, bool is_left);
    scm::math::mat<double, 4u, 4u> get_synchronized(scm::math::mat<double, 4u, 4u> const& matrix);

    // unsigned long id = (unsigned long)this;
    // std::ofstream* file = new std::ofstream();

  private:
    std::shared_ptr<std::queue<scm::math::mat<double, 4u, 4u>>> sync_queue_l_ = std::make_shared<std::queue<scm::math::mat<double, 4u, 4u>>>(std::queue<scm::math::mat<double, 4u, 4u>>());
    std::shared_ptr<std::queue<scm::math::mat<double, 4u, 4u>>> sync_queue_r_ = std::make_shared<std::queue<scm::math::mat<double, 4u, 4u>>>(std::queue<scm::math::mat<double, 4u, 4u>>());
    int sync_length_;
};

} // namespace gua

#endif // GUA_SPOINTS_SYNC_HPP
