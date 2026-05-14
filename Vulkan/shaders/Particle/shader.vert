#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragmentColor;

void main()
{
	gl_PointSize = 14.0; // パーティクルは頂点を画面に視認できる大きさのスプライトで描画するため、指定した半径の矩形を描画してもらうためにこの指示を書く
	gl_Position = vec4(inPos.xy, 1.0, 1.0);
	fragmentColor = inColor.rgb;
}