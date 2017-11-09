
/* ------------------------------------------------------------------------- */
/*

*/
/* ------------------------------------------------------------------------- */

#include "irpru.h"
#include <stdio.h>

int pru_init(tpruss_intc_initdata *ppruss_intc_initdata, PrussDataRam_t **pppruss_data_ram)
{
        // First, initialize the driver and open the kernel device
        prussdrv_init();
        int ret = prussdrv_open(PRU_EVTOUT_0);

        if(ret != 0) {
                fprintf(stderr,"Failed to open PRUSS driver - must be run with root privileges!\n");
                return ret;
        }

        // Set up the interrupt mapping so we can wait on INTC later
        prussdrv_pruintc_init(ppruss_intc_initdata);
        // Map PRU DATARAM; reinterpret the pointer type as a pointer to
        // our defined memory mapping struct. We could also use uint8_t *
        // to access the RAM as a plain array of bytes, or uint32_t * to
        // access it as words.
        prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void * *)pppruss_data_ram);
        // Manually initialize PRU DATARAM - this struct is mapped to the
        // PRU, so these assignments actually modify DATARAM directly.
        return 0; // all good
}

/* ------------------------------------------------------------------------- */

void pru_run_code(const unsigned int *ppru_code, unsigned int code_size )
{
        __sync_synchronize();

        prussdrv_exec_code(0, ppru_code, code_size);

        // Wait for INTC from the PRU, signaling it's about to HALT...
        prussdrv_pru_wait_event(PRU_EVTOUT_0);
        // Clear the event: if you don't do this you will not be able to
        // wait again.
        prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
        // Make absolutely sure we read sum again below, after the PRU
        // writes to it. Otherwise, the compiler or hardware might cache
        // the value we wrote above and just return us that. Again, not
        // actually necessary because the compiler inserts an implicit
        // fence at prussdrv_pru_wait_event(...), but a good habit.
        __sync_synchronize();

        // Read the result returned by the PRU
        // Disable the PRU and exit; if we don't do this the PRU may
        // continue running after our program quits! The TI kernel driver
        // is not very careful about cleaning up after us.
        // Since it is possible for the PRU to trash memory and otherwise
        // cause lockups or crashes, especially if it's manipulating
        // peripherals or writing to shared DDR, this is important!
        prussdrv_pru_disable(0);

        prussdrv_exit();
}
