#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int glswInit();
int glswShutdown();
int glswSetPath(const char* pathPrefix, const char* pathSuffix);
///Ionut: added a line offset for debugging GLSL easier
const char* glswGetShader(const char* effectKey, int offset);
const char* glswGetError();
int glswAddDirectiveToken(const char* token, const char* directive);

#ifdef __cplusplus
}
#endif
