#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <string_view>
#include <stdexcept>
#include <iostream>

std::string to_string(std::string_view str)
{
	return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message)
{
	throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error)
{
	throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

GLuint create_shader(GLenum shader_type, const char* shader_source)
{
    auto shader_object = glCreateShader(shader_type);
    if (!shader_object)
        throw std::runtime_error("Incorrect shader type!");

    glShaderSource(shader_object, 1, &shader_source, nullptr);

    glCompileShader(shader_object);

    GLint status;
    glGetShaderiv(shader_object, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLsizei buff_size, log_len = 0;
        glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &buff_size);
        std::string log(buff_size, '\0');
        glGetShaderInfoLog(shader_object, buff_size, &log_len, const_cast<GLchar *>(log.c_str()));

        throw std::runtime_error("Incorrect shader source!\n"
                                 "Got error: " + log);
    }

    return shader_object;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    auto program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    GLint status;
    glGetShaderiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLsizei buff_size, log_len = 0;
        glGetShaderiv(program, GL_INFO_LOG_LENGTH, &buff_size);
        std::string log(buff_size, '\0');
        glGetShaderInfoLog(program, buff_size, &log_len, const_cast<GLchar *>(log.c_str()));

        throw std::runtime_error("Can`t attach vertex shader!\n"
                                 "Got error: " + log);
    }
    return program;
}

int main() try
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		sdl2_fail("SDL_Init: ");

	SDL_Window * window = SDL_CreateWindow("Graphics course practice 1",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		800, 600,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

	if (!window)
		sdl2_fail("SDL_CreateWindow: ");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	if (!gl_context)
		sdl2_fail("SDL_GL_CreateContext: ");

	if (auto result = glewInit(); result != GLEW_NO_ERROR)
		glew_fail("glewInit: ", result);

	if (!GLEW_VERSION_3_3)
		throw std::runtime_error("OpenGL 3.3 is not supported");

    const char* fragment_source = R"(#version 330 core
layout (location = 0) out vec4 out_color;
in vec3 color;
void main()
{
float pos_x = color.x, pos_y = color.y;
int x_res = int(floor(pos_x * 16 + 16)), y_res = int(floor(pos_y * 16 + 16));
if ((x_res + y_res) % 2 == 0)
out_color = vec4(0.0, 0.0, 0.0, 1.0);
else
out_color = vec4(1.0, 1.0, 1.0, 1.0);
}
)";
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_source);

    const char* vertex_source = R"(#version 330 core
const vec2 VERTICES[3] = vec2[3](
vec2(0.0, 0.0),
vec2(1.0, 0.0),
vec2(0.0, 1.0)
);
out vec3 color;
void main()
{
gl_Position = vec4(VERTICES[gl_VertexID], 0.0, 1.0);
color = vec3(gl_Position.x, gl_Position.y, 0.0);
}
)";

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_source);
    auto program_shader = create_program(vertex_shader, fragment_shader);

    GLuint arr;
    glGenVertexArrays(1, &arr);

	glClearColor(0.8f, 0.8f, 1.f, 0.f);

	bool running = true;
	while (running)
	{
		for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
		{
		case SDL_QUIT:
			running = false;
			break;
		}

		if (!running)
			break;

		glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program_shader);
        glBindVertexArray(arr);
        glDrawArrays(GL_TRIANGLES, 0, 3);

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}