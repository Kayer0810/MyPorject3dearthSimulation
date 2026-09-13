#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;

// Imports the current position from the Vertex Shader
in vec3 crntPos;
// Imports the normal from the Vertex Shader
in vec3 Normal;
// Imports the color from the Vertex Shader
in vec3 color;
// Imports the texture coordinates from the Vertex Shader
in vec2 texCoord;


// Gets the Texture Units from the main function
uniform sampler2D diffuse0;
uniform sampler2D specular0;
// Gets the color of the light from the main function
uniform vec4 lightColor;
// Gets the position of the light (sun) from the main function
uniform vec3 lightPos;
// Gets the position of the camera from the main function
uniform vec3 camPos;


// ============================================================
// 太陽光は地球や月に対して十分遠い位置にあるため、距離による減衰は
// ほぼ無視できる(=平行光線/指向性ライトとみなせる)。
// これにより、実際に宇宙から地球を見たときのようなはっきりした
// 昼/夜の境界線(ターミネーターライン)を再現する。
// ============================================================
vec4 sunLight()
{
	// ambient light
	float ambient = 0.05f;

	// 拡散光(太陽の方向と法線の角度で明るさが決まる)
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - crntPos);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// 鏡面反射光(海面のハイライトなどをうっすら表現)
	float specularStrength = 0.3f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 32.0f);
	float specular = specAmount * specularStrength;

	vec4 texColor = texture(diffuse0, texCoord);
	float specMask = texture(specular0, texCoord).r;

	return texColor * (diffuse + ambient) * lightColor + specMask * specular * lightColor;
}

void main()
{
	FragColor = sunLight();
}
