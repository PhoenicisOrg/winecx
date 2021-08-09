/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_asm(void);
extern void func_core(void);
extern void func_effect(void);
extern void func_line(void);
extern void func_math(void);
extern void func_mesh(void);
extern void func_shader(void);
extern void func_surface(void);
extern void func_texture(void);
extern void func_volume(void);
extern void func_xfile(void);

const struct test winetest_testlist[] =
{
    { "asm", func_asm },
    { "core", func_core },
    { "effect", func_effect },
    { "line", func_line },
    { "math", func_math },
    { "mesh", func_mesh },
    { "shader", func_shader },
    { "surface", func_surface },
    { "texture", func_texture },
    { "volume", func_volume },
    { "xfile", func_xfile },
    { 0, 0 }
};
