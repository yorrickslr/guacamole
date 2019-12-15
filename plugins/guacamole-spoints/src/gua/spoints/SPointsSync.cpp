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
SPointsSync::SPointsSync() : sync_length_{1}
{
    // file->open("C:/Users/HMDEyes/SPointsSync.log");
}

//////////////////////////////////////////////////////////////////////////////
scm::math::mat<double, 4u, 4u> SPointsSync::get_synchronized(scm::math::mat<double, 4u, 4u> const& matrix, bool is_left)
{
    // *file << "sync lenght is " << sync_length << std::endl;
    // *file << "pushing matrix to queue:" << std::endl;
    // *file << matrix << std::endl;
    if(sync_length_ == 0)
    {
        return matrix;
    }
    if(is_left)
    {
        sync_queue_l_->push(matrix);
        if(sync_queue_l_->size() > sync_length_)
        {
            sync_queue_l_->pop();
        }

        return sync_queue_l_->front();
    }
    else
    {
        sync_queue_r_->push(matrix);
        if(sync_queue_r_->size() > sync_length_)
        {
            sync_queue_r_->pop();
        }

        return sync_queue_r_->front();
    }
}

//////////////////////////////////////////////////////////////////////////////
scm::math::mat<double, 4u, 4u> SPointsSync::get_synchronized(scm::math::mat<double, 4u, 4u> const& matrix) { return get_synchronized(matrix, true); }

//////////////////////////////////////////////////////////////////////////////
void SPointsSync::set_sync_length(int new_sync_length)
{
    sync_length_ = new_sync_length;
    while(sync_length_ < sync_queue_l_->size())
    {
        sync_queue_l_->pop();
    }
    while(sync_length_ < sync_queue_r_->size())
    {
        sync_queue_r_->pop();
    }
}

//////////////////////////////////////////////////////////////////////////////
int SPointsSync::get_sync_length() { return sync_length_; }

} // namespace gua
