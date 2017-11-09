#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>
#include <libconfig.h>

#include "irsend_pru_bin.h"
#include "irsend.h"
#include "irpru.h"

/* ------------------------------------------------------------------------- */

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
        /* Get the input argument from argp_parse, which we
           know is a pointer to our arguments structure. */
        arguments_t *arguments = state->input;

        switch (key)
        {
        case 'd':
                arguments->debug = 1;
                break;
        case 'r':
                arguments->burst2_repeats = arg ? atoi (arg) : 1;
                break;
        case 'F':
                arguments->frequency = strtof(arg, NULL);
                break;
        case 'f':
                arguments->codes_file = arg;
                break;
        case 'p':
                arguments->protocol_id = arg ? atoi (arg) : 1;
                break;

        case ARGP_KEY_NO_ARGS:
                argp_usage (state);

        case ARGP_KEY_ARG:
                arguments->code_count = state->argc - state->next + 1;
                arguments->codes = &state->argv[state->next-1];
                state->next = state->argc;
                break;

        default:
                return ARGP_ERR_UNKNOWN;
        }
        return 0;
}

/* ------------------------------------------------------------------------- */
/* Convert a string containing space separated items
   into an string array of items
 */

int convertCodeStringToArray(char ***codes, const char *str)
{
        char *token;
        const char s[2] = " ";

        strncpy(codes_string, str, 1000);
        /* get the first token */
        token = strtok(codes_string, s);
        int i=0;
        /* walk through other tokens */
        while( token != NULL && i < 200) {
                pcodes[i++] = token;
                token = strtok(NULL, s);
        }
        *codes = &pcodes[0];

        return i;
}

/* ------------------------------------------------------------------------- */

void debug_output(void *memPtr,int codes_count, bool debug, bool after)
{
        void *ptr = memPtr;
        if (debug)
        {
                if (!after)
                {
                        printf("Repeats: %08X\n",*(uint32_t*)ptr); ptr+=4;
                        printf("Pattern: %04X\n",*(uint16_t*)ptr); ptr+=2;
                        printf("Freq: %04X\n",*(uint16_t*)ptr); ptr+=2;
                        printf("Burst1: %04X\n",*(uint16_t*)ptr); ptr+=2;
                        printf("Burst2: %04X\n",*(uint16_t*)ptr); ptr+=2;
                        for (int i=0; i < codes_count-4; i++)
                        {
                                printf("Codes: %04X\n",*(uint16_t*)ptr); ptr+=2;
                        }
                }
                else
                {
                        printf("\nReturned Registers\n");
                        for(int r=0; r < 8; r++)
                        {
                                printf("R%d: %08X, %d\n",r,*(uint32_t*)ptr, *(uint32_t*)ptr); ptr+=4;
                        }
                }
        }
}

/* ------------------------------------------------------------------------- */
/* Parse our arguments; every option seen by parse_opt will
   be reflected in arguments. */

void parse_args(int argc, char *argv[], arguments_t *arguments, config_t *pcfg)
{
  const char *default_codes_file = "irsend.codes";
  const char *etc_codes_file = "/etc/irsend.codes";

  /* Default values. */
  arguments->debug = 0;
  arguments->frequency =0;
  arguments->burst2_repeats = 1;
  arguments->protocol_id = 1;
  arguments->codes_file = (char *)default_codes_file;

  argp_parse (&argp, argc, argv, 0, 0, arguments);

  config_init(pcfg);

  /* Read the codes file of it exists. (local dir else /etc/) */
  if(!config_read_file(pcfg, arguments->codes_file))
  {
          if(!config_read_file(pcfg, etc_codes_file))
          {
                  if (config_error_file(pcfg))
                  {
                          fprintf(stderr, "%s:%d - %s\n", config_error_file(pcfg),
                                  config_error_line(pcfg), config_error_text(pcfg));
                          config_destroy(pcfg);
                  }
          }
  }

}

/* ------------------------------------------------------------------------- */


int main(int argc, char *argv[])
{
        tpruss_intc_initdata prussIntCInitData = PRUSS_INTC_INITDATA;
        PrussDataRam_t *ppruss_data_ram;
        config_t cfg;
//        config_setting_t *setting;
        int i, j;
        uint16_t burst_pair_count = 0;
        uint16_t code;
        arguments_t arguments;
        const char *str;

        // Get the pru ready
        if (pru_init(&prussIntCInitData, &ppruss_data_ram))
          exit(1);

        // Parse the command line
        parse_args(argc, argv, &arguments, &cfg);

        /* Command line (arguments.codes) can be either:
           a pronto hex list of codes e.g. 0000 0100 0001 0022 0105 0201.....
            determined by an initial 4 digit numeric value which can be zero
           a raw data string of mark/space timings e.g. +500 -1200 +330 -2000 or 500 1200 330 2000
            determined by a numeric value with or without a +/- sign which > 0
           a list of codes from the codes file e.g. onkyo.on d10 tv.on d5 projector.on
            detmined by a non-numeric value (d10 is a delay in 10ths of sec before executing
           next command) & can be fed from a file e.g. irsend < automate.txt
           /* ------------------------------------------------------------------------- */


//        code_type = analyse_command_args(arguments.codes[0]);
        if (arguments.code_count  == 1)
        {
                if(config_lookup_string(&cfg, arguments.codes[0], &str))
                {
                        if (arguments.debug)
                                printf("Pronto Code: %s\n\n", str);

                        arguments.code_count = convertCodeStringToArray(&arguments.codes, str);
                }
                else
                {
                        fprintf(stderr, "No %s setting in configuration file.\n", arguments.codes[0]);
                        config_destroy(&cfg);
                        exit(1);
                }
        }
        if (arguments.code_count <6 || arguments.code_count > 199 || arguments.code_count %2 != 0)
        {
                fprintf(stderr,"Invalid code length!\n");
                config_destroy(&cfg);
                exit(1);
        }


        // Fill the data ram


        float loop_delay;

        for (j = 0; arguments.codes[j] && j < 200; j++)
        {
                code = strtoul(arguments.codes[j],NULL, 16);
                switch (j)
                {
                case 0:
                        ppruss_data_ram->pattern_id = code;
                        break;
                case 1:
                        ppruss_data_ram->pronto_carrier_freq = code * 24.1246 / 2;
                        break;
                case 2:
                        ppruss_data_ram->burst_pair_sequence_1_count = code;
                        burst_pair_count = code*2;
                        break;
                case 3:
                        ppruss_data_ram->burst_pair_sequence_2_count = code;
                        burst_pair_count += code*2;
                        break;
                default:
                        // no of times around a 10ns delay loop;
                        ppruss_data_ram->data[j-4] = code;
                        break;
                }
        }

        if (arguments.frequency > 0 && arguments.frequency <= 50000) // 50mHz!
        {
                ppruss_data_ram->pronto_carrier_freq = (uint16_t)(50000.0 / arguments.frequency);
        }

        if (burst_pair_count != j-4)
        {
                fprintf(stderr,"Invalid burst pair count!\n%d stated vs %d\n",burst_pair_count,j-4);
                config_destroy(&cfg);
                exit(1);
        }

        if (j<4 || j > 199)
        {
                fprintf(stderr,"Burst pair count out of range: %d\n",j);
                config_destroy(&cfg);
                exit(1);
        }


        ppruss_data_ram->burst2_repeats = arguments.burst2_repeats;

        debug_output((void*)ppruss_data_ram, j, arguments.debug, 0);

        pru_run_code(prussPru0Code, sizeof prussPru0Code);

        debug_output((void*)ppruss_data_ram, j, arguments.debug, 1);

        config_destroy(&cfg);

        exit(0);
}
