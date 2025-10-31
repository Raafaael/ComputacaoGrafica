#version 410

in vec3 vDir;
out vec4 color;

uniform samplerCube envMap;

void main() {
	vec3 dir = normalize(vDir);
	// Sample using the same orientation as object reflections
	color = texture(envMap, dir);
}
