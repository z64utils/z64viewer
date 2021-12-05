#ifndef Z64_SHADER_H_INCLUDED
#define Z64_SHADER_H_INCLUDED

typedef struct Shader Shader; /* opaque */

Shader *Shader_new(void);
void Shader_update(Shader *s, const char *vs, const char *fs);
void Shader_use(Shader *s);
void Shader_delete(Shader *s);

void Shader_setMat4(Shader *s, const char *name, const void *m);
void Shader_setVec2(Shader *s, const char *name, float v0, float v1);
void Shader_setVec3(Shader *s, const char *name, float v0, float v1, float v2);
void Shader_setInt(Shader *s, const char *name, int i);

#endif

