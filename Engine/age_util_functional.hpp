#pragma once

namespace age::util
{
	template <typename t_sig>
	class function_ref;

	template <typename t_ret, typename... t_args>
	class function_ref<t_ret(t_args...)> {
	  public:
		template <typename t_f>
		requires(meta::is_not_same_v<std::decay_t<t_f>, function_ref>)
		FORCE_INLINE constexpr function_ref(t_f&& f) noexcept
			: p_ctx{ const_cast<void*>(static_cast<const void*>(std::addressof(f))) },
			  p_func{ [] INLINE_LAMBDA_FRONT(void* ptr, t_args... arg) noexcept INLINE_LAMBDA_BACK -> t_ret { { return static_cast<std::remove_reference_t<t_f>*>(ptr)->operator()(FWD(arg)...); } } }
		{
		}

		FORCE_INLINE constexpr t_ret
		operator()(t_args... args) const noexcept
		{
			return p_func(p_ctx, FWD(args)...);
		}

	  private:
		void* p_ctx;
		t_ret (*p_func)(void*, t_args...);
	};
}	 // namespace age::util