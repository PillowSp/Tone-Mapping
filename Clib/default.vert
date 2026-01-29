#version 330 core
/* ============================================================
   Vertex Shader (OpenGL 3.3 Core)
   Author: Jakub Hanusiak
   Date: 5 sem, 2026-01-21
   Topic: Tone Mapping

   Description:
   This vertex shader renders a fullscreen quad by passing
   2D vertex positions directly to clip space and forwarding
   texture coordinates to the fragment shader.

   The shader is intended for post-processing or offscreen
   rendering (e.g., tone mapping).
   ============================================================ */

/* ============================================================
   Input attributes
   ============================================================ */

/*
 * aPos
 * Vertex position in normalized device coordinates (NDC).
 * Range:
 *  - x, y ∈ [-1.0, 1.0]
 * Provided per-vertex from the vertex buffer.
 */
layout (location = 0) in vec2 aPos;

/*
 * aTex
 * Texture coordinates corresponding to the vertex.
 * Range:
 *  - u, v ∈ [0.0, 1.0]
 */
layout (location = 1) in vec2 aTex;

/* ============================================================
   Output variables
   ============================================================ */

/*
 * texCoord
 * Texture coordinates passed to the fragment shader.
 * Used for sampling the input texture.
 */
out vec2 texCoord;

/* ============================================================
   Main vertex shader procedure
   ------------------------------------------------------------
   Description:
   Assigns clip-space position for each vertex and forwards
   texture coordinates to the fragment shader.

   Input parameters:
   aPos - vertex position
   aTex - texture coordinates

   Output parameters:
   gl_Position - final vertex position in clip space
   texCoord    - interpolated texture coordinates
   ============================================================ */
void main()
{
    // Pass texture coordinates directly to fragment shader
    texCoord = aTex;

    /*
     * Set the final position of the vertex in clip space.
     * z is set to 0.0 because this is a 2D fullscreen quad.
     * w is set to 1.0 for proper homogeneous coordinates.
     */
    gl_Position = vec4(aPos, 0.0, 1.0);
}
