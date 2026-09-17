#include "age_pch.hpp"
#include "age.hpp"

#if defined(AGE_GRAPHICS_BACKEND_DX12)

namespace age::graphics::shader
{
	void
	init() noexcept
	{
		AGE_HR_CHECK(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g::p_dxc_compiler)));
		AGE_HR_CHECK(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&g::p_dxc_utils)));
		AGE_HR_CHECK(g::p_dxc_utils->CreateDefaultIncludeHandler(&g::p_dxc_include_handler));

		if (fs::create_dir(g::engine_shaders_dir_path) is_false)
		{
			AGE_ASSERT(false, "g::engine_shaders_dir_path create failed, g::engine_shaders_dir_path : {}", g::engine_shaders_dir_path);
			std::abort();
		}
		if (fs::create_dir(g::engine_shaders_compiled_blob_dir_path) is_false)
		{
			AGE_ASSERT(false, "g::engine_shaders_compiled_blob_dir_path create failed, g::engine_shaders_compiled_blob_dir_path : {}", g::engine_shaders_compiled_blob_dir_path);
			std::abort();
		}

		for (const auto&& shader_name : std::views::iota(0ul)
											| std::views::take(e::size<e::engine_shader_kind>())
											| std::views::transform([](auto i) { return e::to_string(static_cast<e::engine_shader_kind>(i)); }))
		{
			c_auto hlsl_path = fs::join(g::engine_shaders_dir_path, std::format("{}{}", shader_name, config::shader_extension));

			AGE_ASSERT(fs::file_exists(hlsl_path));

			c_auto compiled_blob_path = std::format("{}{}.bin", g::engine_shaders_compiled_blob_dir_path, shader_name);

			AGE_ASSERT(shader_name.find_last_of('_') != std::wstring_view::npos);

			c_auto stage	   = shader_name.substr(shader_name.find_last_of('_') + 1);
			c_auto target	   = std::format("{}{}", stage, "_6_8");
			c_auto entry_point = std::format("{}{}", "main_", stage);

			auto [success, newest_include_time] = fs::get_last_write_time(hlsl_path);
			AGE_ASSERT(success, "get shader last write time failed, shader name : {}", shader_name);

			fs::for_each_file(fs::get_parent_path(hlsl_path), [&](const fs::file_entry& entry) {
				c_auto ext = fs::get_file_extension(entry.name);
				if (ext == ".h" or ext == config::shader_include_extension)
				{
					newest_include_time = std::max(newest_include_time, entry.last_write_time);
				}
			});

			c_auto[file_size_ok, file_size]				= fs::get_file_size(compiled_blob_path);
			c_auto[last_write_time_ok, last_write_time] = fs::get_last_write_time(compiled_blob_path);

			if (c_auto need_recompile =
					fs::exists(compiled_blob_path) is_false
					or (last_write_time_ok and newest_include_time > last_write_time)
					or file_size_ok is_false
					or file_size == 0)
			{
				compile_shader(shader_name, hlsl_path, entry_point, target, compiled_blob_path);
			}

			load_shader(compiled_blob_path);
		}
	}

	void
	deinit() noexcept
	{
		for (auto idx : std::views::iota(0ul) | std::views::take(g::shader_blob_vec.size()))
		{
			unload_shader(shader_handle{ .id = idx });
		}

		if constexpr (age::config::debug_mode)
		{
			g::shader_blob_vec.debug_validate();
		}

		g::shader_blob_vec.clear();

		g::p_dxc_include_handler->Release();
		g::p_dxc_utils->Release();
		g::p_dxc_compiler->Release();
	}

	void
	compile_shader(
		std::string_view shader_name,
		std::string_view hlsl_path,
		std::string_view entry_point,
		std::string_view target,
		std::string_view save_path) noexcept
	{
		auto* p_file	 = (IDxcBlobEncoding*)nullptr;
		auto* p_result	 = (IDxcResult*)nullptr;
		auto* p_res_blob = (IDxcBlob*)nullptr;

		c_auto w_hlsl_path	 = fs::detail::to_utf16(hlsl_path);
		c_auto w_dir_path	 = fs::detail::to_utf16(fs::get_parent_path(hlsl_path));
		c_auto w_entry_point = fs::detail::to_utf16(entry_point);
		c_auto w_target		 = fs::detail::to_utf16(target);

		AGE_HR_CHECK(g::p_dxc_utils->LoadFile(w_hlsl_path.data(), nullptr, &p_file));

		// ex) full_screen_ms => #define AGE_SHADER_NAME 'f', 'u', 'l', 'l', 's', 'c', 'r', 'e', 'e', 'n', '_', 'm', 's'
		auto wchar_buffer = dynamic_array<wchar_t>::gen_sized_copy(shader_name.size() * 4, '\0');

		for (auto&& [i, c] : shader_name | std::views::enumerate)
		{
			const wchar_t sep		= cast_to<uint64>(i) < (shader_name.size() - 1) ? L',' : L'\0';
			wchar_buffer[i * 4 + 0] = L'\'';
			wchar_buffer[i * 4 + 1] = c;
			wchar_buffer[i * 4 + 2] = L'\'';
			wchar_buffer[i * 4 + 3] = sep;
		}

		c_auto shader_name_def = std::wstring{ L"AGE_SHADER_NAME=" } + std::wstring{ wchar_buffer.data() };
		c_auto shader_hash_def = std::wstring{ L"AGE_SHADER_HASH=" } + std::to_wstring(cast_to<uint32>(age::hash<std::string_view>{}(shader_name))) + L"u";

		std::println("compiling {}", shader_name);
		{
			auto is_known  = FALSE;
			auto code_page = UINT32{ 0 };
			AGE_HR_CHECK(p_file->GetEncoding(&is_known, &code_page));

			auto buffer = DxcBuffer{
				.Ptr	  = p_file->GetBufferPointer(),
				.Size	  = p_file->GetBufferSize(),
				.Encoding = is_known ? code_page : DXC_CP_ACP
			};

			auto args = age::array{
				w_hlsl_path.data(),
				L"-E",
				w_entry_point.data(),
				L"-T",
				w_target.data(),
				L"-I",
				w_dir_path.data(),
				L"-HV",
				L"202x",
				// L"-Qstrip_reflect",
				// L"-Qstrip_debug",
				L"-D",
				shader_name_def.data(),
				L"-D",
				shader_hash_def.data(),
				L"-enable-16bit-types",
				L"-Zi",

				DXC_ARG_WARNINGS_ARE_ERRORS,
	#if defined(AGE_DEBUG)
				L"-Qembed_debug",
				DXC_ARG_DEBUG,
		#if !AGE_ENABLE_GBV
				DXC_ARG_SKIP_OPTIMIZATIONS
		#endif
	#elif defined(AGE_RELEASE)
				L"-Qembed_debug",
				DXC_ARG_ALL_RESOURCES_BOUND,
				DXC_ARG_SKIP_OPTIMIZATIONS,
				// DXC_ARG_OPTIMIZATION_LEVEL3,
	#endif
			};

			AGE_HR_CHECK(g::p_dxc_compiler->Compile(
				&buffer,
				args.data(),
				static_cast<UINT32>(args.size()),
				g::p_dxc_include_handler,
				IID_PPV_ARGS(&p_result)));
		}

		{
			// #pragma warning(disable : 6387)
			//		p_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&p_error), nullptr);
			// #pragma warning(default : 6387)

			if (auto* p_error = (IDxcBlobUtf8*)nullptr;

				SUCCEEDED(p_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&p_error), nullptr)) and p_error is_not_nullptr)
			{
				if (p_error->GetBufferSize() > 0)
				{
					std::println("{}", p_error->GetStringPointer());
				}

				p_error->Release();
			}

			auto status = (HRESULT)S_OK;
			p_result->GetStatus(&status);
			AGE_HR_CHECK(status);

			// auto* p_pdb		 = (IDxcBlob*)nullptr;
			// auto* p_pdb_name = (IDxcBlobUtf16*)nullptr;

			// AGE_HR_CHECK(p_result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&p_pdb), &p_pdb_name));

			// if (p_pdb is_not_nullptr and p_pdb->GetBufferSize() > 0)
			//{
			//	auto full_pdb_path = std::filesystem::path(dir_path) / p_pdb_name->GetStringPointer();
			//	auto blob_file	   = std::ofstream{ full_pdb_path, std::ios::out | std::ios::binary };
			//	blob_file.clear();

			//	auto& stream = blob_file.write(static_cast<const char*>(p_pdb->GetBufferPointer()), p_pdb->GetBufferSize());

			//	AGE_ASSERT(stream.good());

			//	blob_file.close();

			//	p_pdb->Release();
			//	p_pdb_name->Release();
			//}
		}

		AGE_HR_CHECK(p_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&p_res_blob), nullptr));

		{
			p_file->Release();
			p_result->Release();
		}

		c_auto blob = std::span{ static_cast<const std::byte*>(p_res_blob->GetBufferPointer()), p_res_blob->GetBufferSize() };
		if (fs::write_file(save_path, blob) is_false)
		{
			AGE_ASSERT(false, "failed to write shader blob : {}", save_path);
		}
		p_res_blob->Release();
	}

	shader_handle
	load_shader(std::string_view shader_blob_path) noexcept
	{
		AGE_ASSERT(fs::file_exists(shader_blob_path));
		c_auto[ok, file_size] = fs::get_file_size(shader_blob_path);
		AGE_ASSERT(ok and file_size > 0);

		auto p_blob = ::operator new(file_size, std::align_val_t{ alignof(char) });

		c_auto[read_ok, read_size] = fs::read_file(shader_blob_path, std::span<std::byte>{ static_cast<std::byte*>(p_blob), file_size }, file_size);

		if (read_ok is_false or read_size != file_size)
		{
			AGE_ASSERT(false, "read shader blob failed, shader_blob_path : {}", shader_blob_path);
			std::abort();
		}

		return { .id = g::shader_blob_vec.emplace_back(shader_blob{ .p_blob = p_blob, .size = file_size }) };
	}

	D3D12_SHADER_BYTECODE
	get_d3d12_bytecode(shader_handle sh) noexcept
	{
		const auto& shader_blob = g::shader_blob_vec[sh.id];
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = shader_blob.p_blob,
			.BytecodeLength	 = shader_blob.size,
		};
	}

	D3D12_SHADER_BYTECODE
	get_d3d12_bytecode(e::engine_shader_kind e_kind) noexcept
	{
		const auto& shader_blob = g::shader_blob_vec[to_idx(e_kind)];
		return D3D12_SHADER_BYTECODE{
			.pShaderBytecode = shader_blob.p_blob,
			.BytecodeLength	 = shader_blob.size,
		};
	}

	void
	unload_shader(shader_handle sh) noexcept
	{
		auto& blob = g::shader_blob_vec[sh.id];
		::operator delete((void*)blob.p_blob, std::align_val_t{ alignof(char) });
		g::shader_blob_vec.remove(sh.id);
	}
}	 // namespace age::graphics::shader
#endif