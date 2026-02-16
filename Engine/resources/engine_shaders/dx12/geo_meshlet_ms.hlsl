// precondition:
//  - mask != 0
//  - n < countbits(mask)   (n은 0-based: 0번째 set bit, 1번째 set bit...)
uint select32_nth_set_bit(uint mask, uint n)
{
    uint b0 = (mask) & 0xFFu;
    uint b1 = (mask >> 8) & 0xFFu;
    uint b2 = (mask >> 16) & 0xFFu;
    uint b3 = (mask >> 24) & 0xFFu;

    uint c0 = countbits(b0);
    uint c1 = countbits(b1);
    uint c2 = countbits(b2);
    uint c3 = countbits(b3);

    // uint p0 = 0;
    uint p1 = c0;
    uint p2 = c0 + c1;
    uint p3 = c0 + c1 + c2;

    uint byte = 0 + (n >= p1) + (n >= p2) + (n >= p3);

    uint prefix =
        (byte == 0) ? 0u :
        (byte == 1) ? p1 :
        (byte == 2) ? p2 :
                      p3;

    uint local_n = n - prefix;

    // byte_mask cannot be 0 because n < countbits(mask)
    uint byte_mask = (mask >> (byte * 8u)) & 0xFFu;
    
    uint pos_in_byte = firstbitlow(byte_mask);
    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        if (i < local_n)
        {
            byte_mask &= ~(1u << pos_in_byte);
            pos_in_byte = firstbitlow(byte_mask);
        }
    }

    return byte * 8u + pos_in_byte;
}


struct t_payload
{
    uint meshlet_alive_mask = 0;
};

[numthreads(32, 1, 1)]
[outputtopology("triangle")]
void main_ms(
    in payload t_payload shared_payload,
    uint3 group_id : SV_GroupID,
    uint3 group_thread_id : SV_GroupThreadID,
out vertices t_vertex_out vertex_arr[64],
out indices uint3 triangle_arr[126])
{
    uint8 mashlet_id = select32_nth_set_bit(shared_payload.meshlet_alive_mask, group_thread_id.x);
}