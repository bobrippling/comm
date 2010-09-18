#ifndef CONFIG_H
#define CONFIG_H

#define VERSION_STR      "0.3"
#define VERSION_MAJOR    ((VERSION_STR)[0] - '0')
#define VERSION_MINOR    ((VERSION_STR)[2] - '0')

#define MAX_NAME_LEN     64
#define LINE_SIZE        2048

#define DEFAULT_PORT     "2848"
#define RENAME_SEPARATOR 0x1d /* ^] aka ``group separator'' */

#define LIBCOMM_RECV_TIMEOUT 100

#define LOG_DIR          "Logs"

#endif
