#version 330 core 
layout (location = 0) in vec3 aPosition; 
layout (location = 1) in vec3 aColor; 
layout (location = 2) in vec3 aNormal;  

out vec3 vColor;
out vec3 vViewPos;
out vec3 vNormal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat3 uNormalMatrix; // invTransp(ModelView)

void main(void) 
{ 
    vColor = aColor;

    vec4 viewPos = uView * uModel * vec4(aPosition, 1.0);
    vViewPos = viewPos.xyz;

    vNormal = uNormalMatrix * aNormal; 

    gl_Position = uProj * viewPos; 
}