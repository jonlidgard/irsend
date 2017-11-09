
#include <argp.h>



  /* ------------------------------------------------------------------------- */

  const char *argp_program_version =
    "rfsend 1.2";

  /* Program documentation. */
  static char doc[] =
          "For Beaglebone Microcontrollers only.\nSends an IR command \
    \vCommand can be either a list of pronto hex codes\n\
  e.g. irsend 0000 006c 0022 0002 0156 00ac 0016 0016 0016....\n\
  or a command from the irsend.codes file - e.g. irsend onkyo.on\n\
  irsend will look in the present directory or /etc/ for the\n\
  optional codes file unless one is specified with -f\n\n\
  Uses PRU0 via the pru_uio overlay.\n\
  Ensure you have an appropriate cape loaded\n\
  (e.g. cape_universal + config-pin p8.12 pruout)\n\
  Connect P8_12 to an IR LED (via a transistor)\n\
  ** MUST BE RUN WITH ROOT PRIVILEGES **";

  /* A description of the arguments we accept. */
  static char args_doc[] = "[COMMAND]";
  /* The options we understand. */
  static struct argp_option options[] = {
          {"debug", 'd', 0, 0, "Produce debug output" },
          {0,0,0,0, "" },
          {"repeat", 'r', "COUNT", OPTION_ARG_OPTIONAL,
           "Repeat the output COUNT (default 1) times"},
           {"freq",   'F', "FREQUENCY", OPTION_ARG_OPTIONAL,
           "Override output frequency in kHz - e.g. F40.1" },
           {"file",   'f', "FILE", OPTION_ARG_OPTIONAL,
     "Use codes file <default irsend.codes>" },
          {"protocol", 'p', "ID", OPTION_ARG_OPTIONAL,
           "Message protocol id (default 1)"},
          { 0 }
  };

  /* Used by main to communicate with parse_opt. */
typedef struct
  {
          int debug;
          int burst2_repeats;
          int protocol_id;
          int code_count;
          float frequency;
          char *codes_file;
          char **codes;
  } arguments_t;

  static error_t
  parse_opt (int key, char *arg, struct argp_state *state);

  static struct argp argp = { options, parse_opt, args_doc, doc };

  /* ------------------------------------------------------------------------- */

  config_t cfg;
  config_setting_t *setting;
  char codes_string[1000];
  static char *pcodes[200];
