enum ColoringType : uint32_t{
    UV_AS_COLOR = 0b0001,
    COLORED = 0b0010,
    TEXTURED = 0b0100,
}

struct MetaData{
    ColoringType coloring_type;
    float4 color;
    float specular_reflection;
    float defuse_reflection;
    float ambient_reflection;
    float shininess;
};

struct WorldTransform{
    float4x4 world_matrix;
    float4x4 normal_matrix;
}

struct CameraTransform{
    float4x4 projection;
    float4x4 position;
}

[[vk::binding(0, 1)]]
ConstantBuffer<WorldTransform> WORLD_MATRIX;
[[vk::binding(1, 1)]]
ConstantBuffer<MetaData> MTD;
[[vk::binding(2, 1)]]
Sampler2D OBJECT_TEXTURE;

[[vk::binding(0, 0)]]
ConstantBuffer<CameraTransform> CAMERA_PROJECTION_MATRIX;

struct VertexIn{
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 uv_coord;
    [[vk::location(2)]] float3 normal;
}

struct FragOut{
    [[vk::location(0)]] float4 out_color;
    [[vk::location(1)]] float4 world_position;
    [[vk::location(2)]] float4 world_normal_map;
    [[vk::location(3)]] float4 material_data;
}

struct VertexOut{
    float4 screen_pos: SV_Position;
    float3 world_pos: WORLD_POS;
    float3 normal: NORMAL_DIR;
    float2 uv: UV_COORD;
}

struct FragIn{
    float3 world_pos: WORLD_POS;
    float3 normal: NORMAL_DIR;
    float2 uv: UV_COORD;
}

[shader("vertex")]
VertexOut vertex_shader(VertexIn v_in){
    var world_pos = mul(WORLD_MATRIX.world_matrix, float4(v_in.position, 1.0f));
    var cam_pos = mul(CAMERA_PROJECTION_MATRIX.position, world_pos);
    var screen_pos = mul(CAMERA_PROJECTION_MATRIX.projection, cam_pos);
    var oriented_normal = normalize(mul(float3x3(WORLD_MATRIX.normal_matrix), v_in.normal));

    VertexOut v_out ={};

    v_out.screen_pos = screen_pos;
    v_out.world_pos = world_pos.xyz;
    v_out.normal = oriented_normal;
    v_out.uv = v_in.uv_coord;

    return v_out;
}

[shader("fragment")]
FragOut fragment_shader(FragIn f_in){
    FragOut f_out = {};

    var colored = MTD.color * float((int(MTD.coloring_type) >> 1) & 0b1);
    var uv_d = float4(f_in.uv, 0, 1) * float(int(MTD.coloring_type) & 0b1);
    var texture_color = OBJECT_TEXTURE.Sample(f_in.uv) * float((int(MTD.coloring_type) >> 2) & 0b1);

    f_out.out_color = colored + uv_d + texture_color;

    f_out.world_position = float4(f_in.world_pos, 1.0);
    f_out.world_normal_map = float4(f_in.normal, 1.0);
    f_out.material_data =
    float4(
        MTD.specular_reflection,
        MTD.defuse_reflection,
        MTD.ambient_reflection,
        MTD.shininess
    );

    return f_out;
}
