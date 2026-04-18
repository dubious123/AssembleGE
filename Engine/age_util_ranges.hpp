#pragma once

namespace age::ranges
{
	constexpr auto
	erase_if(auto&& container, auto&& pred) noexcept
	{
		using t_container = BARE_OF(container);
		if constexpr (meta::is_specialization_of_v<t_container, age::vector>)
		{
			using size_type = typename t_container::size_type;
			c_auto old_size = container.size();

			auto write = size_type{ 0 };
			for (auto read = size_type{ 0 }; read < old_size; ++read)
			{
				if (pred(container[read])) { continue; }

				if (write != read)
				{
					container[write] = std::move(container[read]);
				}
				++write;
			}

			container.resize(write);

			return old_size - write;
		}
		else
		{
			static_assert(false, "not implemented yet");
		}
	}
}	 // namespace age::ranges