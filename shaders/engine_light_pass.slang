
enum LightType : uint32_t{
        DIRECTIONAL = 0b0001,
        SPOT = 0b0010,
        POINT = 0b0100
}

struct LightSource{
    float4 light_color;
    float4 position;
    float4 direction;
    LightType type;
}

struct LightInfo{
    uint64_t p_ls;
    float3 ambient_color;
    uint32_t light_source_count;
    float3 camera_position;
}

struct PhongShadingK{
    float specular;
    float difuse;
    float ambient;
    float shininess;
}

Sampler2D position_map;
Sampler2D normal_map;
Sampler2D material_map;

PhongShadingK get_phong_coeffs(float2 uv){
    float4 sampled_dat = material_map.Sample(uv);
    PhongShadingK psk = {};
    psk.specular = sampled_dat.x;
    psk.difuse = sampled_dat.y;
    psk.ambient = sampled_dat.z;
    psk.shininess = sampled_dat.z;
    return psk;
}

ConstantBuffer<LightInfo> LIGHT_INFO;

RWTexture2D result_texture;

[shader("compute")]
[numthreads(32,32,1)]
void phong_shading_model(
    uint3 id: SV_DispatchThreadID
){

    uint32_t width, height;
    result_texture.GetDimensions(width, height);

    float2 uv = float2(float(id.x) / float(width), float(id.y) / float(height));

    var li = LIGHT_INFO;

    LightSource** p_light_source =
    reinterpret<LightSource**>(li.p_ls);

    float4 resulting_light = float4(0,0,0,1.0);

    float3 my_normal = normal_map.Sample(uv).xyz;
    float3 my_position = position_map.Sample(uv).xyz;

    PhongShadingK k_s = get_phong_coeffs(uv);

    for(uint32_t i = 0; i < li.light_source_count; i++){
        LightSource ls = *(p_light_source[i]);
        float4 lc = float4(ls.light_color.xyz, 1.0);
        float3 direction;

        switch(ls.type){
        case LightType::DIRECTIONAL:
            direction = -ls.direction.xyz;
            break;
        case LightType::POINT:
            direction = normalize(ls.position.xyz - my_position);
            break;
        default:
        }

        float3 camera_dir = normalize(li.camera_position - my_position);
        float3 reflected_ray = reflect(-direction, my_normal);

        float diffuse_component = k_s.difuse * max(dot(my_normal, direction), 0.0);
        float specular_component = k_s.specular *
        pow(max(dot(reflected_ray, camera_dir), 0.0), k_s.shininess);
        resulting_light += lc * (diffuse_component + specular_component);
    }

    result_texture.Store(
        id.xy,
        float4(li.ambient_color, 1.0) * k_s.ambient + resulting_light
    );
}
