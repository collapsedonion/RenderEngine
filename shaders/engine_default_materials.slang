
[[vk::binding(0,0)]]
RWTexture2D material_output;

[shader("compute")]
[numthreads(32, 32, 1)]
void check_board_place_holder(
    uint3 id: SV_DispatchThreadID
){

    int width, height;
    material_output.GetDimensions(
        width,
        height
    );

    float3 c3 = float3(0.5, 0.0, 0.5) * (((id.x / 32) - (id.y / 32) % 2) % 2);
    float4 c = float4(c3, 1.0);

    material_output.Store(
        id.xy,
        c
    );
}