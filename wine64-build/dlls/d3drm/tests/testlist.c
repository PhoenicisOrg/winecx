/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_d3drm(void);
extern void func_vector(void);

const struct test winetest_testlist[] =
{
    { "d3drm", func_d3drm },
    { "vector", func_vector },
    { 0, 0 }
};
