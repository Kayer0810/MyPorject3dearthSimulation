#version 330 core

out vec4 FragColor;

in vec2 texCoord;

// 星空テクスチャ(照明の影響を受けない、常に一定の明るさで表示)
uniform sampler2D diffuse0;

void main()
{
	FragColor = texture(diffuse0, texCoord);
}
