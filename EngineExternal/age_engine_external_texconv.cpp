#include <cstdlib>
#include <stdio.h>
#include <format>
#include <span>

namespace age::external::texconv
{
	bool
	bake_texture(const char** pp_src,
				 unsigned int src_count,
				 const char*  p_output_dir,
				 const char*  dxgi_format_name,		 // e.g. "BC7_UNORM_SRGB", "BC5_UNORM"
				 const char*  texture_kind_flags,	 // "", "-array", "-cube", "-array -cube", "-volume"
				 unsigned int mip_count,			 // 0 = full chain
				 bool		  is_srgb_input) noexcept
	{
		if (src_count == 0) { return false; }

		std::string src;
		for (auto i = 0u; i < src_count; ++i)
		{
			auto* p	 = pp_src[i];
			src		+= " \"";
			src		+= p;
			src		+= '"';
		}

		auto cmd = std::format(
			"tools\\texconv\\Texconv.exe -nologo -y"
			" -f {}"
			"{}"
			" {}"
			" -m {}"
			" -o \"{}\""
			"{}",
			dxgi_format_name,
			is_srgb_input ? " -srgb" : "",
			texture_kind_flags,
			mip_count,
			p_output_dir,
			src);

		auto result = std::system(cmd.c_str());

		return result == 0;
	}
}	 // namespace age::external::texconv