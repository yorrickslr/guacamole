// #define RASTERIZATION_COUNT
// #define HIGHLIGHT_FEEDBACK_POINT

// #define FINE_DERIVATIVES

struct padded_vt_address
{
    uvec2 vt_address;
    ivec2 max_level_and_padding;
};

struct physical_texture_info_struct
{
    uvec2 address;
    uvec2 tile_size;
    vec2 tile_padding;
    uvec2 dims;
};

struct idx_tex_positions
{
    int parent_lvl;
    uvec4 parent_idx;
    int child_lvl;
    uvec4 child_idx;
};

uniform bool enable_feedback;
layout(std430, binding = 2) buffer out_lod_feedback { int out_lod_feedback_values[]; };
layout(std430, binding = 4) buffer in_feedback_inv_index { uint in_feedback_inv_index_values[]; };

#ifdef RASTERIZATION_COUNT
layout(std430, binding = 3) buffer out_count_feedback { uint out_count_feedback_values[]; };
#endif

layout(std140, binding = 3) uniform physical_texture_address { physical_texture_info_struct pt; };
layout(std140, binding = 4) uniform virtual_texture_addresses { padded_vt_address vts[1024]; };

uniform int gua_current_vt_idx;

const vec2 tile_padding_by_tile_size = pt.tile_padding / pt.tile_size;
const uint pt_slice_size = pt.dims.x * pt.dims.y;

const int ROW_SKIP = 32;
const int COLUMN_SKIP = 32;

float dxdy(vec2 texture_sampling_coordinates, uint max_level)
{
    vec2 c = texture_sampling_coordinates;

#ifdef FINE_DERIVATIVES
    float dFdxCx = dFdxFine(c.x);
    float dFdxCy = dFdxFine(c.y);

    float dFdyCx = dFdyFine(c.x);
    float dFdyCy = dFdyFine(c.y);
#else
    float dFdxCx = dFdxCoarse(c.x);
    float dFdxCy = dFdxCoarse(c.y);

    float dFdyCx = dFdyCoarse(c.x);
    float dFdyCy = dFdyCoarse(c.y);
#endif

    float dx_sq_norm = dFdxCx * dFdxCx + dFdxCy * dFdxCy;
    float dy_sq_norm = dFdyCx * dFdyCx + dFdyCy * dFdyCy;

    float sqrho = max(1.f / dx_sq_norm, 1.f / dy_sq_norm);

    if(isinf(sqrho) || isnan(sqrho))
    {
        return -float(max_level);
    }

    return 0.5f * log2(pt.tile_size.x * pt.tile_size.x / sqrho); // a valid trick to avoid sqrt and two log2s
}

/*
 * Physical texture lookup
 */
vec4 get_physical_texture_color(uvec4 index_quadruple, vec2 texture_sampling_coordinates, uint current_level, int max_level)
{
    // offset represented as tiles is divided by total num tiles per axis
    // (replace max_width_tiles later by correct uniform)
    // extracting x,y from index texture
    uvec2 base_xy_offset = index_quadruple.xy;

    // base x,y coordinates * number of tiles / number of used index texture pixel
    // taking the factional part by modf
    vec2 physical_tile_ratio_xy = fract(texture_sampling_coordinates * (1 << current_level));

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding_by_tile_size;
    vec2 padding_offset = tile_padding_by_tile_size;

    // adding the ratio for every texel to our base offset to get the right pixel in our tile
    // and dividing it by the dimension of the phy. tex.
    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / pt.dims;

    // outputting the calculated coordinate from our physical texture
    vec4 c = texture(sampler2DArray(pt.address), vec3(physical_texture_coordinates, index_quadruple.z));

    return c;
}

/*
 * Fill the feedback buffer with the feedback value for a given tile.
 * Here, we use the maximum lod required from the rendered tile.
 */
void update_feedback(int feedback_value, uvec4 base_offset)
{
    uint one_d_feedback_ssbo_index = base_offset.x + base_offset.y * pt.dims.x + base_offset.z * pt_slice_size;
    uint compact_position = in_feedback_inv_index_values[one_d_feedback_ssbo_index];

    uint prev = out_lod_feedback_values[compact_position];
    if(prev < feedback_value)
    {
        out_lod_feedback_values[compact_position] = feedback_value;
    }
}

vec4 mix_colors(idx_tex_positions positions, int desired_level, vec2 texture_coordinates, float mix_ratio, int max_level)
{
    vec4 child_color = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl, max_level);
    vec4 parent_color = get_physical_texture_color(positions.parent_idx, texture_coordinates, positions.parent_lvl, max_level);

    return mix(parent_color, child_color, mix_ratio);
}

// #define VIS_1
// #define VIS_2

vec4 illustrate_level(int desired_level, idx_tex_positions positions, float mix_ratio)
{
    vec4 child_color = vec4(0, 0, 0, 1);
    vec4 parent_color = vec4(0, 0, 0, 1);

#ifdef VIS_1
    vec4 c0 = vec4(0, 0, 0, 1); // black   - level 0 and below
    vec4 c1 = vec4(0, 0, 1, 1); // blue    - level 1, 8, 15
    vec4 c2 = vec4(0, 1, 0, 1); // green   - level 2, 9, 16
    vec4 c3 = vec4(0, 1, 1, 1); // cyan    - level 3, 10
    vec4 c4 = vec4(1, 0, 0, 1); // red     - level 4, 11
    vec4 c5 = vec4(1, 0, 1, 1); // magenta - level 5, 12
    vec4 c6 = vec4(1, 1, 0, 1); // yellow  - level 6, 13
    vec4 c7 = vec4(1, 1, 1, 1); // white   - level 7, 14

    switch(desired_level)
    {
    case -2:
    case -1:
    case 0:
        parent_color = c0;
        child_color = c0;
        break;
    case 1:
        parent_color = c0;
        child_color = c1;
        break;
    case 2:
    case 9:
    case 16:
        parent_color = c1;
        child_color = c2;
        break;
    case 3:
    case 10:
        parent_color = c2;
        child_color = c3;
        break;
    case 4:
    case 11:
        parent_color = c3;
        child_color = c4;
        break;
    case 5:
    case 12:
        parent_color = c4;
        child_color = c5;
        break;
    case 6:
    case 13:
        parent_color = c5;
        child_color = c6;
        break;
    case 7:
    case 14:
        parent_color = c6;
        child_color = c7;
        break;
    case 8:
    case 15:
        parent_color = c7;
        child_color = c1;
    }

    // return vec4(lambda / 16.0f, 0.0f, 0.0f, 1.0f);
#endif

#ifdef VIS_2
    vec4 child_idx = positions.child_idx;
    float child_lvl = positions.child_lvl;

    vec4 parent_idx = positions.parent_idx;
    float parent_lvl = positions.parent_lvl;

    child_color = vec4(clamp((child_lvl / 8.0), 0.0, 1.0), (float(child_idx.x + child_idx.y + child_idx.z) / float(pt.dims.x + pt.dims.y + 0)), clamp(((child_lvl - 8.0) / 8.0), 0.0, 1.0), 1);
    parent_color = vec4(clamp((child_lvl / 8.0), 0.0, 1.0), (float(parent_idx.x + parent_idx.y + parent_idx.z) / float(pt.dims.x + pt.dims.y + 0)), clamp(((parent_lvl - 8.0) / 8.0), 0.0, 1.0), 1);
#endif

    return mix(parent_color, child_color, mix_ratio);
}

vec4 traverse_idx_hierarchy(vec2 texture_coordinates, usampler2D idx_tex_mm, int max_level)
{
    float lambda = -dxdy(texture_coordinates, max_level);

    int desired_level = max(0, min(int(ceil(lambda)), int(max_level)));
    float mix_ratio = (lambda < max_level) ? fract(lambda) : 1.f;

    idx_tex_positions positions;

    uvec4 idx_child_pos = textureLod(idx_tex_mm, texture_coordinates, max_level - desired_level).rgba;

    if(idx_child_pos.w == 1u)
    {
        uvec4 idx_parent_pos = textureLod(idx_tex_mm, texture_coordinates, max_level - desired_level + 1).rgba;
        positions = idx_tex_positions(desired_level - 1, idx_parent_pos, desired_level, idx_child_pos);
    }
    else
    {
        mix_ratio = 1.f; // No point blending: desired level is not accessible

        /// Binary-like search for maximum available depth
        int left = 0;
        int right = desired_level;
        while(left < right)
        {
            int i = (left + right) / 2;

            uvec4 idx_child_pos = textureLod(idx_tex_mm, texture_coordinates, max_level - i).rgba;

            if(i == 0)
            {
                positions = idx_tex_positions(0, idx_child_pos, 0, idx_child_pos);
                break;
            }

            if(idx_child_pos.w == 1u)
            {
                if(right - i == 1)
                {
                    uvec4 idx_parent_pos = textureLod(idx_tex_mm, texture_coordinates, max_level - i + 1).rgba;
                    positions = idx_tex_positions(i - 1, idx_parent_pos, i, idx_child_pos);
                    break;
                }

                left = min(i, desired_level);
            }
            else
            {
                right = max(i, 0);
            }
        }
    }

    vec4 c;
#ifdef VIS_1
    c = illustrate_level(desired_level, positions, mix_ratio);
#endif

#ifdef VIS_2
    c = illustrate_level(desired_level, positions, mix_ratio);
#endif

#ifndef VIS_1
#ifndef VIS_2
    c = mix_colors(positions, desired_level, texture_coordinates, mix_ratio, max_level);
#endif
#endif

    if(enable_feedback)
    {
        if(int(gl_FragCoord.x) % COLUMN_SKIP == 0 && int(gl_FragCoord.y) % ROW_SKIP == 0)
        {
            update_feedback(desired_level, positions.child_idx);

#ifdef HIGHLIGHT_FEEDBACK_POINT
            c = vec4(1., 0., 0., 1.);
#endif
        }
    }

    return c;
}