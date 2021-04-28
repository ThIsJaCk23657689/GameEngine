#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D depthMap;
uniform float nearPlane;
uniform float farPlane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth) {
    // �ϱ��^�зǳ]�Ƨ��Шt�A�ó̫��쥻�D�u�ʪ��`�����Y�ܦ^�������Y
    float z = depth * 2.0 - 1.0; 
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));	
}

void main() {             
    float depthValue = texture(depthMap, TexCoords).r;
    // FragColor = vec4(vec3(LinearizeDepth(depthValue) / farPlane), 1.0); // �z��
    FragColor = vec4(vec3(depthValue), 1.0); // ����
}