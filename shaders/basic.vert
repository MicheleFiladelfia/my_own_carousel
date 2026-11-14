#version 330 core 
layout (location = 0) in vec3 aPosition; 
layout (location = 1) in vec3 aColor; 
layout (location = 2) in vec3 aNormal;  
layout (location = 4) in vec2 aTexCoord;

out vec3 vColor;
out vec3 vViewPos;
out vec3 vNormal;
out vec2 vTexCoord;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat3 uNormalMatrix; // invTransp(ModelView)
uniform int uUseVertexUV;

void main(void) 
{ 
    vColor = aColor;

    vec4 viewPos = uView * uModel * vec4(aPosition, 1.0);
    vViewPos = viewPos.xyz;

    vNormal = uNormalMatrix * aNormal; 

    vTexCoord = (uUseVertexUV == 1) ? aTexCoord : (aPosition.xz * 0.03);

    gl_Position = uProj * viewPos; 
}