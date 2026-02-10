#include <span>
#include "earcut/earcut.hpp"

namespace age::external::earcut::detail
{
	unsigned int
	perform(
		void**				pp_boundary_arr,
		const unsigned int* p_vertex_count_arr,
		const unsigned int	boundary_count,
		unsigned int*&		p_idx_out) noexcept
	{
		auto polygon = std::vector<std::span<std::pair<float, float>>>{};
		polygon.resize(boundary_count);

		for (auto i = 0u, offset = 0u; i < boundary_count;)
		{
			polygon[i] = std::span{
				reinterpret_cast<std::pair<float, float>**>(pp_boundary_arr)[i] + offset,
				p_vertex_count_arr[i]
			};

			offset += p_vertex_count_arr[i];
			++i;
		}

		auto res_vec = mapbox::earcut(polygon);

		if (res_vec.empty())
		{
			p_idx_out = nullptr;
			return 0;
		}

		p_idx_out = static_cast<unsigned int*>(::operator new(sizeof(unsigned int) * res_vec.size()));
		for (auto i = 0; i < res_vec.size(); ++i)
		{
			p_idx_out[i] = res_vec[i];
		}

		return static_cast<unsigned int>(res_vec.size());
	}
}	 // namespace age::external::earcut::detail