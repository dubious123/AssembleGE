#include <cstdlib>
#include <stdio.h>
#include <format>

namespace age::external::msdfgen
{
	bool
	bake_font(const char*		 font_path,
			  const char*		 output_image_path,
			  const char*		 output_csv_path,
			  const char*		 output_json_path,
			  unsigned long long font_charset_flag,
			  unsigned short*	 extra_unicode_arr,
			  unsigned short	 extra_unicode_arr_count,
			  unsigned int		 em_size,
			  unsigned int		 px_range) noexcept
	{
		if (extra_unicode_arr_count > 0 and extra_unicode_arr == nullptr)
		{
			return false;
		}

		unsigned long long unicode_count = 0;

		if (font_charset_flag & 0x1)	// ascii
		{
			unicode_count += ('~' - ' ' + 1);
		}

		if (font_charset_flag & 0x2)	// hangul
		{
			unicode_count += (0xD7A3 - 0xAC00 + 1);
		}

		auto* p_unicode_arr = new unsigned short[unicode_count + extra_unicode_arr_count];

		{
			auto i = 0;

			if (font_charset_flag & 0x1)	// ascii
			{
				for (auto c = ' '; c <= '~'; ++c)
				{
					p_unicode_arr[i++] = static_cast<unsigned short>(c);
				}
			}

			if (font_charset_flag & 0x2)	// hangul
			{
				for (auto c = 0xAC00; c <= 0xD7A3; ++c)
				{
					p_unicode_arr[i++] = static_cast<unsigned short>(c);
				}
			}

			for (auto j = 0; j < extra_unicode_arr_count; ++j)
			{
				p_unicode_arr[i++] = extra_unicode_arr[j];
			}
		}

		{
			auto* p_file = (FILE*)nullptr;
			auto  err	 = fopen_s(&p_file, "_temp_charset.txt", "w");
			if (err != 0 || p_file == nullptr)
			{
				delete[] p_unicode_arr;
				return false;
			}

			for (unsigned long long i = 0; i < unicode_count + extra_unicode_arr_count; ++i)
			{
				std::fprintf(p_file, "0x%04X\n", p_unicode_arr[i]);
			}

			fclose(p_file);
		}

		{
			auto cmd = std::format(
				"tools\\msdfgen\\msdf-atlas-gen.exe"
				" -font \"{}\""
				" -type mtsdf"
				" -charset _temp_charset.txt"
				" -imageout \"{}\""
				" -csv \"{}\""
				" -json \"{}\""
				" -size {}"
				" -pxrange {}",
				font_path, output_image_path, output_csv_path, output_json_path, em_size, px_range);

			auto result = std::system(cmd.c_str());

			std::remove("_temp_charset.txt");

			if (result != 0)
			{
				std::remove(output_image_path);
				std::remove(output_csv_path);
				delete[] p_unicode_arr;
				return false;
			}
		}

		delete[] p_unicode_arr;
		return true;
	}
}	 // namespace age::external::msdfgen