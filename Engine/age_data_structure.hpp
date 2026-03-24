#pragma once

#include "age_data_structure_array.hpp"

#ifdef USE_STL_VECTOR
namespace age::inline data_structure
{
	template <typename T>
	using vector = std::vector<T>;
}
#else
	#include "age_data_structure_vector.hpp"
#endif

#include "age_data_structure_stable_dense_vector.hpp"
#include "age_data_structure_sparse_vector.hpp"

#ifdef USE_STL_SET
namespace age::inline data_structure
{
	template <typename T>
	using set = std::set<T>;
}	 // namespace age::inline data_structure
#endif

#ifdef USE_STL_MAP
namespace age::inline data_structure
{
	template <typename T, typename K>
	using map = std::map<T, K>;
}	 // namespace age::inline data_structure
#endif

#ifdef USE_STL_LIST
namespace age::inline data_structure
{
	template <typename T>
	using list = std::list<T>;
}
#else
namespace age::inline data_structure
{
	template <typename T>
	class list {
		struct node
		{
			T	  value;
			node* next = nullptr;
			node* prev = nullptr;

			// node(T&& t)
			//{
			//	value =
			// }
		};

	  private:
		node*  _head;
		node*  _tail;
		size_t _size;

	  public:
		constexpr list()
		{
			_size = 0;
			_head = nullptr;
			_tail = nullptr;
		}

		constexpr ~list()
		{
			auto p_now = _head;
			for (auto i = 0; i < _size; ++i)
			{
				auto p_next = p_now->next;
				if constexpr (std::is_trivially_destructible_v<T> == false)
				{
					(p_now->value).~T();
				}

				free(p_now);
				p_now = p_next;
			}
		}

		constexpr bool
		empty()
		{
			return _size == 0;
		}

		constexpr node*
		front()
		{
			// assert(empty() == false);
			return _head;
		}

		constexpr node*
		back()
		{
			// assert(empty() == false);
			return _tail;
		}

		constexpr size_t
		size()
		{
			return _size;
		}

		template <typename... V>
		T&
		emplace_back(V&&... args)
		{
			if (empty()) [[unlikely]]
			{
				_head = _tail = (node*)malloc(sizeof(node));
				_head->next	  = nullptr;
				_head->prev	  = nullptr;
				new (&_head->value) T{ std::forward<V>(args)... };
			}
			else
			{
				_tail->next		  = (node*)malloc(sizeof(node));
				_tail->next->next = nullptr;
				new (&_tail->next->value) T{ std::forward<V>(args)... };
				_tail->next->prev = _tail;
				_tail			  = _tail->next;
			}

			++_size;
			return _tail->value;
		}

		template <typename... V>
		T&
		emplace_front(V&&... args)
		{
			if (empty()) [[unlikely]]
			{
				_head = _tail = (node*)malloc(sizeof(node));
				_head->next	  = nullptr;
				_head->prev	  = nullptr;
				new (&_head->value) T{ std::forward<V>(args)... };
			}
			else
			{
				_head->prev		  = (node*)malloc(sizeof(node));
				_head->prev->prev = nullptr;
				new (&_head->prev->value) T{ std::forward<V>(args)... };
				_head->prev->next = _head;
				_head			  = _head->prev;
			}

			++_size;
			return _head->value;
		}

		void
		pop_front()
		{
			assert(empty() == false);
			auto* temp = _head->next;

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				(_head->value).~T();
			}

			free(_head);
			_head = temp;
			--_size;
			//_head->prev = nullptr;
		}

		void
		pop_back()
		{
			assert(empty() == false);
			auto* temp = _tail->prev;
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				(_tail->value).~T();
			}
			free(_tail);
			_tail = temp;
			--_size;
			//_tail->next = nullptr;
		}

		template <typename... V>
		node*
		insert(node* at, V&&... args)
		{
			auto* prev = at->prev;
			at->prev   = (node*)malloc(sizeof(node));
			new (&at->prev->value) T{ std::forward<V>(args)... };

			(at->prev)->prev = prev;
			(at->prev)->next = prev->next;

			prev->next = at->prev;
			++_size;

			return at->prev;
		}

		void
		erase(node* target)
		{
			assert(empty() == false and target != nullptr);

			if (target == _tail) [[unlikely]]
			{
				_tail = target->prev;
			}
			if (target == _head) [[unlikely]]
			{
				_head = target->next;
			}

			auto* prev = target->prev;
			auto* next = target->next;
			prev->next = next;
			next->prev = prev;

			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				(target->value).~T();
			}

			free(target);
			--_size;
		}
	};
}	 // namespace age::inline data_structure
#endif

#ifdef USE_STL_UNORDERED_MAP
namespace age::inline data_structure
{
	template <typename t_key,
			  typename t_value,
			  typename t_hash	   = std::hash<t_key>,
			  typename t_key_equal = std::equal_to<t_key>,
			  typename t_allocator = std::allocator<std::pair<const t_key, t_value>>>
	using unordered_map = std::unordered_map<t_key, t_value, t_hash, t_key_equal, t_allocator>;
}
#else
	#include "age_data_structure_unordered_map.hpp"
#endif
