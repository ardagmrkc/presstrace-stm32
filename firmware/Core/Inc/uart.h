#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"

/* USART2'yi 115200-8N1, kesme (TXE/TC) tabanli gonderim icin baslatir.
 * RX de acilir; alinan baytlar command.c'deki senaryo komutu cozucusune gider.
 * notify_task, gonderim tamamlandiginda (TC) bildirim alacak gorevdir ve
 * uart_start_tx/uart_wait_tx_done'u YALNIZCA bu gorev cagirir. */
void uart_init(TaskHandle_t notify_task);

/* buf/len'i kesme guduml (TXE ISR) gondermeye baslar. tag, bu aktarimi
 * tanimlayan sifirdan farkli bir degerdir; TC yalnizca bu tag'e ait
 * aktarimi kapatabilir. Onceki aktarim bitmemisse (mesgul) hicbir sey
 * yapmadan false doner. buf, uart_wait_tx_done donene kadar yasamalidir. */
bool uart_start_tx(const uint8_t *buf, size_t len, uint32_t tag);

typedef enum
{
    UART_TX_DONE = 0,     /* TC bu aktarim icin geldi, *t4_us gecerli */
    UART_TX_TIMEOUT,      /* TC gelmedi; aktarim iptal edildi */
    UART_TX_TAG_MISMATCH, /* uyanildi ama TC baska bir aktarima aitti */
} uart_tx_result_t;

/* TC'yi en fazla timeout kadar bekler. Zaman asiminda aktarimi iptal eder
 * (TXE/TC kesmeleri kapanir), boylece ISR buf'i okumayi birakir ve gec
 * gelen bir TC sonraki aktarimi kapatamaz. */
uart_tx_result_t uart_wait_tx_done(uint32_t tag, TickType_t timeout, uint32_t *t4_us);

/* Ucusta aktarim yokken (tag = 0) gelen veya baska aktarima ait TC sayisi. */
uint32_t uart_get_spurious_tc(void);

/* USART2_IRQHandler (stm32f4xx_it.c) tarafindan cagrilir. TC isleyisinde
 * ilk is t4 zaman damgasini alir: "t4 = UART TC tamamlamasini islerken
 * kaydedilir" — DMA TC degil, son stop bitinin hattan ciktigi an. */
void uart_isr_handler(void);

#endif /* UART_H */
