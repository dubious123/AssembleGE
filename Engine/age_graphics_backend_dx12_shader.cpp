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

		util::ensure_dir_exists(g::engine_shaders_dir_path);
		util::ensure_dir_exists(g::engine_shaders_compiled_blob_dir_path);

		for (const auto&& shader_name : std::views::iota(0ul)
											| std::views::take(e::size<e::engine_shader_kind>())
											| std::views::transform([](auto i) { return e::to_wstring(static_cast<e::engine_shader_kind>(i)); }))
		{
			c_auto hlsl_path = g::engine_shaders_dir_path / std::filesystem::path{ shader_name }.concat(".hlsl");
			AGE_ASSERT(std::filesystem::exists(hlsl_path));

			c_auto compiled_blob_path = g::engine_shaders_compiled_blob_dir_path / std::filesystem::path{ shader_name }.concat(".bin");

			AGE_ASSERT(shader_name.find_last_of('_') != std::wstring_view::npos);

			c_auto stage	   = std::wstring{ shader_name.substr(shader_name.find_last_of('_') + 1) };
			c_auto target	   = stage + L"_6_8";
			c_auto entry_point = L"main_" + stage;

			if (c_auto need_recompile =
					std::filesystem::exists(compiled_blob_path) is_false
					or std::filesystem::last_write_time(hlsl_path) > std::filesystem::last_write_time(compiled_blob_path)
					or std::filesystem::file_size(compiled_blob_path) == 0)
			{
				compile_shader(hlsl_path.c_str(), entry_point, target, compiled_blob_path);
			}

			load_shader(compiled_blob_path);
		}
	}

	void
	deinit() noexcept
	{
		for (auto idx : std::views::iota(0ul) | std::views::take(g::shader_blob_vec.count()))
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
		std::wstring_view	  hlsl_path,
		std::wstring_view	  entry_point,
		std::wstring_view	  target,
		std::filesystem::path save_path) noexcept
	{
		auto* p_file	 = (IDxcBlobEncoding*)nullptr;
		auto* p_result	 = (IDxcResult*)nullptr;
		auto* p_res_blob = (IDxcBlob*)nullptr;

		auto dir_path = std::filesystem::path{ hlsl_path }.root_directory();

		AGE_HR_CHECK(g::p_dxc_utils->LoadFile(hlsl_path.data(), nullptr, &p_file));

		{
			auto buffer = DxcBuffer{
				.Ptr	  = p_file->GetBufferPointer(),
				.Size	  = p_file->GetBufferSize(),
				.Encoding = DXC_CP_UTF8
			};

			auto args = std::array{
				hlsl_path.data(),
				L"-E", entry_point.data(),
				L"-T", target.data(),
				L"-I", dir_path.c_str(),
				L"-Qstrip_reflect",
				L"-Qstrip_debug",
				L"-enable-16bit-types",
				DXC_ARG_WARNINGS_ARE_ERRORS,
	#if defined(AGE_DEBUG)
				DXC_ARG_DEBUG,
				DXC_ARG_SKIP_OPTIMIZATIONS
	#elif defined(AGE_RELEASE)
				DXC_ARG_ALL_RESOURCES_BOUND,
				DXC_ARG_OPTIMIZATION_LEVEL3,
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
			auto* p_error = (IDxcBlobUtf8*)nullptr;

	#pragma warning(disable : 6387)
			p_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&p_error), nullptr);
	#pragma warning(default : 6387)

			if (p_error is_not_nullptr)
			{
				std::println("{}", p_error->GetStringPointer());

				p_error->Release();
			}

			auto status = (HRESULT)S_OK;
			p_result->GetStatus(&status);
			AGE_HR_CHECK(status);
		}

		AGE_HR_CHECK(p_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&p_res_blob), nullptr));

		{
			p_file->Release();
			p_result->Release();
		}

		auto blob_file = std::ofstream{ save_path, std::ios::out | std::ios::binary };
		{
			blob_file.clear();

			auto& stream = blob_file.write(static_cast<const char*>(p_res_blob->GetBufferPointer()), p_res_blob->GetBufferSize());

			AGE_ASSERT(stream.good());

			blob_file.close();
		}

		p_res_blob->Release();
	}

	shader_handle
	load_shader(std::filesystem::path shader_path) noexcept
	{
		AGE_ASSERT(std::filesystem::exists(shader_path));
		AGE_ASSERT(std::filesystem::file_size(shader_path) > 0);

		auto size	= std::filesystem::file_size(shader_path);
		auto p_blob = ::operator new(size, std::align_val_t{ alignof(char) });

		std::ifstream file{ shader_path, std::ios::in | std::ios::binary };

		file.read((char*)p_blob, size);

		file.close();

		return { .id = g::shader_blob_vec.emplace_back(shader_blob{ .p_blob = p_blob, .size = size }) };
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

	void
	unload_shader(shader_handle sh) noexcept
	{
		auto& blob = g::shader_blob_vec[sh.id];
		::operator delete((void*)blob.p_blob, std::align_val_t{ alignof(char) });
		g::shader_blob_vec.remove(sh.id);
	}
}	 // namespace age::graphics::shader
#endif