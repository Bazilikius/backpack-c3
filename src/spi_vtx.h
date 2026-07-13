#ifndef SPI_VTX_H
#define SPI_VTX_H

#include <Arduino.h>

void init_spi_vtx();
void set_vrx_frequency(uint16_t frequency);
void handle_vrx_change(uint8_t band, uint8_t channel, uint16_t freq);

#endif // SPI_VTX_H
