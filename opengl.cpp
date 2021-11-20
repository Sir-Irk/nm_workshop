#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "glad.c"
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <iostream>
#include <cassert>
#include <cstdint>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <iterator>
#include <algorithm>
#include <string_view>
#include <numbers>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if 0
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#define exit_message(message)          \
    do                                 \
    {                                  \
        fmt::print("{}\n", (message)); \
        assert(false);                 \
    } while (0)

static const std::string vertex_shader_text =
    "#version 460 core\n"
    "layout (location = 0) in vec2 vPos;\n"
    "layout (location = 1) in vec3 vCol;\n"
    "uniform mat4 MVP;\n"
    "out vec3 color;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
    "    color = vCol;\n"
    "}\n";

static const std::string fragment_shader_text =
    "#version 460 core\n"
    "out vec4 FragColor;"
    "in vec3 color;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(color, 1.0);\n"
    "}\n";

namespace fsys
{
    template <class T>
    static T read_file(const std::string_view &filename)
    {
        std::ifstream file(std::string(filename), std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            assert(!"Failed to open file");
        }

        auto size = static_cast<size_t>(file.tellg());
        T buffer;
        buffer.reserve(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();

        return buffer;
    }
}

namespace sigl
{
    template <typename T>
    auto identity()
    {
        return T(1.0f);
    }
}

namespace glsys
{
    static const auto _glErrorCodes = std::unordered_map<GLenum, const std::string_view>{
        {GL_INVALID_ENUM, "GL_INVALID_ENUM"},
        {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
        {GL_INVALID_OPERATION, "GL_INVALID_OPERATION: Given when the set of state for a command is not legal for the parameters given to that command"},
        {GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW: Given when a stack pushing operation cannot be done because it would overflow the limit of that stack's size"},
        {GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW: Given when a stack popping operation cannot be done because the stack is already at its lowest point"},
        {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY:"},
        {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"},
        {GL_CONTEXT_LOST, "GL_CONTEXT_LOST"},
        {GL_TABLE_TOO_LARGE, "GL_TABLE_TOO_LARGE"},
    };

    const std::vector<GLenum> get_errors()
    {
        std::vector<GLenum> errors;
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
        {
            errors.push_back(err);
        }

        return errors;
    }

    bool report_errors()
    {
        auto errors = glsys::get_errors();
        for (const auto &e : errors)
        {
            fmt::print(stderr, "{}\n", glsys::_glErrorCodes.at(e));
        }
        return errors.size() > 0;
    }

    GLuint create_shader(GLenum type, const std::string &code)
    {
        auto shader = glCreateShader(type);
        auto size = static_cast<GLint>(code.size());
        const char *c = code.c_str();
        glShaderSource(shader, 1, &c, nullptr);
        glCompileShader(shader);

        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int infoSize;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoSize);
            auto log = std::vector<char>(infoSize);
            glGetShaderInfoLog(shader, infoSize, nullptr, log.data());
            fmt::print(stderr, "{}\n", log.data());
            assert(false);
        }

        assert(!glsys::report_errors());
        return shader;
    }

    GLuint create_program(GLuint vShader, GLuint fShader)
    {
        auto program = glCreateProgram();
        glAttachShader(program, vShader);
        glAttachShader(program, fShader);
        glLinkProgram(program);

        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            int infoSize;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoSize);
            auto log = std::vector<char>(infoSize);
            glGetProgramInfoLog(program, infoSize, nullptr, log.data());
            fmt::print(stderr, "{}\n", log.data());
            assert(false);
        }

        assert(!glsys::report_errors());
        return program;
    }

    GLuint create_program_from_files(const std::string_view &vertexShaderPath, const std::string_view &fragmentShaderPath)
    {
        auto vCode = fsys::read_file<std::string>(vertexShaderPath);
        auto fCode = fsys::read_file<std::string>(fragmentShaderPath);
        auto vShader = glsys::create_shader(GL_VERTEX_SHADER, vCode);
        auto fShader = glsys::create_shader(GL_FRAGMENT_SHADER, fCode);
        auto program = glsys::create_program(vShader, fShader);
        glDeleteShader(vShader);
        glDeleteShader(fShader);
        assert(!glsys::report_errors());
        return program;
    }

    GLuint create_program_from_strings(const std::string &vCode, const std::string &fCode)
    {
        auto vShader = glsys::create_shader(GL_VERTEX_SHADER, vCode);
        auto fShader = glsys::create_shader(GL_FRAGMENT_SHADER, fCode);
        auto program = glsys::create_program(vShader, fShader);
        glDeleteShader(vShader);
        glDeleteShader(fShader);
        assert(!glsys::report_errors());
        return program;
    }

    GLint get_uniform_location(GLuint program, const char *str)
    {
        GLint result = glGetUniformLocation(program, str);
        if (result < 0)
        {
            fmt::print(stderr, "Failed to find uniform: {}", str);
            assert(false);
        }
        return result;
    }
}

void error_callback(int error, const char *description)
{
    fmt::print(stderr, "GLFW Error[{}]: {}\n", error, description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

template <typename T>
size_t sizeof_vector(const std::vector<T> &vec)
{
    return sizeof(T) * vec.size();
}

const GLfloat *matrix_raw(const auto &m)
{
    return &m[0][0];
}

struct vertex_data
{
    glm::vec2 v;
    glm::vec3 c;
};

//Test triangle
static const std::vector<vertex_data> vertices = {
    {.v = {-0.6f, -0.4f}, .c = {1.f, 0.f, 0.f}},
    {.v = {0.6f, -0.4f}, .c = {0.f, 1.f, 0.f}},
    {.v = {0.0f, 0.6f}, .c = {0.f, 0.f, 1.f}},
};

struct render_object
{
    GLuint vbo;
    GLuint vao;
    GLuint program;

    GLint uniMvp;
    GLint uniPos;
    GLint uniCol;

    glm::mat4x4 model;
};

int main()
{
    if (!glfwInit())
    {
        exit_message("failed to initialized glfw");
    }

    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(1920, 1080, "Endless Money", nullptr, nullptr);

    if (!window)
    {
        exit_message("failed to crerate window");
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint program = glsys::create_program_from_strings(vertex_shader_text, fragment_shader_text);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_vector(vertices), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), static_cast<void *>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), reinterpret_cast<void *>(sizeof(float) * 2));
    glEnableVertexAttribArray(1);
    assert(!glsys::report_errors());

    while (!glfwWindowShouldClose(window))
    {
        float ratio = width / static_cast<float>(height);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        glm::mat4x4 perspective = glm::ortho(-ratio, ratio, -1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4x4 model = glm::mat4x4(1.0f);
        glm::mat4x4 mvp = perspective * model;

        GLint uniMvp = glsys::get_uniform_location(program, "MVP");
        glUniformMatrix4fv(uniMvp, 1, GL_FALSE, matrix_raw(mvp));

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}