/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_dmime(void);
extern void func_performance(void);

const struct test winetest_testlist[] =
{
    { "dmime", func_dmime },
    { "performance", func_performance },
    { 0, 0 }
};
