#define GLFW_INCLUDE_NONE
#include "glad.c"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <numbers>
#include <string_view>
#include <unordered_map>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#if 1
#define STB_IMAGE_IMPLEMENTATION
#pragma warning(disable : 4312)
#include "stb_image.h"
#pragma warning(default : 4312)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL4_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include "nuklear.h"
#include "nuklear_glfw3.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define BEGIN_TIMER(name) \
    std::chrono::steady_clock::time_point name##begin = std::chrono::steady_clock::now();

#define END_TIMER(name)                                                                 \
    std::chrono::steady_clock::time_point name##end = std::chrono::steady_clock::now(); \
    fmt::print("{} took {} miliseconds\n", #name, std::chrono::duration_cast<std::chrono::milliseconds>(name##end - name##begin).count());

#define exit_message(message)          \
    do {                               \
        fmt::print("{}\n", (message)); \
        assert(false);                 \
    } while (0)

namespace fsys {
template <class T>
static T read_file(const std::string_view& filepath)
{
    std::ifstream file(std::string(filepath), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        fmt::print(stderr, "Failed to open file from path \"{}\"\n", filepath);
        assert(false);
    }

    auto size = static_cast<size_t>(file.tellg());
    T buffer;
    buffer.resize(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();

    return buffer;
}
}

namespace sigl {
template <typename T>
auto identity()
{
    return T(1.0f);
}
}

namespace glsys {
static const auto _glErrorCodes = std::unordered_map<GLenum, const std::string_view> {
    { GL_INVALID_ENUM, "GL_INVALID_ENUM" },
    { GL_INVALID_VALUE, "GL_INVALID_VALUE" },
    { GL_INVALID_OPERATION, "GL_INVALID_OPERATION: Given when the set of state for a command is not legal for the parameters given to that command" },
    { GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW: Given when a stack pushing operation cannot be done because it would overflow the limit of that stack's size" },
    { GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW: Given when a stack popping operation cannot be done because the stack is already at its lowest point" },
    { GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY:" },
    { GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION" },
    { GL_CONTEXT_LOST, "GL_CONTEXT_LOST" },
    { GL_TABLE_TOO_LARGE, "GL_TABLE_TOO_LARGE" },
};

const std::vector<GLenum> get_errors()
{
    std::vector<GLenum> errors;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        errors.push_back(err);
    }

    return errors;
}

bool report_errors()
{
    auto errors = glsys::get_errors();
    for (const auto& e : errors) {
        fmt::print(stderr, "{}\n", glsys::_glErrorCodes.at(e));
    }
    return errors.size() > 0;
}

GLuint create_shader(GLenum type, const std::string& code)
{
    auto shader = glCreateShader(type);

    const char* c = code.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        int infoSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoSize);
        auto log = std::vector<char>(infoSize);
        glGetShaderInfoLog(shader, infoSize, nullptr, log.data());
        fmt::print(stderr, "{}\n{}\n", code, log.data());
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
    if (!success) {
        int infoSize;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoSize);
        auto log = std::vector<char>(infoSize);
        glGetProgramInfoLog(program, infoSize, nullptr, log.data());
        fmt::print(stderr, "failed to create program: {}\n", log.data());
        assert(false);
    }

    assert(!glsys::report_errors());
    return program;
}

GLuint create_program_from_files(const std::string_view& vertexShaderPath, const std::string_view& fragmentShaderPath)
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

GLuint create_program_from_strings(const std::string& vCode, const std::string& fCode)
{
    auto vShader = glsys::create_shader(GL_VERTEX_SHADER, vCode);
    auto fShader = glsys::create_shader(GL_FRAGMENT_SHADER, fCode);
    auto program = glsys::create_program(vShader, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);
    assert(!glsys::report_errors());
    return program;
}

GLuint create_texture(const uint32_t* data, int32_t w, int32_t h)
{
    GLuint result;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    assert(!glsys::report_errors());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    assert(!glsys::report_errors());

    glGenerateMipmap(GL_TEXTURE_2D);
    assert(!glsys::report_errors());

    return result;
}

GLint get_uniform_location(GLuint program, const char* str)
{
    GLint result = glGetUniformLocation(program, str);
    if (result < 0) {
        fmt::print(stderr, "Failed to find uniform: {}", str);
        assert(false);
    }
    return result;
}
}

#define SI_NORMALMAP_STATIC
#define SI_NORMALMAP_GPU
#define SI_NORMALMAP_IMPLEMENTATION
#include "si_normalmap.h"
void error_callback(int error, const char* description)
{
    fmt::print(stderr, "GLFW Error[{}]: {}\n", error, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

template <typename T>
size_t sizeof_vector(const std::vector<T>& vec)
{
    return sizeof(T) * vec.size();
}

const GLfloat* matrix_raw(const auto& m)
{
    return &m[0][0];
}

struct vertex_data {
    glm::vec3 v;
    glm::vec3 c;
    glm::vec2 t;
};
#if 0
static const std::vector<vertex_data> vertices = {
    { .v = { -0.6f, -0.4f }, .c = { 1.f, 0.f, 0.f } },
    { .v = { 0.6f, -0.4f }, .c = { 0.f, 1.f, 0.f } },
    { .v = { 0.0f, 0.6f }, .c = { 0.f, 0.f, 1.f } },
};
#else

//Test quad
static const std::vector<vertex_data> vertices = {
    { .v = { -0.5f, -0.5f, 0.0f }, .c = { 1.0f, 1.0f, 1.0f }, .t = { 0.0f, 0.0f } }, //first tri
    { .v = { 0.5f, -0.5f, 0.0f }, .c = { 1.0f, 1.0f, 1.0f }, .t = { 1.0f, 0.0f } },
    { .v = { 0.5f, 0.5f, 0.0f }, .c = { 1.0f, 1.0f, 1.0f }, .t = { 1.0f, 1.0f } },

    { .v = { -0.5f, -0.5f, 0.0f }, .c = { 1.0f, 1.0f, 1.0f }, .t = { 0.0f, 1.0f } }, //second tri
    { .v = { 0.5f, 0.5f, 0.0f }, .c = { 1.0f, 1.0f, 1.0f }, .t = { 0.0f, 1.0f } },
    { .v = { -0.5f, 0.5f, 0.0f }, .c = { 1.0f, 1.0f, 1.0f }, .t = { 1.0f, 1.0f } },
};
#endif

struct render_object {
    GLuint vbo;
    GLuint vao;
    GLuint program;

    GLint uniMvp;
    GLint uniPos;
    GLint uniCol;

    glm::mat4x4 model;
};

struct image_data {
    int32_t w, h;
    std::vector<uint32_t> pixels;
};

struct gpu_image {
    int32_t w, h;
    sinm_gpu_buffer gpu;
};

image_data load_image(const char* filepath)
{
    image_data result;
    uint32_t* data = reinterpret_cast<uint32_t*>(stbi_load(filepath, &result.w, &result.h, nullptr, 4));
    if (!data) {
        fmt::print(stderr, "failed to load image {}\n", filepath);
        assert(false);
    }

    //TODO: use automatic memory management with this copy
    result.pixels.reserve(result.w * result.h);
    //std::copy(data, &data[result.w * result.h], std::back_inserter(result.pixels));
    memcpy(result.pixels.data(), data, result.w * result.h * sizeof(uint32_t));
    //std::copy(data, &data[result.w * result.h], result.pixels);

    stbi_image_free(data);

    return result;
}

GLFWwindow* initialize_glfw()
{
    if (!glfwInit()) {
        exit_message("failed to initialized glfw");
    }

    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Normal Map Workshop", nullptr, nullptr);

    if (!window) {
        exit_message("failed to crerate window");
    }

    int x = 0;
    glfwMakeContextCurrent(window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);

    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    return window;
}

struct normal_map_settings {
    int blurPasses;
    float scale;
    sinm_greyscale_type greyscaleType;

    auto operator<=>(const normal_map_settings&) const = default;
};

struct normal_map_layer {
    normal_map_settings settings;
    gpu_image image;
    int nkTextureId;
    struct nk_image nkImage;
};

gpu_image generate_normal_map(const image_data& image, float scale, int blurPasses, sinm_greyscale_type greyscaleType, int flipY = 0)
{
    gpu_image result = { image.w, image.h };
    result.gpu = sinm_normal_map_gpu(image.pixels.data(), image.w, image.h, scale, blurPasses, greyscaleType, flipY);
    assert(result.gpu.fbo != 0);
    assert(result.gpu.buffer != 0);
    return result;
}

normal_map_layer
create_normal_map_layer(const image_data& image, float scale = 1.0f, int blurPasses = 2, sinm_greyscale_type greyscaleType = sinm_greyscale_luminance, int flipY = 0)
{
    normal_map_layer result;
    result.settings.scale = scale;
    result.settings.blurPasses = blurPasses;
    result.settings.greyscaleType = greyscaleType;
    result.image = generate_normal_map(image, result.settings.scale, result.settings.blurPasses, result.settings.greyscaleType, flipY);
    result.nkTextureId = nk_glfw3_add_texture(result.image.gpu.buffer);
    result.nkImage = nk_image_id(result.nkTextureId);
    return result;
}

void regenerate_normal_map_layers(std::vector<normal_map_layer>& layers, const image_data& albedoImage, bool flipY)
{
    int w = albedoImage.w;
    int h = albedoImage.h;
    for (auto& layer : layers) {
        sinm__normal_map_gpu(albedoImage.pixels.data(), layer.image.gpu.fbo, w, h, layer.settings.scale, layer.settings.blurPasses, layer.settings.greyscaleType, (int)flipY);
    }
}

void generate_normal_map_composite(sinm_gpu_buffer& outImage, const std::vector<normal_map_layer>& layers)
{
    if (layers.size() <= 1) {
        return;
    }

    std::vector<sinm_gpu_buffer> buffers;
    for (auto& layer : layers) {
        buffers.push_back(layer.image.gpu);
    }

    sinm_composite_gpu(outImage, buffers.data(), buffers.size(), layers[0].image.w, layers[0].image.h);
    assert(!glsys::report_errors());
}

int main()
{
    GLFWwindow* window = initialize_glfw();
    nk_context* ctx = nk_glfw3_init(window, NK_GLFW3_INSTALL_CALLBACKS, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

    BEGIN_TIMER(sinm_initialization)
    sinm_initialize_opengl();
    END_TIMER(sinm_initialization)

    {
        nk_font_atlas* atlas;
        nk_glfw3_font_stash_begin(&atlas);
        nk_font* clean = nk_font_atlas_add_from_file(atlas, "ProggyClean.ttf", 16, 0);
        nk_glfw3_font_stash_end();
        //nk_style_load_all_cursors(ctx, atlas->cursors);
        nk_style_set_font(ctx, &clean->handle);
    }

    //image_data albedoImage = load_image("textures/broken_tiles_01.tga");
    image_data albedoImage = load_image("textures/test_image.png");

    std::vector<normal_map_layer> normalMapLayers;
    normalMapLayers.push_back(create_normal_map_layer(albedoImage));

    //image_data normalMapResult = generate_normal_map_composite(normalMapLayers);

    int albedoMapTexIndex = nk_glfw3_create_texture(albedoImage.pixels.data(), albedoImage.w, albedoImage.h);
    struct nk_image albedoMapImage = nk_image_id(albedoMapTexIndex);

    sinm_gpu_buffer normalMap = sinm_normal_map_gpu(albedoImage.pixels.data(), albedoImage.w, albedoImage.h, 2.0f, 2, sinm_greyscale_luminance, false);
    image_data normalMapResult;
    normalMapResult.w = albedoImage.w;
    normalMapResult.h = albedoImage.h;
    normalMapResult.pixels.resize(albedoImage.w * albedoImage.h);
    //sinm_gpu_normal_map_to_buffer(normalMapResult.pixels.data(), normalMap.fbo, normalMapResult.w, normalMapResult.h);
    assert(!glsys::report_errors());
    int normalMapResultTexIndex = nk_glfw3_add_texture(normalMap.buffer);
    struct nk_image normalMapResultImage = nk_image_id(normalMapResultTexIndex);
    assert(!glsys::report_errors());

    std::vector<normal_map_settings> normalMapSettings;
    for (auto& layer : normalMapLayers) {
        normalMapSettings.push_back(layer.settings);
    }

    sinm_greyscale_type selectedGreyscaleOption = sinm_greyscale_lightness;
    const char* greyscaleOptionNames[sinm_greyscale_count] = { "average", "luminance", "lightness", "none" };

    int flipY = 0;
    char filenameInputBuffer[256] = "normal_map.png";

    struct nk_colorf bgColor = { 0.1f, 0.18f, 0.24f, 1.0f };
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        nk_glfw3_new_frame();

        glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 350, 350),
                NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {

            nk_layout_row_dynamic(ctx, 30, 2);
            flipY = nk_option_label(ctx, "Flip Y", flipY);

            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_combo_begin_label(ctx, greyscaleOptionNames[static_cast<int>(selectedGreyscaleOption)], nk_vec2(nk_widget_width(ctx), 200))) {
                nk_layout_row_dynamic(ctx, 25, 1);
                for (int i = 0; i < sinm_greyscale_count; ++i) {
                    if (nk_combo_item_label(ctx, greyscaleOptionNames[i], NK_TEXT_LEFT)) {
                        selectedGreyscaleOption = static_cast<sinm_greyscale_type>(i);
                    }
                }
                nk_combo_end(ctx);
            }

            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "Apply")) {
                BEGIN_TIMER(regenerate)
                regenerate_normal_map_layers(normalMapLayers, albedoImage, flipY);
                END_TIMER(regenerate)

                //normalMapResultTexIndex = nk_glfw3_create_texture(normalMapResult.pixels.data(), normalMapResult.w, normalMapResult.h);
                //normalMapResultImage = nk_image_id(normalMapResultTexIndex);
            }

            nk_layout_row_static(ctx, 30, 300, 2);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, filenameInputBuffer, sizeof(filenameInputBuffer) - 1, nk_filter_default);

            nk_layout_row_static(ctx, 30, 80, 1);
            nk_style_button button = {};
            if (nk_button_label(ctx, "Save")) {
                //stbi_write_png(filenameInputBuffer, normalMapResult.w, normalMapResult.h, 4, normalMapResult.pixels.data(), 0);
            }

            nk_layout_row_static(ctx, 30, 250, 1);
            if (nk_button_label(ctx, "Add Layer")) {
                auto map = create_normal_map_layer(albedoImage);
                normalMapLayers.push_back(map);
                normalMapSettings.push_back(map.settings);
            }
        }
        nk_end(ctx);

        std::vector<normal_map_layer> changedLayers;
        int layerNumber = 0;
        for (auto& layer : normalMapLayers) {

            std::string name = fmt::format("Layer {}", layerNumber);
            if (nk_begin(ctx, name.c_str(), nk_rect(250, 150, 230, 250),
                    NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {

                nk_layout_row_dynamic(ctx, 25, 1);
                nk_property_float(ctx, "Scale", 1, &layer.settings.scale, 100, 0.25f, 0.5f);

                nk_layout_row_dynamic(ctx, 25, 1);
                nk_property_int(ctx, "Blur Passes", 0, &layer.settings.blurPasses, 100, 0.25f, 0.5f);

                struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
                struct nk_rect total_space = nk_window_get_content_region(ctx);

                total_space.y += 80;
                total_space.h -= 80;
                total_space.w = total_space.h;
                nk_draw_image(canvas, total_space, &layer.nkImage, nk_white);
            }

            if (layer.settings != normalMapSettings[layerNumber]) {
                changedLayers.push_back(layer);
            }

            normalMapSettings[layerNumber] = layer.settings;

            layerNumber++;
            nk_end(ctx);
        }

        if (changedLayers.size() > 0) {
            regenerate_normal_map_layers(changedLayers, albedoImage, flipY);
            BEGIN_TIMER(compositing)
            generate_normal_map_composite(normalMap, normalMapLayers);
            END_TIMER(compositing)
        }

        if (nk_begin(ctx, "Albedo", nk_rect(500, 700, 230, 250),
                NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
            struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
            struct nk_rect total_space = nk_window_get_content_region(ctx);
            total_space.h = total_space.w;
            nk_draw_image(canvas, total_space, &albedoMapImage, nk_white);
        }
        nk_end(ctx);
        if (nk_begin(ctx, "result", nk_rect(500, 500, 230, 250),
                NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
            struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
            struct nk_rect total_space = nk_window_get_content_region(ctx);
            total_space.h = total_space.w;
            nk_draw_image(canvas, total_space, &normalMapResultImage, nk_white);
        }
        nk_end(ctx);
        nk_glfw3_render(NK_ANTI_ALIASING_ON);
        glfwSwapBuffers(window);

        assert(!glsys::report_errors());
    }

    nk_glfw3_shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}