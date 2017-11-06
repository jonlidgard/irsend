#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>
#include <prussdrv.h>
#include <libconfig.h>
#include <pruss_intc_mapping.h>
#include "irsendpru_bin.h"

const char *argp_program_version =
  "irsend 0.1";

/* Program documentation. */
static char doc[] =
  "For Beaglebone Microcontrollers only.\nSends an IR command \
  \vUses PRU0 via the pru_uio overlay.\nEnsure you have an appropriate cape loaded\n\
  (e.g. cape_universal + config-pin p8.11 pruout)\n\
  Connect P8_11 to an IR LED\n\
  ** MUST BE RUN WITH ROOT PRIVILEGES **";

/* A description of the arguments we accept. */
static char args_doc[] = "[COMMAND]";

/* The options we understand. */
static struct argp_option options[] = {
  {"debug", 'd', 0, 0, "Produce debug output" },
  {0,0,0,0, "" },
  {"repeat", 'r', "COUNT", OPTION_ARG_OPTIONAL,
   "Repeat the output COUNT (default 1) times"},
  {"protocol", 'p', "ID", OPTION_ARG_OPTIONAL,
   "Message protocol id (default 1)"},
  { 0 }
};


/* Used by main to communicate with parse_opt. */
struct arguments
{
  int debug;
  int burst2_repeats;
  int protocol_id;
  int code_count;
  char **codes;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'd':
      arguments->debug = 1;
      break;
    case 'r':
      arguments->burst2_repeats = arg ? atoi (arg) : 1;
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

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

typedef struct {
  uint32_t burst2_repeats;
  uint16_t pattern_id; // always 0 for raw data capture
  uint16_t pronto_carrier_freq; // ( f = 1000000 / (n * .241246) kHz )
  uint16_t burst_pair_sequence_1_count;
  uint16_t burst_pair_sequence_2_count;
  uint16_t data[200];
} PrussDataRam_t;


config_t cfg;
config_setting_t *setting;
char codes_string[1000];
char *pcodes[200];

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


int main(int argc, char *argv[])
{
    tpruss_intc_initdata prussIntCInitData = PRUSS_INTC_INITDATA;
    PrussDataRam_t * prussDataRam;
    config_t cfg;
    config_setting_t *setting;
    int ret, i;
    struct arguments arguments;
    const char *str;

    /* Default values. */
    arguments.debug = 0;
    arguments.burst2_repeats = 1;
    arguments.protocol_id = 1;

    /* Parse our arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    if (arguments.code_count == 1)
    {
      config_init(&cfg);

 /* Read the file. If there is an error, report it and exit. */
      if(! config_read_file(&cfg, "/etc/irsend.conf"))
      {
       fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
	    config_error_line(&cfg), config_error_text(&cfg));
       config_destroy(&cfg);
       return(EXIT_FAILURE);
      }
   

/* Get the store name. */
        if(config_lookup_string(&cfg, arguments.codes[0], &str))
	{
		if (arguments.debug)
		  printf("Pronto Code: %s\n\n", str);

		arguments.code_count = convertCodeStringToArray(&arguments.codes, str);
//		arguments.codes = &pcodes[0];
	}
      		else
		{
		      fprintf(stderr, "No 'name' setting in configuration file.\n");
			exit(0);
		}
    }
     if (arguments.code_count <6 || arguments.code_count > 199 || arguments.code_count %2 != 0)
      {
        printf("Invalid code length!\n");
        exit(0);
      }
   
     // First, initialize the driver and open the kernel device
    prussdrv_init();
    ret = prussdrv_open(PRU_EVTOUT_0);

    if(ret != 0) {
      printf("Failed to open PRUSS driver!\n");
      return ret;
    }

    // Set up the interrupt mapping so we can wait on INTC later
    prussdrv_pruintc_init(&prussIntCInitData);
    // Map PRU DATARAM; reinterpret the pointer type as a pointer to
    // our defined memory mapping struct. We could also use uint8_t *
    // to access the RAM as a plain array of bytes, or uint32_t * to
    // access it as words.
    prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, (void * *)&prussDataRam);
    // Manually initialize PRU DATARAM - this struct is mapped to the
    // PRU, so these assignments actually modify DATARAM directly.


    // Fill the data ram

    uint16_t j;
    uint16_t code;
    float loop_delay;

    for (j = 0; arguments.codes[j] && j < 200 ; j++)
    {
        code = strtoul(arguments.codes[j],NULL, 16);
  	switch (j)
        {
        case 0:
            prussDataRam->pattern_id = code;
            break;
        case 1:
            prussDataRam->pronto_carrier_freq = code * 24.1246 / 2;
            break;
        case 2:
            prussDataRam->burst_pair_sequence_1_count = code;
            break;
        case 3:
            prussDataRam->burst_pair_sequence_2_count = code;
            break;
        default:
            // no of times around a 10ns delay loop;
            prussDataRam->data[j-4] = code;
            break;
        }
    }

    prussDataRam->burst2_repeats = arguments.burst2_repeats;
    
    void *ptr = prussDataRam; 
    if (arguments.debug)
    {
	    printf("Repeats: %08X\n",*(uint32_t*)ptr); ptr+=4;
	    printf("Pattern: %04X\n",*(uint16_t*)ptr); ptr+=2;
	    printf("Freq: %04X\n",*(uint16_t*)ptr); ptr+=2;
	    printf("Burst1: %04X\n",*(uint16_t*)ptr); ptr+=2;
	    printf("Burst2: %04X\n",*(uint16_t*)ptr); ptr+=2;
    	    for (int i=0; i < j; i++) {
	      printf("Codes: %04X\n",*(uint16_t*)ptr); ptr+=2;
	}
    }
	    
    if (j<4 || j > 199)
    {
        printf("Invalid code length2: %d\n",j);
        exit(0);
    }

    __sync_synchronize();

    prussdrv_exec_code(0, prussPru0Code, sizeof prussPru0Code);

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

    ptr = prussDataRam; 
    if (arguments.debug)
    {
	    for(int r=0;r < 8; r++) {
	    printf("R%d: %08X, %d\n",r,*(uint32_t*)ptr, *(uint32_t*)ptr); ptr+=4;
	    }
    }
    // Read the result returned by the PRU
    // Disable the PRU and exit; if we don't do this the PRU may
    // continue running after our program quits! The TI kernel driver
    // is not very careful about cleaning up after us.
    // Since it is possible for the PRU to trash memory and otherwise
    // cause lockups or crashes, especially if it's manipulating
    // peripherals or writing to shared DDR, this is important!
    prussdrv_pru_disable(0);

    prussdrv_exit();

	    
    exit(1);    
    }
	
