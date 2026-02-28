#define AGE_PP_EVAL(...)	 __VA_ARGS__
#define AGE_PP_EVAL1(...)	 AGE_PP_EVAL(AGE_PP_EVAL(AGE_PP_EVAL(__VA_ARGS__)))
#define AGE_PP_EVAL2(...)	 AGE_PP_EVAL1(AGE_PP_EVAL1(AGE_PP_EVAL1(__VA_ARGS__)))
#define AGE_PP_EVAL3(...)	 AGE_PP_EVAL2(AGE_PP_EVAL2(AGE_PP_EVAL2(__VA_ARGS__)))
#define AGE_PP_EVAL4(...)	 AGE_PP_EVAL3(AGE_PP_EVAL3(AGE_PP_EVAL3(__VA_ARGS__)))
#define AGE_PP_EVAL_MAX(...) AGE_PP_EVAL4(AGE_PP_EVAL4(AGE_PP_EVAL4(__VA_ARGS__)))

#define __DEFER_EMPTY__()

// clang-format off
#define AGE_PP_DEFER(macro_id) macro_id __DEFER_EMPTY__ () ()
// clang-format on

#define AGE_PP_SEQN_REVERSED                                                                                \
	511, 510, 509, 508, 507, 506, 505, 504, 503, 502, 501, 500,                                             \
		499, 498, 497, 496, 495, 494, 493, 492, 491, 490, 489, 488, 487, 486, 485, 484, 483, 482, 481, 480, \
		479, 478, 477, 476, 475, 474, 473, 472, 471, 470, 469, 468, 467, 466, 465, 464, 463, 462, 461, 460, \
		459, 458, 457, 456, 455, 454, 453, 452, 451, 450, 449, 448, 447, 446, 445, 444, 443, 442, 441, 440, \
		439, 438, 437, 436, 435, 434, 433, 432, 431, 430, 429, 428, 427, 426, 425, 424, 423, 422, 421, 420, \
		419, 418, 417, 416, 415, 414, 413, 412, 411, 410, 409, 408, 407, 406, 405, 404, 403, 402, 401, 400, \
		399, 398, 397, 396, 395, 394, 393, 392, 391, 390, 389, 388, 387, 386, 385, 384, 383, 382, 381, 380, \
		379, 378, 377, 376, 375, 374, 373, 372, 371, 370, 369, 368, 367, 366, 365, 364, 363, 362, 361, 360, \
		359, 358, 357, 356, 355, 354, 353, 352, 351, 350, 349, 348, 347, 346, 345, 344, 343, 342, 341, 340, \
		339, 338, 337, 336, 335, 334, 333, 332, 331, 330, 329, 328, 327, 326, 325, 324, 323, 322, 321, 320, \
		319, 318, 317, 316, 315, 314, 313, 312, 311, 310, 309, 308, 307, 306, 305, 304, 303, 302, 301, 300, \
		299, 298, 297, 296, 295, 294, 293, 292, 291, 290, 289, 288, 287, 286, 285, 284, 283, 282, 281, 280, \
		279, 278, 277, 276, 275, 274, 273, 272, 271, 270, 269, 268, 267, 266, 265, 264, 263, 262, 261, 260, \
		259, 258, 257, 256, 255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240, \
		239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, \
		219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, \
		199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, \
		179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160, \
		159, 158, 157, 156, 155, 154, 153, 152, 151, 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, \
		139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, \
		119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, \
		099, 098, 097, 096, 095, 094, 093, 092, 091, 090, 089, 088, 087, 086, 085, 084, 083, 082, 081, 080, \
		079, 078, 077, 076, 075, 074, 073, 072, 071, 070, 069, 068, 067, 066, 065, 064, 063, 062, 061, 060, \
		059, 058, 057, 056, 055, 054, 053, 052, 051, 050, 049, 048, 047, 046, 045, 044, 043, 042, 041, 040, \
		039, 038, 037, 036, 035, 034, 033, 032, 031, 030, 029, 028, 027, 026, 025, 024, 023, 022, 021, 020, \
		019, 018, 017, 016, 015, 014, 013, 012, 011, 010, 009, 008, 007, 006, 005, 004, 003, 002, 001, 000

#define __AGE_VA_COUNT_IMPL__(_511, _510, _509, _508, _507, _506, _505, _504, _503, _502, _501, _500,                                                 \
							  _499, _498, _497, _496, _495, _494, _493, _492, _491, _490, _489, _488, _487, _486, _485, _484, _483, _482, _481, _480, \
							  _479, _478, _477, _476, _475, _474, _473, _472, _471, _470, _469, _468, _467, _466, _465, _464, _463, _462, _461, _460, \
							  _459, _458, _457, _456, _455, _454, _453, _452, _451, _450, _449, _448, _447, _446, _445, _444, _443, _442, _441, _440, \
							  _439, _438, _437, _436, _435, _434, _433, _432, _431, _430, _429, _428, _427, _426, _425, _424, _423, _422, _421, _420, \
							  _419, _418, _417, _416, _415, _414, _413, _412, _411, _410, _409, _408, _407, _406, _405, _404, _403, _402, _401, _400, \
							  _399, _398, _397, _396, _395, _394, _393, _392, _391, _390, _389, _388, _387, _386, _385, _384, _383, _382, _381, _380, \
							  _379, _378, _377, _376, _375, _374, _373, _372, _371, _370, _369, _368, _367, _366, _365, _364, _363, _362, _361, _360, \
							  _359, _358, _357, _356, _355, _354, _353, _352, _351, _350, _349, _348, _347, _346, _345, _344, _343, _342, _341, _340, \
							  _339, _338, _337, _336, _335, _334, _333, _332, _331, _330, _329, _328, _327, _326, _325, _324, _323, _322, _321, _320, \
							  _319, _318, _317, _316, _315, _314, _313, _312, _311, _310, _309, _308, _307, _306, _305, _304, _303, _302, _301, _300, \
							  _299, _298, _297, _296, _295, _294, _293, _292, _291, _290, _289, _288, _287, _286, _285, _284, _283, _282, _281, _280, \
							  _279, _278, _277, _276, _275, _274, _273, _272, _271, _270, _269, _268, _267, _266, _265, _264, _263, _262, _261, _260, \
							  _259, _258, _257, _256, _255, _254, _253, _252, _251, _250, _249, _248, _247, _246, _245, _244, _243, _242, _241, _240, \
							  _239, _238, _237, _236, _235, _234, _233, _232, _231, _230, _229, _228, _227, _226, _225, _224, _223, _222, _221, _220, \
							  _219, _218, _217, _216, _215, _214, _213, _212, _211, _210, _209, _208, _207, _206, _205, _204, _203, _202, _201, _200, \
							  _199, _198, _197, _196, _195, _194, _193, _192, _191, _190, _189, _188, _187, _186, _185, _184, _183, _182, _181, _180, \
							  _179, _178, _177, _176, _175, _174, _173, _172, _171, _170, _169, _168, _167, _166, _165, _164, _163, _162, _161, _160, \
							  _159, _158, _157, _156, _155, _154, _153, _152, _151, _150, _149, _148, _147, _146, _145, _144, _143, _142, _141, _140, \
							  _139, _138, _137, _136, _135, _134, _133, _132, _131, _130, _129, _128, _127, _126, _125, _124, _123, _122, _121, _120, \
							  _119, _118, _117, _116, _115, _114, _113, _112, _111, _110, _109, _108, _107, _106, _105, _104, _103, _102, _101, _100, \
							  _099, _098, _097, _096, _095, _094, _093, _092, _091, _090, _089, _088, _087, _086, _085, _084, _083, _082, _081, _080, \
							  _079, _078, _077, _076, _075, _074, _073, _072, _071, _070, _069, _068, _067, _066, _065, _064, _063, _062, _061, _060, \
							  _059, _058, _057, _056, _055, _054, _053, _052, _051, _050, _049, _048, _047, _046, _045, _044, _043, _042, _041, _040, \
							  _039, _038, _037, _036, _035, _034, _033, _032, _031, _030, _029, _028, _027, _026, _025, _024, _023, _022, _021, _020, \
							  _019, _018, _017, _016, _015, _014, _013, _012, _011, _010, _009, _008, _007, _006, _005, _004, _003, _002, _001, N, ...) N

#define __CALL_IMPL_I__(what, ...) what(__VA_ARGS__)
#define AGE_PP_CALL_I(...)		   __CALL_IMPL_I__(__VA_ARGS__)

#define AGE_PP_VA_COUNT(...) AGE_PP_CALL_I(__AGE_VA_COUNT_IMPL__, __VA_ARGS__ __VA_OPT__(, ) AGE_PP_SEQN_REVERSED)

#define AGE_PP_CONCAT_R(x, y) x##y
#define AGE_PP_CONCAT_I(x, y) AGE_PP_CONCAT_R(x, y)

#define AGE_PP_COMMA_R	 ,
#define AGE_PP_COMMA_I() AGE_PP_COMMA_R

#define AGE_PP_SEMICOLON_R	 ;
#define AGE_PP_SEMICOLON_I() AGE_PP_SEMICOLON_R

#define AGE_PP_EMPTY_R
#define AGE_PP_EMPTY_I() AGE_PP_COMMA_R

#define AGE_PP_IDENTITY_I(...) __VA_ARGS__


#define AGE_PP_FIRST_I(a, ...)	   a
#define AGE_PP_SECOND_I(a, b, ...) b
#define AGE_PP_PROBE()			   ~, T
#define AGE_PP_IS_PROBE(...)	   AGE_PP_CALL_I(AGE_PP_SECOND_I, __VA_ARGS__, F)

#define AGE_PP_CONDITIONAL_I(T_or_F) AGE_PP_CONCAT_I(AGE_PP_CONDITIONAL_I_, T_or_F)
#define AGE_PP_CONDITIONAL_I_T(t, f) t
#define AGE_PP_CONDITIONAL_I_F(t, f) f

#define AGE_PP_IS_PAREN(...)	   AGE_PP_IS_PROBE(AGE_PP_IS_PAREN_PROBE AGE_PP_FIRST_I(__VA_ARGS__))
#define AGE_PP_IS_PAREN_PROBE(...) AGE_PP_PROBE()

// AGE_PP_IS_PAREN(A,B,C,...)
// -> AGE_PP_IS_PROBE(AGE_PP_IS_PAREN_PROBE AGE_PP_FIRST(A,B,C,...))
// -> AGE_PP_SECOND(AGE_PP_IS_PAREN_PROBE A, F)
// -> F
// AGE_PP_IS_PAREN((a, b, c, ...))
// -> AGE_PP_IS_PROBE(AGE_PP_IS_PAREN_PROBE AGE_PP_FIRST((a, b, c, ...)))
// -> AGE_PP_IS_PROBE(AGE_PP_PROBE())
// -> AGE_PP_IS_PROBE(~, T)
// -> T
//
#define AGE_PP_REMOVE_PARENS_R(...) __VA_ARGS__
#define AGE_PP_REMOVE_PARENS_I(...) AGE_PP_REMOVE_PARENS_R __VA_ARGS__

#define AGE_PP_STRIP_PARENS(...) \
	AGE_PP_CONDITIONAL_I(AGE_PP_IS_PAREN(__VA_ARGS__))(AGE_PP_REMOVE_PARENS_I, AGE_PP_IDENTITY_I)(__VA_ARGS__)

#define AGE_PP_CALL_UNPACKED_I(ctx, ...) AGE_PP_CALL_I(AGE_PP_STRIP_PARENS(ctx), __VA_ARGS__)


// #define AGE_TUPLE_UNPACK_RIGHT_I(tpl)			   AGE_PP_CALL_I(__AGE_PP_TUPLE_UNPACK_RIGHT_IMPL_I__, AGE_PP_REMOVE_PARENS_R tpl)
#define AGE_TUPLE_UNPACK_RIGHT_I(tpl)			   __AGE_PP_TUPLE_UNPACK_RIGHT_IMPL_I__ tpl
#define __AGE_PP_TUPLE_UNPACK_RIGHT_IMPL_I__(l, r) (l, AGE_PP_REMOVE_PARENS_I(r))


#define AGE_PP_CARTESIAN_PRODUCT(tpl_l, tpl_r)                     \
	FOR_EACH_SEP(AGE_TUPLE_UNPACK_RIGHT_I, AGE_PP_EMPTY_I,         \
				 FOR_EACH_SEP(AGE_PP_SWAP_TUPLE_I, AGE_PP_EMPTY_I, \
							  FOR_EACH_CTX(                        \
								  (AGE_PP_MAKE_TUPLE_I, tpl_r), AGE_PP_EMPTY_I, AGE_PP_REMOVE_PARENS_I(tpl_l))))


#define FOR_EACH_CTX_DEFER_ID()			   FOR_EACH_CTX2
#define AGE_PP_DISTRIBUTE(tpl)			   __AGE_PP_DISTRIBUTE_impl__ tpl
#define __AGE_PP_DISTRIBUTE_impl__(x, tpl) (AGE_PP_DEFER(FOR_EACH_CTX_DEFER_ID)((AGE_PP_MAKE_TUPLE_I, x), AGE_PP_COMMA_I, tpl))

#define AGE_PP_CARTESIAN_PRODUCT2(tpl_l, tpl_r)                      \
	FOR_EACH_CTX((AGE_PP_DISTRIBUTE), AGE_PP_EMPTY_I,                \
				 FOR_EACH_CTX((AGE_PP_SWAP_TUPLE_I), AGE_PP_EMPTY_I, \
							  FOR_EACH_CTX(                          \
								  (AGE_PP_MAKE_TUPLE_I, tpl_r), AGE_PP_EMPTY_I, AGE_PP_REMOVE_PARENS_I(tpl_l))))

// pp_tuple
#define AGE_PP_MAKE_TUPLE_I(l, r) (l, r)
#define AGE_PP_SWAP_TUPLE_I(tpl)  AGE_PP_SWAP_TUPLE_R tpl
#define AGE_PP_SWAP_TUPLE_R(l, r) (r, l)

#define AGE_PP_TUPLE_GET_0_R(a, b) a
#define AGE_PP_TUPLE_GET_0_I(tpl)  AGE_PP_TUPLE_GET_0_R tpl

#define AGE_PP_TUPLE_GET_1_R(a, b) b
#define AGE_PP_TUPLE_GET_1_I(tpl)  AGE_PP_TUPLE_GET_1_R tpl


#define AGE_PP_SEQ_END_()
#define AGE_PP_SEQ_TO_VA_ARGS(seq) AGE_PP_REMOVE_PARENS_I(AGE_PP_CONCAT_I(AGE_PP_SEQ_A seq, _END_))
#define AGE_PP_SEQ_A(x) (x AGE_PP_SEQ_B
#define AGE_PP_SEQ_B(x) , x AGE_PP_SEQ_A

#define AGE_PP_SEQ_A_END_ )
#define AGE_PP_SEQ_B_END_ )

#define __AGE_PP_STRINGIFY_IMPL__(x) #x

#define AGE_PP_STRINGIFY(x) __AGE_PP_STRINGIFY_IMPL__(x)