/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_analyzer(void);
extern void func_font(void);
extern void func_layout(void);

const struct test winetest_testlist[] =
{
    { "analyzer", func_analyzer },
    { "font", func_font },
    { "layout", func_layout },
    { 0, 0 }
};
