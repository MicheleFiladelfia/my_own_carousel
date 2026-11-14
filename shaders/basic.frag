#version 330 core  
out vec4 color; 
in vec3 vColor; 
in vec3 vViewPos;
in vec3 vNormal;
in vec2 vTexCoord;


uniform vec3 uColor;
uniform vec3 uSunDirView; // Direzione verso il Sole in View Space
uniform float uSpecularStrength;
uniform int uLightingEnabled; // 1 = illuminato, 0 = non illuminato (linee/debug)
uniform sampler2D uTexture;
uniform int uUseTexture;
uniform float uSunIntensity;

void main(void) 
{ 
    vec3 baseColor = (uUseTexture == 1) ? texture(uTexture, vTexCoord).rgb : ((uColor.x < 0.0) ? vColor : uColor);

    // Se l'illuminazione e' disabilitata (es. linee, assi, wireframe): mostra il colore base
    if (uLightingEnabled == 0) {
        color = vec4(baseColor, 1.0);
        return;
    }

    // 1. Calcolo/selezione della normale in View Space
    vec3 normal;
    if (dot(vNormal, vNormal) > 0.001) {
        normal = normalize(vNormal);
    } else {
        vec3 crossVal = cross(dFdx(vViewPos), dFdy(vViewPos));
        if (dot(crossVal, crossVal) == 0.0) {
            color = vec4(baseColor, 1.0);
            return;
        }
        normal = normalize(crossVal);
    }

    // 2. Vettori di luce e vista in View Space
    // In View Space, la telecamera e' esattamente all'origine (0, 0, 0), quindi V = normalize(-vViewPos)
    vec3 L = normalize(uSunDirView);
    vec3 V = normalize(-vViewPos);

    // 3. Componente diffusa di Lambert
    float diff = max(dot(normal, L), 0.0);
    float ambient = 0.15;

    // 4. Componente speculare lucida di Blinn-Phong
    vec3 H = normalize(L + V);
    float shininess = 64.0;
    float spec = 0.0;
    if (diff > 0.0 && uSpecularStrength > 0.001) {
        spec = pow(max(dot(normal, H), 0.0), shininess);
    }

    // 5. Composizione finale
    vec3 diffuseColor = baseColor * (ambient + diff * uSunIntensity * (1.0 - ambient));
    vec3 specularColor = vec3(1.0) * spec * uSpecularStrength * uSunIntensity;

    color = vec4(diffuseColor + specularColor, 1.0);
}
