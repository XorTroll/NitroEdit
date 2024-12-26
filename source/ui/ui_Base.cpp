#include <ui/ui_Base.hpp>
#include <extra/lodepng.h>

namespace ui {

    ntr::Result LoadPNGGraphics(const std::string &path, std::shared_ptr<ntr::fs::FileHandle> file_handle, u32 &out_width, u32 &out_height, ntr::gfx::abgr1555::Color *&out_gfx_buf) {
        std::vector<u8> read_data;
        unsigned int w;
        unsigned int h;

        ntr::fs::BinaryFile png_bf;
        NTR_R_TRY(png_bf.Open(file_handle, path, ntr::fs::OpenMode::Read));

        size_t png_size;
        NTR_R_TRY(png_bf.GetSize(png_size));

        auto png_buf = ntr::util::NewArray<u8>(png_size);
        NTR_R_TRY(png_bf.ReadDataExact(png_buf, png_size));
        png_bf.Close();

        lodepng::State state = {};
        const auto lodepng_err = lodepng::decode(read_data, w, h, state, png_buf, png_size);
        if(lodepng_err != 0) {
            NTR_R_FAIL(ResultUiLodepngDecodeError);
        }
        delete[] png_buf;

        out_width = w;
        out_height = h;
        out_gfx_buf = ntr::util::NewArray<ntr::gfx::abgr1555::Color>(w * h);

        for(u32 i = 0; i < read_data.size(); i += 4) {
            const auto dst_idx = i / 4;
            const ntr::gfx::abgr8888::Color clr = {
                .raw_val = *reinterpret_cast<u32*>(read_data.data() + i)
            };
            out_gfx_buf[dst_idx].raw_val = ntr::gfx::ConvertColor<ntr::gfx::abgr8888::Color, ntr::gfx::abgr1555::Color>(clr).raw_val;
        }
        NTR_R_SUCCEED();
    }

}