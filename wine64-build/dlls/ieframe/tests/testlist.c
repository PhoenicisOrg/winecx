/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_ie(void);
extern void func_intshcut(void);
extern void func_webbrowser(void);

const struct test winetest_testlist[] =
{
    { "ie", func_ie },
    { "intshcut", func_intshcut },
    { "webbrowser", func_webbrowser },
    { 0, 0 }
};
