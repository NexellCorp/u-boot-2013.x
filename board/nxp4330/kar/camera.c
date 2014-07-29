#include <common.h>
#include <platform.h>
#include <gpio.h>
#include <camera.h>
#include <vip.h>
#include <mlc.h>

static struct reg_val _sensor_init_data[] =
{
    {0x03, 0xa6},
    {0x07, 0x02},
    {0x08, 0x12},
    {0x09, 0xf0},
    {0x0a, 0x1c},
    /*{0x0b, 0xd0}, // 720 */
    {0x0b, 0xc0}, // 704
    {0x1b, 0x00},
    {0x10, 0x10},
    {0x11, 0x42},
    {0x2f, 0xe6},
    {0x55, 0x00},
    END_MARKER
};

#define CAMERA_RESET        (PAD_GPIO_C + 15)
#define CAMERA_POWER_DOWN   (PAD_GPIO_C + 16)

static int _sensor_power_enable(bool enable)
{
    u32 io = CAMERA_POWER_DOWN;
    u32 reset_io = CAMERA_RESET;

    if (enable) {
        // disable power down
        nxp_gpio_direction_output(io, 0);
        nxp_gpio_set_alt(io, 1);

        // reset to high
        nxp_gpio_direction_output(reset_io, 1);
        nxp_gpio_set_alt(reset_io, 1);
        mdelay(1);

        // reset to low
        nxp_gpio_set_value(reset_io, 0);
        mdelay(10);

        // reset to high
        nxp_gpio_set_value(reset_io, 1);
        mdelay(10);
    }
}

static void _sensor_setup_io(void)
{
    u_int *pad;
    int i, len;
    u_int io, fn;

    /* VIP0:0 = VCLK, VID0 ~ 7 */
    const u_int port[][2] = {
        /* VCLK, HSYNC, VSYNC */
        { PAD_GPIO_E +  4, NX_GPIO_PADFUNC_1 },
        { PAD_GPIO_E +  5, NX_GPIO_PADFUNC_1 },
        { PAD_GPIO_E +  6, NX_GPIO_PADFUNC_1 },
        /* DATA */
        { PAD_GPIO_D + 28, NX_GPIO_PADFUNC_1 }, { PAD_GPIO_D + 29, NX_GPIO_PADFUNC_1 },
        { PAD_GPIO_D + 30, NX_GPIO_PADFUNC_1 }, { PAD_GPIO_D + 31, NX_GPIO_PADFUNC_1 },
        { PAD_GPIO_E +  0, NX_GPIO_PADFUNC_1 }, { PAD_GPIO_E +  1, NX_GPIO_PADFUNC_1 },
        { PAD_GPIO_E +  2, NX_GPIO_PADFUNC_1 }, { PAD_GPIO_E +  3, NX_GPIO_PADFUNC_1 },
    };

    pad = (u_int *)port;
    len = sizeof(port)/sizeof(port[0]);

    for (i = 0; i < len; i++) {
        io = *pad++;
        fn = *pad++;
        nxp_gpio_direction_input(io);
        nxp_gpio_set_alt(io, fn);
    }
}

static struct camera_sensor_data _sensor_data = {
     .bus       = 0,
     .chip      = 0x88 >> 1,
     .reg_val   = _sensor_init_data,
     .power_enable = _sensor_power_enable,
     .set_clock = NULL,
     .setup_io  = _sensor_setup_io,
};

#define CAM_WIDTH       704
#define CAM_HEIGHT      480

static struct nxp_vip_param _vip_param = {
    // interlace
    .port           = 0,
    .external_sync  = false,
    .is_mipi        = false,
    .h_active       = CAM_WIDTH,
    .h_frontporch   = 7,
    .h_syncwidth    = 1,
    .h_backporch    = 10,
    .v_active       = CAM_HEIGHT,
    .v_frontporch   = 0,
    .v_syncwidth    = 2,
    .v_backporch    = 3,
    .data_order     = 0,
    .interlace      = true,
};

static struct nxp_mlc_video_param _mlc_param = {
    .format         = NX_MLC_YUVFMT_420,
    .width          = CAM_WIDTH,
    .height         = CAM_HEIGHT,
    .left           = 0,
    .top            = 0,
    .right          = 0,
    .bottom         = 0,
#if defined(CONFIG_NXP4330_MLC_RGB_OVERLAY)
    .priority       = 1,
#else
    .priority       = 0,
#endif
};

void camera_run(void)
{
    int camera_id = camera_register_sensor(&_sensor_data);
    camera_sensor_run(camera_id);

    nxp_vip_register_param(1, &_vip_param);
    nxp_vip_set_addr(1, CONFIG_VIP_LU_ADDR, CONFIG_VIP_CB_ADDR, CONFIG_VIP_CR_ADDR);

    nxp_mlc_video_set_param(0, &_mlc_param);
    /*nxp_mlc_video_set_addr(0, CONFIG_VIP_LU_ADDR, CONFIG_VIP_CB_ADDR, CONFIG_VIP_CR_ADDR, 704, 384, 384);*/
    nxp_mlc_video_set_addr(0, CONFIG_VIP_LU_ADDR, CONFIG_VIP_CB_ADDR, CONFIG_VIP_CR_ADDR,
            ALIGN(CAM_WIDTH, 64),
            ALIGN(CAM_WIDTH/2, 64),
            ALIGN(CAM_WIDTH/2, 64));
    printf("%s exit\n", __func__);
}

void camera_preview(void)
{
    /*int io = (PAD_GPIO_A + 18);*/
    /*int io = (PAD_GPIO_E + 12); // GMAC_TXER*/
    /*nxp_gpio_direction_input(io);*/
    /*nxp_gpio_set_alt(io, 0);*/
    /*if (nxp_gpio_get_value(io)) {*/
        printf("run vip & mlc\n");
        nxp_vip_run(1);
        nxp_mlc_video_run(0);
    /*}*/
    printf("%s exit\n", __func__);
}
