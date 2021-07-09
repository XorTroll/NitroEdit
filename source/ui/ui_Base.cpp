#include <ui/ui_Base.hpp>
#include <extra/lodepng.h>

namespace ui {

    bool LoadPNGGraphics(const std::string &path, std::shared_ptr<fs::FileHandle> file_handle, u32 &out_width, u32 &out_height, gfx::abgr1555::Color *&out_gfx_buf) {
        std::vector<u8> read_data;
        unsigned int w;
        unsigned int h;

        fs::BinaryFile png_bf;
        if(!png_bf.Open(file_handle, path, fs::OpenMode::Read)) {
            return false;
        }
        const auto png_size = png_bf.GetSize();
        auto png_buf = util::NewArray<u8>(png_size);
        if(!png_bf.ReadData(png_buf, png_size)) {
            delete[] png_buf;
            return false;
        }
        png_bf.Close();

        lodepng::State state = {};
        lodepng::decode(read_data, w, h, state, png_buf, png_size);
        delete[] png_buf;
        out_width = w;
        out_height = h;
        out_gfx_buf = util::NewArray<gfx::abgr1555::Color>(w * h);

        for(u32 i = 0; i < read_data.size(); i += 4) {
            const auto dst_idx = i / 4;
            const gfx::abgr8888::Color clr = {
                .raw_val = *reinterpret_cast<u32*>(read_data.data() + i)
            };
            out_gfx_buf[dst_idx].raw_val = gfx::ConvertColor<gfx::abgr8888::Color, gfx::abgr1555::Color>(clr).raw_val;
        }
        return true;
    }

}