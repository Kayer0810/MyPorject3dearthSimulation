#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 camMatrix;
uniform mat4 model; // Used to position the light bulb in the world

void main()
{
	gl_Position = camMatrix * model * vec4(aPos, 1.0);
}
