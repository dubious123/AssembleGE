#include "age_pch.hpp"
#include "age.hpp"

namespace age::external::meshopt
{
	std::size_t
	generateVertexRemap(
		uint32*		  destination,
		const uint32* indices,
		std::size_t	  index_count,
		const void*	  vertices,
		std::size_t	  vertex_count,
		std::size_t	  vertex_size) noexcept
	{
		return 0;
	}

	void
	remapIndexBuffer(
		uint32*		  destination,
		const uint32* indices,
		std::size_t	  index_count,
		const uint32* remap) noexcept
	{
	}

	void
	remapVertexBuffer(
		void*		  destination,
		const void*	  vertices,
		std::size_t	  vertex_count,
		std::size_t	  vertex_size,
		const uint32* remap) noexcept
	{
	}
}	 // namespace age::external::meshopt