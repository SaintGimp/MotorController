/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_examples.h"
#include "driver_init.h"
#include "utils.h"

/**
 * Example of using DEBUG_IO to write "Hello World" using the IO abstraction.
 *
 * Since the driver is asynchronous we need to use statically allocated memory for string
 * because driver initiates transfer and then returns before the transmission is completed.
 *
 * Once transfer has been completed the tx_cb function will be called.
 */

static uint8_t example_DEBUG_IO[12] = "Hello World!";

static void tx_cb_DEBUG_IO(const struct usart_async_descriptor *const io_descr)
{
	/* Transfer completed */
}

void DEBUG_IO_example(void)
{
	struct io_descriptor *io;

	usart_async_register_callback(&DEBUG_IO, USART_ASYNC_TXC_CB, tx_cb_DEBUG_IO);
	/*usart_async_register_callback(&DEBUG_IO, USART_ASYNC_RXC_CB, rx_cb);
	usart_async_register_callback(&DEBUG_IO, USART_ASYNC_ERROR_CB, err_cb);*/
	usart_async_get_io_descriptor(&DEBUG_IO, &io);
	usart_async_enable(&DEBUG_IO);

	io_write(io, example_DEBUG_IO, 12);
}

/**
 * Example of using SPI_0 to write "Hello World" using the IO abstraction.
 */
static uint8_t example_SPI_0[12] = "Hello World!";

void SPI_0_example(void)
{
	struct io_descriptor *io;
	spi_m_sync_get_io_descriptor(&SPI_0, &io);

	spi_m_sync_enable(&SPI_0);
	io_write(io, example_SPI_0, 12);
}
