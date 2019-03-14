1. 查看模组信息：
   cat sys/devices/virtual/misc/sunwave_fp/sunwave/chip_info
   显示如下，
    chip   : xxxx
    id     : xxxx
    vendor : sunwave
    more   : fingerprint

2. 驱动调试开关：
   查看开关状态：cat sys/devices/virtual/misc/sunwave_fp/sunwave/debug
   显示如下，
   debug_level = 1 
   打开：echo 1 > sys/devices/virtual/misc/sunwave_fp/sunwave/debug
   关闭：echo 0 > sys/devices/virtual/misc/sunwave_fp/sunwave/debug

3. 查看驱动版本：
   cat sys/devices/virtual/misc/sunwave_fp/sunwave/version
   显示如下，
   v0.71.20160903
   
4. config.h中的宏开关介绍：
   __PALTFORM_SPREAD_EN          : 展讯平台支持 

   __SUNWAVE_DETECT_ID_EN        ： 读ID功能 可选
   __SUNWAVE_QUIK_WK_CPU_EN      ： CPU快速唤醒 默认打开
   __SUNWAVE_PRIZE_HW_EN         ： 支持酷赛的硬件信息 默认关闭

   //optional
   __SUNWAVE_SPI_DMA_MODE_EN     ： SPI传输方式选择 1：DMA 0：FIFO
   __SUNWAVE_KEY_BACK_EN         ： 模组按键支持，键值：KEY_BACK
   __SUNWAVE_SCREEN_LOCK_EN      ： 屏幕消息上报，支持亮屏解锁，灭屏不能解锁功能
   __SUNWAVE_HW_INFO_EN          ： 支持设置硬件信息功能