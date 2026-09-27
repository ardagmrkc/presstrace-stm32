#include "uart.h"
#include "main.h"
#include "timestamp.h"
#include "command.h"

static TaskHandle_t s_notify_task = NULL;
static const uint8_t *s_tx_buf = NULL;
static volatile size_t s_tx_len = 0;
static volatile size_t s_tx_idx = 0;

/* Ucustaki aktarim: gorev yazar (baslatmadan once), TC ISR'si okur ve
 * sifirlar. Tek UART'in tek kaydiricisi oldugu icin tek kayit yeterli. */
static volatile uint32_t s_in_flight_tag = 0; /* 0 = ucusta aktarim yok */
static volatile bool s_busy = false;
static volatile uint32_t s_done_tag = 0;
static volatile uint32_t s_done_t4_us = 0;
static volatile uint32_t s_spurious_tc = 0;

/* fCK: USART2 APB1 uzerinde, 42 MHz (bkz. system_clock.c).
 * Kayan noktali sayi kullanmadan, x1000 sabit noktali aritmetikle
 * mantissa/fraction hesabi (16x oversampling, RM0090 27.3.4). */
static uint16_t compute_brr(uint32_t pclk_hz, uint32_t baud)
{
    uint32_t usartdiv_x1000 = (uint32_t)(((uint64_t)pclk_hz * 1000ULL) / (16ULL * baud));
    uint32_t mantissa = usartdiv_x1000 / 1000U;
    uint32_t frac_x1000 = usartdiv_x1000 - (mantissa * 1000U);
    uint32_t fraction = (frac_x1000 * 16U + 500U) / 1000U;
    if (fraction > 15U)
    {
        mantissa++;
        fraction = 0U;
    }
    return (uint16_t)((mantissa << 4) | (fraction & 0xFU));
}

void uart_init(TaskHandle_t notify_task)
{
    s_notify_task = notify_task;

    /* --- GPIO: PA2=TX, PA3=RX, AF7 (USART2), push-pull, yuksek hiz --- */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    uint32_t tx_pin = UART_TX_GPIO_PIN, rx_pin = UART_RX_GPIO_PIN;

    UART_TX_GPIO_PORT->MODER = (UART_TX_GPIO_PORT->MODER & ~(0x3UL << (tx_pin * 2U))) |
                               (0x2UL << (tx_pin * 2U)); /* 10 = alternate function */
    UART_RX_GPIO_PORT->MODER = (UART_RX_GPIO_PORT->MODER & ~(0x3UL << (rx_pin * 2U))) |
                               (0x2UL << (rx_pin * 2U));

    UART_TX_GPIO_PORT->OSPEEDR |= (0x3UL << (tx_pin * 2U)); /* very high speed */
    UART_RX_GPIO_PORT->OSPEEDR |= (0x3UL << (rx_pin * 2U));

    UART_TX_GPIO_PORT->OTYPER &= ~(1UL << tx_pin); /* push-pull */
    UART_TX_GPIO_PORT->PUPDR &= ~(0x3UL << (tx_pin * 2U));
    UART_RX_GPIO_PORT->PUPDR = (UART_RX_GPIO_PORT->PUPDR & ~(0x3UL << (rx_pin * 2U))) |
                              (0x1UL << (rx_pin * 2U)); /* pull-up on RX */

    UART_TX_GPIO_PORT->AFR[tx_pin / 8U] =
        (UART_TX_GPIO_PORT->AFR[tx_pin / 8U] & ~(0xFUL << ((tx_pin % 8U) * 4U))) |
        ((uint32_t)UART_GPIO_AF << ((tx_pin % 8U) * 4U));
    UART_RX_GPIO_PORT->AFR[rx_pin / 8U] =
        (UART_RX_GPIO_PORT->AFR[rx_pin / 8U] & ~(0xFUL << ((rx_pin % 8U) * 4U))) |
        ((uint32_t)UART_GPIO_AF << ((rx_pin % 8U) * 4U));

    /* --- USART2: 115200 8N1. TX kesme guduml; RX yalnizca yer
     *     istasyonunun senaryo komutu (5 bayt) icin acik. --- */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    UART_PERIPH->CR1 = 0;
    UART_PERIPH->CR2 = 0; /* 1 stop bit */
    UART_PERIPH->CR3 = 0;
    UART_PERIPH->BRR = compute_brr(42000000UL, UART_BAUDRATE);
    UART_PERIPH->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE |
                       USART_CR1_UE; /* 8N1: M=0, PCE=0 (varsayilan) */

    NVIC_SetPriority(USART2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(USART2_IRQn);
}

bool uart_start_tx(const uint8_t *buf, size_t len, uint32_t tag)
{
    if (s_busy || tag == 0U)
    {
        return false;
    }

    /* Onceki (iptal edilmis) bir aktarimdan kalmis olabilecek bildirimi ve
     * kapanis kaydini temizle: bu aktarimi yalnizca kendi TC'si kapatir. */
    (void)ulTaskNotifyTake(pdTRUE, 0);
    s_done_tag = 0;

    s_tx_buf = buf;
    s_tx_len = len;
    s_tx_idx = 0;
    s_in_flight_tag = tag; /* once ISR'ye yayinla, sonra baslat */
    s_busy = true;

    /* Bosta TC zaten 1'dir. Bayrak temizlenir ve TCIE ancak son bayt
     * yazildiktan sonra (TXE ISR'si) acilir; aksi halde onceki bosta
     * kalmanin bayragi bu cercevenin t4'u olurdu. */
    UART_PERIPH->SR &= ~USART_SR_TC;
    UART_PERIPH->CR1 |= USART_CR1_TXEIE;
    return true;
}

static void uart_abort_tx(void)
{
    taskENTER_CRITICAL(); /* USART2 kesmesi (oncelik 5) maskelenir */
    UART_PERIPH->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
    s_in_flight_tag = 0;
    s_busy = false;
    taskEXIT_CRITICAL();
}

uart_tx_result_t uart_wait_tx_done(uint32_t tag, TickType_t timeout, uint32_t *t4_us)
{
    if (ulTaskNotifyTake(pdTRUE, timeout) == 0U)
    {
        uart_abort_tx();
        return UART_TX_TIMEOUT;
    }
    if (s_done_tag != tag)
    {
        s_spurious_tc++;
        return UART_TX_TAG_MISMATCH;
    }
    *t4_us = s_done_t4_us;
    return UART_TX_DONE;
}

uint32_t uart_get_spurious_tc(void)
{
    return s_spurious_tc;
}

void uart_isr_handler(void)
{
    uint32_t sr = UART_PERIPH->SR;
    uint32_t cr1 = UART_PERIPH->CR1;
    BaseType_t higher_prio_woken = pdFALSE;

    /* TC once: ayni anda RX bayti da bekliyorsa t4 onun yuzunden gecikmesin. */
    if ((sr & USART_SR_TC) && (cr1 & USART_CR1_TCIE))
    {
        /* t4: son bit sonrasi ISR/callback gozlem zamani. Ilk is: zaman. */
        uint32_t t4_us = timestamp_now_us();

        UART_PERIPH->CR1 &= ~USART_CR1_TCIE;
        UART_PERIPH->SR &= ~USART_SR_TC;

        uint32_t tag = s_in_flight_tag; /* bir kez oku */
        s_in_flight_tag = 0;
        s_busy = false;
        if (tag == 0U)
        {
            s_spurious_tc++; /* ucusta aktarim yok: damga hicbir kayda yazilmaz */
        }
        else
        {
            s_done_t4_us = t4_us;
            s_done_tag = tag;
            vTaskNotifyGiveFromISR(s_notify_task, &higher_prio_woken);
        }
    }

    if ((sr & USART_SR_TXE) && (cr1 & USART_CR1_TXEIE))
    {
        if (s_tx_idx < s_tx_len)
        {
            UART_PERIPH->DR = s_tx_buf[s_tx_idx++];
        }
        if (s_tx_idx >= s_tx_len)
        {
            UART_PERIPH->CR1 &= ~USART_CR1_TXEIE;
            UART_PERIPH->CR1 |= USART_CR1_TCIE; /* son baytin fiziksel olarak
                                                    bitmesini bekle */
        }
    }

    /* RXNEIE, tasma (ORE) durumunda da kesme uretir. SR okunduktan sonra
     * DR okumak ikisini de temizler; temizlenmezse kesme durmadan tekrarlar. */
    if ((sr & (USART_SR_RXNE | USART_SR_ORE)) != 0U)
    {
        uint8_t b = (uint8_t)UART_PERIPH->DR;
        command_rx_byte_from_isr(b, &higher_prio_woken);
    }

    portYIELD_FROM_ISR(higher_prio_woken);
}
