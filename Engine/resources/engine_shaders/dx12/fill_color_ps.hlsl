// Mandelbrot fractal constants
#define M_RE_START -2.8f
#define M_RE_END 1.f
#define M_IM_START -1.5f
#define M_IM_END 1.5f
#define M_MAX_ITERATION 1000

// Julia set constants
#define J_RE_START -2.f
#define J_RE_END 2.f
#define J_IM_START -1.5f
#define J_IM_END 1.5f
#define J_MAX_ITERATION 1000


float3 map_color(float t)
{
    float3 ambient = float3(0.009f, 0.012f, 0.016f);
    return float3(2.f * t, 4.f * t, 8.f * t) + ambient;
}

float2 complex_sq(float2 c)
{
    return float2(c.x * c.x - c.y * c.y, 2.f * c.x * c.y);
}

float3 draw_mandelbrot(float2 uv)
{
    const float2 c = float2(M_RE_START + uv.x * (M_RE_END - M_RE_START),
                            M_IM_START + uv.y * (M_IM_END - M_IM_START));
    float2 z = 0.f;
    for (int i = 0; i < M_MAX_ITERATION; i++)
    {
        z = complex_sq(z) + c;
        const float d = dot(z, z);
        if (d > 4.f)
        {
            const float t = i + 1 - log(log2(d));
            return map_color(t / M_MAX_ITERATION);
        }
    }

    return 1.f;
}

float3 draw_juliaset(float2 uv, uint frame)
{
    float2 z = float2(J_RE_START + uv.x * (J_RE_END - J_RE_START),
                      J_IM_START + uv.y * (J_IM_END - J_IM_START));
    const float f = frame * 0.0002f;
    const float2 w = float2(cos(f), sin(f));
    const float2 c = (2.f * w - complex_sq(w)) * 0.26f;

    for (int i = 0; i < J_MAX_ITERATION; i++)
    {
        z = complex_sq(z) + c;
        const float d = dot(z, z);
        if (d > 4.f)
        {
            const float t = i + 1 - log(log2(d));
            return map_color(t / J_MAX_ITERATION);
        }
    }

    return 1.f;
}

struct shader_constants
{
    float width;
    float height;
    uint  frame;
};

ConstantBuffer<shader_constants>         shader_constant_buffer                    : register(b1);

#define SAMPLES 4

float4 fill_color_ps(in noperspective float4 position : SV_Position,
               in noperspective float2 un_used : TEXCOORD) : SV_Target0
{
	const float offset = 0.2f;
    const float2 offsets[4] =
    {
        float2(-offset, offset),
        float2(offset, offset),
        float2(offset, -offset),
        float2(-offset, -offset)
    };

    const float2 inv_dim = float2(1.f / shader_constant_buffer.width, 1.f / shader_constant_buffer.height);
    float3 color = 0.f;
    for (int i = 0; i < SAMPLES; i++)
    {
        const float2 uv = (position.xy + offsets[i]) * inv_dim;
        //color += DrawMandelbrot(uv);
        color += draw_juliaset(uv, shader_constant_buffer.frame);
    }

    return float4(float3(color.z, color.x, 1.f) * color.x / SAMPLES, 1.f);
    //return float4(color / SAMPLES, 1.f);
}