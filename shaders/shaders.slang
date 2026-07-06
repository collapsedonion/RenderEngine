
StructuredBuffer<float4x4> matrix;
RWTexture2D<float4> image;
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

    out.tex1 = sampled_image.Sample(fi.uv);

    return out;
}


[shader("compute")]
[numthreads(16,16,1)]
void texture_test(uint3 id: SV_DispatchThreadID){
    uint width;
    uint height;

    image.GetDimensions(width, height);

    if(id.x >= width || id.y >= height){
        return;
    }

    float2 samplexy = float2(float2((float)id.x / (float)width, (float)id.y / (float)height));

    image.Store(id.xy, sampled_image.Sample(samplexy));
}

