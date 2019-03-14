#include "finger.h"


#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <linux/wakelock.h>
#include "sunwavecorp.h"
#include "config.h"

#ifdef _DRV_TAG_
#undef _DRV_TAG_
#endif
#define _DRV_TAG_ "misc"

#define USER_KEY_F11              87
#define USER_KEY_ENTER            28
#define USER_KEY_UP               103
#define USER_KEY_LEFT             105
#define USER_KEY_RIGHT            106
#define USER_KEY_DOWN             108

#if __SUNWAVE_KEY_BACK_EN
#define USER_KEY_BACK             158
#endif


extern u8 suspend_flag;

#if __SUNWAVE_QUIK_WK_CPU_EN
//add for open core begin
#include <linux/workqueue.h>
#include <linux/cpu.h>
extern struct work_struct core_work;
extern struct workqueue_struct* core_queue;
//add for open core end
#endif

int create_input_device(sunwave_sensor_t* sunwave)
{
    int status ;
    struct input_dev* input_dev = NULL;;
    input_dev = input_allocate_device();

    if (!input_dev) {
        dev_err(&sunwave->spi->dev, "Failed to allocate memory\n");
        return -ENOMEM;
    }

    input_dev->name = "sunwave_btn";
    //__set_bit(EV_ABS, input_dev->evbit);
    __set_bit(EV_KEY, input_dev->evbit);
    //__set_bit(BTN_TOUCH, input_dev->keybit);
    //__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
    input_set_capability(input_dev, EV_KEY, KEY_F11);
    input_set_capability(input_dev, EV_KEY, KEY_ENTER);
    input_set_capability(input_dev, EV_KEY, KEY_UP);
    input_set_capability(input_dev, EV_KEY, KEY_LEFT );
    input_set_capability(input_dev, EV_KEY, KEY_RIGHT);
    input_set_capability(input_dev, EV_KEY, KEY_DOWN);
    input_set_capability(input_dev, EV_KEY, KEY_WAKEUP);
#if __SUNWAVE_KEY_BACK_EN
    input_set_capability(input_dev, EV_KEY, KEY_BACK);
#endif
    status = input_register_device(input_dev);

    if (status) {
        dev_err(&sunwave->spi->dev, "%s: failed to register input device: %s\n",
                __func__, dev_name(&sunwave->spi->dev));
        input_free_device(input_dev);
        return status;
    }

    sunwave->input = input_dev;
    return 0;
}
EXPORT_SYMBOL_GPL(create_input_device);

void release_input_device(sunwave_sensor_t* sunwave)
{
    if (sunwave->input == NULL) {
        return ;
    }

    input_free_device(sunwave->input);
    sunwave->input = NULL;
}
EXPORT_SYMBOL_GPL(release_input_device);



void  sunwave_key_report(sunwave_sensor_t* sunwave, int value, int down_up)
{
    int key_value = KEY_F11;

    switch (value) {
        case USER_KEY_ENTER:
            key_value = KEY_ENTER;
            break;

        case USER_KEY_UP :
            key_value = KEY_UP ;
            break;

        case USER_KEY_LEFT:
            key_value = KEY_LEFT;
            break;

        case USER_KEY_RIGHT:
            key_value = KEY_RIGHT;
            break;

        case USER_KEY_DOWN:
            key_value = KEY_DOWN;
            break;
#if __SUNWAVE_KEY_BACK_EN

        case USER_KEY_BACK:
            key_value = KEY_BACK;
            break;
#endif

        default:
            key_value = KEY_F11;
            break;
    }

    input_report_key(sunwave->input, key_value, down_up);
    input_sync(sunwave->input);
}

void  sunwave_wakeupSys(sunwave_sensor_t* sunwave)
{
    input_report_key(sunwave->input, KEY_WAKEUP, 1);
    input_sync(sunwave->input);
    msleep(1);
    //usleep(1000);
    /*WARNING:no sleep
     */
    input_report_key(sunwave->input, KEY_WAKEUP, 0);
    input_sync(sunwave->input);
}
EXPORT_SYMBOL_GPL(sunwave_wakeupSys);

irqreturn_t sunwave_irq_hander_default_process(int irq, void* dev)
{
    sunwave_sensor_t* sunwave = (sunwave_sensor_t*) dev ;
#if __SUNWAVE_QUIK_WK_CPU_EN
    //add for open core begin
    //schedule_work(&core_work);
    queue_work(core_queue, &core_work);
    //add for open core end
#endif
    schedule_work(&sunwave->irq_work);
    wake_lock_timeout(&sunwave->wakelock, msecs_to_jiffies(5000));
    return IRQ_HANDLED;
}
EXPORT_SYMBOL_GPL(sunwave_irq_hander_default_process);

void sunwave_irq_hander_default_process_2(void)
{
    sunwave_sensor_t* sunwave = get_current_sunwave();
    schedule_work(&sunwave->irq_work);
    wake_lock_timeout(&sunwave->wakelock, msecs_to_jiffies(5000));
}
EXPORT_SYMBOL_GPL(sunwave_irq_hander_default_process_2);

void sunwave_irq_hander_default_work(struct work_struct* work)
{
    char* finger[2] = { "SPI_STATE=finger", NULL };
    sunwave_sensor_t* sunwave = container_of(work, sunwave_sensor_t, irq_work);

    if (sunwave->finger && sunwave->finger->irq_hander) {
        sunwave->finger->irq_hander(&sunwave->spi);
    }

    kobject_uevent_env(&sunwave->spi->dev.kobj, KOBJ_CHANGE, finger);
    sw_dbg("irq: sunwave\n");
    sw_dbg("sunwave->standby_irq = %d \n", sunwave->standby_irq);
    return ;
}

int sunwave_irq_request(sunwave_sensor_t* sunwave)
{
    int status;
    // wake_lock_init(&sunwave->wakelock, WAKE_LOCK_SUSPEND, dev_name(&sunwave->spi->dev));
#ifdef CONFIG_OF

    if (sunwave->standby_irq > 0) {
#else

    if (sunwave->finger->gpio_irq > 0) {
#endif
        sunwave->spi->irq = sunwave->standby_irq;
        INIT_WORK(&sunwave->irq_work, sunwave_irq_hander_default_work);
        status = request_irq(sunwave->standby_irq, sunwave_irq_hander_default_process,  IRQF_TRIGGER_FALLING, "sw_irq",
                             sunwave);

        if (status) {
            sw_err("sunwave failed request_irq\n");
            sunwave->standby_irq = 0;
            return -1;
        }
        else {
            // enable_irq(sunwave->standby_irq);
            enable_irq_wake(sunwave->standby_irq);
        }

        return 0;
    }
    else {
        if (sunwave->finger && sunwave->finger->irq_request) {
            return sunwave->finger->irq_request(&sunwave->spi);
        }

        return -2;
    }

    return 0;
}

EXPORT_SYMBOL_GPL(sunwave_irq_request);

void  sunwave_irq_free(sunwave_sensor_t* sunwave)
{
    if (sunwave->standby_irq > 0) {
        if (sunwave->finger && sunwave->finger->irq_request) {
            sunwave->finger->irq_free(&sunwave->spi);
        }
        else {
            free_irq(sunwave->standby_irq, sunwave);
        }

        sunwave->standby_irq = 0;
    }
}
EXPORT_SYMBOL_GPL(sunwave_irq_free);
