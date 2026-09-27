#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include "FreeRTOS.h"
#include "queue.h"

/* Orta oncelikli (PRIO_BUTTON_TASK = 2) gorev. evt_queue'dan buton
 * olaylarini alir (t1), BTN mesaji olusturup (t2) uart_tx_queue'ya gonderir.
 * Kendi GPIO/EXTI kuyrugunu olusturur ve button_init() ile kaydeder. */
void button_task_create(QueueHandle_t uart_tx_queue);

#endif /* BUTTON_TASK_H */
