
//2d_vertex_descriptor_set
struct VertexData2D{
    float2 position;
    float2 scale;
    float angle;
};

struct ImageResolution{
    uint width;
    uint height;
};

[[vk::binding(0, 0)]] ConstantBuffer<ImageResolution> imageData;
[[vk::binding(1, 0)]] ConstantBuffer<VertexData2D> data2DV;
[[vk::binding(0, 1)]] ConstantBuffer<float3> dataColor;

struct ShaderInput{
    [[vk::location(0)]] float2 position;
};

struct FragmentOutput{
    [[vk::location(0)]] float4 tex1;
};

struct VertexOutput{
    float4 position: SV_Position;
};

[shader("vertex")]
VertexOutput direct_vertex_shader_2D(
    ShaderInput input
){
    VertexOutput vo = {};

    float angle = data2DV.angle;
    float2 scale = data2DV.scale;
    float2 position = data2DV.position;

    float2 rotated = float2(
        input.position.x * cos(angle) + input.position.y * sin(angle),
        -input.position.x * sin(angle) + input.position.y * cos(angle)
    );

    float2 scaled = float2(
        rotated.x * scale.x,
        rotated.y * scale.y
    );

    float2 world = scaled + position;
    float2 ndc = float2(
        world.x / (float(imageData.width)  * 0.5f),
        world.y / (float(imageData.height) * 0.5f)
    );
    vo.position = float4(ndc, 1.0f, 1);

    return vo;
}

[shader("fragment")]
FragmentOutput colored_fragment_shader(){
    FragmentOutput res = {};

    res.tex1 = float4(dataColor, 1.0f);

    return res;
}