#include "forward_plus_common.asli"

uint32
main_ps(selection_outline_ms_to_ps fragment) sv_target_0
{
	return fragment.selection_outline_id_and_extra & 0xff;	  // &0xff;
}