/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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
#include "wine/port.h"

#include "wined3d_private.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define DEFAULT_REFRESH_RATE 0

enum wined3d_driver_model
{
    DRIVER_MODEL_GENERIC,
    DRIVER_MODEL_WIN9X,
    DRIVER_MODEL_NT40,
    DRIVER_MODEL_NT5X,
    DRIVER_MODEL_NT6X
};

/* The d3d device ID */
static const GUID IID_D3DDEVICE_D3DUID = { 0xaeb2cdd4, 0x6e41, 0x43ea, { 0x94,0x1c,0x83,0x61,0xcc,0x76,0x07,0x81 } };

/**********************************************************
 * Utility functions follow
 **********************************************************/

const struct min_lookup minMipLookup[] =
{
    /* NONE         POINT                       LINEAR */
    {{GL_NEAREST,   GL_NEAREST,                 GL_NEAREST}},               /* NONE */
    {{GL_NEAREST,   GL_NEAREST_MIPMAP_NEAREST,  GL_NEAREST_MIPMAP_LINEAR}}, /* POINT*/
    {{GL_LINEAR,    GL_LINEAR_MIPMAP_NEAREST,   GL_LINEAR_MIPMAP_LINEAR}},  /* LINEAR */
};

const GLenum magLookup[] =
{
    /* NONE     POINT       LINEAR */
    GL_NEAREST, GL_NEAREST, GL_LINEAR,
};

/* Adjust the amount of used texture memory */
UINT64 adapter_adjust_memory(struct wined3d_adapter *adapter, INT64 amount)
{
    adapter->vram_bytes_used += amount;
    TRACE("Adjusted used adapter memory by 0x%s to 0x%s.\n",
            wine_dbgstr_longlong(amount),
            wine_dbgstr_longlong(adapter->vram_bytes_used));
    return adapter->vram_bytes_used;
}

static void wined3d_adapter_cleanup(struct wined3d_adapter *adapter)
{
    heap_free(adapter->formats);
    heap_free(adapter->cfgs);
}

ULONG CDECL wined3d_incref(struct wined3d *wined3d)
{
    ULONG refcount = InterlockedIncrement(&wined3d->ref);

    TRACE("%p increasing refcount to %u.\n", wined3d, refcount);

    return refcount;
}

ULONG CDECL wined3d_decref(struct wined3d *wined3d)
{
    ULONG refcount = InterlockedDecrement(&wined3d->ref);

    TRACE("%p decreasing refcount to %u.\n", wined3d, refcount);

    if (!refcount)
    {
        unsigned int i;

        for (i = 0; i < wined3d->adapter_count; ++i)
        {
            wined3d_adapter_cleanup(&wined3d->adapters[i]);
        }
        heap_free(wined3d);
    }

    return refcount;
}

/* Certain applications (Steam) complain if we report an outdated driver version. In general,
 * reporting a driver version is moot because we are not the Windows driver, and we have different
 * bugs, features, etc.
 *
 * The driver version has the form "x.y.z.w".
 *
 * "x" is the Windows version the driver is meant for:
 * 4 -> 95/98/NT4
 * 5 -> 2000
 * 6 -> 2000/XP
 * 7 -> Vista
 * 8 -> Win 7
 *
 * "y" is the maximum Direct3D version the driver supports.
 * y  -> d3d version mapping:
 * 11 -> d3d6
 * 12 -> d3d7
 * 13 -> d3d8
 * 14 -> d3d9
 * 15 -> d3d10
 * 16 -> d3d10.1
 * 17 -> d3d11
 *
 * "z" is the subversion number.
 *
 * "w" is the vendor specific driver build number.
 */

struct driver_version_information
{
    enum wined3d_display_driver driver;
    enum wined3d_driver_model driver_model;
    const char *driver_name;            /* name of Windows driver */
    WORD version;                       /* version word ('y'), contained in low word of DriverVersion.HighPart */
    WORD subversion;                    /* subversion word ('z'), contained in high word of DriverVersion.LowPart */
    WORD build;                         /* build number ('w'), contained in low word of DriverVersion.LowPart */
};

/* The driver version table contains driver information for different devices on several OS versions. */
static const struct driver_version_information driver_version_table[] =
{
    /* AMD
     * - Radeon HD2x00 (R600) and up supported by current drivers.
     * - Radeon 9500 (R300) - X1*00 (R5xx) supported up to Catalyst 9.3 (Linux) and 10.2 (XP/Vista/Win7)
     * - Radeon 7xxx (R100) - 9250 (RV250) supported up to Catalyst 6.11 (XP)
     * - Rage 128 supported up to XP, latest official build 6.13.3279 dated October 2001 */
    {DRIVER_AMD_RAGE_128PRO,    DRIVER_MODEL_NT5X,  "ati2dvaa.dll", 13, 3279,  0},
    {DRIVER_AMD_R100,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 14, 10, 6614},
    {DRIVER_AMD_R300,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 14, 10, 6764},
    {DRIVER_AMD_R600,           DRIVER_MODEL_NT5X,  "ati2dvag.dll", 17, 10, 1280},
    {DRIVER_AMD_R300,           DRIVER_MODEL_NT6X,  "atiumdag.dll", 14, 10, 741 },
    {DRIVER_AMD_R600,           DRIVER_MODEL_NT6X,  "atiumdag.dll", 17, 10, 1280},
    {DRIVER_AMD_RX,             DRIVER_MODEL_NT6X,  "aticfx32.dll", 17, 10, 1474},

    /* Intel
     * The drivers are unified but not all versions support all GPUs. At some point the 2k/xp
     * drivers used ialmrnt5.dll for GMA800/GMA900 but at some point the file was renamed to
     * igxprd32.dll but the GMA800 driver was never updated. */
    {DRIVER_INTEL_GMA800,       DRIVER_MODEL_NT5X,  "ialmrnt5.dll", 14, 10, 3889},
    {DRIVER_INTEL_GMA900,       DRIVER_MODEL_NT5X,  "igxprd32.dll", 14, 10, 4764},
    {DRIVER_INTEL_GMA950,       DRIVER_MODEL_NT5X,  "igxprd32.dll", 14, 10, 4926},
    {DRIVER_INTEL_GMA3000,      DRIVER_MODEL_NT5X,  "igxprd32.dll", 14, 10, 5218},
    {DRIVER_INTEL_GMA950,       DRIVER_MODEL_NT6X,  "igdumd32.dll", 14, 10, 1504},
    {DRIVER_INTEL_GMA3000,      DRIVER_MODEL_NT6X,  "igdumd32.dll", 15, 10, 1666},
    {DRIVER_INTEL_HD4000,       DRIVER_MODEL_NT6X,  "igdumdim32.dll", 19, 15, 4352},

    /* Nvidia
     * - Geforce8 and newer is supported by the current 340.52 driver on XP-Win8
     * - Geforce6 and 7 support is up to 307.83 on XP-Win8
     * - GeforceFX support is up to 173.x on <= XP
     * - Geforce2MX/3/4 up to 96.x on <= XP
     * - TNT/Geforce1/2 up to 71.x on <= XP
     * All version numbers used below are from the Linux nvidia drivers. */
    {DRIVER_NVIDIA_TNT,         DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 10, 7186},
    {DRIVER_NVIDIA_GEFORCE2MX,  DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 10, 9371},
    {DRIVER_NVIDIA_GEFORCEFX,   DRIVER_MODEL_NT5X,  "nv4_disp.dll", 14, 11, 7516},
    {DRIVER_NVIDIA_GEFORCE6,    DRIVER_MODEL_NT5X,  "nv4_disp.dll", 18, 13,  783},
    {DRIVER_NVIDIA_GEFORCE8,    DRIVER_MODEL_NT5X,  "nv4_disp.dll", 18, 13, 4052},
    {DRIVER_NVIDIA_GEFORCE6,    DRIVER_MODEL_NT6X,  "nvd3dum.dll",  18, 13,  783},
    {DRIVER_NVIDIA_GEFORCE8,    DRIVER_MODEL_NT6X,  "nvd3dum.dll",  18, 13, 4052},

    /* Red Hat */
    {DRIVER_REDHAT_VIRGL,       DRIVER_MODEL_GENERIC, "virgl.dll",   0,  0,    0},

    /* VMware */
    {DRIVER_VMWARE,             DRIVER_MODEL_NT5X,  "vm3dum.dll",   14, 1,  1134},

    /* Wine */
    {DRIVER_WINE,               DRIVER_MODEL_GENERIC, "wined3d.dll", 0, 0,     0},
};

/* The amount of video memory stored in the gpu description table is the minimum amount of video memory
 * found on a board containing a specific GPU. */
static const struct wined3d_gpu_description gpu_description_table[] =
{
    /* Nvidia cards */
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_128,           "NVIDIA RIVA 128",                  DRIVER_NVIDIA_TNT,       4   },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT,           "NVIDIA RIVA TNT",                  DRIVER_NVIDIA_TNT,       16  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_RIVA_TNT2,          "NVIDIA RIVA TNT2/TNT2 Pro",        DRIVER_NVIDIA_TNT,       32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE,            "NVIDIA GeForce 256",               DRIVER_NVIDIA_TNT,       32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2,           "NVIDIA GeForce2 GTS/GeForce2 Pro", DRIVER_NVIDIA_TNT,       32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE2_MX,        "NVIDIA GeForce2 MX/MX 400",        DRIVER_NVIDIA_GEFORCE2MX,32  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE3,           "NVIDIA GeForce3",                  DRIVER_NVIDIA_GEFORCE2MX,64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_MX,        "NVIDIA GeForce4 MX 460",           DRIVER_NVIDIA_GEFORCE2MX,64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE4_TI4200,    "NVIDIA GeForce4 Ti 4200",          DRIVER_NVIDIA_GEFORCE2MX,64, },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5200,     "NVIDIA GeForce FX 5200",           DRIVER_NVIDIA_GEFORCEFX, 64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5600,     "NVIDIA GeForce FX 5600",           DRIVER_NVIDIA_GEFORCEFX, 128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCEFX_5800,     "NVIDIA GeForce FX 5800",           DRIVER_NVIDIA_GEFORCEFX, 256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6200,       "NVIDIA GeForce 6200",              DRIVER_NVIDIA_GEFORCE6,  64  },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6600GT,     "NVIDIA GeForce 6600 GT",           DRIVER_NVIDIA_GEFORCE6,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_6800,       "NVIDIA GeForce 6800",              DRIVER_NVIDIA_GEFORCE6,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7300,       "NVIDIA GeForce Go 7300",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7400,       "NVIDIA GeForce Go 7400",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7600,       "NVIDIA GeForce 7600 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_7800GT,     "NVIDIA GeForce 7800 GT",           DRIVER_NVIDIA_GEFORCE6,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8200,       "NVIDIA GeForce 8200",              DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8300GS,     "NVIDIA GeForce 8300 GS",           DRIVER_NVIDIA_GEFORCE8,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8400GS,     "NVIDIA GeForce 8400 GS",           DRIVER_NVIDIA_GEFORCE8,  128 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8500GT,     "NVIDIA GeForce 8500 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600GT,     "NVIDIA GeForce 8600 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8600MGT,    "NVIDIA GeForce 8600M GT",          DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTS,    "NVIDIA GeForce 8800 GTS",          DRIVER_NVIDIA_GEFORCE8,  320 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_8800GTX,    "NVIDIA GeForce 8800 GTX",          DRIVER_NVIDIA_GEFORCE8,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9200,       "NVIDIA GeForce 9200",              DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9300,       "NVIDIA GeForce 9300",              DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400M,      "NVIDIA GeForce 9400M",             DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9400GT,     "NVIDIA GeForce 9400 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9500GT,     "NVIDIA GeForce 9500 GT",           DRIVER_NVIDIA_GEFORCE8,  256 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9600GT,     "NVIDIA GeForce 9600 GT",           DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9700MGT,    "NVIDIA GeForce 9700M GT",          DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_9800GT,     "NVIDIA GeForce 9800 GT",           DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_210,        "NVIDIA GeForce 210",               DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT220,      "NVIDIA GeForce GT 220",            DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT240,      "NVIDIA GeForce GT 240",            DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS250,     "NVIDIA GeForce GTS 250",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX260,     "NVIDIA GeForce GTX 260",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX275,     "NVIDIA GeForce GTX 275",           DRIVER_NVIDIA_GEFORCE8,  896 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX280,     "NVIDIA GeForce GTX 280",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_315M,       "NVIDIA GeForce 315M",              DRIVER_NVIDIA_GEFORCE8,  512 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_320M,       "NVIDIA GeForce 320M",              DRIVER_NVIDIA_GEFORCE8,  256},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT320M,     "NVIDIA GeForce GT 320M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT325M,     "NVIDIA GeForce GT 325M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT330,      "NVIDIA GeForce GT 330",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS350M,    "NVIDIA GeForce GTS 350M",          DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_410M,       "NVIDIA GeForce 410M",              DRIVER_NVIDIA_GEFORCE8,  512},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT420,      "NVIDIA GeForce GT 420",            DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT425M,     "NVIDIA GeForce GT 425M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT430,      "NVIDIA GeForce GT 430",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT440,      "NVIDIA GeForce GT 440",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTS450,     "NVIDIA GeForce GTS 450",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460,     "NVIDIA GeForce GTX 460",           DRIVER_NVIDIA_GEFORCE8,  768 },
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX460M,    "NVIDIA GeForce GTX 460M",          DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX465,     "NVIDIA GeForce GTX 465",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX470,     "NVIDIA GeForce GTX 470",           DRIVER_NVIDIA_GEFORCE8,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX480,     "NVIDIA GeForce GTX 480",           DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT520,      "NVIDIA GeForce GT 520",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT525M,     "NVIDIA GeForce GT 525M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT540M,     "NVIDIA GeForce GT 540M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX550,     "NVIDIA GeForce GTX 550 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT555M,     "NVIDIA GeForce GT 555M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560TI,   "NVIDIA GeForce GTX 560 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560M,    "NVIDIA GeForce GTX 560M",          DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX560,     "NVIDIA GeForce GTX 560",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX570,     "NVIDIA GeForce GTX 570",           DRIVER_NVIDIA_GEFORCE8,  1280},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX580,     "NVIDIA GeForce GTX 580",           DRIVER_NVIDIA_GEFORCE8,  1536},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT610,      "NVIDIA GeForce GT 610",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT630,      "NVIDIA GeForce GT 630",            DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT630M,     "NVIDIA GeForce GT 630M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT640M,     "NVIDIA GeForce GT 640M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT650M,     "NVIDIA GeForce GT 650M",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX650,     "NVIDIA GeForce GTX 650",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX650TI,   "NVIDIA GeForce GTX 650 Ti",        DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660,     "NVIDIA GeForce GTX 660",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660M,    "NVIDIA GeForce GTX 660M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX660TI,   "NVIDIA GeForce GTX 660 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX670,     "NVIDIA GeForce GTX 670",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX670MX,   "NVIDIA GeForce GTX 670MX",         DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX675MX,   "NVIDIA GeForce GTX 675MX",         DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX680,     "NVIDIA GeForce GTX 680",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX690,     "NVIDIA GeForce GTX 690",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT720,      "NVIDIA GeForce GT 720",            DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT730,      "NVIDIA GeForce GT 730",            DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT730M,     "NVIDIA GeForce GT 730M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT740M,     "NVIDIA GeForce GT 740M",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GT750M,     "NVIDIA GeForce GT 750M",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX750,     "NVIDIA GeForce GTX 750",           DRIVER_NVIDIA_GEFORCE8,  1024},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX750TI,   "NVIDIA GeForce GTX 750 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX760,     "NVIDIA GeForce GTX 760",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX760TI,   "NVIDIA GeForce GTX 760 Ti",        DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX765M,    "NVIDIA GeForce GTX 765M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX770M,    "NVIDIA GeForce GTX 770M",          DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX770,     "NVIDIA GeForce GTX 770",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX780,     "NVIDIA GeForce GTX 780",           DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX780TI,   "NVIDIA GeForce GTX 780 Ti",        DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTXTITAN,   "NVIDIA GeForce GTX TITAN",         DRIVER_NVIDIA_GEFORCE8,  6144},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTXTITANB,  "NVIDIA GeForce GTX TITAN Black",   DRIVER_NVIDIA_GEFORCE8,  6144},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTXTITANX,  "NVIDIA GeForce GTX TITAN X",       DRIVER_NVIDIA_GEFORCE8,  12288},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTXTITANZ,  "NVIDIA GeForce GTX TITAN Z",       DRIVER_NVIDIA_GEFORCE8,  12288},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_820M,       "NVIDIA GeForce 820M",              DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_830M,       "NVIDIA GeForce 830M",              DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_840M,       "NVIDIA GeForce 840M",              DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_845M,       "NVIDIA GeForce 845M",              DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX850M,    "NVIDIA GeForce GTX 850M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX860M,    "NVIDIA GeForce GTX 860M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX870M,    "NVIDIA GeForce GTX 870M",          DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX880M,    "NVIDIA GeForce GTX 880M",          DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_940M,       "NVIDIA GeForce 940M",              DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX950,     "NVIDIA GeForce GTX 950",           DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX950M,    "NVIDIA GeForce GTX 950M",          DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX960,     "NVIDIA GeForce GTX 960",           DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX960M,    "NVIDIA GeForce GTX 960M",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX970,     "NVIDIA GeForce GTX 970",           DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX970M,    "NVIDIA GeForce GTX 970M",          DRIVER_NVIDIA_GEFORCE8,  3072},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX980,     "NVIDIA GeForce GTX 980",           DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX980TI,   "NVIDIA GeForce GTX 980 Ti",        DRIVER_NVIDIA_GEFORCE8,  6144},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX1050,    "NVIDIA GeForce GTX 1050",          DRIVER_NVIDIA_GEFORCE8,  2048},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX1050TI,  "NVIDIA GeForce GTX 1050 Ti",       DRIVER_NVIDIA_GEFORCE8,  4096},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX1060,    "NVIDIA GeForce GTX 1060",          DRIVER_NVIDIA_GEFORCE8,  6144},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX1070,    "NVIDIA GeForce GTX 1070",          DRIVER_NVIDIA_GEFORCE8,  8192},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX1080,    "NVIDIA GeForce GTX 1080",          DRIVER_NVIDIA_GEFORCE8,  8192},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_GEFORCE_GTX1080TI,  "NVIDIA GeForce GTX 1080 Ti",       DRIVER_NVIDIA_GEFORCE8,  11264},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_TITANX_PASCAL,      "NVIDIA TITAN X (Pascal)",          DRIVER_NVIDIA_GEFORCE8,  12288},
    {HW_VENDOR_NVIDIA,     CARD_NVIDIA_TITANV,             "NVIDIA TITAN V",                   DRIVER_NVIDIA_GEFORCE8,  12288},

    /* AMD cards */
    {HW_VENDOR_AMD,        CARD_AMD_RAGE_128PRO,           "ATI Rage Fury",                    DRIVER_AMD_RAGE_128PRO,  16  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_7200,           "ATI RADEON 7200 SERIES",           DRIVER_AMD_R100,         32  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_8500,           "ATI RADEON 8500 SERIES",           DRIVER_AMD_R100,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_9500,           "ATI Radeon 9500",                  DRIVER_AMD_R300,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_XPRESS_200M,    "ATI RADEON XPRESS 200M Series",    DRIVER_AMD_R300,         64  },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_X700,           "ATI Radeon X700 SE",               DRIVER_AMD_R300,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_X1600,          "ATI Radeon X1600 Series",          DRIVER_AMD_R300,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2350,         "ATI Mobility Radeon HD 2350",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2600,         "ATI Mobility Radeon HD 2600",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD2900,         "ATI Radeon HD 2900 XT",            DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD3200,         "ATI Radeon HD 3200 Graphics",      DRIVER_AMD_R600,         128 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD3850,         "ATI Radeon HD 3850 AGP",           DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4200M,        "ATI Mobility Radeon HD 4200",      DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4350,         "ATI Radeon HD 4350",               DRIVER_AMD_R600,         256 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4600,         "ATI Radeon HD 4600 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4700,         "ATI Radeon HD 4700 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD4800,         "ATI Radeon HD 4800 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5400,         "ATI Radeon HD 5400 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5600,         "ATI Radeon HD 5600 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5700,         "ATI Radeon HD 5700 Series",        DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5800,         "ATI Radeon HD 5800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD5900,         "ATI Radeon HD 5900 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6300,         "AMD Radeon HD 6300 series Graphics", DRIVER_AMD_R600,       1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6400,         "AMD Radeon HD 6400 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6410D,        "AMD Radeon HD 6410D",              DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6480G,        "AMD Radeon HD 6480G",              DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6550D,        "AMD Radeon HD 6550D",              DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6600,         "AMD Radeon HD 6600 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6600M,        "AMD Radeon HD 6600M Series",       DRIVER_AMD_R600,         512 },
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6700,         "AMD Radeon HD 6700 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6800,         "AMD Radeon HD 6800 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD6900,         "AMD Radeon HD 6900 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7660D,        "AMD Radeon HD 7660D",              DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7700,         "AMD Radeon HD 7700 Series",        DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7800,         "AMD Radeon HD 7800 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD7900,         "AMD Radeon HD 7900 Series",        DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8600M,        "AMD Radeon HD 8600M Series",       DRIVER_AMD_R600,         1024},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8670,         "AMD Radeon HD 8670",               DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_HD8770,         "AMD Radeon HD 8770",               DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R3,             "AMD Radeon HD 8400 / R3 Series",   DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R7,             "AMD Radeon(TM) R7 Graphics",       DRIVER_AMD_R600,         2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R9_285,         "AMD Radeon R9 285",                DRIVER_AMD_RX,           2048},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R9_290,         "AMD Radeon R9 290",                DRIVER_AMD_RX,           4096},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_R9_FURY,        "AMD Radeon (TM) R9 Fury Series",   DRIVER_AMD_RX,           4096},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_RX_460,         "Radeon(TM) RX 460 Graphics",       DRIVER_AMD_RX,           4096},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_RX_480,         "Radeon (TM) RX 480 Graphics",      DRIVER_AMD_RX,           4096},
    {HW_VENDOR_AMD,        CARD_AMD_RADEON_RX_VEGA,        "Radeon RX Vega",                   DRIVER_AMD_RX,           8192},

    /* Red Hat */
    {HW_VENDOR_REDHAT,     CARD_REDHAT_VIRGL,              "Red Hat VirtIO GPU",                                        DRIVER_REDHAT_VIRGL,  1024},

    /* VMware */
    {HW_VENDOR_VMWARE,     CARD_VMWARE_SVGA3D,             "VMware SVGA 3D (Microsoft Corporation - WDDM)",             DRIVER_VMWARE,        1024},

    /* Intel cards */
    {HW_VENDOR_INTEL,      CARD_INTEL_830M,                "Intel(R) 82830M Graphics Controller",                       DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_855GM,               "Intel(R) 82852/82855 GM/GME Graphics Controller",           DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_845G,                "Intel(R) 845G",                                             DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_865G,                "Intel(R) 82865G Graphics Controller",                       DRIVER_INTEL_GMA800,  32 },
    {HW_VENDOR_INTEL,      CARD_INTEL_915G,                "Intel(R) 82915G/GV/910GL Express Chipset Family",           DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_E7221G,              "Intel(R) E7221G",                                           DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_915GM,               "Mobile Intel(R) 915GM/GMS,910GML Express Chipset Family",   DRIVER_INTEL_GMA900,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945G,                "Intel(R) 945G",                                             DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945GM,               "Mobile Intel(R) 945GM Express Chipset Family",              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_945GME,              "Intel(R) 945GME",                                           DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_Q35,                 "Intel(R) Q35",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_G33,                 "Intel(R) G33",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_Q33,                 "Intel(R) Q33",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_PNVG,                "Intel(R) IGD",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_PNVM,                "Intel(R) IGD",                                              DRIVER_INTEL_GMA950,  64 },
    {HW_VENDOR_INTEL,      CARD_INTEL_965Q,                "Intel(R) 965Q",                                             DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965G,                "Intel(R) 965G",                                             DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_946GZ,               "Intel(R) 946GZ",                                            DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965GM,               "Mobile Intel(R) 965 Express Chipset Family",                DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_965GME,              "Intel(R) 965GME",                                           DRIVER_INTEL_GMA3000, 128},
    {HW_VENDOR_INTEL,      CARD_INTEL_GM45,                "Mobile Intel(R) GM45 Express Chipset Family",               DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_IGD,                 "Intel(R) Integrated Graphics Device",                       DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_G45,                 "Intel(R) G45/G43",                                          DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_Q45,                 "Intel(R) Q45/Q43",                                          DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_G41,                 "Intel(R) G41",                                              DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_B43,                 "Intel(R) B43",                                              DRIVER_INTEL_GMA3000, 512},
    {HW_VENDOR_INTEL,      CARD_INTEL_ILKD,                "Intel(R) HD Graphics",                                      DRIVER_INTEL_GMA3000, 1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_ILKM,                "Intel(R) HD Graphics",                                      DRIVER_INTEL_GMA3000, 1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBD,                "Intel(R) HD Graphics 3000",                                 DRIVER_INTEL_GMA3000, 1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBM,                "Intel(R) HD Graphics 3000",                                 DRIVER_INTEL_GMA3000, 1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_SNBS,                "Intel(R) HD Graphics Family",                               DRIVER_INTEL_GMA3000, 1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBD,                "Intel(R) HD Graphics 4000",                                 DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBM,                "Intel(R) HD Graphics 4000",                                 DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IVBS,                "Intel(R) HD Graphics Family",                               DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_HWD,                 "Intel(R) HD Graphics 4600",                                 DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_HWM,                 "Intel(R) HD Graphics 4600",                                 DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD5000,              "Intel(R) HD Graphics 5000",                                 DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_I5100_1,             "Intel(R) Iris(TM) Graphics 5100",                           DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_I5100_2,             "Intel(R) Iris(TM) Graphics 5100",                           DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_I5100_3,             "Intel(R) Iris(TM) Graphics 5100",                           DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_I5100_4,             "Intel(R) Iris(TM) Graphics 5100",                           DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP5200_1,            "Intel(R) Iris(TM) Pro Graphics 5200",                       DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP5200_2,            "Intel(R) Iris(TM) Pro Graphics 5200",                       DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP5200_3,            "Intel(R) Iris(TM) Pro Graphics 5200",                       DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP5200_4,            "Intel(R) Iris(TM) Pro Graphics 5200",                       DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP5200_5,            "Intel(R) Iris(TM) Pro Graphics 5200",                       DRIVER_INTEL_HD4000,  1536},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD5300,              "Intel(R) HD Graphics 5300",                                 DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD5500,              "Intel(R) HD Graphics 5500",                                 DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD5600,              "Intel(R) HD Graphics 5600",                                 DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD6000,              "Intel(R) HD Graphics 6000",                                 DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_I6100,               "Intel(R) Iris(TM) Graphics 6100",                           DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP6200,              "Intel(R) Iris(TM) Pro Graphics 6200",                       DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IPP6300,             "Intel(R) Iris(TM) Pro Graphics P6300",                      DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD510_1,             "Intel(R) HD Graphics 510",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD510_2,             "Intel(R) HD Graphics 510",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD510_3,             "Intel(R) HD Graphics 510",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD515,               "Intel(R) HD Graphics 515",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD520_1,             "Intel(R) HD Graphics 520",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD520_2,             "Intel(R) HD Graphics 520",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD530_1,             "Intel(R) HD Graphics 530",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD530_2,             "Intel(R) HD Graphics 530",                                  DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HDP530,              "Intel(R) HD Graphics P530",                                 DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_I540,                "Intel(R) Iris(TM) Graphics 540",                            DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_I550,                "Intel(R) Iris(TM) Graphics 550",                            DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_I555,                "Intel(R) Iris(TM) Graphics 555",                            DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP555,               "Intel(R) Iris(TM) Graphics P555",                           DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP580_1,             "Intel(R) Iris(TM) Pro Graphics 580",                        DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IP580_2,             "Intel(R) Iris(TM) Pro Graphics 580",                        DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IPP580_1,            "Intel(R) Iris(TM) Pro Graphics P580",                       DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_IPP580_2,            "Intel(R) Iris(TM) Pro Graphics P580",                       DRIVER_INTEL_HD4000,  2048},
    {HW_VENDOR_INTEL,      CARD_INTEL_HD630,               "Intel(R) HD Graphics 630",                                  DRIVER_INTEL_HD4000,  3072},
};

static const struct driver_version_information *get_driver_version_info(enum wined3d_display_driver driver,
        enum wined3d_driver_model driver_model)
{
    unsigned int i;

    TRACE("Looking up version info for driver %#x, driver_model %#x.\n", driver, driver_model);

    for (i = 0; i < ARRAY_SIZE(driver_version_table); ++i)
    {
        const struct driver_version_information *entry = &driver_version_table[i];

        if (entry->driver == driver && (driver_model == DRIVER_MODEL_GENERIC
                || entry->driver_model == driver_model))
        {
            TRACE("Found driver \"%s\", version %u, subversion %u, build %u.\n",
                    entry->driver_name, entry->version, entry->subversion, entry->build);
            return entry;
        }
    }
    return NULL;
}

const struct wined3d_gpu_description *wined3d_get_gpu_description(enum wined3d_pci_vendor vendor,
        enum wined3d_pci_device device)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(gpu_description_table); ++i)
    {
        if (vendor == gpu_description_table[i].vendor && device == gpu_description_table[i].device)
            return &gpu_description_table[i];
    }

    return NULL;
}

void wined3d_driver_info_init(struct wined3d_driver_info *driver_info,
        const struct wined3d_gpu_description *gpu_desc, UINT64 vram_bytes)
{
    const struct driver_version_information *version_info;
    enum wined3d_driver_model driver_model;
    enum wined3d_display_driver driver;
    MEMORYSTATUSEX memory_status;
    OSVERSIONINFOW os_version;
    WORD driver_os_version;

    memset(&os_version, 0, sizeof(os_version));
    os_version.dwOSVersionInfoSize = sizeof(os_version);
    if (!GetVersionExW(&os_version))
    {
        ERR("Failed to get OS version, reporting 2000/XP.\n");
        driver_os_version = 6;
        driver_model = DRIVER_MODEL_NT5X;
    }
    else
    {
        TRACE("OS version %u.%u.\n", os_version.dwMajorVersion, os_version.dwMinorVersion);
        switch (os_version.dwMajorVersion)
        {
            case 4:
                /* If needed we could distinguish between 9x and NT4, but this code won't make
                 * sense for NT4 since it had no way to obtain this info through DirectDraw 3.0.
                 */
                driver_os_version = 4;
                driver_model = DRIVER_MODEL_WIN9X;
                break;

            case 5:
                driver_os_version = 6;
                driver_model = DRIVER_MODEL_NT5X;
                break;

            case 6:
                if (os_version.dwMinorVersion == 0)
                {
                    driver_os_version = 7;
                    driver_model = DRIVER_MODEL_NT6X;
                }
                else if (os_version.dwMinorVersion == 1)
                {
                    driver_os_version = 8;
                    driver_model = DRIVER_MODEL_NT6X;
                }
                else
                {
                    if (os_version.dwMinorVersion > 2)
                    {
                        FIXME("Unhandled OS version %u.%u, reporting Win 8.\n",
                                os_version.dwMajorVersion, os_version.dwMinorVersion);
                    }
                    driver_os_version = 9;
                    driver_model = DRIVER_MODEL_NT6X;
                }
                break;

            case 10:
                driver_os_version = 10;
                driver_model = DRIVER_MODEL_NT6X;
                break;

            default:
                FIXME("Unhandled OS version %u.%u, reporting 2000/XP.\n",
                        os_version.dwMajorVersion, os_version.dwMinorVersion);
                driver_os_version = 6;
                driver_model = DRIVER_MODEL_NT5X;
                break;
        }
    }

    driver_info->vendor = gpu_desc->vendor;
    driver_info->device = gpu_desc->device;
    driver_info->description = gpu_desc->description;
    driver_info->vram_bytes = vram_bytes ? vram_bytes : (UINT64)gpu_desc->vidmem * 1024 * 1024;
    driver = gpu_desc->driver;

    if (wined3d_settings.emulated_textureram)
    {
        driver_info->vram_bytes = wined3d_settings.emulated_textureram;
        TRACE("Overriding amount of video memory with 0x%s bytes.\n",
                wine_dbgstr_longlong(driver_info->vram_bytes));
    }

    /**
     * Diablo 2 crashes when the amount of video memory is greater than 0x7fffffff.
     * In order to avoid this application bug we limit the amount of video memory
     * to LONG_MAX for older Windows versions.
     */
    if (driver_model < DRIVER_MODEL_NT6X && driver_info->vram_bytes > LONG_MAX)
    {
        TRACE("Limiting amount of video memory to %#lx bytes for OS version older than Vista.\n", LONG_MAX);
        driver_info->vram_bytes = LONG_MAX;
    }

    driver_info->sysmem_bytes = 64 * 1024 * 1024;
    memory_status.dwLength = sizeof(memory_status);
    if (GlobalMemoryStatusEx(&memory_status))
        driver_info->sysmem_bytes = max(memory_status.ullTotalPhys / 2, driver_info->sysmem_bytes);
    else
        ERR("Failed to get global memory status.\n");

    /* Try to obtain driver version information for the current Windows version. This fails in
     * some cases:
     * - the gpu is not available on the currently selected OS version:
     *   - Geforce GTX480 on Win98. When running applications in compatibility mode on Windows,
     *     version information for the current Windows version is returned instead of faked info.
     *     We do the same and assume the default Windows version to emulate is WinXP.
     *
     *   - Videocard is a Riva TNT but winver is set to win7 (there are no drivers for this beast)
     *     For now return the XP driver info. Perhaps later on we should return VESA.
     *
     * - the gpu is not in our database (can happen when the user overrides the vendor_id / device_id)
     *   This could be an indication that our database is not up to date, so this should be fixed.
     */
    if ((version_info = get_driver_version_info(driver, driver_model))
            || (version_info = get_driver_version_info(driver, DRIVER_MODEL_GENERIC)))
    {
        driver_info->name = version_info->driver_name;
        driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, version_info->version);
        driver_info->version_low = MAKEDWORD_VERSION(version_info->subversion, version_info->build);
    }
    else
    {
        ERR("No driver version info found for device %04x:%04x, driver model %#x.\n",
                driver_info->vendor, driver_info->device, driver_model);
        driver_info->name = "Display";
        driver_info->version_high = MAKEDWORD_VERSION(driver_os_version, 15);
        driver_info->version_low = MAKEDWORD_VERSION(8, 6); /* NVIDIA RIVA TNT, arbitrary */
    }
}

enum wined3d_pci_device wined3d_gpu_from_feature_level(enum wined3d_pci_vendor *vendor,
        enum wined3d_feature_level feature_level)
{
    static const struct wined3d_fallback_card
    {
        enum wined3d_feature_level feature_level;
        enum wined3d_pci_device device_id;
    }
    card_fallback_nvidia[] =
    {
        {WINED3D_FEATURE_LEVEL_5,     CARD_NVIDIA_RIVA_128},
        {WINED3D_FEATURE_LEVEL_6,     CARD_NVIDIA_RIVA_TNT},
        {WINED3D_FEATURE_LEVEL_7,     CARD_NVIDIA_GEFORCE},
        {WINED3D_FEATURE_LEVEL_8,     CARD_NVIDIA_GEFORCE3},
        {WINED3D_FEATURE_LEVEL_9_SM2, CARD_NVIDIA_GEFORCEFX_5800},
        {WINED3D_FEATURE_LEVEL_9_SM3, CARD_NVIDIA_GEFORCE_6800},
        {WINED3D_FEATURE_LEVEL_10,    CARD_NVIDIA_GEFORCE_8800GTX},
        {WINED3D_FEATURE_LEVEL_11,    CARD_NVIDIA_GEFORCE_GTX470},
        {WINED3D_FEATURE_LEVEL_NONE},
    },
    card_fallback_amd[] =
    {
        {WINED3D_FEATURE_LEVEL_5,     CARD_AMD_RAGE_128PRO},
        {WINED3D_FEATURE_LEVEL_7,     CARD_AMD_RADEON_7200},
        {WINED3D_FEATURE_LEVEL_8,     CARD_AMD_RADEON_8500},
        {WINED3D_FEATURE_LEVEL_9_SM2, CARD_AMD_RADEON_9500},
        {WINED3D_FEATURE_LEVEL_9_SM3, CARD_AMD_RADEON_X1600},
        {WINED3D_FEATURE_LEVEL_10,    CARD_AMD_RADEON_HD2900},
        {WINED3D_FEATURE_LEVEL_11,    CARD_AMD_RADEON_HD5600},
        {WINED3D_FEATURE_LEVEL_NONE},
    },
    card_fallback_intel[] =
    {
        {WINED3D_FEATURE_LEVEL_5,     CARD_INTEL_845G},
        {WINED3D_FEATURE_LEVEL_8,     CARD_INTEL_915G},
        {WINED3D_FEATURE_LEVEL_9_SM3, CARD_INTEL_945G},
        {WINED3D_FEATURE_LEVEL_10,    CARD_INTEL_G45},
        {WINED3D_FEATURE_LEVEL_11,    CARD_INTEL_IVBD},
        {WINED3D_FEATURE_LEVEL_NONE},
    };

    static const struct
    {
        enum wined3d_pci_vendor vendor;
        const struct wined3d_fallback_card *cards;
    }
    fallbacks[] =
    {
        {HW_VENDOR_AMD,    card_fallback_amd},
        {HW_VENDOR_NVIDIA, card_fallback_nvidia},
        {HW_VENDOR_VMWARE, card_fallback_amd},
        {HW_VENDOR_INTEL,  card_fallback_intel},
    };

    const struct wined3d_fallback_card *cards;
    enum wined3d_pci_device device_id;
    unsigned int i;

    cards = NULL;
    for (i = 0; i < ARRAY_SIZE(fallbacks); ++i)
    {
        if (*vendor == fallbacks[i].vendor)
            cards = fallbacks[i].cards;
    }
    if (!cards)
    {
        *vendor = HW_VENDOR_NVIDIA;
        cards = card_fallback_nvidia;
    }

    device_id = cards->device_id;
    for (i = 0; cards[i].feature_level; ++i)
    {
        if (feature_level >= cards[i].feature_level)
            device_id = cards[i].device_id;
    }
    return device_id;
}

UINT CDECL wined3d_get_adapter_count(const struct wined3d *wined3d)
{
    TRACE("wined3d %p, reporting %u adapters.\n",
            wined3d, wined3d->adapter_count);

    return wined3d->adapter_count;
}

HRESULT CDECL wined3d_register_software_device(struct wined3d *wined3d, void *init_function)
{
    FIXME("wined3d %p, init_function %p stub!\n", wined3d, init_function);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_output_desc(const struct wined3d *wined3d, unsigned int adapter_idx,
        struct wined3d_output_desc *desc)
{
    enum wined3d_display_rotation rotation;
    const struct wined3d_adapter *adapter;
    struct wined3d_display_mode mode;
    HMONITOR monitor;
    HRESULT hr;

    TRACE("wined3d %p, adapter_idx %u, desc %p.\n", wined3d, adapter_idx, desc);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];
    if (!(monitor = MonitorFromPoint(adapter->monitor_position, MONITOR_DEFAULTTOPRIMARY)))
        return WINED3DERR_INVALIDCALL;

    if (FAILED(hr = wined3d_get_adapter_display_mode(wined3d, adapter_idx, &mode, &rotation)))
        return hr;

    memcpy(desc->device_name, adapter->device_name, sizeof(desc->device_name));
    SetRect(&desc->desktop_rect, 0, 0, mode.width, mode.height);
    OffsetRect(&desc->desktop_rect, adapter->monitor_position.x, adapter->monitor_position.y);
    /* FIXME: We should get this from EnumDisplayDevices() when the adapters
     * are created. */
    desc->attached_to_desktop = TRUE;
    desc->rotation = rotation;
    desc->monitor = monitor;

    return WINED3D_OK;
}

/* FIXME: GetAdapterModeCount and EnumAdapterModes currently only returns modes
     of the same bpp but different resolutions                                  */

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
UINT CDECL wined3d_get_adapter_mode_count(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, enum wined3d_scanline_ordering scanline_ordering)
{
    const struct wined3d_adapter *adapter;
    const struct wined3d_format *format;
    unsigned int i = 0;
    unsigned int j = 0;
    UINT format_bits;
    DEVMODEW mode;

    TRACE("wined3d %p, adapter_idx %u, format %s, scanline_ordering %#x.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), scanline_ordering);

    if (adapter_idx >= wined3d->adapter_count)
        return 0;

    adapter = &wined3d->adapters[adapter_idx];
    format = wined3d_get_format(adapter, format_id, WINED3D_BIND_RENDER_TARGET);
    format_bits = format->byte_count * CHAR_BIT;

    memset(&mode, 0, sizeof(mode));
    mode.dmSize = sizeof(mode);

    while (EnumDisplaySettingsExW(adapter->device_name, j++, &mode, 0))
    {
        if (mode.dmFields & DM_DISPLAYFLAGS)
        {
            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_PROGRESSIVE
                    && (mode.u2.dmDisplayFlags & DM_INTERLACED))
                continue;

            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_INTERLACED
                    && !(mode.u2.dmDisplayFlags & DM_INTERLACED))
                continue;
        }

        if (format_id == WINED3DFMT_UNKNOWN)
        {
            /* This is for d3d8, do not enumerate P8 here. */
            if (mode.dmBitsPerPel == 32 || mode.dmBitsPerPel == 16) ++i;
        }
        else if (mode.dmBitsPerPel == format_bits)
        {
            ++i;
        }
    }

    TRACE("Returning %u matching modes (out of %u total) for adapter %u.\n", i, j, adapter_idx);

    return i;
}

/* Note: dx9 supplies a format. Calls from d3d8 supply WINED3DFMT_UNKNOWN */
HRESULT CDECL wined3d_enum_adapter_modes(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, enum wined3d_scanline_ordering scanline_ordering,
        UINT mode_idx, struct wined3d_display_mode *mode)
{
    const struct wined3d_adapter *adapter;
    const struct wined3d_format *format;
    UINT format_bits;
    DEVMODEW m;
    UINT i = 0;
    int j = 0;

    TRACE("wined3d %p, adapter_idx %u, format %s, scanline_ordering %#x, mode_idx %u, mode %p.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), scanline_ordering, mode_idx, mode);

    if (!mode || adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];
    format = wined3d_get_format(adapter, format_id, WINED3D_BIND_RENDER_TARGET);
    format_bits = format->byte_count * CHAR_BIT;

    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    while (i <= mode_idx)
    {
        if (!EnumDisplaySettingsExW(adapter->device_name, j++, &m, 0))
        {
            WARN("Invalid mode_idx %u.\n", mode_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (m.dmFields & DM_DISPLAYFLAGS)
        {
            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_PROGRESSIVE
                    && (m.u2.dmDisplayFlags & DM_INTERLACED))
                continue;

            if (scanline_ordering == WINED3D_SCANLINE_ORDERING_INTERLACED
                    && !(m.u2.dmDisplayFlags & DM_INTERLACED))
                continue;
        }

        if (format_id == WINED3DFMT_UNKNOWN)
        {
            /* This is for d3d8, do not enumerate P8 here. */
            if (m.dmBitsPerPel == 32 || m.dmBitsPerPel == 16) ++i;
        }
        else if (m.dmBitsPerPel == format_bits)
        {
            ++i;
        }
    }

    mode->width = m.dmPelsWidth;
    mode->height = m.dmPelsHeight;
    mode->refresh_rate = DEFAULT_REFRESH_RATE;
    if (m.dmFields & DM_DISPLAYFREQUENCY)
        mode->refresh_rate = m.dmDisplayFrequency;

    if (format_id == WINED3DFMT_UNKNOWN)
        mode->format_id = pixelformat_for_depth(m.dmBitsPerPel);
    else
        mode->format_id = format_id;

    if (!(m.dmFields & DM_DISPLAYFLAGS))
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    else if (m.u2.dmDisplayFlags & DM_INTERLACED)
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_INTERLACED;
    else
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_PROGRESSIVE;

    TRACE("%ux%u@%u %u bpp, %s %#x.\n", mode->width, mode->height, mode->refresh_rate,
            m.dmBitsPerPel, debug_d3dformat(mode->format_id), mode->scanline_ordering);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_find_closest_matching_adapter_mode(const struct wined3d *wined3d,
        unsigned int adapter_idx, struct wined3d_display_mode *mode)
{
    unsigned int i, j, mode_count, matching_mode_count, closest;
    struct wined3d_display_mode **matching_modes;
    struct wined3d_display_mode *modes;
    HRESULT hr;

    TRACE("wined3d %p, adapter_idx %u, mode %p.\n", wined3d, adapter_idx, mode);

    if (!(mode_count = wined3d_get_adapter_mode_count(wined3d, adapter_idx,
            mode->format_id, WINED3D_SCANLINE_ORDERING_UNKNOWN)))
    {
        WARN("Adapter has 0 matching modes.\n");
        return E_FAIL;
    }

    if (!(modes = heap_calloc(mode_count, sizeof(*modes))))
        return E_OUTOFMEMORY;
    if (!(matching_modes = heap_calloc(mode_count, sizeof(*matching_modes))))
    {
        heap_free(modes);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < mode_count; ++i)
    {
        if (FAILED(hr = wined3d_enum_adapter_modes(wined3d, adapter_idx,
                mode->format_id, WINED3D_SCANLINE_ORDERING_UNKNOWN, i, &modes[i])))
        {
            heap_free(matching_modes);
            heap_free(modes);
            return hr;
        }
        matching_modes[i] = &modes[i];
    }

    matching_mode_count = mode_count;

    if (mode->scanline_ordering != WINED3D_SCANLINE_ORDERING_UNKNOWN)
    {
        for (i = 0, j = 0; i < matching_mode_count; ++i)
        {
            if (matching_modes[i]->scanline_ordering == mode->scanline_ordering)
                matching_modes[j++] = matching_modes[i];
        }
        if (j > 0)
            matching_mode_count = j;
    }

    if (mode->refresh_rate)
    {
        for (i = 0, j = 0; i < matching_mode_count; ++i)
        {
            if (matching_modes[i]->refresh_rate == mode->refresh_rate)
                matching_modes[j++] = matching_modes[i];
        }
        if (j > 0)
            matching_mode_count = j;
    }

    if (!mode->width || !mode->height)
    {
        struct wined3d_display_mode current_mode;
        if (FAILED(hr = wined3d_get_adapter_display_mode(wined3d, adapter_idx,
                &current_mode, NULL)))
        {
            heap_free(matching_modes);
            heap_free(modes);
            return hr;
        }
        mode->width = current_mode.width;
        mode->height = current_mode.height;
    }

    closest = ~0u;
    for (i = 0, j = 0; i < matching_mode_count; ++i)
    {
        unsigned int d = abs(mode->width - matching_modes[i]->width)
                + abs(mode->height - matching_modes[i]->height);

        if (closest > d)
        {
            closest = d;
            j = i;
        }
    }

    *mode = *matching_modes[j];

    heap_free(matching_modes);
    heap_free(modes);

    TRACE("Returning %ux%u@%u %s %#x.\n", mode->width, mode->height,
            mode->refresh_rate, debug_d3dformat(mode->format_id),
            mode->scanline_ordering);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_adapter_display_mode(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation)
{
    const struct wined3d_adapter *adapter;
    DEVMODEW m;

    TRACE("wined3d %p, adapter_idx %u, display_mode %p, rotation %p.\n",
            wined3d, adapter_idx, mode, rotation);

    if (!mode || adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];

    memset(&m, 0, sizeof(m));
    m.dmSize = sizeof(m);

    EnumDisplaySettingsExW(adapter->device_name, ENUM_CURRENT_SETTINGS, &m, 0);
    mode->width = m.dmPelsWidth;
    mode->height = m.dmPelsHeight;
    mode->refresh_rate = DEFAULT_REFRESH_RATE;
    if (m.dmFields & DM_DISPLAYFREQUENCY)
        mode->refresh_rate = m.dmDisplayFrequency;
    mode->format_id = pixelformat_for_depth(m.dmBitsPerPel);

    /* Lie about the format. X11 can't change the color depth, and some apps
     * are pretty angry if they SetDisplayMode from 24 to 16 bpp and find out
     * that GetDisplayMode still returns 24 bpp. This should probably be
     * handled in winex11 instead. */
    if (adapter->screen_format && adapter->screen_format != mode->format_id)
    {
        WARN("Overriding format %s with stored format %s.\n",
                debug_d3dformat(mode->format_id),
                debug_d3dformat(adapter->screen_format));
        mode->format_id = adapter->screen_format;
    }

    if (!(m.dmFields & DM_DISPLAYFLAGS))
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    else if (m.u2.dmDisplayFlags & DM_INTERLACED)
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_INTERLACED;
    else
        mode->scanline_ordering = WINED3D_SCANLINE_ORDERING_PROGRESSIVE;

    if (rotation)
    {
        switch (m.u1.s2.dmDisplayOrientation)
        {
            case DMDO_DEFAULT:
                *rotation = WINED3D_DISPLAY_ROTATION_0;
                break;
            case DMDO_90:
                *rotation = WINED3D_DISPLAY_ROTATION_90;
                break;
            case DMDO_180:
                *rotation = WINED3D_DISPLAY_ROTATION_180;
                break;
            case DMDO_270:
                *rotation = WINED3D_DISPLAY_ROTATION_270;
                break;
            default:
                FIXME("Unhandled display rotation %#x.\n", m.u1.s2.dmDisplayOrientation);
                *rotation = WINED3D_DISPLAY_ROTATION_UNSPECIFIED;
                break;
        }
    }

    TRACE("Returning %ux%u@%u %s %#x.\n", mode->width, mode->height,
            mode->refresh_rate, debug_d3dformat(mode->format_id),
            mode->scanline_ordering);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_set_adapter_display_mode(struct wined3d *wined3d,
        UINT adapter_idx, const struct wined3d_display_mode *mode)
{
    struct wined3d_adapter *adapter;
    DEVMODEW new_mode, current_mode;
    RECT clip_rc;
    LONG ret;
    enum wined3d_format_id new_format_id;

    TRACE("wined3d %p, adapter_idx %u, mode %p.\n", wined3d, adapter_idx, mode);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;
    adapter = &wined3d->adapters[adapter_idx];

    memset(&new_mode, 0, sizeof(new_mode));
    new_mode.dmSize = sizeof(new_mode);
    memset(&current_mode, 0, sizeof(current_mode));
    current_mode.dmSize = sizeof(current_mode);
    if (mode)
    {
        const struct wined3d_format *format;

        TRACE("mode %ux%u@%u %s %#x.\n", mode->width, mode->height, mode->refresh_rate,
                debug_d3dformat(mode->format_id), mode->scanline_ordering);

        format = wined3d_get_format(adapter, mode->format_id, WINED3D_BIND_RENDER_TARGET);

        new_mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        new_mode.dmBitsPerPel = format->byte_count * CHAR_BIT;
        new_mode.dmPelsWidth = mode->width;
        new_mode.dmPelsHeight = mode->height;

        new_mode.dmDisplayFrequency = mode->refresh_rate;
        if (mode->refresh_rate)
            new_mode.dmFields |= DM_DISPLAYFREQUENCY;

        if (mode->scanline_ordering != WINED3D_SCANLINE_ORDERING_UNKNOWN)
        {
            new_mode.dmFields |= DM_DISPLAYFLAGS;
            if (mode->scanline_ordering == WINED3D_SCANLINE_ORDERING_INTERLACED)
                new_mode.u2.dmDisplayFlags |= DM_INTERLACED;
        }
        new_format_id = mode->format_id;
    }
    else
    {
        if (!EnumDisplaySettingsW(adapter->device_name, ENUM_REGISTRY_SETTINGS, &new_mode))
        {
            ERR("Failed to read mode from registry.\n");
            return WINED3DERR_NOTAVAILABLE;
        }
        new_format_id = pixelformat_for_depth(new_mode.dmBitsPerPel);
    }

    /* Only change the mode if necessary. */
    if (!EnumDisplaySettingsW(adapter->device_name, ENUM_CURRENT_SETTINGS, &current_mode))
    {
        ERR("Failed to get current display mode.\n");
    }
    else if (current_mode.dmPelsWidth == new_mode.dmPelsWidth
            && current_mode.dmPelsHeight == new_mode.dmPelsHeight
            && current_mode.dmBitsPerPel == new_mode.dmBitsPerPel
            && (current_mode.dmDisplayFrequency == new_mode.dmDisplayFrequency
            || !(new_mode.dmFields & DM_DISPLAYFREQUENCY))
            && (current_mode.u2.dmDisplayFlags == new_mode.u2.dmDisplayFlags
            || !(new_mode.dmFields & DM_DISPLAYFLAGS)))
    {
        TRACE("Skipping redundant mode setting call.\n");
        adapter->screen_format = new_format_id;
        return WINED3D_OK;
    }

    ret = ChangeDisplaySettingsExW(adapter->device_name, &new_mode, NULL, CDS_FULLSCREEN, NULL);
    if (ret != DISP_CHANGE_SUCCESSFUL)
    {
        if (new_mode.dmFields & DM_DISPLAYFREQUENCY)
        {
            WARN("ChangeDisplaySettingsExW failed, trying without the refresh rate.\n");
            new_mode.dmFields &= ~DM_DISPLAYFREQUENCY;
            new_mode.dmDisplayFrequency = 0;
            ret = ChangeDisplaySettingsExW(adapter->device_name, &new_mode, NULL, CDS_FULLSCREEN, NULL);
        }
        if (ret != DISP_CHANGE_SUCCESSFUL)
            return WINED3DERR_NOTAVAILABLE;
    }

    /* Store the new values. */
    adapter->screen_format = new_format_id;

    /* And finally clip mouse to our screen. */
    SetRect(&clip_rc, 0, 0, new_mode.dmPelsWidth, new_mode.dmPelsHeight);
    ClipCursor(&clip_rc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_adapter_identifier(const struct wined3d *wined3d,
        UINT adapter_idx, DWORD flags, struct wined3d_adapter_identifier *identifier)
{
    const struct wined3d_adapter *adapter;
    size_t len;

    TRACE("wined3d %p, adapter_idx %u, flags %#x, identifier %p.\n",
            wined3d, adapter_idx, flags, identifier);

    wined3d_mutex_lock();

    if (adapter_idx >= wined3d->adapter_count)
        goto fail;

    adapter = &wined3d->adapters[adapter_idx];

    if (identifier->driver_size)
    {
        const char *name = adapter->driver_info.name;
        len = min(strlen(name), identifier->driver_size - 1);
        memcpy(identifier->driver, name, len);
        memset(&identifier->driver[len], 0, identifier->driver_size - len);
    }

    if (identifier->description_size)
    {
        const char *description = adapter->driver_info.description;
        len = min(strlen(description), identifier->description_size - 1);
        memcpy(identifier->description, description, len);
        memset(&identifier->description[len], 0, identifier->description_size - len);
    }

    /* Note that d3d8 doesn't supply a device name. */
    if (identifier->device_name_size)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, adapter->device_name, -1, identifier->device_name,
                identifier->device_name_size, NULL, NULL))
        {
            ERR("Failed to convert device name, last error %#x.\n", GetLastError());
            goto fail;
        }
    }

    identifier->driver_version.u.HighPart = adapter->driver_info.version_high;
    identifier->driver_version.u.LowPart = adapter->driver_info.version_low;
    identifier->vendor_id = adapter->driver_info.vendor;
    identifier->device_id = adapter->driver_info.device;
    identifier->subsystem_id = 0;
    identifier->revision = 0;
    memcpy(&identifier->device_identifier, &IID_D3DDEVICE_D3DUID, sizeof(identifier->device_identifier));
    identifier->whql_level = (flags & WINED3DENUM_NO_WHQL_LEVEL) ? 0 : 1;
    identifier->adapter_luid = adapter->luid;
    identifier->video_memory = min(~(SIZE_T)0, adapter->driver_info.vram_bytes);
    identifier->shared_system_memory = min(~(SIZE_T)0, adapter->driver_info.sysmem_bytes);

    wined3d_mutex_unlock();

    return WINED3D_OK;

fail:
    wined3d_mutex_unlock();
    return WINED3DERR_INVALIDCALL;
}

HRESULT CDECL wined3d_get_adapter_raster_status(const struct wined3d *wined3d, UINT adapter_idx,
        struct wined3d_raster_status *raster_status)
{
    LONGLONG freq_per_frame, freq_per_line;
    LARGE_INTEGER counter, freq_per_sec;
    struct wined3d_display_mode mode;
    static UINT once;

    if (!once++)
        FIXME("wined3d %p, adapter_idx %u, raster_status %p semi-stub!\n",
                wined3d, adapter_idx, raster_status);
    else
        WARN("wined3d %p, adapter_idx %u, raster_status %p semi-stub!\n",
                wined3d, adapter_idx, raster_status);

    /* Obtaining the raster status is a widely implemented but optional
     * feature. When this method returns OK StarCraft 2 expects the
     * raster_status->InVBlank value to actually change over time.
     * And Endless Alice Crysis doesn't care even if this method fails.
     * Thus this method returns OK and fakes raster_status by
     * QueryPerformanceCounter. */

    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&freq_per_sec))
        return WINED3DERR_INVALIDCALL;
    if (FAILED(wined3d_get_adapter_display_mode(wined3d, adapter_idx, &mode, NULL)))
        return WINED3DERR_INVALIDCALL;
    if (mode.refresh_rate == DEFAULT_REFRESH_RATE)
        mode.refresh_rate = 60;

    freq_per_frame = freq_per_sec.QuadPart / mode.refresh_rate;
    /* Assume 20 scan lines in the vertical blank. */
    freq_per_line = freq_per_frame / (mode.height + 20);
    raster_status->scan_line = (counter.QuadPart % freq_per_frame) / freq_per_line;
    if (raster_status->scan_line < mode.height)
        raster_status->in_vblank = FALSE;
    else
    {
        raster_status->scan_line = 0;
        raster_status->in_vblank = TRUE;
    }

    TRACE("Returning fake value, in_vblank %u, scan_line %u.\n",
            raster_status->in_vblank, raster_status->scan_line);

    return WINED3D_OK;
}

static BOOL wined3d_check_pixel_format_color(const struct wined3d_pixel_format *cfg,
        const struct wined3d_format *format)
{
    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
        return FALSE;

    /* Probably a RGBA_float or color index mode. */
    if (cfg->iPixelType != WGL_TYPE_RGBA_ARB)
        return FALSE;

    if (cfg->redSize < format->red_size
            || cfg->greenSize < format->green_size
            || cfg->blueSize < format->blue_size
            || cfg->alphaSize < format->alpha_size)
        return FALSE;

    return TRUE;
}

static BOOL wined3d_check_pixel_format_depth(const struct wined3d_pixel_format *cfg,
        const struct wined3d_format *format)
{
    BOOL lockable = FALSE;

    /* Float formats need FBOs. If FBOs are used this function isn't called */
    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
        return FALSE;

    if ((format->id == WINED3DFMT_D16_LOCKABLE) || (format->id == WINED3DFMT_D32_FLOAT))
        lockable = TRUE;

    /* On some modern cards like the Geforce8/9, GLX doesn't offer some
     * depth/stencil formats which D3D9 reports. We can safely report
     * "compatible" formats (e.g. D24 can be used for D16) as long as we
     * aren't dealing with a lockable format. This also helps D3D <= 7 as they
     * expect D16 which isn't offered without this on Geforce8 cards. */
    if (!(cfg->depthSize == format->depth_size || (!lockable && cfg->depthSize > format->depth_size)))
        return FALSE;

    /* Some cards like Intel i915 ones only offer D24S8 but lots of games also
     * need a format without stencil. We can allow a mismatch if the format
     * doesn't have any stencil bits. If it does have stencil bits the size
     * must match, or stencil wrapping would break. */
    if (format->stencil_size && cfg->stencilSize != format->stencil_size)
        return FALSE;

    return TRUE;
}

HRESULT CDECL wined3d_check_depth_stencil_match(const struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id,
        enum wined3d_format_id render_target_format_id, enum wined3d_format_id depth_stencil_format_id)
{
    const struct wined3d_format *rt_format;
    const struct wined3d_format *ds_format;
    const struct wined3d_adapter *adapter;

    TRACE("wined3d %p, adapter_idx %u, device_type %s,\n"
            "adapter_format %s, render_target_format %s, depth_stencil_format %s.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(adapter_format_id),
            debug_d3dformat(render_target_format_id), debug_d3dformat(depth_stencil_format_id));

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];
    rt_format = wined3d_get_format(adapter, render_target_format_id, WINED3D_BIND_RENDER_TARGET);
    ds_format = wined3d_get_format(adapter, depth_stencil_format_id, WINED3D_BIND_DEPTH_STENCIL);
    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        if ((rt_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_RENDERTARGET)
                && (ds_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
        {
            TRACE("Formats match.\n");
            return WINED3D_OK;
        }
    }
    else
    {
        const struct wined3d_pixel_format *cfgs;
        unsigned int cfg_count;
        unsigned int i;

        cfgs = adapter->cfgs;
        cfg_count = adapter->cfg_count;
        for (i = 0; i < cfg_count; ++i)
        {
            if (wined3d_check_pixel_format_color(&cfgs[i], rt_format)
                    && wined3d_check_pixel_format_depth(&cfgs[i], ds_format))
            {
                TRACE("Formats match.\n");
                return WINED3D_OK;
            }
        }
    }

    TRACE("Unsupported format pair: %s and %s.\n",
            debug_d3dformat(render_target_format_id),
            debug_d3dformat(depth_stencil_format_id));

    return WINED3DERR_NOTAVAILABLE;
}

HRESULT CDECL wined3d_check_device_multisample_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id surface_format_id, BOOL windowed,
        enum wined3d_multisample_type multisample_type, DWORD *quality_levels)
{
    const struct wined3d_adapter *adapter;
    const struct wined3d_format *format;
    HRESULT hr = WINED3D_OK;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, surface_format %s, "
            "windowed %#x, multisample_type %#x, quality_levels %p.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(surface_format_id),
            windowed, multisample_type, quality_levels);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;
    if (surface_format_id == WINED3DFMT_UNKNOWN)
        return WINED3DERR_INVALIDCALL;
    if (multisample_type < WINED3D_MULTISAMPLE_NONE)
        return WINED3DERR_INVALIDCALL;
    if (multisample_type > WINED3D_MULTISAMPLE_16_SAMPLES)
    {
        FIXME("multisample_type %u not handled yet.\n", multisample_type);
        return WINED3DERR_NOTAVAILABLE;
    }

    adapter = &wined3d->adapters[adapter_idx];
    format = wined3d_get_format(adapter, surface_format_id, 0);

    if (multisample_type && !(format->multisample_types & 1u << (multisample_type - 1)))
        hr = WINED3DERR_NOTAVAILABLE;

    if (SUCCEEDED(hr) || (multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE && format->multisample_types))
    {
        if (quality_levels)
        {
            if (multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE)
                *quality_levels = wined3d_popcount(format->multisample_types);
            else
                *quality_levels = 1;
        }
        return WINED3D_OK;
    }

    TRACE("Returning not supported.\n");
    return hr;
}

/* Check if the given DisplayFormat + DepthStencilFormat combination is valid for the Adapter */
static BOOL CheckDepthStencilCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *display_format, const struct wined3d_format *ds_format,
        enum wined3d_gl_resource_type gl_type)
{
    /* Only allow depth/stencil formats */
    if (!(ds_format->depth_size || ds_format->stencil_size))
        return FALSE;

    /* Blacklist formats not supported on Windows */
    switch (ds_format->id)
    {
        case WINED3DFMT_S1_UINT_D15_UNORM: /* Breaks the shadowvol2 dx7 sdk sample */
        case WINED3DFMT_S4X4_UINT_D24_UNORM:
            TRACE("[FAILED] - not supported on windows.\n");
            return FALSE;

        default:
            break;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        /* With FBOs WGL limitations do not apply, but the format needs to be FBO attachable */
        if (ds_format->flags[gl_type] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            return TRUE;
    }
    else
    {
        unsigned int i;

        /* Walk through all WGL pixel formats to find a match */
        for (i = 0; i < adapter->cfg_count; ++i)
        {
            const struct wined3d_pixel_format *cfg = &adapter->cfgs[i];
            if (wined3d_check_pixel_format_color(cfg, display_format)
                    && wined3d_check_pixel_format_depth(cfg, ds_format))
                return TRUE;
        }
    }

    return FALSE;
}

/* Check the render target capabilities of a format */
static BOOL CheckRenderTargetCapability(const struct wined3d_adapter *adapter,
        const struct wined3d_format *adapter_format, const struct wined3d_format *check_format,
        enum wined3d_gl_resource_type gl_type)
{
    /* Filter out non-RT formats */
    if (!(check_format->flags[gl_type] & WINED3DFMT_FLAG_RENDERTARGET))
        return FALSE;
    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        return TRUE;
    if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        const struct wined3d_pixel_format *cfgs = adapter->cfgs;
        unsigned int i;

        /* In backbuffer mode the front and backbuffer share the same WGL
         * pixelformat. The format must match in RGB, alpha is allowed to be
         * different. (Only the backbuffer can have alpha.) */
        if (adapter_format->red_size != check_format->red_size
                || adapter_format->green_size != check_format->green_size
                || adapter_format->blue_size != check_format->blue_size)
        {
            TRACE("[FAILED]\n");
            return FALSE;
        }

        /* Check if there is a WGL pixel format matching the requirements, the format should also be window
         * drawable (not offscreen; e.g. Nvidia offers R5G6B5 for pbuffers even when X is running at 24bit) */
        for (i = 0; i < adapter->cfg_count; ++i)
        {
            if (cfgs[i].windowDrawable
                    && wined3d_check_pixel_format_color(&cfgs[i], check_format))
            {
                TRACE("Pixel format %d is compatible with format %s.\n",
                        cfgs[i].iPixelFormat, debug_d3dformat(check_format->id));
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL wined3d_check_surface_capability(const struct wined3d_format *format)
{
    if ((format->flags[WINED3D_GL_RES_TYPE_TEX_2D] | format->flags[WINED3D_GL_RES_TYPE_RB]) & WINED3DFMT_FLAG_BLIT)
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    if ((format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_EXTENSION | WINED3DFMT_FLAG_TEXTURE))
            == (WINED3DFMT_FLAG_EXTENSION | WINED3DFMT_FLAG_TEXTURE))
    {
        TRACE("[OK]\n");
        return TRUE;
    }

    /* Reject other formats */
    TRACE("[FAILED]\n");
    return FALSE;
}

/* OpenGL supports mipmapping on all formats. Wrapping is unsupported, but we
 * have to report mipmapping so we cannot reject WRAPANDMIP. Tests show that
 * Windows reports WRAPANDMIP on unfilterable surfaces as well, apparently to
 * show that wrapping is supported. The lack of filtering will sort out the
 * mipmapping capability anyway.
 *
 * For now lets report this on all formats, but in the future we may want to
 * restrict it to some should applications need that. */
HRESULT CDECL wined3d_check_device_format(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id adapter_format_id, DWORD usage,
        unsigned int bind_flags, enum wined3d_resource_type resource_type, enum wined3d_format_id check_format_id)
{
    const struct wined3d_format *adapter_format, *format;
    enum wined3d_gl_resource_type gl_type, gl_type_end;
    const struct wined3d_adapter *adapter;
    BOOL mipmap_gen_supported = TRUE;
    unsigned int allowed_bind_flags;
    DWORD format_flags = 0;
    DWORD allowed_usage;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, adapter_format %s, usage %s, %s, "
            "bind_flags %s, resource_type %s, check_format %s.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(adapter_format_id),
            debug_d3dusage(usage), debug_d3dusagequery(usage), wined3d_debug_bind_flags(bind_flags),
            debug_d3dresourcetype(resource_type), debug_d3dformat(check_format_id));

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    adapter = &wined3d->adapters[adapter_idx];
    adapter_format = wined3d_get_format(adapter, adapter_format_id, WINED3D_BIND_RENDER_TARGET);
    format = wined3d_get_format(adapter, check_format_id, bind_flags);

    switch (resource_type)
    {
        case WINED3D_RTYPE_NONE:
            allowed_usage = 0;
            allowed_bind_flags = WINED3D_BIND_RENDER_TARGET
                    | WINED3D_BIND_DEPTH_STENCIL;
            gl_type = WINED3D_GL_RES_TYPE_TEX_2D;
            gl_type_end = WINED3D_GL_RES_TYPE_TEX_3D;
            break;

        case WINED3D_RTYPE_TEXTURE_1D:
            allowed_usage = WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_SOFTWAREPROCESSING
                    | WINED3DUSAGE_QUERY_FILTER
                    | WINED3DUSAGE_QUERY_GENMIPMAP
                    | WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
                    | WINED3DUSAGE_QUERY_SRGBREAD
                    | WINED3DUSAGE_QUERY_SRGBWRITE
                    | WINED3DUSAGE_QUERY_VERTEXTEXTURE
                    | WINED3DUSAGE_QUERY_WRAPANDMIP;
            allowed_bind_flags = WINED3D_BIND_SHADER_RESOURCE;
            gl_type = gl_type_end = WINED3D_GL_RES_TYPE_TEX_1D;
            break;

        case WINED3D_RTYPE_TEXTURE_2D:
            allowed_usage = WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
            if (bind_flags & WINED3D_BIND_RENDER_TARGET)
                allowed_usage |= WINED3DUSAGE_QUERY_SRGBWRITE;
            allowed_bind_flags = WINED3D_BIND_RENDER_TARGET
                    | WINED3D_BIND_DEPTH_STENCIL;
            if (!(bind_flags & WINED3D_BIND_SHADER_RESOURCE))
            {
                if (!wined3d_check_surface_capability(format))
                {
                    TRACE("[FAILED] - Not supported for plain surfaces.\n");
                    return WINED3DERR_NOTAVAILABLE;
                }

                gl_type = gl_type_end = WINED3D_GL_RES_TYPE_RB;
                break;
            }
            allowed_usage |= WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_LEGACY_CUBEMAP
                    | WINED3DUSAGE_SOFTWAREPROCESSING
                    | WINED3DUSAGE_QUERY_FILTER
                    | WINED3DUSAGE_QUERY_GENMIPMAP
                    | WINED3DUSAGE_QUERY_LEGACYBUMPMAP
                    | WINED3DUSAGE_QUERY_SRGBREAD
                    | WINED3DUSAGE_QUERY_SRGBWRITE
                    | WINED3DUSAGE_QUERY_VERTEXTEXTURE
                    | WINED3DUSAGE_QUERY_WRAPANDMIP;
            allowed_bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
            gl_type = gl_type_end = WINED3D_GL_RES_TYPE_TEX_2D;
            if (usage & WINED3DUSAGE_LEGACY_CUBEMAP)
            {
                allowed_usage &= ~WINED3DUSAGE_QUERY_LEGACYBUMPMAP;
                allowed_bind_flags &= ~WINED3D_BIND_DEPTH_STENCIL;
                gl_type = gl_type_end = WINED3D_GL_RES_TYPE_TEX_CUBE;
            }
            else if ((bind_flags & WINED3D_BIND_DEPTH_STENCIL)
                    && (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_SHADOW)
                    && !adapter->gl_info.supported[ARB_SHADOW])
            {
                TRACE("[FAILED] - No shadow sampler support.\n");
                return WINED3DERR_NOTAVAILABLE;
            }
            break;

        case WINED3D_RTYPE_TEXTURE_3D:
            allowed_usage = WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_SOFTWAREPROCESSING
                    | WINED3DUSAGE_QUERY_FILTER
                    | WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING
                    | WINED3DUSAGE_QUERY_SRGBREAD
                    | WINED3DUSAGE_QUERY_SRGBWRITE
                    | WINED3DUSAGE_QUERY_VERTEXTEXTURE
                    | WINED3DUSAGE_QUERY_WRAPANDMIP;
            allowed_bind_flags = WINED3D_BIND_SHADER_RESOURCE;
            gl_type = gl_type_end = WINED3D_GL_RES_TYPE_TEX_3D;
            break;

        default:
            FIXME("Unhandled resource type %s.\n", debug_d3dresourcetype(resource_type));
            return WINED3DERR_NOTAVAILABLE;
    }

    if ((usage & allowed_usage) != usage)
    {
        TRACE("Requested usage %#x, but resource type %s only allows %#x.\n",
                usage, debug_d3dresourcetype(resource_type), allowed_usage);
        return WINED3DERR_NOTAVAILABLE;
    }

    if ((bind_flags & allowed_bind_flags) != bind_flags)
    {
        TRACE("Requested bind flags %s, but resource type %s only allows %s.\n",
                wined3d_debug_bind_flags(bind_flags), debug_d3dresourcetype(resource_type),
                wined3d_debug_bind_flags(allowed_bind_flags));
        return WINED3DERR_NOTAVAILABLE;
    }

    if (bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        format_flags |= WINED3DFMT_FLAG_TEXTURE;
    if (usage & WINED3DUSAGE_QUERY_FILTER)
        format_flags |= WINED3DFMT_FLAG_FILTERING;
    if (usage & WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING)
        format_flags |= WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING;
    if (usage & WINED3DUSAGE_QUERY_SRGBREAD)
        format_flags |= WINED3DFMT_FLAG_SRGB_READ;
    if (usage & WINED3DUSAGE_QUERY_SRGBWRITE)
        format_flags |= WINED3DFMT_FLAG_SRGB_WRITE;
    if (usage & WINED3DUSAGE_QUERY_VERTEXTEXTURE)
        format_flags |= WINED3DFMT_FLAG_VTF;
    if (usage & WINED3DUSAGE_QUERY_LEGACYBUMPMAP)
        format_flags |= WINED3DFMT_FLAG_BUMPMAP;

    if ((format_flags & WINED3DFMT_FLAG_TEXTURE) && (wined3d->flags & WINED3D_NO3D))
    {
        TRACE("Requested texturing support, but wined3d was created with WINED3D_NO3D.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    for (; gl_type <= gl_type_end; ++gl_type)
    {
        if ((format->flags[gl_type] & format_flags) != format_flags)
        {
            TRACE("Requested format flags %#x, but format %s only has %#x.\n",
                    format_flags, debug_d3dformat(check_format_id), format->flags[gl_type]);
            return WINED3DERR_NOTAVAILABLE;
        }

        if ((bind_flags & WINED3D_BIND_RENDER_TARGET)
                && !CheckRenderTargetCapability(adapter, adapter_format, format, gl_type))
        {
            TRACE("Requested WINED3D_BIND_RENDER_TARGET, but format %s is not supported for render targets.\n",
                    debug_d3dformat(check_format_id));
            return WINED3DERR_NOTAVAILABLE;
        }

        /* 3D depth / stencil textures are never supported. */
        if (bind_flags == WINED3D_BIND_DEPTH_STENCIL && gl_type == WINED3D_GL_RES_TYPE_TEX_3D)
            continue;

        if ((bind_flags & WINED3D_BIND_DEPTH_STENCIL)
                && !CheckDepthStencilCapability(adapter, adapter_format, format, gl_type))
        {
            TRACE("Requested WINED3D_BIND_DEPTH_STENCIL, but format %s is not supported for depth/stencil buffers.\n",
                    debug_d3dformat(check_format_id));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (!(format->flags[gl_type] & WINED3DFMT_FLAG_GEN_MIPMAP))
            mipmap_gen_supported = FALSE;
    }

    if ((usage & WINED3DUSAGE_QUERY_GENMIPMAP) && !mipmap_gen_supported)
    {
        TRACE("No WINED3DUSAGE_AUTOGENMIPMAP support, returning WINED3DOK_NOAUTOGEN.\n");
        return WINED3DOK_NOMIPGEN;
    }

    return WINED3D_OK;
}

UINT CDECL wined3d_calculate_format_pitch(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_format_id format_id, UINT width)
{
    const struct wined3d_adapter *adapter;
    unsigned int row_pitch, slice_pitch;

    TRACE("wined3d %p, adapter_idx %u, format_id %s, width %u.\n",
            wined3d, adapter_idx, debug_d3dformat(format_id), width);

    if (adapter_idx >= wined3d->adapter_count)
        return ~0u;

    adapter = &wined3d->adapters[adapter_idx];
    wined3d_format_calculate_pitch(wined3d_get_format(adapter, format_id, 0),
            1, width, 1, &row_pitch, &slice_pitch);

    return row_pitch;
}

HRESULT CDECL wined3d_check_device_format_conversion(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id src_format, enum wined3d_format_id dst_format)
{
    FIXME("wined3d %p, adapter_idx %u, device_type %s, src_format %s, dst_format %s stub!\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(src_format),
            debug_d3dformat(dst_format));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_check_device_type(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, enum wined3d_format_id display_format,
        enum wined3d_format_id backbuffer_format, BOOL windowed)
{
    BOOL present_conversion = wined3d->flags & WINED3D_PRESENT_CONVERSION;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, display_format %s, backbuffer_format %s, windowed %#x.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), debug_d3dformat(display_format),
            debug_d3dformat(backbuffer_format), windowed);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    /* The task of this function is to check whether a certain display / backbuffer format
     * combination is available on the given adapter. In fullscreen mode microsoft specified
     * that the display format shouldn't provide alpha and that ignoring alpha the backbuffer
     * and display format should match exactly.
     * In windowed mode format conversion can occur and this depends on the driver. */

    /* There are only 4 display formats. */
    if (!(display_format == WINED3DFMT_B5G6R5_UNORM
            || display_format == WINED3DFMT_B5G5R5X1_UNORM
            || display_format == WINED3DFMT_B8G8R8X8_UNORM
            || display_format == WINED3DFMT_B10G10R10A2_UNORM))
    {
        TRACE("Format %s is not supported as display format.\n", debug_d3dformat(display_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    if (!windowed)
    {
        /* If the requested display format is not available, don't continue. */
        if (!wined3d_get_adapter_mode_count(wined3d, adapter_idx,
                display_format, WINED3D_SCANLINE_ORDERING_UNKNOWN))
        {
            TRACE("No available modes for display format %s.\n", debug_d3dformat(display_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        present_conversion = FALSE;
    }
    else if (display_format == WINED3DFMT_B10G10R10A2_UNORM)
    {
        /* WINED3DFMT_B10G10R10A2_UNORM is only allowed in fullscreen mode. */
        TRACE("Unsupported format combination %s / %s in windowed mode.\n",
                debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    if (present_conversion)
    {
        /* Use the display format as back buffer format if the latter is
         * WINED3DFMT_UNKNOWN. */
        if (backbuffer_format == WINED3DFMT_UNKNOWN)
            backbuffer_format = display_format;

        if (FAILED(wined3d_check_device_format_conversion(wined3d, adapter_idx,
                device_type, backbuffer_format, display_format)))
        {
            TRACE("Format conversion from %s to %s not supported.\n",
                    debug_d3dformat(backbuffer_format), debug_d3dformat(display_format));
            return WINED3DERR_NOTAVAILABLE;
        }
    }
    else
    {
        /* When format conversion from the back buffer format to the display
         * format is not allowed, only a limited number of combinations are
         * valid. */

        if (display_format == WINED3DFMT_B5G6R5_UNORM && backbuffer_format != WINED3DFMT_B5G6R5_UNORM)
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (display_format == WINED3DFMT_B5G5R5X1_UNORM
                && !(backbuffer_format == WINED3DFMT_B5G5R5X1_UNORM || backbuffer_format == WINED3DFMT_B5G5R5A1_UNORM))
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (display_format == WINED3DFMT_B8G8R8X8_UNORM
                && !(backbuffer_format == WINED3DFMT_B8G8R8X8_UNORM || backbuffer_format == WINED3DFMT_B8G8R8A8_UNORM))
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }

        if (display_format == WINED3DFMT_B10G10R10A2_UNORM
                && backbuffer_format != WINED3DFMT_B10G10R10A2_UNORM)
        {
            TRACE("Unsupported format combination %s / %s.\n",
                    debug_d3dformat(display_format), debug_d3dformat(backbuffer_format));
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    /* Validate that the back buffer format is usable for render targets. */
    if (FAILED(wined3d_check_device_format(wined3d, adapter_idx, device_type, display_format,
            0, WINED3D_BIND_RENDER_TARGET, WINED3D_RTYPE_TEXTURE_2D, backbuffer_format)))
    {
        TRACE("Format %s not allowed for render targets.\n", debug_d3dformat(backbuffer_format));
        return WINED3DERR_NOTAVAILABLE;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_get_device_caps(const struct wined3d *wined3d, UINT adapter_idx,
        enum wined3d_device_type device_type, struct wined3d_caps *caps)
{
    const struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;
    const struct wined3d_gl_info *gl_info = &adapter->gl_info;
    struct wined3d_vertex_caps vertex_caps;
    DWORD ckey_caps, blit_caps, fx_caps;
    struct fragment_caps fragment_caps;
    struct shader_caps shader_caps;

    TRACE("wined3d %p, adapter_idx %u, device_type %s, caps %p.\n",
            wined3d, adapter_idx, debug_d3ddevicetype(device_type), caps);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    caps->DeviceType = (device_type == WINED3D_DEVICE_TYPE_HAL) ? WINED3D_DEVICE_TYPE_HAL : WINED3D_DEVICE_TYPE_REF;
    caps->AdapterOrdinal           = adapter_idx;

    caps->Caps                     = 0;
    caps->Caps2                    = WINED3DCAPS2_CANRENDERWINDOWED |
                                     WINED3DCAPS2_FULLSCREENGAMMA |
                                     WINED3DCAPS2_DYNAMICTEXTURES;
    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT] || gl_info->supported[EXT_FRAMEBUFFER_OBJECT])
        caps->Caps2 |= WINED3DCAPS2_CANGENMIPMAP;

    caps->Caps3                    = WINED3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD |
                                     WINED3DCAPS3_COPY_TO_VIDMEM                   |
                                     WINED3DCAPS3_COPY_TO_SYSTEMMEM;

    caps->CursorCaps               = WINED3DCURSORCAPS_COLOR            |
                                     WINED3DCURSORCAPS_LOWRES;

    caps->DevCaps                  = WINED3DDEVCAPS_FLOATTLVERTEX       |
                                     WINED3DDEVCAPS_EXECUTESYSTEMMEMORY |
                                     WINED3DDEVCAPS_TLVERTEXSYSTEMMEMORY|
                                     WINED3DDEVCAPS_TLVERTEXVIDEOMEMORY |
                                     WINED3DDEVCAPS_DRAWPRIMTLVERTEX    |
                                     WINED3DDEVCAPS_HWTRANSFORMANDLIGHT |
                                     WINED3DDEVCAPS_EXECUTEVIDEOMEMORY  |
                                     WINED3DDEVCAPS_PUREDEVICE          |
                                     WINED3DDEVCAPS_HWRASTERIZATION     |
                                     WINED3DDEVCAPS_TEXTUREVIDEOMEMORY  |
                                     WINED3DDEVCAPS_TEXTURESYSTEMMEMORY |
                                     WINED3DDEVCAPS_CANRENDERAFTERFLIP  |
                                     WINED3DDEVCAPS_DRAWPRIMITIVES2     |
                                     WINED3DDEVCAPS_DRAWPRIMITIVES2EX;

    caps->PrimitiveMiscCaps        = WINED3DPMISCCAPS_CULLNONE              |
                                     WINED3DPMISCCAPS_CULLCCW               |
                                     WINED3DPMISCCAPS_CULLCW                |
                                     WINED3DPMISCCAPS_COLORWRITEENABLE      |
                                     WINED3DPMISCCAPS_CLIPTLVERTS           |
                                     WINED3DPMISCCAPS_CLIPPLANESCALEDPOINTS |
                                     WINED3DPMISCCAPS_MASKZ                 |
                                     WINED3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING;
                                    /* TODO:
                                        WINED3DPMISCCAPS_NULLREFERENCE
                                        WINED3DPMISCCAPS_FOGANDSPECULARALPHA
                                        WINED3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                                        WINED3DPMISCCAPS_FOGVERTEXCLAMPED */

    if (gl_info->supported[WINED3D_GL_BLEND_EQUATION])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_BLENDOP;
    if (gl_info->supported[EXT_BLEND_EQUATION_SEPARATE] && gl_info->supported[EXT_BLEND_FUNC_SEPARATE])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_SEPARATEALPHABLEND;
    if (gl_info->supported[EXT_DRAW_BUFFERS2])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_INDEPENDENTWRITEMASKS;
    if (gl_info->supported[ARB_FRAMEBUFFER_SRGB])
        caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_POSTBLENDSRGBCONVERT;

    caps->RasterCaps               = WINED3DPRASTERCAPS_DITHER    |
                                     WINED3DPRASTERCAPS_PAT       |
                                     WINED3DPRASTERCAPS_WFOG      |
                                     WINED3DPRASTERCAPS_ZFOG      |
                                     WINED3DPRASTERCAPS_FOGVERTEX |
                                     WINED3DPRASTERCAPS_FOGTABLE  |
                                     WINED3DPRASTERCAPS_STIPPLE   |
                                     WINED3DPRASTERCAPS_SUBPIXEL  |
                                     WINED3DPRASTERCAPS_ZTEST     |
                                     WINED3DPRASTERCAPS_SCISSORTEST   |
                                     WINED3DPRASTERCAPS_SLOPESCALEDEPTHBIAS |
                                     WINED3DPRASTERCAPS_DEPTHBIAS;

    if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
    {
        caps->RasterCaps  |= WINED3DPRASTERCAPS_ANISOTROPY    |
                             WINED3DPRASTERCAPS_ZBIAS         |
                             WINED3DPRASTERCAPS_MIPMAPLODBIAS;
    }

    caps->ZCmpCaps =  WINED3DPCMPCAPS_ALWAYS       |
                      WINED3DPCMPCAPS_EQUAL        |
                      WINED3DPCMPCAPS_GREATER      |
                      WINED3DPCMPCAPS_GREATEREQUAL |
                      WINED3DPCMPCAPS_LESS         |
                      WINED3DPCMPCAPS_LESSEQUAL    |
                      WINED3DPCMPCAPS_NEVER        |
                      WINED3DPCMPCAPS_NOTEQUAL;

    /* WINED3DPBLENDCAPS_BOTHINVSRCALPHA and WINED3DPBLENDCAPS_BOTHSRCALPHA
     * are legacy settings for srcblend only. */
    caps->SrcBlendCaps  =  WINED3DPBLENDCAPS_BOTHINVSRCALPHA |
                           WINED3DPBLENDCAPS_BOTHSRCALPHA    |
                           WINED3DPBLENDCAPS_DESTALPHA       |
                           WINED3DPBLENDCAPS_DESTCOLOR       |
                           WINED3DPBLENDCAPS_INVDESTALPHA    |
                           WINED3DPBLENDCAPS_INVDESTCOLOR    |
                           WINED3DPBLENDCAPS_INVSRCALPHA     |
                           WINED3DPBLENDCAPS_INVSRCCOLOR     |
                           WINED3DPBLENDCAPS_ONE             |
                           WINED3DPBLENDCAPS_SRCALPHA        |
                           WINED3DPBLENDCAPS_SRCALPHASAT     |
                           WINED3DPBLENDCAPS_SRCCOLOR        |
                           WINED3DPBLENDCAPS_ZERO;

    caps->DestBlendCaps =  WINED3DPBLENDCAPS_DESTALPHA       |
                           WINED3DPBLENDCAPS_DESTCOLOR       |
                           WINED3DPBLENDCAPS_INVDESTALPHA    |
                           WINED3DPBLENDCAPS_INVDESTCOLOR    |
                           WINED3DPBLENDCAPS_INVSRCALPHA     |
                           WINED3DPBLENDCAPS_INVSRCCOLOR     |
                           WINED3DPBLENDCAPS_ONE             |
                           WINED3DPBLENDCAPS_SRCALPHA        |
                           WINED3DPBLENDCAPS_SRCCOLOR        |
                           WINED3DPBLENDCAPS_ZERO;

    if (gl_info->supported[ARB_BLEND_FUNC_EXTENDED])
        caps->DestBlendCaps |= WINED3DPBLENDCAPS_SRCALPHASAT;

    if (gl_info->supported[EXT_BLEND_COLOR])
    {
        caps->SrcBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
        caps->DestBlendCaps |= WINED3DPBLENDCAPS_BLENDFACTOR;
    }


    caps->AlphaCmpCaps  = WINED3DPCMPCAPS_ALWAYS       |
                          WINED3DPCMPCAPS_EQUAL        |
                          WINED3DPCMPCAPS_GREATER      |
                          WINED3DPCMPCAPS_GREATEREQUAL |
                          WINED3DPCMPCAPS_LESS         |
                          WINED3DPCMPCAPS_LESSEQUAL    |
                          WINED3DPCMPCAPS_NEVER        |
                          WINED3DPCMPCAPS_NOTEQUAL;

    caps->ShadeCaps      = WINED3DPSHADECAPS_SPECULARGOURAUDRGB |
                           WINED3DPSHADECAPS_COLORGOURAUDRGB    |
                           WINED3DPSHADECAPS_ALPHAFLATBLEND     |
                           WINED3DPSHADECAPS_ALPHAGOURAUDBLEND  |
                           WINED3DPSHADECAPS_COLORFLATRGB       |
                           WINED3DPSHADECAPS_FOGFLAT            |
                           WINED3DPSHADECAPS_FOGGOURAUD         |
                           WINED3DPSHADECAPS_SPECULARFLATRGB;

    caps->TextureCaps   = WINED3DPTEXTURECAPS_ALPHA              |
                          WINED3DPTEXTURECAPS_ALPHAPALETTE       |
                          WINED3DPTEXTURECAPS_TRANSPARENCY       |
                          WINED3DPTEXTURECAPS_BORDER             |
                          WINED3DPTEXTURECAPS_MIPMAP             |
                          WINED3DPTEXTURECAPS_PROJECTED          |
                          WINED3DPTEXTURECAPS_PERSPECTIVE;

    if (!d3d_info->texture_npot)
    {
        caps->TextureCaps |= WINED3DPTEXTURECAPS_POW2;
        if (d3d_info->texture_npot_conditional)
            caps->TextureCaps |= WINED3DPTEXTURECAPS_NONPOW2CONDITIONAL;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->TextureCaps |= WINED3DPTEXTURECAPS_VOLUMEMAP
                | WINED3DPTEXTURECAPS_MIPVOLUMEMAP;
        if (!d3d_info->texture_npot)
            caps->TextureCaps |= WINED3DPTEXTURECAPS_VOLUMEMAP_POW2;
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        caps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP
                | WINED3DPTEXTURECAPS_MIPCUBEMAP;
        if (!d3d_info->texture_npot)
            caps->TextureCaps |= WINED3DPTEXTURECAPS_CUBEMAP_POW2;
    }

    caps->TextureFilterCaps =  WINED3DPTFILTERCAPS_MAGFLINEAR       |
                               WINED3DPTFILTERCAPS_MAGFPOINT        |
                               WINED3DPTFILTERCAPS_MINFLINEAR       |
                               WINED3DPTFILTERCAPS_MINFPOINT        |
                               WINED3DPTFILTERCAPS_MIPFLINEAR       |
                               WINED3DPTFILTERCAPS_MIPFPOINT        |
                               WINED3DPTFILTERCAPS_LINEAR           |
                               WINED3DPTFILTERCAPS_LINEARMIPLINEAR  |
                               WINED3DPTFILTERCAPS_LINEARMIPNEAREST |
                               WINED3DPTFILTERCAPS_MIPLINEAR        |
                               WINED3DPTFILTERCAPS_MIPNEAREST       |
                               WINED3DPTFILTERCAPS_NEAREST;

    if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
    {
        caps->TextureFilterCaps  |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                    WINED3DPTFILTERCAPS_MINFANISOTROPIC;
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        caps->CubeTextureFilterCaps =  WINED3DPTFILTERCAPS_MAGFLINEAR       |
                                       WINED3DPTFILTERCAPS_MAGFPOINT        |
                                       WINED3DPTFILTERCAPS_MINFLINEAR       |
                                       WINED3DPTFILTERCAPS_MINFPOINT        |
                                       WINED3DPTFILTERCAPS_MIPFLINEAR       |
                                       WINED3DPTFILTERCAPS_MIPFPOINT        |
                                       WINED3DPTFILTERCAPS_LINEAR           |
                                       WINED3DPTFILTERCAPS_LINEARMIPLINEAR  |
                                       WINED3DPTFILTERCAPS_LINEARMIPNEAREST |
                                       WINED3DPTFILTERCAPS_MIPLINEAR        |
                                       WINED3DPTFILTERCAPS_MIPNEAREST       |
                                       WINED3DPTFILTERCAPS_NEAREST;

        if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
        {
            caps->CubeTextureFilterCaps  |= WINED3DPTFILTERCAPS_MAGFANISOTROPIC |
                                            WINED3DPTFILTERCAPS_MINFANISOTROPIC;
        }
    }
    else
    {
        caps->CubeTextureFilterCaps = 0;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->VolumeTextureFilterCaps  = WINED3DPTFILTERCAPS_MAGFLINEAR       |
                                         WINED3DPTFILTERCAPS_MAGFPOINT        |
                                         WINED3DPTFILTERCAPS_MINFLINEAR       |
                                         WINED3DPTFILTERCAPS_MINFPOINT        |
                                         WINED3DPTFILTERCAPS_MIPFLINEAR       |
                                         WINED3DPTFILTERCAPS_MIPFPOINT        |
                                         WINED3DPTFILTERCAPS_LINEAR           |
                                         WINED3DPTFILTERCAPS_LINEARMIPLINEAR  |
                                         WINED3DPTFILTERCAPS_LINEARMIPNEAREST |
                                         WINED3DPTFILTERCAPS_MIPLINEAR        |
                                         WINED3DPTFILTERCAPS_MIPNEAREST       |
                                         WINED3DPTFILTERCAPS_NEAREST;
    }
    else
    {
        caps->VolumeTextureFilterCaps = 0;
    }

    caps->TextureAddressCaps  =  WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                 WINED3DPTADDRESSCAPS_CLAMP  |
                                 WINED3DPTADDRESSCAPS_WRAP;

    if (gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
    {
        caps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
    }
    if (gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT])
    {
        caps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
    }
    if (gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE])
    {
        caps->TextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        caps->VolumeTextureAddressCaps =   WINED3DPTADDRESSCAPS_INDEPENDENTUV |
                                           WINED3DPTADDRESSCAPS_CLAMP  |
                                           WINED3DPTADDRESSCAPS_WRAP;
        if (gl_info->supported[ARB_TEXTURE_BORDER_CLAMP])
        {
            caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_BORDER;
        }
        if (gl_info->supported[ARB_TEXTURE_MIRRORED_REPEAT])
        {
            caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRROR;
        }
        if (gl_info->supported[ARB_TEXTURE_MIRROR_CLAMP_TO_EDGE])
        {
            caps->VolumeTextureAddressCaps |= WINED3DPTADDRESSCAPS_MIRRORONCE;
        }
    }
    else
    {
        caps->VolumeTextureAddressCaps = 0;
    }

    caps->LineCaps  = WINED3DLINECAPS_TEXTURE       |
                      WINED3DLINECAPS_ZTEST         |
                      WINED3DLINECAPS_BLEND         |
                      WINED3DLINECAPS_ALPHACMP      |
                      WINED3DLINECAPS_FOG;
    /* WINED3DLINECAPS_ANTIALIAS is not supported on Windows, and dx and gl seem to have a different
     * idea how generating the smoothing alpha values works; the result is different
     */

    caps->MaxTextureWidth = d3d_info->limits.texture_size;
    caps->MaxTextureHeight = d3d_info->limits.texture_size;

    if (gl_info->supported[EXT_TEXTURE3D])
        caps->MaxVolumeExtent = gl_info->limits.texture3d_size;
    else
        caps->MaxVolumeExtent = 0;

    caps->MaxTextureRepeat = 32768;
    caps->MaxTextureAspectRatio = d3d_info->limits.texture_size;
    caps->MaxVertexW = 1.0f;

    caps->GuardBandLeft = 0.0f;
    caps->GuardBandTop = 0.0f;
    caps->GuardBandRight = 0.0f;
    caps->GuardBandBottom = 0.0f;

    caps->ExtentsAdjust = 0.0f;

    caps->StencilCaps   = WINED3DSTENCILCAPS_DECRSAT |
                          WINED3DSTENCILCAPS_INCRSAT |
                          WINED3DSTENCILCAPS_INVERT  |
                          WINED3DSTENCILCAPS_KEEP    |
                          WINED3DSTENCILCAPS_REPLACE |
                          WINED3DSTENCILCAPS_ZERO;
    if (gl_info->supported[EXT_STENCIL_WRAP])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_DECR  |
                              WINED3DSTENCILCAPS_INCR;
    }
    if (gl_info->supported[WINED3D_GL_VERSION_2_0] || gl_info->supported[EXT_STENCIL_TWO_SIDE]
            || gl_info->supported[ATI_SEPARATE_STENCIL])
    {
        caps->StencilCaps |= WINED3DSTENCILCAPS_TWOSIDED;
    }

    caps->MaxAnisotropy = gl_info->limits.anisotropy;
    caps->MaxPointSize = d3d_info->limits.pointsize_max;

    caps->MaxPrimitiveCount   = 0x555555; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    caps->MaxVertexIndex      = 0xffffff; /* Taken from an AMD Radeon HD 5700 (Evergreen) GPU. */
    caps->MaxStreams          = MAX_STREAMS;
    caps->MaxStreamStride     = 1024;

    /* d3d9.dll sets D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES here because StretchRects is implemented in d3d9 */
    caps->DevCaps2                          = WINED3DDEVCAPS2_STREAMOFFSET |
                                              WINED3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET;
    caps->MaxNpatchTessellationLevel        = 0;
    caps->MasterAdapterOrdinal              = 0;
    caps->AdapterOrdinalInGroup             = 0;
    caps->NumberOfAdaptersInGroup           = 1;

    caps->NumSimultaneousRTs = d3d_info->limits.max_rt_count;

    caps->StretchRectFilterCaps               = WINED3DPTFILTERCAPS_MINFPOINT  |
                                                WINED3DPTFILTERCAPS_MAGFPOINT  |
                                                WINED3DPTFILTERCAPS_MINFLINEAR |
                                                WINED3DPTFILTERCAPS_MAGFLINEAR;
    caps->VertexTextureFilterCaps             = 0;

    adapter->shader_backend->shader_get_caps(gl_info, &shader_caps);
    adapter->fragment_pipe->get_caps(gl_info, &fragment_caps);
    adapter->vertex_pipe->vp_get_caps(gl_info, &vertex_caps);

    /* Add shader misc caps. Only some of them belong to the shader parts of the pipeline */
    caps->PrimitiveMiscCaps |= fragment_caps.PrimitiveMiscCaps;

    caps->VertexShaderVersion = shader_caps.vs_version;
    caps->MaxVertexShaderConst = shader_caps.vs_uniform_count;

    caps->PixelShaderVersion = shader_caps.ps_version;
    caps->PixelShader1xMaxValue = shader_caps.ps_1x_max_value;

    caps->TextureOpCaps                    = fragment_caps.TextureOpCaps;
    caps->MaxTextureBlendStages            = fragment_caps.MaxTextureBlendStages;
    caps->MaxSimultaneousTextures          = fragment_caps.MaxSimultaneousTextures;

    caps->MaxUserClipPlanes                = vertex_caps.max_user_clip_planes;
    caps->MaxActiveLights                  = vertex_caps.max_active_lights;
    caps->MaxVertexBlendMatrices           = vertex_caps.max_vertex_blend_matrices;
    caps->MaxVertexBlendMatrixIndex        = vertex_caps.max_vertex_blend_matrix_index;
    caps->VertexProcessingCaps             = vertex_caps.vertex_processing_caps;
    caps->FVFCaps                          = vertex_caps.fvf_caps;
    caps->RasterCaps                      |= vertex_caps.raster_caps;

    /* The following caps are shader specific, but they are things we cannot detect, or which
     * are the same among all shader models. So to avoid code duplication set the shader version
     * specific, but otherwise constant caps here
     */
    if (caps->VertexShaderVersion >= 3)
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the VS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * VS3.0 value. */
        caps->VS20Caps.caps = WINED3DVS20CAPS_PREDICATION;
        /* VS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        caps->VS20Caps.dynamic_flow_control_depth = WINED3DVS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        caps->VS20Caps.temp_count = max(32, gl_info->limits.arb_vs_temps);
        /* level of nesting in loops / if-statements; VS 3.0 requires MAX (4) */
        caps->VS20Caps.static_flow_control_depth = WINED3DVS20_MAX_STATICFLOWCONTROLDEPTH;

        caps->MaxVShaderInstructionsExecuted    = 65535; /* VS 3.0 needs at least 65535, some cards even use 2^32-1 */
        caps->MaxVertexShader30InstructionSlots = max(512, gl_info->limits.arb_vs_instructions);
        caps->VertexTextureFilterCaps = WINED3DPTFILTERCAPS_MINFPOINT | WINED3DPTFILTERCAPS_MAGFPOINT;
    }
    else if (caps->VertexShaderVersion == 2)
    {
        caps->VS20Caps.caps = 0;
        caps->VS20Caps.dynamic_flow_control_depth = WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH;
        caps->VS20Caps.temp_count = max(12, gl_info->limits.arb_vs_temps);
        caps->VS20Caps.static_flow_control_depth = 1;

        caps->MaxVShaderInstructionsExecuted    = 65535;
        caps->MaxVertexShader30InstructionSlots = 0;
    }
    else
    { /* VS 1.x */
        caps->VS20Caps.caps = 0;
        caps->VS20Caps.dynamic_flow_control_depth = 0;
        caps->VS20Caps.temp_count = 0;
        caps->VS20Caps.static_flow_control_depth = 0;

        caps->MaxVShaderInstructionsExecuted    = 0;
        caps->MaxVertexShader30InstructionSlots = 0;
    }

    if (caps->PixelShaderVersion >= 3)
    {
        /* Where possible set the caps based on OpenGL extensions and if they
         * aren't set (in case of software rendering) use the PS 3.0 from
         * MSDN or else if there's OpenGL spec use a hardcoded value minimum
         * PS 3.0 value. */

        /* Caps is more or less undocumented on MSDN but it appears to be
         * used for PS20Caps based on results from R9600/FX5900/Geforce6800
         * cards from Windows */
        caps->PS20Caps.caps = WINED3DPS20CAPS_ARBITRARYSWIZZLE |
                WINED3DPS20CAPS_GRADIENTINSTRUCTIONS |
                WINED3DPS20CAPS_PREDICATION          |
                WINED3DPS20CAPS_NODEPENDENTREADLIMIT |
                WINED3DPS20CAPS_NOTEXINSTRUCTIONLIMIT;
        /* PS 3.0 requires MAX_DYNAMICFLOWCONTROLDEPTH (24) */
        caps->PS20Caps.dynamic_flow_control_depth = WINED3DPS20_MAX_DYNAMICFLOWCONTROLDEPTH;
        caps->PS20Caps.temp_count = max(32, gl_info->limits.arb_ps_temps);
        /* PS 3.0 requires MAX_STATICFLOWCONTROLDEPTH (4) */
        caps->PS20Caps.static_flow_control_depth = WINED3DPS20_MAX_STATICFLOWCONTROLDEPTH;
        /* PS 3.0 requires MAX_NUMINSTRUCTIONSLOTS (512) */
        caps->PS20Caps.instruction_slot_count = WINED3DPS20_MAX_NUMINSTRUCTIONSLOTS;

        caps->MaxPShaderInstructionsExecuted = 65535;
        caps->MaxPixelShader30InstructionSlots = max(WINED3DMIN30SHADERINSTRUCTIONS,
                gl_info->limits.arb_ps_instructions);
    }
    else if(caps->PixelShaderVersion == 2)
    {
        /* Below we assume PS2.0 specs, not extended 2.0a(GeforceFX)/2.0b(Radeon R3xx) ones */
        caps->PS20Caps.caps = 0;
        caps->PS20Caps.dynamic_flow_control_depth = 0; /* WINED3DVS20_MIN_DYNAMICFLOWCONTROLDEPTH = 0 */
        caps->PS20Caps.temp_count = max(12, gl_info->limits.arb_ps_temps);
        caps->PS20Caps.static_flow_control_depth = WINED3DPS20_MIN_STATICFLOWCONTROLDEPTH; /* Minimum: 1 */
        /* Minimum number (64 ALU + 32 Texture), a GeforceFX uses 512 */
        caps->PS20Caps.instruction_slot_count = WINED3DPS20_MIN_NUMINSTRUCTIONSLOTS;

        caps->MaxPShaderInstructionsExecuted    = 512; /* Minimum value, a GeforceFX uses 1024 */
        caps->MaxPixelShader30InstructionSlots  = 0;
    }
    else /* PS 1.x */
    {
        caps->PS20Caps.caps = 0;
        caps->PS20Caps.dynamic_flow_control_depth = 0;
        caps->PS20Caps.temp_count = 0;
        caps->PS20Caps.static_flow_control_depth = 0;
        caps->PS20Caps.instruction_slot_count = 0;

        caps->MaxPShaderInstructionsExecuted    = 0;
        caps->MaxPixelShader30InstructionSlots  = 0;
    }

    if (caps->VertexShaderVersion >= 2)
    {
        /* OpenGL supports all the formats below, perhaps not always
         * without conversion, but it supports them.
         * Further GLSL doesn't seem to have an official unsigned type so
         * don't advertise it yet as I'm not sure how we handle it.
         * We might need to add some clamping in the shader engine to
         * support it.
         * TODO: WINED3DDTCAPS_USHORT2N, WINED3DDTCAPS_USHORT4N, WINED3DDTCAPS_UDEC3, WINED3DDTCAPS_DEC3N */
        caps->DeclTypes = WINED3DDTCAPS_UBYTE4    |
                          WINED3DDTCAPS_UBYTE4N   |
                          WINED3DDTCAPS_SHORT2N   |
                          WINED3DDTCAPS_SHORT4N;
        if (gl_info->supported[ARB_HALF_FLOAT_VERTEX])
        {
            caps->DeclTypes |= WINED3DDTCAPS_FLOAT16_2 |
                               WINED3DDTCAPS_FLOAT16_4;
        }
    }
    else
    {
        caps->DeclTypes = 0;
    }

    /* Set DirectDraw helper Caps */
    ckey_caps =                         WINEDDCKEYCAPS_DESTBLT              |
                                        WINEDDCKEYCAPS_SRCBLT;
    fx_caps =                           WINEDDFXCAPS_BLTALPHA               |
                                        WINEDDFXCAPS_BLTMIRRORLEFTRIGHT     |
                                        WINEDDFXCAPS_BLTMIRRORUPDOWN        |
                                        WINEDDFXCAPS_BLTROTATION90          |
                                        WINEDDFXCAPS_BLTSHRINKX             |
                                        WINEDDFXCAPS_BLTSHRINKXN            |
                                        WINEDDFXCAPS_BLTSHRINKY             |
                                        WINEDDFXCAPS_BLTSHRINKYN            |
                                        WINEDDFXCAPS_BLTSTRETCHX            |
                                        WINEDDFXCAPS_BLTSTRETCHXN           |
                                        WINEDDFXCAPS_BLTSTRETCHY            |
                                        WINEDDFXCAPS_BLTSTRETCHYN;
    blit_caps =                         WINEDDCAPS_BLT                      |
                                        WINEDDCAPS_BLTCOLORFILL             |
                                        WINEDDCAPS_BLTDEPTHFILL             |
                                        WINEDDCAPS_BLTSTRETCH               |
                                        WINEDDCAPS_CANBLTSYSMEM             |
                                        WINEDDCAPS_CANCLIP                  |
                                        WINEDDCAPS_CANCLIPSTRETCHED         |
                                        WINEDDCAPS_COLORKEY                 |
                                        WINEDDCAPS_COLORKEYHWASSIST;

    /* Fill the ddraw caps structure */
    caps->ddraw_caps.caps =             WINEDDCAPS_GDI                      |
                                        WINEDDCAPS_PALETTE                  |
                                        blit_caps;
    caps->ddraw_caps.caps2 =            WINEDDCAPS2_CERTIFIED               |
                                        WINEDDCAPS2_NOPAGELOCKREQUIRED      |
                                        WINEDDCAPS2_PRIMARYGAMMA            |
                                        WINEDDCAPS2_WIDESURFACES            |
                                        WINEDDCAPS2_CANRENDERWINDOWED;
    caps->ddraw_caps.color_key_caps = ckey_caps;
    caps->ddraw_caps.fx_caps = fx_caps;
    caps->ddraw_caps.svb_caps = blit_caps;
    caps->ddraw_caps.svb_color_key_caps = ckey_caps;
    caps->ddraw_caps.svb_fx_caps = fx_caps;
    caps->ddraw_caps.vsb_caps = blit_caps;
    caps->ddraw_caps.vsb_color_key_caps = ckey_caps;
    caps->ddraw_caps.vsb_fx_caps = fx_caps;
    caps->ddraw_caps.ssb_caps = blit_caps;
    caps->ddraw_caps.ssb_color_key_caps = ckey_caps;
    caps->ddraw_caps.ssb_fx_caps = fx_caps;

    caps->ddraw_caps.dds_caps = WINEDDSCAPS_ALPHA
            | WINEDDSCAPS_BACKBUFFER
            | WINEDDSCAPS_FLIP
            | WINEDDSCAPS_FRONTBUFFER
            | WINEDDSCAPS_OFFSCREENPLAIN
            | WINEDDSCAPS_PALETTE
            | WINEDDSCAPS_PRIMARYSURFACE
            | WINEDDSCAPS_SYSTEMMEMORY
            | WINEDDSCAPS_VISIBLE;

    if (!(wined3d->flags & WINED3D_NO3D))
    {
        caps->ddraw_caps.dds_caps |= WINEDDSCAPS_3DDEVICE
                | WINEDDSCAPS_MIPMAP
                | WINEDDSCAPS_TEXTURE
                | WINEDDSCAPS_VIDEOMEMORY
                | WINEDDSCAPS_ZBUFFER;
        caps->ddraw_caps.caps |= WINEDDCAPS_3D;
    }

    caps->shader_double_precision = d3d_info->shader_double_precision;
    caps->viewport_array_index_any_shader = d3d_info->viewport_array_index_any_shader;

    caps->max_feature_level = d3d_info->feature_level;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_create(struct wined3d *wined3d, unsigned int adapter_idx,
        enum wined3d_device_type device_type, HWND focus_window, DWORD flags, BYTE surface_alignment,
        const enum wined3d_feature_level *feature_levels, unsigned int feature_level_count,
        struct wined3d_device_parent *device_parent, struct wined3d_device **device)
{
    struct wined3d_device_gl *device_gl;
    HRESULT hr;

    TRACE("wined3d %p, adapter_idx %u, device_type %#x, focus_window %p, flags %#x, "
            "surface_alignment %u, feature_levels %p, feature_level_count %u, device_parent %p, device %p.\n",
            wined3d, adapter_idx, device_type, focus_window, flags, surface_alignment,
            feature_levels, feature_level_count, device_parent, device);

    if (adapter_idx >= wined3d->adapter_count)
        return WINED3DERR_INVALIDCALL;

    if (!(device_gl = heap_alloc_zero(sizeof(*device_gl))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = device_init(&device_gl->d, wined3d, adapter_idx,
            device_type, focus_window, flags, surface_alignment,
            feature_levels, feature_level_count, device_parent)))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        heap_free(device_gl);
        return hr;
    }

    TRACE("Created device %p.\n", device_gl);
    *device = &device_gl->d;

    device_parent->ops->wined3d_device_created(device_parent, *device);

    return WINED3D_OK;
}

static BOOL wined3d_adapter_no3d_create_context(struct wined3d_context *context,
        struct wined3d_texture *target, const struct wined3d_format *ds_format)
{
    return TRUE;
}

static const struct wined3d_adapter_ops wined3d_adapter_no3d_ops =
{
    wined3d_adapter_no3d_create_context,
};

static void wined3d_adapter_no3d_init_d3d_info(struct wined3d_adapter *adapter, DWORD wined3d_creation_flags)
{
    struct wined3d_d3d_info *d3d_info = &adapter->d3d_info;

    d3d_info->wined3d_creation_flags = wined3d_creation_flags;
    d3d_info->texture_npot = TRUE;
    d3d_info->feature_level = WINED3D_FEATURE_LEVEL_5;
}

static BOOL wined3d_adapter_no3d_init(struct wined3d_adapter *adapter, DWORD wined3d_creation_flags)
{
    static const struct wined3d_gpu_description gpu_description =
    {
        HW_VENDOR_SOFTWARE, CARD_WINE, "WineD3D DirectDraw Emulation", DRIVER_WINE, 128,
    };

    TRACE("adapter %p.\n", adapter);

    wined3d_driver_info_init(&adapter->driver_info, &gpu_description, 0);
    adapter->vram_bytes_used = 0;
    TRACE("Emulating 0x%s bytes of video ram.\n", wine_dbgstr_longlong(adapter->driver_info.vram_bytes));

    if (!wined3d_adapter_no3d_init_format_info(adapter))
        return FALSE;

    adapter->vertex_pipe = &none_vertex_pipe;
    adapter->fragment_pipe = &none_fragment_pipe;
    adapter->shader_backend = &none_shader_backend;
    adapter->adapter_ops = &wined3d_adapter_no3d_ops;

    wined3d_adapter_no3d_init_d3d_info(adapter, wined3d_creation_flags);

    return TRUE;
}

static BOOL wined3d_adapter_init(struct wined3d_adapter *adapter, unsigned int ordinal, DWORD wined3d_creation_flags)
{
    DISPLAY_DEVICEW display_device;

    adapter->ordinal = ordinal;

    display_device.cb = sizeof(display_device);
    EnumDisplayDevicesW(NULL, ordinal, &display_device, 0);
    TRACE("Display device: %s.\n", debugstr_w(display_device.DeviceName));
    strcpyW(adapter->device_name, display_device.DeviceName);

    if (!AllocateLocallyUniqueId(&adapter->luid))
    {
        ERR("Failed to set adapter LUID (%#x).\n", GetLastError());
        return FALSE;
    }
    TRACE("Allocated LUID %08x:%08x for adapter %p.\n",
            adapter->luid.HighPart, adapter->luid.LowPart, adapter);

    adapter->formats = NULL;

    if (wined3d_creation_flags & WINED3D_NO3D)
        return wined3d_adapter_no3d_init(adapter, wined3d_creation_flags);
    return wined3d_adapter_gl_init(adapter, wined3d_creation_flags);
}

static void STDMETHODCALLTYPE wined3d_null_wined3d_object_destroyed(void *parent) {}

const struct wined3d_parent_ops wined3d_null_parent_ops =
{
    wined3d_null_wined3d_object_destroyed,
};

HRESULT wined3d_init(struct wined3d *wined3d, DWORD flags)
{
    wined3d->ref = 1;
    wined3d->flags = flags;

    TRACE("Initialising adapters.\n");

    if (!wined3d_adapter_init(&wined3d->adapters[0], 0, flags))
    {
        WARN("Failed to initialise adapter.\n");
        return E_FAIL;
    }
    wined3d->adapter_count = 1;

    return WINED3D_OK;
}
