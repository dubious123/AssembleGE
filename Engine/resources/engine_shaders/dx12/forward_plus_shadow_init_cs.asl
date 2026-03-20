#include "forward_plus_common.asli"

[numthreads(1, 1, 1)] void
main_cs()

{
	store_z_min_max(0xffffffff, 0);
}