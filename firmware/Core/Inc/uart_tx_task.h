#ifndef UART_TX_TASK_H
#define UART_TX_TASK_H

#include "FreeRTOS.h"
#include "queue.h"

/* En dusuk oncelikli (PRIO_UART_TX_TASK = 1) gorev. UART donanimina yalnizca
 * bu gorev dokunur (polling/kesme yukunu diger gorevlerden ayri tutmak
 * icin). uart_tx_queue'dan FIFO sirayla mesaj alir, gonderir (t3), TC
 * tamamlanmasini sureli bekler (t4). BTN olaylarinin t0..t4'unu olcum
 * kaydi havuzunda (64) tutar; havuz yalnizca "olcumu bitir" komutuyla
 * 'R' kayitlari + 'E' ozeti olarak dokulur. */
void uart_tx_task_create(QueueHandle_t uart_tx_queue);

#endif /* UART_TX_TASK_H */
