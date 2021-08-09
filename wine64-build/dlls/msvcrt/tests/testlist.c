/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_cpp(void);
extern void func_data(void);
extern void func_dir(void);
extern void func_environ(void);
extern void func_file(void);
extern void func_headers(void);
extern void func_heap(void);
extern void func_locale(void);
extern void func_misc(void);
extern void func_printf(void);
extern void func_scanf(void);
extern void func_signal(void);
extern void func_string(void);
extern void func_time(void);

const struct test winetest_testlist[] =
{
    { "cpp", func_cpp },
    { "data", func_data },
    { "dir", func_dir },
    { "environ", func_environ },
    { "file", func_file },
    { "headers", func_headers },
    { "heap", func_heap },
    { "locale", func_locale },
    { "misc", func_misc },
    { "printf", func_printf },
    { "scanf", func_scanf },
    { "signal", func_signal },
    { "string", func_string },
    { "time", func_time },
    { 0, 0 }
};
