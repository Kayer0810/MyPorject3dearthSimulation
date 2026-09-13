#version 330 core

out vec4 FragColor;

// 太陽マーカーの色にも、月の公転軌道ハイライト線の色にも使う単色ユニフォーム
uniform vec4 flatColor;

void main()
{
	FragColor = flatColor;
}
