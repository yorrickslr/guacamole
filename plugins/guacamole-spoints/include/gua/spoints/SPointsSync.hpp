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

#include <queue>

#include <gua/utils.hpp>

namespace gua
{

class GUA_SPOINTS_DLL SPointsSync
{
  public: 
     SPointsSync();
    ~SPointsSync() {};


    void set_sync_length(unsigned const& new_sync_length);
    unsigned get_sync_length();

  private:
    
    std::queue<scm::math::mat<double, 4u, 4u>> sync_queue;
    unsigned sync_length;
};

} // namespace gua

#endif // GUA_SPOINTS_SYNC_HPP
