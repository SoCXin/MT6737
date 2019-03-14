#ifndef __DISP_DTS_GPIO_H__
#define __DISP_DTS_GPIO_H__

/*
 * This module helps you to set GPIO pin according to linux device tree (DTS). To
 * use this module, you MUST init this module once before any operation.
 */

#include <linux/platform_device.h> /* struct platform_device */

/* DTS state */
typedef enum tagDTS_GPIO_STATE {
	DTS_GPIO_STATE_TE_MODE_GPIO = 0,    /* mode_te_gpio */
	DTS_GPIO_STATE_TE_MODE_TE,          /* mode_te_te */
	DTS_GPIO_STATE_PWM_TEST_PINMUX_55,  /* pwm_test_pin_mux_gpio55 */
	DTS_GPIO_STATE_PWM_TEST_PINMUX_69,  /* pwm_test_pin_mux_gpio69 */
	DTS_GPIO_STATE_PWM_TEST_PINMUX_129, /* pwm_test_pin_mux_gpio129 */

	DTS_GPIO_STATE_MAX,                 /* for array size */
} DTS_GPIO_STATE;

/* this function MUST be called in mtkfb_probe.
 *  @param *pdev    - reference of struct platform_device which contains pinctrl
 *                    state information of GPIO
 *  @return         - 0 for OK, otherwise returns PTR_ERR(pdev).
 */
long    disp_dts_gpio_init(struct platform_device *pdev);

/* set gpio according sepcified DTS state.
 *  @notice         - to call this function, you MUST init this module first.
 *                    If not, we will trigger BUG_ON(0).
 *  @param s        - state which describes GPIO statement.
 *  @return         - 0 for OK, otherwise returns PTR_ERR(pdev).
 */
long    disp_dts_gpio_select_state(DTS_GPIO_STATE s);

/* repo of initialization */
#ifdef CONFIG_MTK_LEGACY
#define disp_dts_gpio_init_repo(x)  (0)
#else
#define disp_dts_gpio_init_repo(x)  (disp_dts_gpio_init(x))
#endif

#endif/*__DISP_DTS_GPIO_H__ */
