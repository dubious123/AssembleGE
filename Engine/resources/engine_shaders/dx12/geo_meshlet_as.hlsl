struct t_payload
{
    uint meshlet_alive_mask = 0;
};

groupshared
t_payload shared_payload;

//01001101 find nth set bit ?

[numthreads(32, 1, 1)]
void main_as(
    uint3 group_id : SV_GroupID,
    uint3 group_thread_id : SV_GroupThreadID)
{
    uint meshlet_offset = group_id.x * 32 + group_thread_id.x;
    
    t_payload p;
    
    bool visible = true; // visible = cull(...);
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        shared_payload.meshlet_alive_mask |= 1 << index;
    }
    
    uint visible_count = WaveActiveCountBits(visible);
    
    DispatchMesh(visible_count, 1, 1, shared_payload);
}