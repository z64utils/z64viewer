#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 *
 * private
 *
 */
struct Shader {
	GLuint id;
};

static void cleanup(Shader* s) {
	if (!s)
		return;
	
	if (s->id)
		glDeleteProgram(s->id);
}

static void assertOpenGlCompilation(GLuint id, const char* type) {
	GLint success;
	GLchar log[1024];
	
	if (!strcmp(type, "PROGRAM")) {
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(id, sizeof(log) / sizeof(log[0]), 0, log);
			fprintf(stderr, "program linker error:\n%s\n", log);
		}
	} else {
		glGetShaderiv(id, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(id, sizeof(log) / sizeof(log[0]), 0, log);
			fprintf(stderr, "shader '%s' compiler error:\n%s\n", type, log);
		}
	}
}

/*
 *
 * public
 *
 */

void Shader_update(Shader* s, const char* vs, const char* fs) {
	GLuint v;
	GLuint f;
	
	if (!s || !vs || !fs)
		return;
	
	cleanup(s);
	
	/* compile vertex shader */
	v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vs, 0);
	glCompileShader(v);
	assertOpenGlCompilation(v, "VERTEX");
	
	/* compile fragment shader */
	f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fs, 0);
	glCompileShader(f);
	assertOpenGlCompilation(f, "FRAGMENT");
	
	/* link shader program */
	s->id = glCreateProgram();
	glAttachShader(s->id, v);
	glAttachShader(s->id, f);
	glLinkProgram(s->id);
	assertOpenGlCompilation(s->id, "PROGRAM");
	
	/* cleanup */
	glDeleteShader(v);
	glDeleteShader(f);
}

bool Shader_use(Shader* s) {
	static Shader* prev;
	
	if (s == prev)
		return false;
	
	if (s)
		glUseProgram(s->id);
	
	prev = s;
	
	return true;
}

Shader* Shader_new(void) {
	Shader* s = calloc(1, sizeof(*s));
	
	if (!s)
		return 0;
	
	return s;
}

void Shader_delete(Shader* s) {
	if (!s)
		return;
	
	cleanup(s);
	
	free(s);
}

void Shader_setInt(Shader* s, const char* name, int v) {
	glUniform1i(glGetUniformLocation(s->id, name), v);
}

void Shader_setFloat(Shader* s, const char* name, float v) {
	glUniform1f(glGetUniformLocation(s->id, name), v);
}

void Shader_setVec2(Shader* s, const char* name, float v0, float v1) {
	glUniform2f(glGetUniformLocation(s->id, name), v0, v1);
}

void Shader_setVec3(Shader* s, const char* name, float v0, float v1, float v2) {
	glUniform3f(glGetUniformLocation(s->id, name), v0, v1, v2);
}

void Shader_setVec4(Shader* s, const char* name, float v0, float v1, float v2, float v3) {
	glUniform4f(glGetUniformLocation(s->id, name), v0, v1, v2, v3);
}

void Shader_setMat4(Shader* s, const char* name, const void* m) {
	glUniformMatrix4fv(glGetUniformLocation(s->id, name), 1, GL_FALSE, m);
}
