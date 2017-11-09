#ifndef __LIBPRU_H__
#define __LIBPRU_H__

#include <stdint.h>

#include <pruss_intc_mapping.h>
#include <prussdrv.h>

typedef struct {
        uint32_t burst2_repeats;
        uint16_t pattern_id; // always 0 for raw data capture
        uint16_t pronto_carrier_freq; // ( f = 1000000 / (n * .241246) kHz )
        uint16_t burst_pair_sequence_1_count;
        uint16_t burst_pair_sequence_2_count;
        uint16_t data[200];
} PrussDataRam_t;

int pru_init(tpruss_intc_initdata *ppruss_intc_initdata, PrussDataRam_t **pppruss_data_ram);
void pru_run_code(const unsigned int *ppru_code, unsigned int code_size );

#endif
