/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011        Sonny Chen @ EPRO/DX
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "console_async.h"
#include "esp_log.h"

#define EX_UART_NUM UART_NUM_0
static const char *TAG = "uart_events";

ConsoleAsync::ConsoleAsync()
  {
  uart_config_t uart_config =
    {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 122,
    };
  // Set UART parameters
  uart_param_config(EX_UART_NUM, &uart_config);

  // Install UART driver, and get the queue.
  uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 10, &m_queue, 0);

  Initialize("Async");
  }

ConsoleAsync::~ConsoleAsync()
  {
  }

int ConsoleAsync::puts(const char* s)
  {
  return ::puts(s);
  }

int ConsoleAsync::printf(const char* fmt, ...)
  {
  va_list args;
  va_start(args,fmt);
  vprintf(fmt,args);
  va_end(args);
  fflush(stdout);
  return 0;
  }

ssize_t ConsoleAsync::write(const void *buf, size_t nbyte)
  {
  ssize_t n = fwrite(buf, nbyte, 1, stdout);
  fflush(stdout);
  return n;
  }

void ConsoleAsync::finalise()
  {
  }

void ConsoleAsync::HandleDeviceEvent(void* pEvent)
  {
  uart_event_t event = *(uart_event_t*)pEvent;
  size_t buffered_size;

  switch (event.type)
    {
    case UART_DATA:
      uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
      if (buffered_size > 0)
        {
        int len = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 100 / portTICK_RATE_MS);
        ProcessChars((char*)data, len);
        }
      break;
    case UART_FIFO_OVF:
      ESP_LOGI(TAG, "hw fifo overflow\n");
      // If fifo overflow happened, you should consider adding flow control for your application.
      // We can read data out out the buffer, or directly flush the rx buffer.
      uart_flush(EX_UART_NUM);
      break;
    case UART_BUFFER_FULL:
      ESP_LOGI(TAG, "ring buffer full\n");
      // If buffer full happened, you should consider encreasing your buffer size
      // We can read data out out the buffer, or directly flush the rx buffer.
      uart_flush(EX_UART_NUM);
      break;
    default:
      ESP_LOGI(TAG, "uart event type: %d\n", event.type);
      break;
    }
  }
