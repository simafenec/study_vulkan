#version 450

layout(location = 0) in vec3 fragmentColor;

layout(location = 0) out vec4 outColor;

void main()
{
	vec2 coord = gl_PointCoord - vec2(0.5); // PointCoord は [0 - 1] の間なので0.5を引いて中心を(0,0)にしている　Vertex Shaderで指定した大きさの正方形の中のローカルUV座標が gl_PointCoord
	outColor = vec4(fragmentColor, 0.5 - length(coord)); // 中心から離れるほど透明度を上げることで円形パーティクルを表現している
}