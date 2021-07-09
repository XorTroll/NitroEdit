#include <util/util_Compression.hpp>
#include <util/util_Memory.hpp>

namespace util {

    namespace {

        // TODO: improve these?

        void LZ77CompressSearch(const u8 *data, const size_t data_size, const size_t pos, size_t &match, size_t &length)
		{
			size_t num = 4096;
			size_t num2 = 18;
			match = 0;
			length = 0;
			size_t num3 = pos - num;
			if (num3 < 0)
			{
				num3 = 0;
			}
			for (size_t i = num3; i < pos; i++)
			{
				size_t num4 = 0;
				while ((num4 < num2) && ((i + num4) < pos) && ((pos + num4) < data_size) && (data[pos + num4] == data[i + num4]))
				{
					num4++;
				}
				if (num4 > length)
				{
					match = i;
					length = num4;
				}
				if (length == num2)
				{
					return;
				}
			}
		}

    }

    void LZ77Compress(u8 *&out_data, size_t &out_size, const u8 *data, const size_t data_size)
    {
        auto tmp_array = util::NewArray<u8>(data_size + data_size / 8 + 4);
        *(u32*)tmp_array = static_cast<u32>(data_size << 8 | 16);
        size_t offset = sizeof(u32);
        u8 array[16] = {};
        size_t i = 0;
        while (i < data_size)
        {
            size_t num = 0;
            u8 b = 0;
            for (int j = 0; j < 8; j++)
            {
                if (i >= data_size)
                {
                    array[num++] = 0;
                }
                else
                {
                    size_t num2 = 0;
                    size_t num3 = 0;
                    LZ77CompressSearch(data, data_size, i, num2, num3);
                    size_t num4 = i - num2 - 1;
                    if (num3 > 2)
                    {
                        b |= (u8)(1 << (7 - j));
                        array[num++] = (u8)((((num3 - 3) & 15) << 4) + (num4 >> 8 & 15));
                        array[num++] = (u8)(num4 & 255);
                        i += num3;
                    }
                    else
                    {
                        array[num++] = data[i++];
                    }
                }
            }
            tmp_array[offset] = b;
            offset++;
            for (size_t k = 0; k < num; k++)
            {
                tmp_array[offset] = array[k];
                offset++;
            }
        }

        out_data = util::NewArray<u8>(offset);
        out_size = offset;
        std::memcpy(out_data, tmp_array, offset);
        delete[] tmp_array;
    }

    void LZ77Decompress(u8 *&out_data, size_t &out_size, const u8 *data) {
        auto num = data[1] | (data[2] << 8) | (data[3] << 16);
        out_data = NewArray<u8>(num);
        out_size = num;
        int num2 = 4;
        int num3 = 0;
        while (num > 0)
        {
            auto b = data[num2++];
            if (b != 0)
            {
                for (int i = 0; i < 8; i++)
                {
                    if ((b & 0x80) != 0)
                    {
                        int num5 = (data[num2] << 8) | data[num2 + 1];
                        num2 += 2;
                        int num6 = (num5 >> 12) + 3;
                        int num7 = num5 & 0xFFF;
                        int num8 = num3 - num7 - 1;
                        for (int j = 0; j < num6; j++)
                        {
                            out_data[num3++] = out_data[num8++];
                            num--;
                            if (num == 0)
                            {
                                return;
                            }
                        }
                    }
                    else
                    {
                        out_data[num3++] = data[num2++];
                        num--;
                        if (num == 0)
                        {
                            return;
                        }
                    }
                    b = (u8)(b << 1);
                }
                continue;
            }
            for (int i = 0; i < 8; i++)
            {
                out_data[num3++] = data[num2++];
                num--;
                if (num == 0)
                {
                    return;
                }
            }
        }
    }

}