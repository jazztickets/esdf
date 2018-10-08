#version 330 core

#define MAX_LIGHTS 50

uniform sampler2D sampler0;
uniform vec4 ambient_light;
uniform vec4 color;

uniform int light_count;
uniform struct light {
	vec3 position;
	vec4 color;
	float radius;
} lights[MAX_LIGHTS];

smooth in vec3 world_position;
in vec3 world_normal;
in vec2 texture_coord;
out vec4 out_color;

void main() {

	// Set ambient light
	vec4 light_color = ambient_light;

	// Calculate lighting
	for(int i = 0; i < light_count; i++) {

		// Get direction to light
		vec3 light_direction = lights[i].position - world_position;
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
	vec4 texture_color = texture(sampler0, texture_coord);

	// Final color
	out_color = color * texture_color * light_color;
}
