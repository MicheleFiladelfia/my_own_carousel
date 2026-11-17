#version 330 core

const int MAX_LAMPS = 32;
const float AMBIENT_STRENGTH = 0.15;
const float SHININESS = 64.0;
const vec3 LAMP_COLOR = vec3(1.0, 0.85, 0.55);
const float LAMP_BOOST = 1.4;

in vec3 vColor;
in vec3 vViewPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 color;

uniform vec3 uColor;
uniform vec3 uSunDirView;      // Direction toward the sun in View Space
uniform float uSunIntensity;
uniform float uSpecularStrength;
uniform int uLightingEnabled;  // 1 = lit, 0 = unlit (lines/debug)
uniform sampler2D uTexture;
uniform int uUseTexture;
uniform int uNumLamps;
uniform vec3 uLampPosView[MAX_LAMPS];
uniform float uLampRadiusView;
uniform vec3 uEmissive;

vec3 resolveBaseColor() {
    if (uUseTexture == 1) {
        vec4 tex = texture(uTexture, vTexCoord);
        if (tex.a < 0.5)
            discard;
        return tex.rgb;
    }
    return (uColor.x < 0.0) ? vColor : uColor;
}

// Returns false if degenerate geometry prevents computing a valid normal
bool computeViewNormal(out vec3 normal) {
    if (dot(vNormal, vNormal) > 0.001) {
        normal = normalize(vNormal);
        return true;
    }
    vec3 faceCross = cross(dFdx(vViewPos), dFdy(vViewPos));
    if (dot(faceCross, faceCross) == 0.0)
        return false;
    normal = normalize(faceCross);
    return true;
}

vec3 computeSunLight(vec3 N, vec3 V, vec3 baseColor) {
    vec3 L = normalize(uSunDirView);
    float diff = max(dot(N, L), 0.0);

    float spec = 0.0;
    if (diff > 0.0 && uSpecularStrength > 0.001) {
        vec3 H = normalize(L + V);
        spec = pow(max(dot(N, H), 0.0), SHININESS);
    }

    vec3 diffuse = baseColor * (AMBIENT_STRENGTH + diff * uSunIntensity * (1.0 - AMBIENT_STRENGTH));
    vec3 specular = vec3(1.0) * spec * uSpecularStrength * uSunIntensity;
    return diffuse + specular;
}

vec3 computeLampLights(vec3 N, vec3 baseColor) {
    // Turn lamps on early at dusk and keep them on until morning
    float lampPower = (1.0 - smoothstep(0.25, 0.85, uSunIntensity))*0.40;
    if (lampPower <= 0.001 || uLampRadiusView <= 0.0)
        return vec3(0.0);

    vec3 total = vec3(0.0);
    int count = min(uNumLamps, MAX_LAMPS);
    for (int i = 0; i < count; ++i) {
        vec3 toLight = uLampPosView[i] - vViewPos;
        float dist = length(toLight);
        float d = dist / uLampRadiusView;
        if (d < 1.0 && dist > 0.0001) {
            float atten = 1.0 - d * d;
            float diff = max(dot(N, toLight / dist), 0.0);
            total += baseColor * LAMP_COLOR * diff * atten * lampPower * LAMP_BOOST;
        }
    }
    return total + uEmissive;
}

void main(void) {
    vec3 baseColor = resolveBaseColor();
    vec3 normal;

    if (uLightingEnabled == 0 || !computeViewNormal(normal)) {
        color = vec4(baseColor, 1.0);
        return;
    }

    vec3 viewDir = normalize(-vViewPos); // Camera is at origin in View Space
    float lampPower = 1.0 - smoothstep(0.25, 0.85, uSunIntensity);
    vec3 totalLight = computeSunLight(normal, viewDir, baseColor)
                    + computeLampLights(normal, baseColor)
                    + uEmissive * lampPower;

    color = vec4(totalLight, 1.0);
}
