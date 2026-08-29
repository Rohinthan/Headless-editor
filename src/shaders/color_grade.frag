#version 330 core

in vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_inputTexture;

// Primary Controls
uniform float u_exposure;    // EV adjustment [-3.0 .. 3.0], default 0.0
uniform float u_brightness;  // Offset [-1.0 .. 1.0], default 0.0
uniform float u_contrast;    // Contrast factor [0.0 .. 3.0], default 1.0
uniform float u_saturation;  // Saturation factor [0.0 .. 3.0], default 1.0
uniform float u_temperature; // Color temperature [-1.0 .. 1.0], default 0.0
uniform float u_tint;        // Green/Magenta tint [-1.0 .. 1.0], default 0.0

// Lift / Gamma / Gain (CDL 3-Way Color Grading)
uniform vec3 u_lift;         // Shadows offset, default vec3(0.0)
uniform vec3 u_gamma_rgb;    // Midtones power, default vec3(1.0)
uniform vec3 u_gain;         // Highlights slope, default vec3(1.0)

// Rec.709 Luminance weights
const vec3 LUMA_REC709 = vec3(0.2126, 0.7152, 0.0722);

void main() {
    vec4 src = texture(u_inputTexture, v_texCoord);
    vec3 rgb = src.rgb;

    // 1. Exposure (Photographic EV scaling: rgb * 2^EV)
    if (abs(u_exposure) > 0.001) {
        rgb *= exp2(u_exposure);
    }

    // 2. Color Temperature (Warm / Cool) and Tint (Magenta / Green)
    if (abs(u_temperature) > 0.001 || abs(u_tint) > 0.001) {
        vec3 tempShift = vec3(
            u_temperature * 0.15 + u_tint * 0.10,
            -u_tint * 0.15,
            -u_temperature * 0.15 + u_tint * 0.05
        );
        rgb = clamp(rgb + tempShift, 0.0, 1.0);
    }

    // 3. Lift / Gamma / Gain
    // Output = (Lift * (1 - Input) + Gain * Input) ^ (1 / Gamma)
    vec3 liftFactor = u_lift * (vec3(1.0) - rgb);
    vec3 gainFactor = u_gain * rgb;
    rgb = liftFactor + gainFactor;
    rgb = clamp(rgb, 0.0, 1.0);

    vec3 invGammaRGB = vec3(1.0) / max(u_gamma_rgb, vec3(0.01));
    rgb = pow(rgb, invGammaRGB);

    // 4. Brightness & Contrast
    rgb = (rgb - vec3(0.5)) * u_contrast + vec3(0.5) + vec3(u_brightness);
    rgb = clamp(rgb, 0.0, 1.0);

    // 5. Saturation (Luminance preserving)
    float luma = dot(rgb, LUMA_REC709);
    rgb = mix(vec3(luma), rgb, u_saturation);
    rgb = clamp(rgb, 0.0, 1.0);

    fragColor = vec4(rgb, src.a);
}
