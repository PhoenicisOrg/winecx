/* Automatically generated by make depend; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_address(void);
extern void func_discovery(void);
extern void func_memory(void);
extern void func_msgparams(void);
extern void func_xml(void);

const struct test winetest_testlist[] =
{
    { "address", func_address },
    { "discovery", func_discovery },
    { "memory", func_memory },
    { "msgparams", func_msgparams },
    { "xml", func_xml },
    { 0, 0 }
};