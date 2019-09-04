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

#ifndef GUA_SCREENGRAB_PASS_HPP
#define GUA_SCREENGRAB_PASS_HPP

#include <gua/config.hpp>

#include <gua/renderer/Pipeline.hpp>
#include <gua/renderer/PipelinePass.hpp>

#include <memory>

namespace gua
{
class Pipeline;

class GUA_DLL ScreenGrabPassDescription : public PipelinePassDescription
{
  public:
    ScreenGrabPassDescription();
    std::shared_ptr<PipelinePassDescription> make_copy() const override;
    friend class Pipeline;

    void set_output_prefix(const std::string& output_prefix);
    void set_grab_next(bool grab_next);

  protected:
    PipelinePass make_pass(RenderContext const&, SubstitutionMap&) override;

  private:
    std::string output_prefix_;
    bool grab_next_;
};

#ifdef GUACAMOLE_ENABLE_TURBOJPEG

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#define F2B(f) ((f) >= 1.0 ? (unsigned char)255 : (unsigned char)((f)*256.0))

class GUA_DLL ScreenGrabJPEGSaver
{
  public:
    static ScreenGrabJPEGSaver* get_instance()
    {
        static ScreenGrabJPEGSaver saver;
        return &saver;
    }

    ~ScreenGrabJPEGSaver()
    {
        should_save_.store(false);
        should_quit_.store(true);
        save_cv_.notify_one();
        worker_.join();
    }

    void save(std::string& output_prefix, scm::math::vec2ui& dims, std::vector<float>& rgb_32f)
    {
        output_prefix_ = output_prefix;
        dims_ = dims;
        rgb_32f_.swap(rgb_32f);

        size_t length = dims_.x * dims_.y * 3;

        if(rgb_8_.size() != length)
        {
            rgb_8_.resize(length);
        }

        should_save_.store(true);
        save_cv_.notify_one();
    }

  private:
    ScreenGrabJPEGSaver() : save_lock_(), should_quit_(false), should_save_(false), save_cv_(), output_prefix_(""), dims_(), rgb_32f_(), rgb_8_()
    {
        worker_ = std::thread([&]() -> void {
            while(!should_quit_.load())
            {
                std::unique_lock<std::mutex> lk(save_lock_, std::defer_lock);
                if(should_save_.load())
                {
                    save_();
                }
                should_save_.store(false);

                while(!save_cv_.wait_for(lk, std::chrono::milliseconds(16), [&]() -> bool { return should_save_.load(); }))
                {
                    if(should_quit_.load())
                    {
                        break;
                    }
                }
            }
        });
    }

    bool write_JPEG_file(const char* filename, int quality)
    {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE* outfile;           /* target file */
        JSAMPROW row_pointer[1]; /* pointer to JSAMPLE row[s] */
        int row_stride;          /* physical row width in image buffer */
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        if((outfile = fopen(filename, "wb")) == NULL)
        {
            fprintf(stderr, "can't open %s\n", filename);
            return false;
        }
        jpeg_stdio_dest(&cinfo, outfile);

        cinfo.image_width = dims_.x; /* image width and height, in pixels */
        cinfo.image_height = dims_.y;
        cinfo.input_components = 3;     /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB; /* colorspace of input image */

        jpeg_set_defaults(&cinfo);

        jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

        jpeg_start_compress(&cinfo, TRUE);

        row_stride = dims_.x * 3; /* JSAMPLEs per row in image_buffer */

        while(cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer[0] = &rgb_8_[cinfo.next_scanline * row_stride];
            (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        fclose(outfile);

        jpeg_destroy_compress(&cinfo);

        return true;
    }

    void save_()
    {
        unsigned i;
        unsigned length = dims_.x * dims_.y;

        for(i = 0; i < length; i++)
        {
            unsigned offset = i * 3;
            unsigned flipped_offset = ((dims_.y - i / dims_.x) * dims_.x + i % dims_.x) * 3;

            rgb_8_[offset] = F2B(rgb_32f_[flipped_offset]);
            rgb_8_[offset + 1] = F2B(rgb_32f_[flipped_offset + 1]);
            rgb_8_[offset + 2] = F2B(rgb_32f_[flipped_offset + 2]);
        }

        long millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::string timestamp{std::to_string(millis)};

        if(!write_JPEG_file((output_prefix_ + timestamp + ".jpg").c_str(), 100))
        {
            std::cerr << "Could not write JPEG" << std::endl;
        }
    }

    std::mutex save_lock_;
    std::atomic_bool should_quit_;
    std::atomic_bool should_save_;
    std::condition_variable save_cv_;
    std::thread worker_;

    std::string output_prefix_;
    scm::math::vec2ui dims_;
    std::vector<float> rgb_32f_;
    std::vector<unsigned char> rgb_8_;
};
#endif //GUACAMOLE_ENABLE_TURBOJPEG

} // namespace gua

#endif // GUA_SCREENGRAB_PASS_HPP
