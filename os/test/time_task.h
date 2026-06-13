#ifndef TIME_TASK_H
#define TIME_TASK_H
#include <stdint.h>
#include <stdbool.h>
#include "../task/task.h"
#include "../driver/timer.h"
#include "../driver/adc/adc.h"
#include "../driver/spi/spi.h"
#include "../driver/i2c/i2c.h"
#include "../driver/wdg.h"
#include "../driver/pwm.h"


#define GET_TIME_TASK_VTABLE(obj) GET_TASK_VTABLE(obj) //(*(Time_taskVTable **)obj)
#define GET_TIME_TASK(obj) ((Time_task *)obj)

// 派生类声明
typedef struct _Time_task Time_task;
typedef struct _Time_taskFun Time_taskFun;
// 类成员函数结构
struct _Time_taskFun {
    void (*destroy)(Time_task* self);
};
struct _Time_task {
    Task base;  // 基类作为第一个成员
    const Time_taskFun* fun;
    Device *timer;
    Device *adc;
    Device *spi;
    Device *i2c;
    Device *wdg;
    Device *pwm;
    Device *icm20948;         /* ICM-20948 设备 */
    Device *adxl345;          /* ADXL345 设备 */
};

// 构造函数声明
Time_task* time_task_create(const task_into_t *info);
void time_task_init(Time_task* self, const task_into_t *info);

// 析构函数声明
void time_task_deinit(Time_task* self);

#endif // TIME_TASK_H