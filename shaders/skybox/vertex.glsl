#version 410

layout(location = 0) in vec4 coord;

// We'll provide a custom MVP that uses view rotation only (no translation) when locked to camera
uniform mat4 SkyMVP;

out vec3 vDir;

void main() {
	// Centered cube direction (account for model's original Y in [0,1])
	vDir = vec3(coord.x, coord.y - 0.5, coord.z);

	// Position on screen; push depth to far plane to avoid clipping with scene
	gl_Position = SkyMVP * coord;
	gl_Position.z = gl_Position.w;
}
