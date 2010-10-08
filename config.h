#ifndef CONFIG_H
#define CONFIG_H

#define VERSION_STR      "0.5"
#define VERSION_MAJOR    ((VERSION_STR)[0] - '0')
#define VERSION_MINOR    ((VERSION_STR)[2] - '0')

#define MAX_NAME_LEN     64
#define MAX_PORT_LEN     16
#define MAX_PASS_LEN     32
#define MAX_DESC_LEN     64
#define MAX_COLOUR_LEN   16

#define MAX_LINE_LEN     2048
#define MAX_CFG_LEN      128

#define DEFAULT_BROWSER  "firefox"
#define DEFAULT_PORT     "2848"
#define GROUP_SEPARATOR  0x1d /* ^] aka ``group separator'' */

#define LIBCOMM_RECV_TIMEOUT 100

#define LOG_DIR          "Logs"

#endif
