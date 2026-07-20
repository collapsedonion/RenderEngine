
StructuredBuffer<float4x4> matrix;
Sampler2D<float4> sampled_image;

struct ShaderInput{
    [[vk::location(0)]] float3 vertex_pos;
    [[vk::location(1)]] float2 uv;
}

struct ShaderOutput{
    float4 position : SV_Position;
    float2 uv: TEXCOORD;
}

struct FRAGInput{
    float2 uv: TEXCOORD;
}

struct FragOut{
    [[vk::location(0)]] float4 tex1;
}

[shader("vertex")]
ShaderOutput vertex_shader(ShaderInput si){
    ShaderOutput so = {};

    float3 coord = float3(si.vertex_pos.x, -si.vertex_pos.y, si.vertex_pos.z);
    so.position = mul(matrix[0], float4(coord, 1.0f));
    so.uv = float2(si.uv.x, abs(si.uv.y - 1.0f));

    return so;
}

[shader("fragment")]
FragOut fragment_shader(FRAGInput fi){
    FragOut out = {};

    float4 color = sampled_image.Sample(fi.uv);
    out.tex1 = float4(pow(color.rgb, float3(1.0 / 2.2)), color.a);

    return out;
}
