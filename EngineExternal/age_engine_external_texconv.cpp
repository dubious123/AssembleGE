#include <cstdlib>
#include <stdio.h>
#include <format>
#include <filesystem>

namespace age::external::texconv
{
	namespace
	{
		const char*
		derive_intermediate_format(const char* final_format) noexcept
		{
			if (std::strstr(final_format, "BC6H") != nullptr)
			{
				return "R16G16B16A16_FLOAT";
			}
			if (std::strstr(final_format, "_SRGB") != nullptr)
			{
				return "R8G8B8A8_UNORM_SRGB";
			}
			return "R8G8B8A8_UNORM";
		}

		std::string
		build_texconv_cmd_pre_or_single(const detail::bake_options& opt,
										const char*					format_override,	   // nullptr = use opt.dxgi_format_name
										unsigned int				mip_count_override,	   // UINT_MAX = use opt.mip_count
										bool						apply_bc_options,
										const char*					out_dir,
										const char* const*			pp_src_arr,
										unsigned int				src_count) noexcept
		{
			auto cmd  = std::string{ "tools\\texconv\\Texconv.exe -nologo" };
			cmd		 += " -y";

			cmd += " -f ";
			cmd += (format_override != nullptr) ? format_override : opt.dxgi_format_name;

			if (opt.width > 0) { cmd += std::format(" -w {}", opt.width); }
			if (opt.height > 0) { cmd += std::format(" -h {}", opt.height); }

			cmd += std::format(" -m {}", (mip_count_override != UINT_MAX) ? mip_count_override : opt.mip_count);

			if (opt.fit_pow2) { cmd += " -pow2"; }

			if (opt.image_filter)
			{
				cmd += " -if ";
				cmd += opt.image_filter;
			}
			if (opt.wrap) { cmd += " -wrap"; }
			if (opt.mirror) { cmd += " -mirror"; }

			if (opt.srgb_both) { cmd += " -srgb"; }
			if (opt.srgb_in) { cmd += " -srgbi"; }
			if (opt.srgb_out) { cmd += " -srgbo"; }
			if (opt.ignore_srgb) { cmd += " --ignore-srgb"; }
			if (opt.rotate_color)
			{
				cmd += " --rotate-color ";
				cmd += opt.rotate_color;
			}
			if (opt.paper_white_nits > 0.0f) { cmd += std::format(" --nits {}", opt.paper_white_nits); }
			if (opt.tonemap) { cmd += " --tonemap"; }

			if (opt.premultiply_alpha) { cmd += " -pmalpha"; }
			if (opt.straight_alpha) { cmd += " -alpha"; }
			if (opt.separate_alpha) { cmd += " -sepalpha"; }
			if (opt.alpha_threshold >= 0.0f) { cmd += std::format(" -at {}", opt.alpha_threshold); }
			if (opt.keep_coverage >= 0.0f) { cmd += std::format(" --keep-coverage {}", opt.keep_coverage); }
			if (opt.color_key_hex)
			{
				cmd += " -c ";
				cmd += opt.color_key_hex;
			}

			if (apply_bc_options)
			{
				if (opt.gpu_index >= 0) { cmd += std::format(" -gpu {}", opt.gpu_index); }
				if (opt.bc_flags)
				{
					cmd += " -bc ";
					cmd += opt.bc_flags;
				}
				if (opt.alpha_weight >= 0.0f) { cmd += std::format(" -aw {}", opt.alpha_weight); }
			}

			if (opt.nmap_flags)
			{
				cmd += " -nmap ";
				cmd += opt.nmap_flags;
			}
			if (opt.nmap_amplitude >= 0.0f) { cmd += std::format(" -nmapamp {}", opt.nmap_amplitude); }
			if (opt.invert_y) { cmd += " --invert-y"; }
			if (opt.reconstruct_z) { cmd += " --reconstruct-z"; }
			if (opt.x2_bias) { cmd += " --x2-bias"; }

			if (opt.hflip) { cmd += " -hflip"; }
			if (opt.vflip) { cmd += " -vflip"; }
			if (opt.swizzle)
			{
				cmd += " --swizzle ";
				cmd += opt.swizzle;
			}

			cmd += " -dx10";

			cmd += " -o \"";
			cmd += out_dir;
			cmd += '"';
			for (auto i = 0u; i < src_count; ++i)
			{
				cmd += " \"";
				cmd += pp_src_arr[i];
				cmd += '"';
			}
			return cmd;
		}

		std::string
		build_texconv_cmd_post(const detail::bake_options& opt,
							   const char*				   out_dir,
							   const char*				   input_dds_path) noexcept
		{
			auto cmd  = std::string{ "tools\\texconv\\Texconv.exe -nologo" };
			cmd		 += " -y";

			cmd += " -f ";
			cmd += opt.dxgi_format_name;
			cmd += std::format(" -m {}", opt.mip_count);

			if (opt.image_filter)
			{
				cmd += " -if ";
				cmd += opt.image_filter;
			}
			if (opt.wrap) { cmd += " -wrap"; }
			if (opt.mirror) { cmd += " -mirror"; }

			if (opt.gpu_index >= 0) { cmd += std::format(" -gpu {}", opt.gpu_index); }
			if (opt.bc_flags)
			{
				cmd += " -bc ";
				cmd += opt.bc_flags;
			}
			if (opt.alpha_weight >= 0.0f) { cmd += std::format(" -aw {}", opt.alpha_weight); }

			if (opt.keep_coverage >= 0.0f) { cmd += std::format(" --keep-coverage {}", opt.keep_coverage); }

			cmd += " -dx10";

			cmd += " -o \"";
			cmd += out_dir;
			cmd += '"';
			cmd += " \"";
			cmd += input_dds_path;
			cmd += '"';
			return cmd;
		}

		std::string
		build_texassemble_cmd(const detail::bake_options&				opt,
							  const std::vector<std::filesystem::path>& dds_paths,
							  const std::string&						final_path) noexcept
		{
			auto cmd  = std::string{ "tools\\texconv\\Texassemble.exe " };
			cmd		 += opt.assemble_kind;
			cmd		 += " -nologo";

			if (opt.width > 0) { cmd += std::format(" -w {}", opt.width); }
			if (opt.height > 0) { cmd += std::format(" -h {}", opt.height); }

			if (opt.image_filter)
			{
				cmd += " -if ";
				cmd += opt.image_filter;
			}
			if (opt.wrap) { cmd += " -wrap"; }
			if (opt.mirror) { cmd += " -mirror"; }

			cmd += " -dx10";

			if (opt.assemble_gif_bg_color) { cmd += " --gif-bg-color"; }
			if (opt.assemble_swizzle)
			{
				cmd += " --swizzle ";
				cmd += opt.assemble_swizzle;
			}

			cmd += " -o \"";
			cmd += final_path;
			cmd += '"';
			for (const auto& p : dds_paths)
			{
				cmd += " \"";
				cmd += p.string();
				cmd += '"';
			}
			return cmd;
		}
	}	 // namespace

	bool
	bake_texture(const char* const*			 pp_src_arr,
				 unsigned int				 src_count,
				 const detail::bake_options& opt) noexcept
	{
		if (src_count == 0 or pp_src_arr == nullptr or opt.output_dir == nullptr or opt.dxgi_format_name == nullptr) { return false; }

		const auto need_assemble = opt.assemble_kind != nullptr and opt.assemble_kind[0] != '\0';
		const auto needs_post	 = need_assemble and opt.mip_count != 1;

		std::filesystem::create_directories(opt.output_dir);

		auto tmp_dir = std::filesystem::path{};
		if (need_assemble)
		{
			tmp_dir = std::filesystem::path{ opt.output_dir } / "__texconv_tmp__";
			std::filesystem::create_directories(tmp_dir);
		}

		// texconv pre
		const char*	 pre_format		  = nullptr;
		unsigned int pre_mip_override = UINT_MAX;
		bool		 pre_apply_bc	  = true;
		auto		 pre_out_dir	  = std::string(opt.output_dir);

		if (need_assemble)
		{
			pre_out_dir		 = tmp_dir.string();
			pre_mip_override = 1;
			if (needs_post)
			{
				pre_format	 = opt.intermediate_format != nullptr
								 ? opt.intermediate_format
								 : derive_intermediate_format(opt.dxgi_format_name);
				pre_apply_bc = false;
			}
		}

		{
			auto cmd = build_texconv_cmd_pre_or_single(opt, pre_format, pre_mip_override, pre_apply_bc,
													   pre_out_dir.c_str(), pp_src_arr, src_count);
			if (std::system(cmd.c_str()) != 0)
			{
				if (need_assemble) { std::filesystem::remove_all(tmp_dir); }
				return false;
			}
		}

		if (need_assemble == false)
		{
			if (opt.assemble_output_filename != nullptr)
			{
				auto first	  = std::filesystem::path{ pp_src_arr[0] };
				auto produced = std::filesystem::path{ opt.output_dir } / (first.stem().string() + ".dds");
				auto desired  = std::filesystem::path{ opt.output_dir } / opt.assemble_output_filename;

				if (produced != desired)
				{
					auto ec = std::error_code{};
					std::filesystem::remove(desired, ec);
					std::filesystem::rename(produced, desired, ec);
					if (ec) { return false; }
				}
			}

			return true;
		}


		// texassemble
		auto dds_paths = std::vector<std::filesystem::path>{};
		dds_paths.reserve(src_count);
		for (auto i = 0u; i < src_count; ++i)
		{
			auto p = std::filesystem::path{ pp_src_arr[i] };
			dds_paths.push_back(tmp_dir / (p.stem().string() + ".dds"));
		}

		auto assemble_out_dir = needs_post ? tmp_dir : std::filesystem::path{ opt.output_dir };

		auto assemble_filename = std::string{};
		if (opt.assemble_output_filename)
		{
			assemble_filename = opt.assemble_output_filename;
		}
		else
		{
			auto first		  = std::filesystem::path{ pp_src_arr[0] };
			assemble_filename = first.stem().string() + ".dds";
		}

		auto assemble_full_path = std::filesystem::path{};
		if (needs_post)
		{
			assemble_full_path = assemble_out_dir / "__assembled__.dds";
		}
		else
		{
			assemble_full_path = assemble_out_dir / assemble_filename;
		}

		{
			if (std::system(build_texassemble_cmd(opt, dds_paths, assemble_full_path.string()).c_str()) != 0)
			{
				std::filesystem::remove_all(tmp_dir);
				return false;
			}
		}

		if (needs_post == false)
		{
			std::filesystem::remove_all(tmp_dir);
			for (auto i = 0u; i < src_count; ++i)
			{
				std::remove(pp_src_arr[i]);
			}
			return true;
		}

		// texconv post
		{
			if (std::system(build_texconv_cmd_post(opt, opt.output_dir, assemble_full_path.string().c_str()).c_str()) != 0)
			{
				std::filesystem::remove_all(tmp_dir);
				return false;
			}
		}

		auto post_out_path	= std::filesystem::path(opt.output_dir) / "__assembled__.dds";
		auto final_out_path = std::filesystem::path(opt.output_dir) / assemble_filename;
		if (post_out_path != final_out_path)
		{
			auto ec = std::error_code{};
			std::filesystem::rename(post_out_path, final_out_path, ec);
			if (ec)
			{
				std::filesystem::remove_all(tmp_dir);
				return false;
			}
		}

		std::filesystem::remove_all(tmp_dir);

		for (auto i = 0u; i < src_count; ++i)
		{
			std::remove(pp_src_arr[i]);
		}

		return true;
	}
}	 // namespace age::external::texconv