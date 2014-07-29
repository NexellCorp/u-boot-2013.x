#include <common.h>
#include <platform.h>
#include <mlc.h>

static struct nxp_mlc_rgb_overlay_param rgb_param = {
    .format = NX_MLC_RGBFMT_A8R8G8B8,
    .width  = 1024,
    .height = 600,
    .addr   = CONFIG_RGB_OVERLAY_ADDR,
};

void overlay_set(void)
{
    nxp_mlc_rgb_overlay_set_param(0, &rgb_param);
}

void overlay_draw()
{
    int i;
    u32 *addr = (u32 *)CONFIG_RGB_OVERLAY_ADDR;
    memset(addr, 0, 1024*600*4);
    addr += 1024 * 200;
    for (i = 0; i < 1024*10; i++) {
        *addr = 0xffff0000;
        addr++;
    }
}

void overlay_on()
{
    nxp_mlc_rgb_overlay_run(0);
}
