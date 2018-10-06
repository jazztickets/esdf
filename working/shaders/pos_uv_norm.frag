#version 120

#define MAX_LIGHTS 10

uniform sampler2D sampler0;
uniform vec4 ambient_light;

varying vec3 world_vertex;
varying vec3 world_normal;
varying vec2 texture_coord;

uniform int light_count;
uniform struct light {
	vec3 position;
	vec4 color;
} lights[MAX_LIGHTS];

void main() {

	// Set ambient light
	vec4 light_color = ambient_light;

	// Calculate lighting
	for(int i = 0; i < light_count; i++) {

		// Get direction to light
		vec3 light_direction = lights[i].position - world_vertex;
		float light_distance = length(light_direction);

		// Normalize
		light_direction /= light_distance;

		// Calculate diffuse color
		vec4 diffuse_light = lights[i].color * max(dot(world_normal, light_direction), 0.0);
		//float attenuation = 1.0 / (light_attenuation.x + light_distance * light_attenuation.y + light_distance * light_distance * light_attenuation.z);
		float attenuation = 1;

		// Add lights up
		light_color += diffuse_light * attenuation;
	}

	// Get texture color
	vec4 texture_color = texture2D(sampler0, texture_coord);

	// Final color
	gl_FragColor = gl_Color * texture_color * light_color;
}
