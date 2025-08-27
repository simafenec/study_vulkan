#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

// 0番から頂点データ、1番から色データを受け取る。
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

// フラグメントシェーダーに頂点の色を渡すため、
// フレームバッファの0番に色情報を書き出す
layout(location = 0) out vec3 fragColor;

void main() {
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
	fragColor  = inColor;
}