#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uart_dev_t uart;
  uint32_t   timer;
  int        tick;
} chip_state_t;

static void send_nmea(void *data);

void chip_init(void) {
  chip_state_t *s = malloc(sizeof(chip_state_t));
  s->tick = 0;

  uart_config_t ucfg = {
    .tx       = pin_init("TX", OUTPUT),
    .rx       = pin_init("RX", INPUT),
    .baud_rate = 9600,
  };
  s->uart = uart_init(&ucfg);

  timer_config_t tcfg = {
    .callback  = send_nmea,
    .user_data = s,
  };
  s->timer = timer_init(&tcfg);
  timer_start(s->timer, 1000000, true);  /* every 1 s */
}

static uint8_t nmea_checksum(const char *body) {
  uint8_t cs = 0;
  for (int i = 0; body[i]; i++) cs ^= (uint8_t)body[i];
  return cs;
}

static void send_nmea(void *data) {
  chip_state_t *s = (chip_state_t *)data;
  s->tick++;

  int hh = 12;
  int mm = (s->tick / 60) % 60;
  int ss = s->tick % 60;

  /* Fixed position: Burlington MA  42.5048 N, 71.1956 W
     NMEA ddmm.mmmm:  4230.2880,N  07111.7360,W           */
  char body[128];
  snprintf(body, sizeof(body),
    "GPGGA,%02d%02d%02d.00,4230.2880,N,07111.7360,W,1,08,0.9,50.0,M,-33.9,M,,",
    hh, mm, ss);

  uint8_t cs = nmea_checksum(body);

  char sentence[160];
  snprintf(sentence, sizeof(sentence), "$%s*%02X\r\n", body, cs);

  for (int i = 0; sentence[i]; i++) {
    uart_write(s->uart, (uint8_t *)&sentence[i], 1);
  }
}