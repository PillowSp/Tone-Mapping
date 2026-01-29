#version 330 core
// Outputs colors in RGBA
out vec4 FragColor;

// Inputs the color from the Vertex Shader
in vec3 color;
// Inputs the texture coordinates from the Vertex Shader
in vec2 texCoord;

// Gets the Texture Unit from the main function
uniform sampler2D tex0;

// Controls the scale of the vertices
uniform float exposure;

// Extended Reinhard white point
uniform float whitePoint; 

// Framebuffer gamma (typically 2.2 for standard monitors)
uniform float gamma;


// Compute perceived luminance (Rec.709)
float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    // Sample HDR color
    vec3 hdr = texture(tex0, texCoord).rgb;

    // Apply exposure
    hdr *= exposure;

    // Compute luminance
    float L = luminance(hdr);

    // Extended Reinhard tone mapping on luminance
    float Lmapped = (L * (1.0 + L / (whitePoint * whitePoint))) /
                    (1.0 + L);

    // Re-scale color while preserving chroma
    vec3 mapped = hdr * (Lmapped / max(L, 0.0001));

    // Gamma correction (if framebuffer is not sRGB)
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
}