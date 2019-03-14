/*++

 File Name:    fpsensor_capture.h
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/


#ifndef LINUX_SPI_FPSENSOR_CAPTURE_H
#define LINUX_SPI_FPSENSOR_CAPTURE_H

extern int fpsensor_init_capture(fpsensor_data_t *fpsensor);

extern int fpsensor_write_capture_setup(fpsensor_data_t *fpsensor);
extern int fpsensor_write_gesture_setup(fpsensor_data_t *fpsensor, u8 sample_ratio);

extern int fpsensor_write_test_setup(fpsensor_data_t *fpsensor, u16 pattern);

extern bool fpsensor_capture_check_ready(fpsensor_data_t *fpsensor);

extern int fpsensor_capture_task(fpsensor_data_t *fpsensor);

extern int fpsensor_capture_wait_finger_down(fpsensor_data_t *fpsensor);

extern int fpsensor_capture_wait_finger_up(fpsensor_data_t *fpsensor);

extern int fpsensor_capture_settings(fpsensor_data_t *fpsensor, int select);

extern int fpsensor_capture_set_sample_mode(fpsensor_data_t *fpsensor,
                                            bool single);

extern int fpsensor_capture_set_crop(fpsensor_data_t *fpsensor,
                                     int first_column,
                                     int num_columns,
                                     int first_row,
                                     int num_rows);

extern size_t fpsensor_calc_image_size(fpsensor_data_t *fpsensor);

extern int fpsensor_capture_buffer(fpsensor_data_t *fpsensor,
                                   u8 *data,
                                   size_t offset,
                                   size_t image_size_bytes);

extern int fpsensor_capture_deferred_task(fpsensor_data_t *fpsensor);

extern int fpsensor_capture_finger_detect_settings(fpsensor_data_t *fpsensor);

#endif /* LINUX_SPI_FPSENSOR_CAPTURE_H */

