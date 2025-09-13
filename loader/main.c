/*
 * Emulator initialisation code
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

#ifdef __APPLE__ /* CrossOver Hack 13438 */
#include <crt_externs.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#endif

#include "main.h"

#if defined(__APPLE__) && defined(__x86_64__) && !defined(HAVE_WINE_PRELOADER)

/* Not using the preloader on x86_64:
 * Reserve the same areas as the preloader does, but using zero-fill sections
 * (the only way to prevent system frameworks from using them, including allocations
 * before main() runs).
 */
__asm__(".zerofill WINE_RESERVE,WINE_RESERVE");
static char __wine_reserve[0x1fffff000] __attribute__((section("WINE_RESERVE, WINE_RESERVE")));

__asm__(".zerofill WINE_TOP_DOWN,WINE_TOP_DOWN");
static char __wine_top_down[0x001ff0000] __attribute__((section("WINE_TOP_DOWN, WINE_TOP_DOWN")));

static const struct wine_preload_info preload_info[] =
{
    { __wine_reserve,  sizeof(__wine_reserve)  }, /*         0x1000 -    0x200000000: low 8GB */
    { __wine_top_down, sizeof(__wine_top_down) }, /* 0x7ff000000000 - 0x7ff001ff0000: top-down allocations + virtual heap */
    { 0, 0 }                                      /* end of list */
};

const __attribute((visibility("default"))) struct wine_preload_info *wine_main_preload_info = preload_info;

static void init_reserved_areas(void)
{
    int i;

    for (i = 0; wine_main_preload_info[i].size != 0; i++)
    {
        /* Match how the preloader maps reserved areas: */
        mmap(wine_main_preload_info[i].addr, wine_main_preload_info[i].size, PROT_NONE,
             MAP_FIXED | MAP_NORESERVE | MAP_PRIVATE | MAP_ANON, -1, 0);
    }
}

#else

/* the preloader will set this variable */
const __attribute((visibility("default"))) struct wine_preload_info *wine_main_preload_info = NULL;

static void init_reserved_areas(void)
{
}

#endif

/* canonicalize path and return its directory name */
static char *realpath_dirname( const char *name )
{
    char *p, *fullpath = realpath( name, NULL );

    if (fullpath)
    {
        p = strrchr( fullpath, '/' );
        if (p == fullpath) p++;
        if (p) *p = 0;
    }
    return fullpath;
}

/* if string ends with tail, remove it */
static char *remove_tail( const char *str, const char *tail )
{
    size_t len = strlen( str );
    size_t tail_len = strlen( tail );
    char *ret;

    if (len < tail_len) return NULL;
    if (strcmp( str + len - tail_len, tail )) return NULL;
    ret = malloc( len - tail_len + 1 );
    memcpy( ret, str, len - tail_len );
    ret[len - tail_len] = 0;
    return ret;
}

/* build a path from the specified dir and name */
static char *build_path( const char *dir, const char *name )
{
    size_t len = strlen( dir );
    char *ret = malloc( len + strlen( name ) + 2 );

    memcpy( ret, dir, len );
    if (len && ret[len - 1] != '/') ret[len++] = '/';
    strcpy( ret + len, name );
    return ret;
}

/* build a path with the relative dir from 'from' to 'dest' appended to base */
static char *build_relative_path( const char *base, const char *from, const char *dest )
{
    const char *start;
    char *ret;
    unsigned int dotdots = 0;

    for (;;)
    {
        while (*from == '/') from++;
        while (*dest == '/') dest++;
        start = dest;  /* save start of next path element */
        if (!*from) break;

        while (*from && *from != '/' && *from == *dest) { from++; dest++; }
        if ((!*from || *from == '/') && (!*dest || *dest == '/')) continue;

        do  /* count remaining elements in 'from' */
        {
            dotdots++;
            while (*from && *from != '/') from++;
            while (*from == '/') from++;
        }
        while (*from);
        break;
    }

    ret = malloc( strlen(base) + 3 * dotdots + strlen(start) + 2 );
    strcpy( ret, base );
    while (dotdots--) strcat( ret, "/.." );

    if (!start[0]) return ret;
    strcat( ret, "/" );
    strcat( ret, start );
    return ret;
}

static const char *get_self_exe( char *argv0 )
{
#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    return "/proc/self/exe";
#elif defined (__FreeBSD__) || defined(__DragonFly__)
    static int pathname[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t path_size = PATH_MAX;
    char *path = malloc( path_size );
    if (path && !sysctl( pathname, sizeof(pathname)/sizeof(pathname[0]), path, &path_size, NULL, 0 ))
        return path;
    free( path );
#endif

    if (!strchr( argv0, '/' )) /* search in PATH */
    {
        char *p, *path = getenv( "PATH" );

        if (!path || !(path = strdup(path))) return NULL;
        for (p = strtok( path, ":" ); p; p = strtok( NULL, ":" ))
        {
            char *name = build_path( p, argv0 );
            if (!access( name, X_OK ))
            {
                free( path );
                return name;
            }
            free( name );
        }
        free( path );
        return NULL;
    }
    return argv0;
}

static void *try_dlopen( const char *dir, const char *name )
{
    char *path = build_path( dir, name );
    void *handle = dlopen( path, RTLD_NOW );
    free( path );
    return handle;
}

static void *load_ntdll( char *argv0 )
{
#ifdef __i386__
#define SO_DIR "i386-unix/"
#elif defined(__x86_64__)
#define SO_DIR "x86_64-unix/"
#elif defined(__arm__)
#define SO_DIR "arm-unix/"
#elif defined(__aarch64__)
#define SO_DIR "aarch64-unix/"
#else
#define SO_DIR ""
#endif
    const char *self = get_self_exe( argv0 );
    char *path, *p;
    void *handle = NULL;

    if (self && ((path = realpath_dirname( self ))))
    {
        if ((p = remove_tail( path, "/loader" )))
            handle = try_dlopen( p, "dlls/ntdll/ntdll.so" );
        else if ((p = build_relative_path( path, BINDIR, LIBDIR )))
            handle = try_dlopen( p, "wine/" SO_DIR "ntdll.so" );
        free( p );
        free( path );
    }

    if (!handle && (path = getenv( "WINEDLLPATH" )))
    {
        path = strdup( path );
        for (p = strtok( path, ":" ); p; p = strtok( NULL, ":" ))
        {
            handle = try_dlopen( p, SO_DIR "ntdll.so" );
            if (!handle) handle = try_dlopen( p, "ntdll.so" );
            if (handle) break;
        }
        free( path );
    }

    if (!handle && !self) handle = try_dlopen( LIBDIR, "wine/" SO_DIR "ntdll.so" );

    return handle;
}

#ifdef __APPLE__
#define min(a,b)   (((a) < (b)) ? (a) : (b))
/***********************************************************************
 *           apple_override_bundle_name
 *
 * Rewrite the bundle name in the Info.plist embedded in the loader.
 * This is the only way to control the title of the application menu
 * when using the Mac driver.  The GUI frameworks call down into Core
 * Foundation to get the bundle name for that.
 *
 * CrossOver Hack 13438
 */
static void apple_override_bundle_name( int argc, char *argv[] )
{
    char* info_plist;
    unsigned long remaining;
    static const char prefix[] = "<key>CFBundleName</key>\n    <string>";
    const size_t prefix_len = strlen(prefix);
    static const char suffix[] = "</string>";
    const size_t suffix_len = strlen(suffix);
    static const char padding[] = "<!-- bundle name padding -->";
    const size_t padding_len = strlen(padding);
    char* bundle_name;
    const char* p;
    size_t bundle_name_len, max_bundle_name_len;
    vm_address_t start, end;
    const char* new_bundle_name = getenv("WINEPRELOADERAPPNAME");
    size_t new_bundle_name_len;

    unsetenv("WINEPRELOADERAPPNAME");

    if (!new_bundle_name && argc < 2)
        return;

    info_plist = (char *)getsectiondata(_NSGetMachExecuteHeader(), "__TEXT", "__info_plist", &remaining);
    if (!info_plist || !remaining)
        return;

    bundle_name = strnstr(info_plist, prefix, remaining);
    if (!bundle_name)
        return;

    bundle_name += prefix_len;
    remaining -= bundle_name - info_plist;
    p = strnstr(bundle_name, suffix, remaining);
    if (!p)
        return;

    bundle_name_len = p - bundle_name;
    remaining -= bundle_name_len + suffix_len;

    max_bundle_name_len = bundle_name_len;
    if (padding_len <= remaining &&
        !memcmp(bundle_name + bundle_name_len + suffix_len, padding, padding_len))
        max_bundle_name_len += padding_len;

    if (!new_bundle_name)
    {
        new_bundle_name = argv[1];
        if ((p = strrchr(new_bundle_name, '\\'))) new_bundle_name = p + 1;
        if ((p = strrchr(new_bundle_name, '/'))) new_bundle_name = p + 1;
        if (strspn(new_bundle_name, "0123456789abcdefABCDEF") == 32 &&
            new_bundle_name[32] == '.')
            new_bundle_name += 33;
        if ((p = strrchr(new_bundle_name, '.')) && p != new_bundle_name)
            new_bundle_name_len = p - new_bundle_name;
        else
            new_bundle_name_len = strlen(new_bundle_name);
    }
    else
        new_bundle_name_len = strlen(new_bundle_name);

    if (!new_bundle_name_len)
        return;

    start = (vm_address_t)bundle_name;
    end = (vm_address_t)(bundle_name + max_bundle_name_len + suffix_len);
    start &= ~(getpagesize() - 1);
    end = (end + getpagesize() - 1) & ~(getpagesize() - 1);
    if (vm_protect(mach_task_self(), start, end - start, 0,
                   VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE|VM_PROT_COPY) == KERN_SUCCESS)
    {
        size_t copy_len = min(new_bundle_name_len, max_bundle_name_len);
        memcpy(bundle_name, new_bundle_name, copy_len);
        memcpy(bundle_name + copy_len, suffix, suffix_len);
        if (copy_len < max_bundle_name_len)
            memset(bundle_name + copy_len + suffix_len, ' ', max_bundle_name_len - copy_len);
        vm_protect(mach_task_self(), start, end - start, 0, VM_PROT_READ|VM_PROT_EXECUTE);
    }
}
#endif

/**********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{
    void *handle;

    init_reserved_areas();

#ifdef __APPLE__ /* CrossOver Hack 13438 */
    apple_override_bundle_name(argc, argv);
#endif

    if ((handle = load_ntdll( argv[0] )))
    {
        void (*init_func)(int, char **) = dlsym( handle, "__wine_main" );
        if (init_func) init_func( argc, argv );
        fprintf( stderr, "wine: __wine_main function not found in ntdll.so\n" );
        exit(1);
    }

    fprintf( stderr, "wine: could not load ntdll.so: %s\n", dlerror() );
    pthread_detach( pthread_self() );  /* force importing libpthread for OpenGL */
    exit(1);
}
