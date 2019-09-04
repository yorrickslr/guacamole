@include "common/header.glsl"

// varyings
in vec2 gua_quad_coords;

@include "common/gua_camera_uniforms.glsl"
@include "common/gua_gbuffer_input.glsl"
@include "common/gua_shading.glsl"


// output
layout(location=0) out vec3 gua_out_color;

void main() {

  ivec2 fragment_position = ivec2(gl_FragCoord.xy);
  const int number_of_gbuffers = 5;

  int debug_window_width  = int(gua_resolution.x / number_of_gbuffers);
  int debug_window_height = int((debug_window_width * gua_resolution.y) / gua_resolution.x);

  int shadow_debug_size  = 150;

  if ( fragment_position.y < debug_window_height)
  {
    vec2 texcoord  = vec2(float(mod(fragment_position.x, debug_window_width)) / debug_window_width,
                          float(mod(fragment_position.y, debug_window_height)) / debug_window_height);

    // output depth
    if ( fragment_position.x < debug_window_width) {
      gua_out_color = vec3(gua_get_depth(texcoord));
    } else if ( fragment_position.x < 2*debug_window_width) {
        // output color
      gua_out_color = gua_get_color(texcoord);
    } else if ( fragment_position.x < 3*debug_window_width) {
        // output normal
      gua_out_color = gua_get_normal(texcoord);
    } else if ( fragment_position.x < 4*debug_window_width) {
        // output position
      gua_out_color = gua_get_position(texcoord);
    } else if ( fragment_position.x < 6*debug_window_width) {
      unsigned int nlights = gua_sun_lights_num;
      int bitset_words = ((gua_lights_num - 1) >> 5) + 1;

      ivec2 tile = ivec2(mod(fragment_position.x, debug_window_width ),
                         mod(fragment_position.y, debug_window_height ));

      tile = 5 * tile >> @light_table_tile_power@;

      for (int sl = 0; sl < bitset_words; ++sl) {
        nlights += bitCount(texelFetch(usampler3D(gua_light_bitset), ivec3(tile, sl), 0).r);
      }
      gua_out_color = vec3(float(nlights) / gua_lights_num);
    }

  } else if (fragment_position.x < shadow_debug_size && fragment_position.y >= debug_window_height) {

    int shadow_map = (fragment_position.y - debug_window_height) / shadow_debug_size + 1;
    int light_id = -1;

    for (int i = 0; i < gua_lights_num; ++i) {
      if (gua_lights[i].casts_shadow && --shadow_map == 0) {
        light_id = i;
        break;
      }
    }

    if (light_id >= 0) {
      vec2 texcoord = vec2(float(mod(fragment_position.x, shadow_debug_size)) / shadow_debug_size,
                           float(mod(fragment_position.y-debug_window_height, shadow_debug_size)) / shadow_debug_size);

      float intensity = 0.0;
      const int slices = 30;
      for (int i=0; i < slices; ++i) {
        intensity += texture(sampler2DShadow(gua_lights[light_id].shadow_map), vec3(texcoord, i * 1.0/slices)).r;
      }

      gua_out_color = vec3(intensity/slices);
    } else {
      discard;
    }

  } else {
    discard;
  }

}

