#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // 未使用(頂点レイアウトを他メッシュと揃えるために受け取るだけ)
layout (location = 2) in vec3 aColor;  // 未使用
layout (location = 3) in vec2 aTex;

out vec2 texCoord;

uniform mat4 camMatrix;
uniform mat4 model;

void main()
{
	texCoord = aTex;
	gl_Position = camMatrix * model * vec4(aPos, 1.0f);
}
