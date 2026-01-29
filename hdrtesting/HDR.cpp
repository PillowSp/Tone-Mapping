#include<iostream>
#include<vector>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include<stb/stb_image.h>

#include"Texture.h"
#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

GLfloat vertices[] =
{ //     COORDINATES     /        COLORS      /   TexCoord  //
    -0.5f, -0.5f, 0.0f,     1.0f, 0.0f, 0.0f,	0.0f, 0.0f, // Lower left corner
    -0.5f,  0.5f, 0.0f,     0.0f, 1.0f, 0.0f,	0.0f, 1.0f, // Upper left corner
     0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,	1.0f, 1.0f, // Upper right corner
     0.5f, -0.5f, 0.0f,     1.0f, 1.0f, 1.0f,	1.0f, 0.0f  // Lower right corner
};

// Indices for vertices order
GLuint indices[] =
{
    0, 2, 1, // Upper triangle
    0, 3, 2 // Lower triangle
};

GLuint quadVAO = 0;
GLuint quadVBO = 0;

void InitFullscreenQuad()
{
    float quadVertices[] = {
        // pos     // uv
        -1,  1,    0, 1,
        -1, -1,    0, 0,
         1, -1,    1, 0,
        -1,  1,    0, 1,
         1, -1,    1, 0,
         1,  1,    1, 1
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void RenderFullscreenQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UploadToGL(float* linearRGB, int width, int height, unsigned char* outputBGRA) {
    gladLoadGL();
	InitFullscreenQuad();
    Shader shaderProgram("default.vert", "default.frag");

    // Generates Vertex Array Object and binds it
    VAO VAO1;
    VAO1.Bind();

    // Generates Vertex Buffer Object and links it to vertices
    VBO VBO1(vertices, sizeof(vertices));
    // Generates Element Buffer Object and links it to indices
    EBO EBO1(indices, sizeof(indices));

    // Links VBO attributes such as coordinates and colors to VAO
    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    VAO1.LinkAttrib(VBO1, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    // Unbind all to prevent accidentally modifying them
    VAO1.Unbind();
    VBO1.Unbind();
    EBO1.Unbind();

    GLuint hdrTex;
    glGenTextures(1, &hdrTex);
    glBindTexture(GL_TEXTURE_2D, hdrTex);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB16F,      // HDR texture
        width,
        height,
        0,
        GL_RGB,
        GL_FLOAT,
        linearRGB
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //----------------------------------------------------------------------

    GLuint fbo, colorTex;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);

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

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        colorTex,
        0
    );
    //----------------------------------------------------------------------
    //glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);

    shaderProgram.Activate();   	// <-- glUseProgram(shaderProgram.ID);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    /*glBindVertexArray(VAO1.ID);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);*/

    glUniform1f(glGetUniformLocation(shaderProgram.ID, "exposure"), 0.5f);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "aspect"), (float)width / (float)height);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "whitePoint"), 4.0f);
    glUniform1f(glGetUniformLocation(shaderProgram.ID, "gamma"), 2.2f);

    RenderFullscreenQuad(); // <-- SHADER RUNS HERE

    // ----------------------------
    // 4. Read pixels back
    // ----------------------------

    glReadPixels(
        0, 0,
        width, height,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        outputBGRA
    );

    // ----------------------------
    // 5. Cleanup
    // ----------------------------
    glDeleteTextures(1, &hdrTex);
    glDeleteTextures(1, &colorTex);
    glDeleteFramebuffers(1, &fbo);
}