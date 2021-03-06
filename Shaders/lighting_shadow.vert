#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoords;

out VS_OUT {
    vec3 NaivePos;
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} vs_out;

uniform mat4 model;
uniform mat3 normalModel;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform bool isCubeMap;

void main() {
    vs_out.NaivePos = aPosition;
    vs_out.FragPos = vec3(model * vec4(aPosition, 1.0));
    vs_out.Normal = normalModel * aNormal;
    vs_out.TexCoords = aTextureCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);

    if (isCubeMap) {
		// ø?s?ѪŲ?
		mat4 view_new = mat4(mat3(view));
		vec4 pos = projection * view_new * vec4(vs_out.FragPos, 1.0);
		gl_Position = pos.xyww;
	} else {
		gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
	}
}