#define PLATFORM_DRIVER_NAME "fy6203"

	
struct pinctrl *fy6203ctrl = NULL; 
struct pinctrl_state *en_set_low = NULL;
struct pinctrl_state *en_set_high = NULL;

#define FY6203_SET_EN_LOW()			  pinctrl_select_state(fy6203ctrl, en_set_low);
#define FY6203_SET_EN_HIGH()			pinctrl_select_state(fy6203ctrl, en_set_high);