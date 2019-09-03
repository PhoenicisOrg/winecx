/*
 * Unit test suite for ntdll path functions
 *
 * Copyright 2002 Alexandre Julliard
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

#include "ntdll_test.h"
#include "winnls.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlUnicodeToMultiByteN)(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
static UINT (WINAPI *pRtlDetermineDosPathNameType_U)( PCWSTR path );
static ULONG (WINAPI *pRtlIsDosDeviceName_U)( PCWSTR dos_name );
static NTSTATUS (WINAPI *pRtlOemStringToUnicodeString)(UNICODE_STRING *, const STRING *, BOOLEAN );
static BOOLEAN (WINAPI *pRtlIsNameLegalDOS8Dot3)(const UNICODE_STRING*,POEM_STRING,PBOOLEAN);
static DWORD (WINAPI *pRtlGetFullPathName_U)(const WCHAR*,ULONG,WCHAR*,WCHAR**);
static BOOLEAN (WINAPI *pRtlDosPathNameToNtPathName_U)(const WCHAR*, UNICODE_STRING*, WCHAR**, CURDIR*);
static NTSTATUS (WINAPI *pRtlDosPathNameToNtPathName_U_WithStatus)(const WCHAR*, UNICODE_STRING*, WCHAR**, CURDIR*);

static void test_RtlDetermineDosPathNameType_U(void)
{
    struct test
    {
        const char *path;
        UINT ret;
    };

    static const struct test tests[] =
    {
        { "\\\\foo", 1 },
        { "//foo", 1 },
        { "\\/foo", 1 },
        { "/\\foo", 1 },
        { "\\\\", 1 },
        { "//", 1 },
        { "c:\\foo", 2 },
        { "c:/foo", 2 },
        { "c://foo", 2 },
        { "c:\\", 2 },
        { "c:/", 2 },
        { "c:foo", 3 },
        { "c:f\\oo", 3 },
        { "c:foo/bar", 3 },
        { "\\foo", 4 },
        { "/foo", 4 },
        { "\\", 4 },
        { "/", 4 },
        { "foo", 5 },
        { "", 5 },
        { "\0:foo", 5 },
        { "\\\\.\\foo", 6 },
        { "//./foo", 6 },
        { "/\\./foo", 6 },
        { "\\\\.foo", 1 },
        { "//.foo", 1 },
        { "\\\\.", 7 },
        { "//.", 7 },
        { "\\\\?\\foo", 6 },
        { "//?/foo", 6 },
        { "/\\?/foo", 6 },
        { "\\\\?foo", 1 },
        { "//?foo", 1 },
        { "\\\\?", 7 },
        { "//?", 7 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[MAX_PATH];
    UINT ret;

    if (!pRtlDetermineDosPathNameType_U)
    {
        win_skip("RtlDetermineDosPathNameType_U is not available\n");
        return;
    }

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlDetermineDosPathNameType_U( buffer );
        ok( ret == test->ret, "Wrong result %d/%d for %s\n", ret, test->ret, test->path );
    }
}


static void test_RtlIsDosDeviceName_U(void)
{
    struct test
    {
        const char *path;
        WORD pos;
        WORD len;
        BOOL fails;
    };

    static const struct test tests[] =
    {
        { "\\\\.\\CON",    8, 6, TRUE },  /* fails on win8 */
        { "\\\\.\\con",    8, 6, TRUE },  /* fails on win8 */
        { "\\\\.\\CON2",   0, 0 },
        { "",              0, 0 },
        { "\\\\foo\\nul",  0, 0 },
        { "c:\\nul:",      6, 6 },
        { "c:\\nul\\",     0, 0 },
        { "c:\\nul\\foo",  0, 0 },
        { "c:\\nul::",     6, 6, TRUE },  /* fails on nt4 */
        { "c:\\nul::::::", 6, 6, TRUE },  /* fails on nt4 */
        { "c:prn     ",    4, 6 },
        { "c:prn.......",  4, 6 },
        { "c:prn... ...",  4, 6 },
        { "c:NUL  ....  ", 4, 6, TRUE },  /* fails on nt4 */
        { "c: . . .",      0, 0 },
        { "c:",            0, 0 },
        { " . . . :",      0, 0 },
        { ":",             0, 0 },
        { "c:nul. . . :",  4, 6 },
        { "c:nul . . :",   4, 6, TRUE },  /* fails on nt4 */
        { "c:nul0",        0, 0 },
        { "c:prn:aaa",     4, 6, TRUE },  /* fails on win9x */
        { "c:PRN:.txt",    4, 6 },
        { "c:aux:.txt...", 4, 6 },
        { "c:prn:.txt:",   4, 6 },
        { "c:nul:aaa",     4, 6, TRUE },  /* fails on win9x */
        { "con:",          0, 6 },
        { "lpt1:",         0, 8 },
        { "c:com5:",       4, 8 },
        { "CoM4:",         0, 8 },
        { "lpt9:",         0, 8 },
        { "c:\\lpt0.txt",  0, 0 },
        { "c:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\nul.txt", 1000, 6 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[2000];
    ULONG ret;

    if (!pRtlIsDosDeviceName_U)
    {
        win_skip("RtlIsDosDeviceName_U is not available\n");
        return;
    }

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlIsDosDeviceName_U( buffer );
        ok( ret == MAKELONG( test->len, test->pos ) ||
            (test->fails && broken( ret == 0 )),
            "Wrong result (%d,%d)/(%d,%d) for %s\n",
            HIWORD(ret), LOWORD(ret), test->pos, test->len, test->path );
    }
}

static void test_RtlIsNameLegalDOS8Dot3(void)
{
    struct test
    {
        const char *path;
        BOOLEAN result;
        BOOLEAN spaces;
    };

    static const struct test tests[] =
    {
        { "12345678",     TRUE,  FALSE },
        { "123 5678",     TRUE,  TRUE  },
        { "12345678.",    FALSE, 2 /*not set*/ },
        { "1234 678.",    FALSE, 2 /*not set*/ },
        { "12345678.a",   TRUE,  FALSE },
        { "12345678.a ",  FALSE, 2 /*not set*/ },
        { "12345678.a c", TRUE,  TRUE  },
        { " 2345678.a ",  FALSE, 2 /*not set*/ },
        { "1 345678.abc", TRUE,  TRUE },
        { "1      8.a c", TRUE,  TRUE },
        { "1 3 5 7 .abc", FALSE, 2 /*not set*/ },
        { "12345678.  c", TRUE,  TRUE },
        { "123456789.a",  FALSE, 2 /*not set*/ },
        { "12345.abcd",   FALSE, 2 /*not set*/ },
        { "12345.ab d",   FALSE, 2 /*not set*/ },
        { ".abc",         FALSE, 2 /*not set*/ },
        { "12.abc.d",     FALSE, 2 /*not set*/ },
        { ".",            TRUE,  FALSE },
        { "..",           TRUE,  FALSE },
        { "...",          FALSE, 2 /*not set*/ },
        { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", FALSE, 2 /*not set*/ },
        { NULL, 0 }
    };

    const struct test *test;
    UNICODE_STRING ustr;
    OEM_STRING oem, oem_ret;
    WCHAR buffer[200];
    char buff2[12];
    BOOLEAN ret, spaces;

    if (!pRtlIsNameLegalDOS8Dot3)
    {
        win_skip("RtlIsNameLegalDOS8Dot3 is not available\n");
        return;
    }

    ustr.MaximumLength = sizeof(buffer);
    ustr.Buffer = buffer;
    for (test = tests; test->path; test++)
    {
        char path[100];
        strcpy(path, test->path);
        oem.Buffer = path;
        oem.Length = strlen(test->path);
        oem.MaximumLength = oem.Length + 1;
        pRtlOemStringToUnicodeString( &ustr, &oem, FALSE );
        spaces = 2;
        oem_ret.Length = oem_ret.MaximumLength = sizeof(buff2);
        oem_ret.Buffer = buff2;
        ret = pRtlIsNameLegalDOS8Dot3( &ustr, &oem_ret, &spaces );
        ok( ret == test->result, "Wrong result %d/%d for '%s'\n", ret, test->result, test->path );
        ok( spaces == test->spaces, "Wrong spaces value %d/%d for '%s'\n", spaces, test->spaces, test->path );
        if (strlen(test->path) <= 12)
        {
            char str[13];
            int i;
            strcpy( str, test->path );
            for (i = 0; str[i]; i++) str[i] = toupper(str[i]);
            ok( oem_ret.Length == strlen(test->path), "Wrong length %d/%d for '%s'\n",
                oem_ret.Length, lstrlenA(test->path), test->path );
            ok( !memcmp( oem_ret.Buffer, str, oem_ret.Length ),
                "Wrong string '%.*s'/'%s'\n", oem_ret.Length, oem_ret.Buffer, str );
        }
    }
}
static void test_RtlGetFullPathName_U(void)
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR deadbeefW[] = {'d','e','a','d','b','e','e','f',0};

    struct test
    {
        const char *path;
        const char *rname;
        const char *rfile;
        const char *alt_rname;
        const char *alt_rfile;
    };

    static const struct test tests[] =
        {
            { "c:/test",                     "c:\\test",         "test"},
            { "c:/test/",                    "c:\\test\\",       NULL},
            { "c:/test     ",                "c:\\test",         "test"},
            { "c:/test.",                    "c:\\test",         "test"},
            { "c:/test  ....   ..   ",       "c:\\test",         "test"},
            { "c:/test/  ....   ..   ",      "c:\\test\\",       NULL},
            { "c:/test/..",                  "c:\\",             NULL},
            { "c:/test/.. ",                 "c:\\test\\",       NULL},
            { "c:/TEST",                     "c:\\TEST",         "TEST"},
            { "c:/test/file",                "c:\\test\\file",   "file"},
            { "c:/test./file",               "c:\\test\\file",   "file"},
            { "c:/test.. /file",             "c:\\test.. \\file","file"},
            { "c:/test/././file",            "c:\\test\\file",   "file"},
            { "c:/test\\.\\.\\file",         "c:\\test\\file",   "file"},
            { "c:/test/\\.\\.\\file",        "c:\\test\\file",   "file"},
            { "c:/test\\\\.\\.\\file",       "c:\\test\\file",   "file"},
            { "c:/test\\test1\\..\\.\\file", "c:\\test\\file",   "file"},
            { "c:///test\\.\\.\\file//",     "c:\\test\\file\\", NULL,
                                             "c:\\test\\file",   "file"},  /* nt4 */
            { "c:///test\\..\\file\\..\\//", "c:\\",             NULL},
            { "c:/test../file",              "c:\\test.\\file",  "file",
                                             "c:\\test..\\file", "file"},  /* vista */
            { "c:\\test",                    "c:\\test",         "test"},
            { "C:\\test",                    "C:\\test",         "test"},
            { "c:/",                         "c:\\",             NULL},
            { "c:.",                         "C:\\windows",      "windows"},
            { "c:foo",                       "C:\\windows\\foo", "foo"},
            { "c:foo/bar",                   "C:\\windows\\foo\\bar", "bar"},
            { "c:./foo",                     "C:\\windows\\foo", "foo"},
            { "\\foo",                       "C:\\foo",          "foo"},
            { "foo",                         "C:\\windows\\foo", "foo"},
            { ".",                           "C:\\windows",      "windows"},
            { "..",                          "C:\\",             NULL},
            { "...",                         "C:\\windows\\",    NULL},
            { "./foo",                       "C:\\windows\\foo", "foo"},
            { "foo/..",                      "C:\\windows",      "windows"},
            { "AUX",                         "\\\\.\\AUX",       NULL},
            { "COM1",                        "\\\\.\\COM1",      NULL},
            { "?<>*\"|:",                    "C:\\windows\\?<>*\"|:", "?<>*\"|:"},

            { "\\\\foo",                     "\\\\foo",          NULL},
            { "//foo",                       "\\\\foo",          NULL},
            { "\\/foo",                      "\\\\foo",          NULL},
            { "//",                          "\\\\",             NULL},
            { "//foo/",                      "\\\\foo\\",        NULL},

            { "//.",                         "\\\\.\\",          NULL},
            { "//./",                        "\\\\.\\",          NULL},
            { "//.//",                       "\\\\.\\",          NULL},
            { "//./foo",                     "\\\\.\\foo",       "foo"},
            { "//./foo/",                    "\\\\.\\foo\\",     NULL},
            { "//./foo/bar",                 "\\\\.\\foo\\bar",  "bar"},
            { "//./foo/.",                   "\\\\.\\foo",       "foo"},
            { "//./foo/..",                  "\\\\.\\",          NULL},

            { "//?/",                        "\\\\?\\",          NULL},
            { "//?//",                       "\\\\?\\",          NULL},
            { "//?/foo",                     "\\\\?\\foo",       "foo"},
            { "//?/foo/",                    "\\\\?\\foo\\",     NULL},
            { "//?/foo/bar",                 "\\\\?\\foo\\bar",  "bar"},
            { "//?/foo/.",                   "\\\\?\\foo",       "foo"},
            { "//?/foo/..",                  "\\\\?\\",          NULL},

            /* RtlGetFullPathName_U() can't understand the global namespace prefix */
            { "\\??\\foo",                   "C:\\??\\foo",      "foo"},
            { 0 }
        };

    const struct test *test;
    WCHAR pathbufW[2*MAX_PATH], rbufferW[MAX_PATH];
    char rbufferA[MAX_PATH], rfileA[MAX_PATH], curdir[MAX_PATH];
    ULONG ret;
    WCHAR *file_part;
    DWORD reslen;
    UINT len;

    GetCurrentDirectoryA(sizeof(curdir), curdir);
    SetCurrentDirectoryA("C:\\windows\\");

    file_part = (WCHAR *)0xdeadbeef;
    lstrcpyW(rbufferW, deadbeefW);
    ret = pRtlGetFullPathName_U(NULL, MAX_PATH, rbufferW, &file_part);
    ok(!ret, "Expected RtlGetFullPathName_U to return 0, got %u\n", ret);
    ok(!lstrcmpW(rbufferW, deadbeefW),
       "Expected the output buffer to be untouched, got %s\n", wine_dbgstr_w(rbufferW));
    ok(file_part == (WCHAR *)0xdeadbeef ||
       file_part == NULL, /* Win7 */
       "Expected file part pointer to be untouched, got %p\n", file_part);

    file_part = (WCHAR *)0xdeadbeef;
    lstrcpyW(rbufferW, deadbeefW);
    ret = pRtlGetFullPathName_U(emptyW, MAX_PATH, rbufferW, &file_part);
    ok(!ret, "Expected RtlGetFullPathName_U to return 0, got %u\n", ret);
    ok(!lstrcmpW(rbufferW, deadbeefW),
       "Expected the output buffer to be untouched, got %s\n", wine_dbgstr_w(rbufferW));
    ok(file_part == (WCHAR *)0xdeadbeef ||
       file_part == NULL, /* Win7 */
       "Expected file part pointer to be untouched, got %p\n", file_part);

    for (test = tests; test->path; test++)
    {
        len= strlen(test->rname) * sizeof(WCHAR);
        pRtlMultiByteToUnicodeN(pathbufW , sizeof(pathbufW), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlGetFullPathName_U( pathbufW,MAX_PATH, rbufferW, &file_part);
        ok( ret == len || (test->alt_rname && ret == strlen(test->alt_rname)*sizeof(WCHAR)),
            "Wrong result %d/%d for \"%s\"\n", ret, len, test->path );
        ok(pRtlUnicodeToMultiByteN(rbufferA,MAX_PATH,&reslen,rbufferW,(lstrlenW(rbufferW) + 1) * sizeof(WCHAR)) == STATUS_SUCCESS,
           "RtlUnicodeToMultiByteN failed\n");
        ok(!lstrcmpA(rbufferA,test->rname) || (test->alt_rname && !lstrcmpA(rbufferA,test->alt_rname)),
           "Got \"%s\" expected \"%s\"\n",rbufferA,test->rname);
        if (file_part)
        {
            ok(pRtlUnicodeToMultiByteN(rfileA,MAX_PATH,&reslen,file_part,(lstrlenW(file_part) + 1) * sizeof(WCHAR)) == STATUS_SUCCESS,
               "RtlUnicodeToMultiByteN failed\n");
            ok((test->rfile && !lstrcmpA(rfileA,test->rfile)) ||
               (test->alt_rfile && !lstrcmpA(rfileA,test->alt_rfile)),
               "Got \"%s\" expected \"%s\"\n",rfileA,test->rfile);
        }
        else
        {
            ok( !test->rfile, "Got NULL expected \"%s\"\n", test->rfile );
        }
    }

    SetCurrentDirectoryA(curdir);
}

static void test_RtlDosPathNameToNtPathName_U(void)
{
    static const WCHAR broken_global_prefix[] = {'\\','?','?','\\','C',':','\\','?','?'};

    char curdir[MAX_PATH];
    WCHAR path[MAX_PATH];
    UNICODE_STRING nameW;
    WCHAR *file_part;
    NTSTATUS status;
    BOOL ret, expect;
    int i;

    static const struct
    {
        const char *dos;
        const char *nt;
        int file_offset;    /* offset to file part */
        NTSTATUS status;
        NTSTATUS alt_status;
        int broken;
    }
    tests[] =
    {
        { "c:\\",           "\\??\\c:\\",                  -1, STATUS_SUCCESS },
        { "c:/",            "\\??\\c:\\",                  -1, STATUS_SUCCESS },
        { "c:/foo",         "\\??\\c:\\foo",                7, STATUS_SUCCESS },
        { "c:/foo.",        "\\??\\c:\\foo",                7, STATUS_SUCCESS },
        { "c:/foo/",        "\\??\\c:\\foo\\",             -1, STATUS_SUCCESS },
        { "c:/foo//",       "\\??\\c:\\foo\\",             -1, STATUS_SUCCESS },
        { "C:/foo",         "\\??\\C:\\foo",                7, STATUS_SUCCESS },
        { "C:/foo/bar",     "\\??\\C:\\foo\\bar",          11, STATUS_SUCCESS },
        { "C:/foo/bar",     "\\??\\C:\\foo\\bar",          11, STATUS_SUCCESS },
        { "c:.",            "\\??\\C:\\windows",            7, STATUS_SUCCESS },
        { "c:foo",          "\\??\\C:\\windows\\foo",      15, STATUS_SUCCESS },
        { "c:foo/bar",      "\\??\\C:\\windows\\foo\\bar", 19, STATUS_SUCCESS },
        { "c:./foo",        "\\??\\C:\\windows\\foo",      15, STATUS_SUCCESS },
        { "c:/./foo",       "\\??\\c:\\foo",                7, STATUS_SUCCESS },
        { "c:/foo/.",       "\\??\\c:\\foo",                7, STATUS_SUCCESS },
        { "c:/foo/./bar",   "\\??\\c:\\foo\\bar",          11, STATUS_SUCCESS },
        { "c:/foo/../bar",  "\\??\\c:\\bar",                7, STATUS_SUCCESS },
        { "\\foo",          "\\??\\C:\\foo",                7, STATUS_SUCCESS },
        { "foo",            "\\??\\C:\\windows\\foo",      15, STATUS_SUCCESS },
        { ".",              "\\??\\C:\\windows",            7, STATUS_SUCCESS },
        { "./",             "\\??\\C:\\windows\\",         -1, STATUS_SUCCESS },
        { "..",             "\\??\\C:\\",                  -1, STATUS_SUCCESS },
        { "...",            "\\??\\C:\\windows\\",         -1, STATUS_SUCCESS },
        { "./foo",          "\\??\\C:\\windows\\foo",      15, STATUS_SUCCESS },
        { "foo/..",         "\\??\\C:\\windows",            7, STATUS_SUCCESS },
        { "AUX" ,           "\\??\\AUX",                   -1, STATUS_SUCCESS },
        { "COM1" ,          "\\??\\COM1",                  -1, STATUS_SUCCESS },
        { "?<>*\"|:",       "\\??\\C:\\windows\\?<>*\"|:", 15, STATUS_SUCCESS },

        { "",   NULL, -1, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL, NULL, -1, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_PATH_NOT_FOUND },
        { " ",  NULL, -1, STATUS_OBJECT_NAME_INVALID, STATUS_OBJECT_PATH_NOT_FOUND },

        { "\\\\foo",        "\\??\\UNC\\foo",              -1, STATUS_SUCCESS },
        { "//foo",          "\\??\\UNC\\foo",              -1, STATUS_SUCCESS },
        { "\\/foo",         "\\??\\UNC\\foo",              -1, STATUS_SUCCESS },
        { "//",             "\\??\\UNC\\",                 -1, STATUS_SUCCESS },
        { "//foo/",         "\\??\\UNC\\foo\\",            -1, STATUS_SUCCESS },

        { "//.",            "\\??\\",                      -1, STATUS_SUCCESS },
        { "//./",           "\\??\\",                      -1, STATUS_SUCCESS },
        { "//.//",          "\\??\\",                      -1, STATUS_SUCCESS },
        { "//./foo",        "\\??\\foo",                    4, STATUS_SUCCESS },
        { "//./foo/",       "\\??\\foo\\",                 -1, STATUS_SUCCESS },
        { "//./foo/bar",    "\\??\\foo\\bar",               8, STATUS_SUCCESS },
        { "//./foo/.",      "\\??\\foo",                    4, STATUS_SUCCESS },
        { "//./foo/..",     "\\??\\",                      -1, STATUS_SUCCESS },

        { "//?",            "\\??\\",                      -1, STATUS_SUCCESS },
        { "//?/",           "\\??\\",                      -1, STATUS_SUCCESS },
        { "//?//",          "\\??\\",                      -1, STATUS_SUCCESS },
        { "//?/foo",        "\\??\\foo",                    4, STATUS_SUCCESS },
        { "//?/foo/",       "\\??\\foo\\",                 -1, STATUS_SUCCESS },
        { "//?/foo/bar",    "\\??\\foo\\bar",               8, STATUS_SUCCESS },
        { "//?/foo/.",      "\\??\\foo",                    4, STATUS_SUCCESS },
        { "//?/foo/..",     "\\??\\",                      -1, STATUS_SUCCESS },

        { "\\\\?",           "\\??\\",                     -1, STATUS_SUCCESS },
        { "\\\\?\\",         "\\??\\",                     -1, STATUS_SUCCESS },

        { "\\\\?\\/",        "\\??\\/",                     4, STATUS_SUCCESS },
        { "\\\\?\\foo",      "\\??\\foo",                   4, STATUS_SUCCESS },
        { "\\\\?\\foo/",     "\\??\\foo/",                  4, STATUS_SUCCESS },
        { "\\\\?\\foo/bar",  "\\??\\foo/bar",               4, STATUS_SUCCESS },
        { "\\\\?\\foo/.",    "\\??\\foo/.",                 4, STATUS_SUCCESS },
        { "\\\\?\\foo/..",   "\\??\\foo/..",                4, STATUS_SUCCESS },
        { "\\\\?\\\\",       "\\??\\\\",                   -1, STATUS_SUCCESS },
        { "\\\\?\\\\\\",     "\\??\\\\\\",                 -1, STATUS_SUCCESS },
        { "\\\\?\\foo\\",    "\\??\\foo\\",                -1, STATUS_SUCCESS },
        { "\\\\?\\foo\\bar", "\\??\\foo\\bar",              8, STATUS_SUCCESS },
        { "\\\\?\\foo\\.",   "\\??\\foo\\.",                8, STATUS_SUCCESS },
        { "\\\\?\\foo\\..",  "\\??\\foo\\..",               8, STATUS_SUCCESS },

        { "\\??",           "\\??\\C:\\??",                 7, STATUS_SUCCESS },
        { "\\??\\",         "\\??\\C:\\??\\",              -1, STATUS_SUCCESS },

        { "\\??\\/",        "\\??\\/",                      4, STATUS_SUCCESS },
        { "\\??\\foo",      "\\??\\foo",                    4, STATUS_SUCCESS },
        { "\\??\\foo/",     "\\??\\foo/",                   4, STATUS_SUCCESS },
        { "\\??\\foo/bar",  "\\??\\foo/bar",                4, STATUS_SUCCESS },
        { "\\??\\foo/.",    "\\??\\foo/.",                  4, STATUS_SUCCESS },
        { "\\??\\foo/..",   "\\??\\foo/..",                 4, STATUS_SUCCESS },
        { "\\??\\\\",       "\\??\\\\",                    -1, STATUS_SUCCESS },
        { "\\??\\\\\\",     "\\??\\\\\\",                  -1, STATUS_SUCCESS },
        { "\\??\\foo\\",    "\\??\\foo\\",                 -1, STATUS_SUCCESS },
        { "\\??\\foo\\bar", "\\??\\foo\\bar",               8, STATUS_SUCCESS },
        { "\\??\\foo\\.",   "\\??\\foo\\.",                 8, STATUS_SUCCESS },
        { "\\??\\foo\\..",  "\\??\\foo\\..",                8, STATUS_SUCCESS },
    };

    GetCurrentDirectoryA(sizeof(curdir), curdir);
    SetCurrentDirectoryA("C:\\windows\\");

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        MultiByteToWideChar(CP_ACP, 0, tests[i].dos, -1, path, ARRAY_SIZE(path));
        ret = pRtlDosPathNameToNtPathName_U(path, &nameW, &file_part, NULL);

        if (pRtlDosPathNameToNtPathName_U_WithStatus)
        {
            RtlFreeUnicodeString(&nameW);
            status = pRtlDosPathNameToNtPathName_U_WithStatus(path, &nameW, &file_part, NULL);
            ok(status == tests[i].status || status == tests[i].alt_status,
                "%s: Expected status %#x, got %#x.\n", tests[i].dos, tests[i].status, status);
        }

        expect = (tests[i].status == STATUS_SUCCESS);
        ok(ret == expect, "%s: Expected %#x, got %#x.\n", tests[i].dos, expect, ret);

        if (ret != TRUE) continue;

        if (!strncmp(tests[i].dos, "\\??\\", 4) && tests[i].dos[4] &&
            broken(!memcmp(nameW.Buffer, broken_global_prefix, sizeof(broken_global_prefix))))
        {
            /* Windows version prior to 2003 don't interpret the \??\ prefix */
            continue;
        }

        MultiByteToWideChar(CP_ACP, 0, tests[i].nt, -1, path, ARRAY_SIZE(path));
        ok(!lstrcmpW(nameW.Buffer, path), "%s: Expected %s, got %s.\n",
            tests[i].dos, tests[i].nt, wine_dbgstr_w(nameW.Buffer));

        if (tests[i].file_offset > 0)
            ok(file_part == nameW.Buffer + tests[i].file_offset,
                "%s: Expected file part %s, got %s.\n", tests[i].dos,
                wine_dbgstr_w(nameW.Buffer + tests[i].file_offset), wine_dbgstr_w(file_part));
        else
            ok(file_part == NULL, "%s: Expected NULL file part, got %s.\n",
                tests[i].dos, wine_dbgstr_w(file_part));

        RtlFreeUnicodeString(&nameW);
    }

    SetCurrentDirectoryA(curdir);
}

START_TEST(path)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    if (!mod)
    {
        win_skip("Not running on NT, skipping tests\n");
        return;
    }

    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlUnicodeToMultiByteN = (void *)GetProcAddress(mod,"RtlUnicodeToMultiByteN");
    pRtlDetermineDosPathNameType_U = (void *)GetProcAddress(mod,"RtlDetermineDosPathNameType_U");
    pRtlIsDosDeviceName_U = (void *)GetProcAddress(mod,"RtlIsDosDeviceName_U");
    pRtlOemStringToUnicodeString = (void *)GetProcAddress(mod,"RtlOemStringToUnicodeString");
    pRtlIsNameLegalDOS8Dot3 = (void *)GetProcAddress(mod,"RtlIsNameLegalDOS8Dot3");
    pRtlGetFullPathName_U = (void *)GetProcAddress(mod,"RtlGetFullPathName_U");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(mod, "RtlDosPathNameToNtPathName_U");
    pRtlDosPathNameToNtPathName_U_WithStatus = (void *)GetProcAddress(mod, "RtlDosPathNameToNtPathName_U_WithStatus");

    test_RtlDetermineDosPathNameType_U();
    test_RtlIsDosDeviceName_U();
    test_RtlIsNameLegalDOS8Dot3();
    test_RtlGetFullPathName_U();
    test_RtlDosPathNameToNtPathName_U();
}
