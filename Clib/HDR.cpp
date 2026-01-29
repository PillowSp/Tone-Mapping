// ============================================================
// File: HDR.cpp
// Author: Jakub Hanusiak
// Date: 5 sem, 2026-01-21
// Topic: Tone Mapping
// ============================================================
#include <iostream>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shaderClass.h"

/* ============================================================
   Global variables
   ============================================================ */

   /*
    * gWindow
    * Pointer to the hidden GLFW window used for offscreen rendering.
    * Range: nullptr (not initialized) or valid GLFWwindow pointer.
    */
static GLFWwindow* gWindow = nullptr;

/*
 * gGLReady
 * Indicates whether GLFW and OpenGL have been successfully initialized.
 * Range: false (not initialized), true (initialized).
 */
static bool gGLReady = false;

/*
 * quadVAO
 * Vertex Array Object identifier for the fullscreen quad.
 * Range: 0 (not created) or valid OpenGL VAO ID.
 */
static GLuint quadVAO = 0;

/*
 * quadVBO
 * Vertex Buffer Object identifier for the fullscreen quad vertex data.
 * Range: 0 (not created) or valid OpenGL VBO ID.
 */
static GLuint quadVBO = 0;

/* ============================================================
   Procedure: InitFullscreenQuad
   ------------------------------------------------------------
   Description:
   Initializes a fullscreen quad used to render a texture over
   the entire viewport.
   ============================================================ */
void InitFullscreenQuad()
{
    // If the VAO already exists, do nothing
    if (quadVAO)
        return;

    /*
     * quadVertices
     * Vertex data for a fullscreen quad.
     * Format per vertex:
     *  - Position (x, y) in normalized device coordinates
     *  - Texture coordinates (u, v)
     */
    float quadVertices[] = {
        // position   // uv
        -1,  1,       0, 1,
        -1, -1,       0, 0,
         1, -1,       1, 0,
        -1,  1,       0, 1,
         1, -1,       1, 0,
         1,  1,       1, 1
    };

    // Generate OpenGL objects
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    // Bind VAO and VBO
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

    // Upload vertex data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Enable and configure vertex attribute 0 (position)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,                      // attribute index
        2,                      // number of components
        GL_FLOAT,               // data type
        GL_FALSE,               // normalized
        4 * sizeof(float),      // stride
        (void*)0                // offset
    );

    // Enable and configure vertex attribute 1 (texture coordinates)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,                      // attribute index
        2,                      // number of components
        GL_FLOAT,               // data type
        GL_FALSE,               // normalized
        4 * sizeof(float),      // stride
        (void*)(2 * sizeof(float)) // offset
    );

    // Unbind VAO
    glBindVertexArray(0);
}

/* ============================================================
   Procedure: InitGLFW
   ------------------------------------------------------------
   Description:
   Initializes GLFW, creates a hidden OpenGL context, and loads
   OpenGL function pointers using GLAD.

   Output parameters:
   Returns true if initialization succeeds, false otherwise.
   ============================================================ */
extern "C" __declspec(dllexport) bool InitGLFW()
{
    // If already initialized, return success
    if (gGLReady)
        return true;

    // Initialize GLFW
    if (!glfwInit())
        return false;

    // Configure GLFW for hidden offscreen rendering
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a hidden 1x1 window
    gWindow = glfwCreateWindow(1, 1, "", nullptr, nullptr);
    if (!gWindow)
        return false;

    // Make the OpenGL context current
    glfwMakeContextCurrent(gWindow);

    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;

    gGLReady = true;
    return true;
}

/* ============================================================
   Procedure: UploadToGL
   ------------------------------------------------------------
   Description:
   Uploads a linear HDR RGB image to OpenGL, applies tone mapping
   using a fragment shader, and reads back the result.

   Input parameters:
   linearRGB   - Pointer to linear RGB float data (RGBRGB...)
   width       - Image width in pixels (must be > 0)
   height      - Image height in pixels (must be > 0)
   exposure    - Exposure multiplier for tone mapping
   whitePoint  - White point value for tone mapping

   Output parameters:
   outputBGRA  - Pointer to output BGRA8 image buffer

   Notes:
   This function performs offscreen rendering using an FBO.
   ============================================================ */
extern "C" __declspec(dllexport)
void UploadToGL(
    float* linearRGB,
    int width,
    int height,
    unsigned char* outputBGRA,
    float exposure,
    float whitePoint)
{
    /* ----------------------------
       1. Initialize GLFW and OpenGL
       ---------------------------- */

    if (!InitGLFW())
        return;

    InitFullscreenQuad();
    glfwMakeContextCurrent(gWindow);

    /* ----------------------------
       2. Upload input HDR texture
       ---------------------------- */

    GLuint hdrTex;
    glGenTextures(1, &hdrTex);
    glBindTexture(GL_TEXTURE_2D, hdrTex);

    // Upload HDR image as floating-point RGB texture
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB16F,
        width,
        height,
        0,
        GL_RGB,
        GL_FLOAT,
        linearRGB
    );

    // Set texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /* ----------------------------
       3. Create output texture and FBO
       ---------------------------- */

    GLuint fbo, colorTex;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);

    // Allocate BGRA8 output texture
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach texture to framebuffer
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTex,
        0
    );

    // Verify framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);

    /* ----------------------------
       4. Render using shader
       ---------------------------- */
	   // Load and compile shader program
    std::string p = std::filesystem::current_path()
        .parent_path()
        .parent_path()
        .parent_path()
        .parent_path()
        .string();
	std::string path_vert = p + "\\Clib\\default.vert"; // path to vertex shader
	std::string path_frag = p + "\\Clib\\default.frag"; // path to fragment shader
    Shader shaderProgram(
        path_vert.c_str(),
        path_frag.c_str()
    );

    shaderProgram.Activate();

    // Pass uniform values to shader
    glUniform1i(glGetUniformLocation(shaderProgram.ID, "tex0"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "exposure"), exposure);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "whitePoint"), whitePoint);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "gamma"), 2.2f);

    // Render fullscreen quad
    glBindVertexArray(quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    /* ----------------------------
       5. Read back pixels
       ---------------------------- */

    glReadPixels(
        0, 0,
        width, height,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        outputBGRA
    );

    /* ----------------------------
       6. Cleanup OpenGL resources
       ---------------------------- */

    glDeleteTextures(1, &hdrTex);
    glDeleteTextures(1, &colorTex);
    glDeleteFramebuffers(1, &fbo);
    shaderProgram.Delete();
}

/* ============================================================
   Procedure: CleanupGLFW
   ------------------------------------------------------------
   Description:
   Destroys the GLFW window and terminates GLFW.
   ============================================================ */
extern "C" __declspec(dllexport) void CleanupGLFW()
{
    if (gGLReady)
    {
        glfwDestroyWindow(gWindow);
        glfwTerminate();
        gGLReady = false;
    }
}
