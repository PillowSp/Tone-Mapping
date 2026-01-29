#version 330 core
/* ============================================================
   Fragment Shader (OpenGL 3.3 Core)
   Author: Jakub Hanusiak
   Date: 5 sem, 2026-01-21
   Topic: Tone Mapping

   Description:
   This fragment shader performs HDR tone mapping using the
   Extended Reinhard operator. It converts linear HDR RGB input
   into display-ready LDR output with exposure control, white
   point compression, and gamma correction.

   The shader is intended for fullscreen quad rendering.
   ============================================================ */

/* ============================================================
   Output variables
   ============================================================ */

/*
 * FragColor
 * Final fragment color written to the framebuffer.
 * Format:
 *  - RGBA, each channel in range [0.0, 1.0]
 */
out vec4 FragColor;

/* ============================================================
   Input variables
   ============================================================ */

/*
 * texCoord
 * Texture coordinates interpolated from the vertex shader.
 * Range:
 *  u, v ∈ [0.0, 1.0]
 */
in vec2 texCoord;

/* ============================================================
   Uniform variables
   ============================================================ */

/*
 * tex0
 * Sampler for the HDR input texture.
 * Texture format:
 *  - RGB, 16-bit floating point (GL_RGB16F)
 */
uniform sampler2D tex0;

/*
 * exposure
 * Exposure multiplier applied to HDR values.
 * Range:
 *  > 0.0
 */
uniform float exposure;

/*
 * whitePoint
 * White point value for Extended Reinhard tone mapping.
 * Controls highlight compression.
 * Range:
 *  > 0.0
 */
uniform float whitePoint;

/*
 * gamma
 * Display gamma correction factor.
 * Typical value:
 *  - 2.2 for standard monitors
 */
uniform float gamma;

/* ============================================================
   Helper functions
   ============================================================ */

/*
 * luminance
 * Computes perceived luminance using Rec.709 coefficients.
 *
 * Input:
 *  c - RGB color in linear space
 *
 * Output:
 *  Scalar luminance value
 */
float luminance(vec3 c)
{
    // Weighted sum matching human visual sensitivity
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

/* ============================================================
   Main fragment shader procedure
   ------------------------------------------------------------
   Description:
   Samples the HDR texture, applies exposure scaling, performs
   luminance-based tone mapping using the Extended Reinhard
   operator, rescales color to preserve chroma, and applies
   gamma correction for display.

   Input parameters:
   texCoord  - texture coordinates
   tex0      - HDR texture
   exposure  - exposure scale
   whitePoint- highlight compression parameter
   gamma     - display gamma

   Output parameters:
   FragColor - final RGBA color
   ============================================================ */
void main()
{
    /*
     * Sample HDR color from the texture.
     * Values are in linear color space and may exceed 1.0.
     */
    vec3 hdr = texture(tex0, texCoord).rgb;

    // Apply exposure scaling
    hdr *= exposure;

    /*
     * Compute scene luminance from HDR color.
     */
    float L = luminance(hdr);

    /*
     * Extended Reinhard tone mapping:
     * Lmapped = (L * (1 + L / wp²)) / (1 + L)
     *
     * Compresses highlights while preserving midtones.
     */
    float Lmapped = (L * (1.0 + L / (whitePoint * whitePoint))) /
                    (1.0 + L);

    /*
     * Re-scale RGB color based on luminance mapping.
     * This preserves color ratios (chroma).
     * A small epsilon avoids division by zero.
     */
    vec3 mapped = hdr * (Lmapped / max(L, 0.0001));

    /*
     * Apply gamma correction to convert from linear space
     * to display (non-linear) space.
     */
    mapped = pow(mapped, vec3(1.0 / gamma));

    // Output final color with full opacity
    FragColor = vec4(mapped, 1.0);
}
