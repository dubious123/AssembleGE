#pragma once
#define CALL(f) f();

#define __EXPAND(x) x

#pragma region for each arg
#define __FOR_EACH_ARG_0(what, x, ...)
#define __FOR_EACH_ARG_1(what, x, ...) what(x)
#define __FOR_EACH_ARG_2(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_1(what, __VA_ARGS__))
#define __FOR_EACH_ARG_3(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_2(what, __VA_ARGS__))
#define __FOR_EACH_ARG_4(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_3(what, __VA_ARGS__))
#define __FOR_EACH_ARG_5(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_4(what, __VA_ARGS__))
#define __FOR_EACH_ARG_6(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_5(what, __VA_ARGS__))
#define __FOR_EACH_ARG_7(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_6(what, __VA_ARGS__))
#define __FOR_EACH_ARG_8(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_7(what, __VA_ARGS__))
#define __FOR_EACH_ARG_9(what, x, ...) \
	what(x),                           \
		__EXPAND(__FOR_EACH_ARG_8(what, __VA_ARGS__))
#define __FOR_EACH_ARG_10(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_9(what, __VA_ARGS__))
#define __FOR_EACH_ARG_11(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_10(what, __VA_ARGS__))
#define __FOR_EACH_ARG_12(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_11(what, __VA_ARGS__))
#define __FOR_EACH_ARG_13(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_12(what, __VA_ARGS__))
#define __FOR_EACH_ARG_14(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_13(what, __VA_ARGS__))
#define __FOR_EACH_ARG_15(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_14(what, __VA_ARGS__))
#define __FOR_EACH_ARG_16(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_15(what, __VA_ARGS__))
#define __FOR_EACH_ARG_17(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_16(what, __VA_ARGS__))
#define __FOR_EACH_ARG_18(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_17(what, __VA_ARGS__))
#define __FOR_EACH_ARG_19(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_18(what, __VA_ARGS__))
#define __FOR_EACH_ARG_20(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_19(what, __VA_ARGS__))
#define __FOR_EACH_ARG_21(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_20(what, __VA_ARGS__))
#define __FOR_EACH_ARG_22(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_21(what, __VA_ARGS__))
#define __FOR_EACH_ARG_23(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_22(what, __VA_ARGS__))
#define __FOR_EACH_ARG_24(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_23(what, __VA_ARGS__))
#define __FOR_EACH_ARG_25(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_24(what, __VA_ARGS__))
#define __FOR_EACH_ARG_26(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_25(what, __VA_ARGS__))
#define __FOR_EACH_ARG_27(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_26(what, __VA_ARGS__))
#define __FOR_EACH_ARG_28(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_27(what, __VA_ARGS__))
#define __FOR_EACH_ARG_29(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_28(what, __VA_ARGS__))
#define __FOR_EACH_ARG_30(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_29(what, __VA_ARGS__))
#define __FOR_EACH_ARG_31(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_30(what, __VA_ARGS__))
#define __FOR_EACH_ARG_32(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_31(what, __VA_ARGS__))
#define __FOR_EACH_ARG_33(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_32(what, __VA_ARGS__))
#define __FOR_EACH_ARG_34(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_33(what, __VA_ARGS__))
#define __FOR_EACH_ARG_35(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_34(what, __VA_ARGS__))
#define __FOR_EACH_ARG_36(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_35(what, __VA_ARGS__))
#define __FOR_EACH_ARG_37(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_36(what, __VA_ARGS__))
#define __FOR_EACH_ARG_38(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_37(what, __VA_ARGS__))
#define __FOR_EACH_ARG_39(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_38(what, __VA_ARGS__))
#define __FOR_EACH_ARG_40(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_39(what, __VA_ARGS__))

#define __FOR_EACH_ARG_41(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_40(what, __VA_ARGS__))
#define __FOR_EACH_ARG_42(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_41(what, __VA_ARGS__))
#define __FOR_EACH_ARG_43(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_42(what, __VA_ARGS__))
#define __FOR_EACH_ARG_44(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_43(what, __VA_ARGS__))
#define __FOR_EACH_ARG_45(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_44(what, __VA_ARGS__))
#define __FOR_EACH_ARG_46(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_45(what, __VA_ARGS__))
#define __FOR_EACH_ARG_47(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_46(what, __VA_ARGS__))
#define __FOR_EACH_ARG_48(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_47(what, __VA_ARGS__))
#define __FOR_EACH_ARG_49(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_48(what, __VA_ARGS__))
#define __FOR_EACH_ARG_50(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_49(what, __VA_ARGS__))

#define __FOR_EACH_ARG_51(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_50(what, __VA_ARGS__))
#define __FOR_EACH_ARG_52(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_51(what, __VA_ARGS__))
#define __FOR_EACH_ARG_53(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_52(what, __VA_ARGS__))
#define __FOR_EACH_ARG_54(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_53(what, __VA_ARGS__))
#define __FOR_EACH_ARG_55(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_54(what, __VA_ARGS__))
#define __FOR_EACH_ARG_56(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_55(what, __VA_ARGS__))
#define __FOR_EACH_ARG_57(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_56(what, __VA_ARGS__))
#define __FOR_EACH_ARG_58(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_57(what, __VA_ARGS__))
#define __FOR_EACH_ARG_59(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_58(what, __VA_ARGS__))
#define __FOR_EACH_ARG_60(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_59(what, __VA_ARGS__))

#define __FOR_EACH_ARG_61(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_60(what, __VA_ARGS__))
#define __FOR_EACH_ARG_62(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_61(what, __VA_ARGS__))
#define __FOR_EACH_ARG_63(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_62(what, __VA_ARGS__))
#define __FOR_EACH_ARG_64(what, x, ...) \
	what(x),                            \
		__EXPAND(__FOR_EACH_ARG_63(what, __VA_ARGS__))
#pragma endregion

#pragma region for each
#define __FOR_EACH_0(what, x, ...) ;

#define __FOR_EACH_1(what, x, ...) what(x)
#define __FOR_EACH_2(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_1(what, __VA_ARGS__))
#define __FOR_EACH_3(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_2(what, __VA_ARGS__))
#define __FOR_EACH_4(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_3(what, __VA_ARGS__))
#define __FOR_EACH_5(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_4(what, __VA_ARGS__))
#define __FOR_EACH_6(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_5(what, __VA_ARGS__))
#define __FOR_EACH_7(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_6(what, __VA_ARGS__))
#define __FOR_EACH_8(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_7(what, __VA_ARGS__))
#define __FOR_EACH_9(what, x, ...) \
	what(x)                        \
		__EXPAND(__FOR_EACH_8(what, __VA_ARGS__))
#define __FOR_EACH_10(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_9(what, __VA_ARGS__))
#define __FOR_EACH_11(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_10(what, __VA_ARGS__))
#define __FOR_EACH_12(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_11(what, __VA_ARGS__))
#define __FOR_EACH_13(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_12(what, __VA_ARGS__))
#define __FOR_EACH_14(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_13(what, __VA_ARGS__))
#define __FOR_EACH_15(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_14(what, __VA_ARGS__))
#define __FOR_EACH_16(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_15(what, __VA_ARGS__))
#define __FOR_EACH_17(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_16(what, __VA_ARGS__))
#define __FOR_EACH_18(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_17(what, __VA_ARGS__))
#define __FOR_EACH_19(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_18(what, __VA_ARGS__))
#define __FOR_EACH_20(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_19(what, __VA_ARGS__))
#define __FOR_EACH_21(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_20(what, __VA_ARGS__))
#define __FOR_EACH_22(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_21(what, __VA_ARGS__))
#define __FOR_EACH_23(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_22(what, __VA_ARGS__))
#define __FOR_EACH_24(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_23(what, __VA_ARGS__))
#define __FOR_EACH_25(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_24(what, __VA_ARGS__))
#define __FOR_EACH_26(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_25(what, __VA_ARGS__))
#define __FOR_EACH_27(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_26(what, __VA_ARGS__))
#define __FOR_EACH_28(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_27(what, __VA_ARGS__))
#define __FOR_EACH_29(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_28(what, __VA_ARGS__))
#define __FOR_EACH_30(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_29(what, __VA_ARGS__))
#define __FOR_EACH_31(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_30(what, __VA_ARGS__))
#define __FOR_EACH_32(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_31(what, __VA_ARGS__))
#define __FOR_EACH_33(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_32(what, __VA_ARGS__))
#define __FOR_EACH_34(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_33(what, __VA_ARGS__))
#define __FOR_EACH_35(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_34(what, __VA_ARGS__))
#define __FOR_EACH_36(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_35(what, __VA_ARGS__))
#define __FOR_EACH_37(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_36(what, __VA_ARGS__))
#define __FOR_EACH_38(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_37(what, __VA_ARGS__))
#define __FOR_EACH_39(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_38(what, __VA_ARGS__))
#define __FOR_EACH_40(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_39(what, __VA_ARGS__))

#define __FOR_EACH_41(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_40(what, __VA_ARGS__))
#define __FOR_EACH_42(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_41(what, __VA_ARGS__))
#define __FOR_EACH_43(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_42(what, __VA_ARGS__))
#define __FOR_EACH_44(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_43(what, __VA_ARGS__))
#define __FOR_EACH_45(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_44(what, __VA_ARGS__))
#define __FOR_EACH_46(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_45(what, __VA_ARGS__))
#define __FOR_EACH_47(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_46(what, __VA_ARGS__))
#define __FOR_EACH_48(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_47(what, __VA_ARGS__))
#define __FOR_EACH_49(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_48(what, __VA_ARGS__))
#define __FOR_EACH_50(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_49(what, __VA_ARGS__))

#define __FOR_EACH_51(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_50(what, __VA_ARGS__))
#define __FOR_EACH_52(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_51(what, __VA_ARGS__))
#define __FOR_EACH_53(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_52(what, __VA_ARGS__))
#define __FOR_EACH_54(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_53(what, __VA_ARGS__))
#define __FOR_EACH_55(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_54(what, __VA_ARGS__))
#define __FOR_EACH_56(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_55(what, __VA_ARGS__))
#define __FOR_EACH_57(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_56(what, __VA_ARGS__))
#define __FOR_EACH_58(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_57(what, __VA_ARGS__))
#define __FOR_EACH_59(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_58(what, __VA_ARGS__))
#define __FOR_EACH_60(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_59(what, __VA_ARGS__))

#define __FOR_EACH_61(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_60(what, __VA_ARGS__))
#define __FOR_EACH_62(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_61(what, __VA_ARGS__))
#define __FOR_EACH_63(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_62(what, __VA_ARGS__))
#define __FOR_EACH_64(what, x, ...) \
	what(x)                         \
		__EXPAND(__FOR_EACH_ARG_63(what, __VA_ARGS__))
#pragma endregion


#define __FOR_EACH_NARG(...)  __FOR_EACH_NARG_(__VA_ARGS__, __FOR_EACH_RSEQ_N())
#define __FOR_EACH_NARG_(...) __EXPAND(__FOR_EACH_ARG_N(__VA_ARGS__))
#define __FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,          \
						 _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
						 _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
						 _61, _62, _63, _64, N, ...) N
#define __FOR_EACH_RSEQ_N()                                                             \
	64, 63, 62, 61, 60,                                                                 \
		59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
		39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
		19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define __CONCATENATE(x, y)			 x##y
#define __FOR_EACH_(N, what, ...)	 __EXPAND(__CONCATENATE(__FOR_EACH_, N)(what __VA_OPT__(, ) __VA_ARGS__))
#define FOR_EACH(what, ...)			 __FOR_EACH_(__FOR_EACH_NARG(__VA_ARGS__), what __VA_OPT__(, ) __VA_ARGS__)
#define __FOR_EACH_ARG(N, what, ...) __EXPAND(__CONCATENATE(__FOR_EACH_ARG_, N)(what __VA_OPT__(, ) __VA_ARGS__))
#define FOR_EACH_ARG(what, ...)		 __FOR_EACH_ARG(__FOR_EACH_NARG(__VA_ARGS__), what __VA_OPT__(, ) __VA_ARGS__)

#define ARGS_COUNT(...) __FOR_EACH_ARG_N(__VA_ARGS__, 64, 63, 62, 61, 60,                                                \
										 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
										 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
										 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
// �ε��� ����Ʈ (X-Macro)
// clang-format off
#define __X_REPEAT_LIST_512											\
    X(  0)  X(  1)  X(  2)  X(  3)  X(  4)  X(  5)  X(  6)  X(  7)  \
    X(  8)  X(  9)  X( 10)  X( 11)  X( 12)  X( 13)  X( 14)  X( 15)  \
    X( 16)  X( 17)  X( 18)  X( 19)  X( 20)  X( 21)  X( 22)  X( 23)  \
    X( 24)  X( 25)  X( 26)  X( 27)  X( 28)  X( 29)  X( 30)  X( 31)  \
    X( 32)  X( 33)  X( 34)  X( 35)  X( 36)  X( 37)  X( 38)  X( 39)  \
    X( 40)  X( 41)  X( 42)  X( 43)  X( 44)  X( 45)  X( 46)  X( 47)  \
    X( 48)  X( 49)  X( 50)  X( 51)  X( 52)  X( 53)  X( 54)  X( 55)  \
    X( 56)  X( 57)  X( 58)  X( 59)  X( 60)  X( 61)  X( 62)  X( 63)  \
    X( 64)  X( 65)  X( 66)  X( 67)  X( 68)  X( 69)  X( 70)  X( 71)  \
    X( 72)  X( 73)  X( 74)  X( 75)  X( 76)  X( 77)  X( 78)  X( 79)  \
    X( 80)  X( 81)  X( 82)  X( 83)  X( 84)  X( 85)  X( 86)  X( 87)  \
    X( 88)  X( 89)  X( 90)  X( 91)  X( 92)  X( 93)  X( 94)  X( 95)  \
    X( 96)  X( 97)  X( 98)  X( 99)  X(100)  X(101)  X(102)  X(103)  \
    X(104)  X(105)  X(106)  X(107)  X(108)  X(109)  X(110)  X(111)  \
    X(112)  X(113)  X(114)  X(115)  X(116)  X(117)  X(118)  X(119)  \
    X(120)  X(121)  X(122)  X(123)  X(124)  X(125)  X(126)  X(127)  \
    X(128)  X(129)  X(130)  X(131)  X(132)  X(133)  X(134)  X(135)  \
    X(136)  X(137)  X(138)  X(139)  X(140)  X(141)  X(142)  X(143)  \
    X(144)  X(145)  X(146)  X(147)  X(148)  X(149)  X(150)  X(151)  \
    X(152)  X(153)  X(154)  X(155)  X(156)  X(157)  X(158)  X(159)  \
    X(160)  X(161)  X(162)  X(163)  X(164)  X(165)  X(166)  X(167)  \
    X(168)  X(169)  X(170)  X(171)  X(172)  X(173)  X(174)  X(175)  \
    X(176)  X(177)  X(178)  X(179)  X(180)  X(181)  X(182)  X(183)  \
    X(184)  X(185)  X(186)  X(187)  X(188)  X(189)  X(190)  X(191)  \
    X(192)  X(193)  X(194)  X(195)  X(196)  X(197)  X(198)  X(199)  \
    X(200)  X(201)  X(202)  X(203)  X(204)  X(205)  X(206)  X(207)  \
    X(208)  X(209)  X(210)  X(211)  X(212)  X(213)  X(214)  X(215)  \
    X(216)  X(217)  X(218)  X(219)  X(220)  X(221)  X(222)  X(223)  \
    X(224)  X(225)  X(226)  X(227)  X(228)  X(229)  X(230)  X(231)  \
    X(232)  X(233)  X(234)  X(235)  X(236)  X(237)  X(238)  X(239)  \
    X(240)  X(241)  X(242)  X(243)  X(244)  X(245)  X(246)  X(247)  \
    X(248)  X(249)  X(250)  X(251)  X(252)  X(253)  X(254)  X(255)  \
    X(256)  X(257)  X(258)  X(259)  X(260)  X(261)  X(262)  X(263)  \
    X(264)  X(265)  X(266)  X(267)  X(268)  X(269)  X(270)  X(271)  \
    X(272)  X(273)  X(274)  X(275)  X(276)  X(277)  X(278)  X(279)  \
    X(280)  X(281)  X(282)  X(283)  X(284)  X(285)  X(286)  X(287)  \
    X(288)  X(289)  X(290)  X(291)  X(292)  X(293)  X(294)  X(295)  \
    X(296)  X(297)  X(298)  X(299)  X(300)  X(301)  X(302)  X(303)  \
    X(304)  X(305)  X(306)  X(307)  X(308)  X(309)  X(310)  X(311)  \
    X(312)  X(313)  X(314)  X(315)  X(316)  X(317)  X(318)  X(319)  \
    X(320)  X(321)  X(322)  X(323)  X(324)  X(325)  X(326)  X(327)  \
    X(328)  X(329)  X(330)  X(331)  X(332)  X(333)  X(334)  X(335)  \
    X(336)  X(337)  X(338)  X(339)  X(340)  X(341)  X(342)  X(343)  \
    X(344)  X(345)  X(346)  X(347)  X(348)  X(349)  X(350)  X(351)  \
    X(352)  X(353)  X(354)  X(355)  X(356)  X(357)  X(358)  X(359)  \
    X(360)  X(361)  X(362)  X(363)  X(364)  X(365)  X(366)  X(367)  \
    X(368)  X(369)  X(370)  X(371)  X(372)  X(373)  X(374)  X(375)  \
    X(376)  X(377)  X(378)  X(379)  X(380)  X(381)  X(382)  X(383)  \
    X(384)  X(385)  X(386)  X(387)  X(388)  X(389)  X(390)  X(391)  \
    X(392)  X(393)  X(394)  X(395)  X(396)  X(397)  X(398)  X(399)  \
    X(400)  X(401)  X(402)  X(403)  X(404)  X(405)  X(406)  X(407)  \
    X(408)  X(409)  X(410)  X(411)  X(412)  X(413)  X(414)  X(415)  \
    X(416)  X(417)  X(418)  X(419)  X(420)  X(421)  X(422)  X(423)  \
    X(424)  X(425)  X(426)  X(427)  X(428)  X(429)  X(430)  X(431)  \
    X(432)  X(433)  X(434)  X(435)  X(436)  X(437)  X(438)  X(439)  \
    X(440)  X(441)  X(442)  X(443)  X(444)  X(445)  X(446)  X(447)  \
    X(448)  X(449)  X(450)  X(451)  X(452)  X(453)  X(454)  X(455)  \
    X(456)  X(457)  X(458)  X(459)  X(460)  X(461)  X(462)  X(463)  \
    X(464)  X(465)  X(466)  X(467)  X(468)  X(469)  X(470)  X(471)  \
    X(472)  X(473)  X(474)  X(475)  X(476)  X(477)  X(478)  X(479)  \
    X(480)  X(481)  X(482)  X(483)  X(484)  X(485)  X(486)  X(487)  \
    X(488)  X(489)  X(490)  X(491)  X(492)  X(493)  X(494)  X(495)  \
    X(496)  X(497)  X(498)  X(499)  X(500)  X(501)  X(502)  X(503)  \
    X(504)  X(505)  X(506)  X(507)  X(508)  X(509)  X(510)  X(511)
// clang-format on