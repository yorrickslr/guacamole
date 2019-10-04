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

#include <gua/spoints/SPointsSync.hpp>

// guacamole headers
#include <gua/guacamole.hpp>
#include <gua/platform.hpp>

namespace gua
{
////////////////////////////////////////////////////////////////////////////////
SPointsSync::SPointsSync() : sync_length{1000} {
	// file->open("C:/Users/HMDEyes/SPointsSync.log");
}

//////////////////////////////////////////////////////////////////////////////
void SPointsSync::synchronize(scm::math::mat<double, 4u, 4u>& matrix)
{
    return;
    sync_queue->push(matrix);
    matrix = sync_queue->front();
    if(sync_queue->size() > sync_length)
    {
        sync_queue->pop();
    }
}

//////////////////////////////////////////////////////////////////////////////
scm::math::mat<double, 4u, 4u> SPointsSync::get_synchronized(scm::math::mat<double, 4u, 4u> const& matrix)
{
    // *file << "sync lenght is " << sync_length << std::endl;
	// *file << "pushing matrix to queue:" << std::endl;
    // *file << matrix << std::endl;
    sync_queue->push(matrix);
    if(sync_queue->size() > sync_length)
    {
        sync_queue->pop();
    }

    return sync_queue->front();
}

//////////////////////////////////////////////////////////////////////////////
void SPointsSync::set_sync_length(int new_sync_length) { sync_length = new_sync_length; }

//////////////////////////////////////////////////////////////////////////////
int SPointsSync::get_sync_length() { return sync_length; }

} // namespace gua
