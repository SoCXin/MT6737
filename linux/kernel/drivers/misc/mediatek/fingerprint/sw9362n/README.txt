1. �鿴ģ����Ϣ��
   cat sys/devices/virtual/misc/sunwave_fp/sunwave/chip_info
   ��ʾ���£�
    chip   : xxxx
    id     : xxxx
    vendor : sunwave
    more   : fingerprint

2. �������Կ��أ�
   �鿴����״̬��cat sys/devices/virtual/misc/sunwave_fp/sunwave/debug
   ��ʾ���£�
   debug_level = 1 
   �򿪣�echo 1 > sys/devices/virtual/misc/sunwave_fp/sunwave/debug
   �رգ�echo 0 > sys/devices/virtual/misc/sunwave_fp/sunwave/debug

3. �鿴�����汾��
   cat sys/devices/virtual/misc/sunwave_fp/sunwave/version
   ��ʾ���£�
   v0.71.20160903
   
4. config.h�еĺ꿪�ؽ��ܣ�
   __PALTFORM_SPREAD_EN          : չѶƽ̨֧�� 

   __SUNWAVE_DETECT_ID_EN        �� ��ID���� ��ѡ
   __SUNWAVE_QUIK_WK_CPU_EN      �� CPU���ٻ��� Ĭ�ϴ�
   __SUNWAVE_PRIZE_HW_EN         �� ֧�ֿ�����Ӳ����Ϣ Ĭ�Ϲر�

   //optional
   __SUNWAVE_SPI_DMA_MODE_EN     �� SPI���䷽ʽѡ�� 1��DMA 0��FIFO
   __SUNWAVE_KEY_BACK_EN         �� ģ�鰴��֧�֣���ֵ��KEY_BACK
   __SUNWAVE_SCREEN_LOCK_EN      �� ��Ļ��Ϣ�ϱ���֧�������������������ܽ�������
   __SUNWAVE_HW_INFO_EN          �� ֧������Ӳ����Ϣ����