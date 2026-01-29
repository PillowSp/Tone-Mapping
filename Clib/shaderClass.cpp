// ============================================================
// File: shaderClass.cpp
// Author: Jakub Hanusiak
// Date: 5 sem, 2026-01-21
// Topic: Tone Mapping
// ============================================================
#include "shaderClass.h"

// ----------------------------------------------------
// get_file_contents
//
// Description:
// Reads the entire contents of a text file (e.g. GLSL
// shader source) and returns it as a single string.
//
// Parameters:
// filename - Path to the file to be loaded
//
// Returns:
// String containing the file contents, or an empty
// string if the file cannot be opened.
// ----------------------------------------------------
std::string get_file_contents(const char* filename)
{
    // Open file in binary mode
    std::ifstream in(filename, std::ios::binary);

    // Check if file exists
    if (!in)
    {
        std::cout << "file not found: " << filename << std::endl;
        return "";
    }

    // Read entire file into string
    std::string out;
    in.seekg(0, std::ios::end);           // Move to end of file
    out.resize((size_t)in.tellg());       // Allocate required size
    in.seekg(0, std::ios::beg);           // Move back to beginning
    in.read(&out[0], out.size());         // Read file contents
    in.close();

    return out;
}

// ----------------------------------------------------
// Shader constructor
//
// Description:
// Builds an OpenGL shader program from a vertex shader
// and a fragment shader source file.
// Handles loading, compilation, linking, and cleanup.
//
// Parameters:
// vertexFile   - Path to the vertex shader source
// fragmentFile - Path to the fragment shader source
// ----------------------------------------------------
Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
    // Load shader source code from files
    std::string vertexCode = get_file_contents(vertexFile);
    std::string fragmentCode = get_file_contents(fragmentFile);

    // Convert source strings to C-style strings
    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();

    // ----------------------------
    // Vertex shader compilation
    // ----------------------------

    // Create Vertex Shader object
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

    // Attach source code and compile
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Check for compilation errors
    compileErrors(vertexShader, "VERTEX");

    // ----------------------------
    // Fragment shader compilation
    // ----------------------------

    // Create Fragment Shader object
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    // Attach source code and compile
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Check for compilation errors
    compileErrors(fragmentShader, "FRAGMENT");

    // ----------------------------
    // Shader program linking
    // ----------------------------

    // Create Shader Program
    ID = glCreateProgram();

    // Attach compiled shaders
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);

    // Link shader program
    glLinkProgram(ID);

    // Check for linking errors
    compileErrors(ID, "PROGRAM");

    // Delete shader objects after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// ----------------------------------------------------
// Activate
//
// Description:
// Activates the shader program for rendering.
// ----------------------------------------------------
void Shader::Activate()
{
    glUseProgram(ID);
}

// ----------------------------------------------------
// Delete
//
// Description:
// Deletes the shader program and frees GPU resources.
// ----------------------------------------------------
void Shader::Delete()
{
    glDeleteProgram(ID);
}

// ----------------------------------------------------
// compileErrors
//
// Description:
// Checks and reports compilation or linking errors
// for shaders and shader programs.
//
// Parameters:
// shader - Shader or program ID
// type   - "VERTEX", "FRAGMENT", or "PROGRAM"
// ----------------------------------------------------
void Shader::compileErrors(unsigned int shader, const char* type)
{
    GLint hasCompiled;
    char infoLog[1024];

    // Shader compilation error check
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "SHADER_COMPILATION_ERROR for: "
                << type << "\n"
                << infoLog << std::endl;
        }
    }
    // Program linking error check
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "SHADER_LINKING_ERROR for: "
                << type << "\n"
                << infoLog << std::endl;
        }
    }
}
