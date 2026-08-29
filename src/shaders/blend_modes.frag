#version 330 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_baseTexture;   // Accumulated background from ping-pong FBO
uniform sampler2D u_layerTexture;  // Foreground incoming layer
uniform int u_blendMode;          // 0:Normal, 1:Add, 2:Multiply, 3:Screen, 4:Overlay, 5:Soft Light, 6:Color Dodge
uniform float u_opacity;          // Layer opacity multiplier [0.0 .. 1.0]

// Blend mode functions
vec3 blendNormal(vec3 base, vec3 blend) {
    return blend;
}

vec3 blendAdd(vec3 base, vec3 blend) {
    return min(vec3(1.0), base + blend);
}

vec3 blendMultiply(vec3 base, vec3 blend) {
    return base * blend;
}

vec3 blendScreen(vec3 base, vec3 blend) {
    return vec3(1.0) - (vec3(1.0) - base) * (vec3(1.0) - blend);
}

float blendOverlayChannel(float b, float l) {
    return (b < 0.5) ? (2.0 * b * l) : (1.0 - 2.0 * (1.0 - b) * (1.0 - l));
}

vec3 blendOverlay(vec3 base, vec3 blend) {
    return vec3(
        blendOverlayChannel(base.r, blend.r),
        blendOverlayChannel(base.g, blend.g),
        blendOverlayChannel(base.b, blend.b)
    );
}

float blendSoftLightChannel(float b, float l) {
    return (l < 0.5)
        ? (2.0 * b * l + b * b * (1.0 - 2.0 * l))
        : (sqrt(b) * (2.0 * l - 1.0) + 2.0 * b * (1.0 - l));
}

vec3 blendSoftLight(vec3 base, vec3 blend) {
    return vec3(
        blendSoftLightChannel(base.r, blend.r),
        blendSoftLightChannel(base.g, blend.g),
        blendSoftLightChannel(base.b, blend.b)
    );
}

float blendColorDodgeChannel(float b, float l) {
    return (l >= 1.0) ? 1.0 : min(1.0, b / (1.0 - l));
}

vec3 blendColorDodge(vec3 base, vec3 blend) {
    return vec3(
        blendColorDodgeChannel(base.r, blend.r),
        blendColorDodgeChannel(base.g, blend.g),
        blendColorDodgeChannel(base.b, blend.b)
    );
}

void main() {
    vec4 baseColor = texture(u_baseTexture, v_texCoord);
    vec4 layerColor = texture(u_layerTexture, v_texCoord);

    // Apply layer opacity
    float alpha = layerColor.a * clamp(u_opacity, 0.0, 1.0);
    if (alpha <= 0.0) {
        fragColor = baseColor;
        return;
    }

    vec3 blendedRGB = layerColor.rgb;
    if (u_blendMode == 0) {
        blendedRGB = blendNormal(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 1) {
        blendedRGB = blendAdd(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 2) {
        blendedRGB = blendMultiply(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 3) {
        blendedRGB = blendScreen(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 4) {
        blendedRGB = blendOverlay(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 5) {
        blendedRGB = blendSoftLight(baseColor.rgb, layerColor.rgb);
    } else if (u_blendMode == 6) {
        blendedRGB = blendColorDodge(baseColor.rgb, layerColor.rgb);
    }

    // Porter-Duff Over alpha composite
    vec3 outRGB = mix(baseColor.rgb, blendedRGB, alpha);
    float outAlpha = clamp(baseColor.a + alpha * (1.0 - baseColor.a), 0.0, 1.0);

    fragColor = vec4(outRGB, outAlpha);
}
