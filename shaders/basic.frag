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
uniform int uNumLamps;
uniform vec3 uLampPosView[32];
uniform float uLampRadiusView;

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

    // 5. Solar light
    vec3 diffuseColor = baseColor * (ambient + diff * uSunIntensity * (1.0 - ambient));
    vec3 specularColor = vec3(1.0) * spec * uSpecularStrength * uSunIntensity;

    vec3 totalLight = diffuseColor + specularColor;

    // 6. lamps light
    float lampPower = 1.0 - uSunIntensity;
    for (int i = 0; i < uNumLamps; ++i) {
        vec3 toLight = uLampPosView[i] - vViewPos;
        float d = length(toLight) / uLampRadiusView;
        if (d < 1.0) {
            float atten = 1.0 - d * d;
            float lDiff = max(dot(normal, normalize(toLight)), 0.0);
            totalLight += baseColor * vec3(1.0, 0.85, 0.55) * lDiff * atten * lampPower * 1.4;
        }
    }

    color = vec4(totalLight, 1.0);
}
