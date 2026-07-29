/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup DOC DOC
 * @ingroup  DOCUMENT
 * @brief    templete
 * @details  shell system common
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "shell.h"
#include "shell_common.h"
#include "om_common.h"
#include "common_def.h"


#define SHELL_LOG_DEBUG(format, ...)               	log_debug(format,  ## __VA_ARGS__)
/*******************************************************************************
 * CONST & VARIABLES
 */
static void cmd_mem32r(int argc, char *argv[]);
static void cmd_mem32w(int argc, char *argv[]);

const shell_cmd_t shell_cmd [] = {
    { "mem32r",  cmd_mem32r,  "read                        Usage: mem32r addr [count]/[start_bit end_bit]"    },
    { "mem32w",  cmd_mem32w,  "write                       Usage: mem32w addr [data]/[start_bit end_bit data]"},
    //ADD other command
    { NULL,        NULL,        NULL                                                                  },     /* donot deleted */
};


/*******************************************************************************
 * MACROS
 */
#define    SWAP(a, b)           do {                                           \
                                    uint32_t swap;                             \
                                    swap = a;                                  \
                                    a = b;                                     \
                                    b = swap;                                  \
                                } while(0)


#define SHELL_CMD_SPLIT     "split"


/*******************************************************************************
 * LOCAL FUNCTIONS
 */
								

static void cmd_mem32r(int argc, char *argv[])
{
    volatile uint32_t *mem_ptr;
    uint32_t address;
    uint32_t cnt;
    uint32_t i;
    uint32_t start_bit;
    uint32_t end_bit;
    uint32_t bit_len;

    start_bit = 0U;
    end_bit = 0U;
    if(argc == 1) {             //mem32r addr
        cnt = 1;
        bit_len = 0;
    } else if (argc == 2) {     //mem32 addr count
        cnt = strtoul(argv[1], NULL, 0);
        bit_len = 0;
    } else if (argc == 3) {     //mem32 addr start_bit end_bit
        cnt = 1;
        start_bit = (strtoul(argv[1], NULL, 0)) % 32;
        end_bit = strtoul(argv[2], NULL, 0) % 32;
        if (start_bit > end_bit) {
            SWAP(start_bit, end_bit);
        }
        bit_len = start_bit - end_bit + 1U;
    } else {
        SHELL_LOG_DEBUG("Usage: mem32r address count\r\n");
        return;
    }

    address   = strtoul(argv[0], NULL, 0);
    mem_ptr = (volatile uint32_t *)address;
    for(i = 0; i < cnt; i++) {
        SHELL_LOG_DEBUG("\r\n0x%08X:\t", (address+i*4));
        SHELL_LOG_DEBUG("0x%08X ", mem_ptr[i]);
    }

    // mem32 addr start_bit end_bit
    if (bit_len) {
        uint32_t bit_val;
        uint32_t mask;

        /*lint -save -e644*/
        mask = ((end_bit - start_bit) == 31) ? 0xFFFFFFFFU : (((1u << bit_len) - 1) << start_bit);
        bit_val = ((*(volatile uint32_t *)address) >> start_bit) & mask;
        SHELL_LOG_DEBUG("[%d, %d]'s value is 0x%08X\r\n\r\n", end_bit, start_bit, bit_val);
        /*lint -restore*/
    }
    SHELL_LOG_DEBUG("\r\n\r\n");
}

static void cmd_mem32w(int argc, char *argv[])
{
    uint32_t address;
    uint32_t wdata;
    uint32_t start_bit;
    uint32_t end_bit;
    uint32_t bit_len;

    address = strtoul(argv[0], NULL, 0);
    if (argc == 2) {                 // mem32w addr data
        wdata = strtoul(argv[1], NULL, 0);
        *((volatile uint32_t *)address) = wdata;
        SHELL_LOG_DEBUG("[0x%08X] set 0x%08x completed\r\n\r\n", address, wdata);
    } else if (argc == 4) {         // mem32w addr start_bit end_bit data
        uint32_t mask;
        uint32_t mem_data;

        start_bit = strtoul(argv[1], NULL, 0) % 32;
        end_bit   = strtoul(argv[2], NULL, 0) % 32;
        wdata = strtoul(argv[3], NULL, 0);

        if (start_bit > end_bit) {
            SWAP(start_bit, end_bit);
        }
        bit_len = end_bit - start_bit + 1;
        mask = ((end_bit - start_bit) == 31) ? 0xFFFFFFFFU : (((1u << bit_len) - 1) << start_bit);
        mem_data = *((volatile uint32_t *)address);
        *((volatile uint32_t *)address) = (mem_data & (~mask)) | ((wdata << start_bit) & mask);

        SHELL_LOG_DEBUG("0x%08x [%d, %d] set 0x%08x completed\r\n\r\n", address, start_bit, end_bit, wdata);
    } else {
        SHELL_LOG_DEBUG("Usage: mem32w address value\r\n");
    }
    SHELL_LOG_DEBUG("\r\n\r\n");
}

static char *_strtok(char *str, const char *delim, char **saveptr)
{
    char *token;

    if (str) {
        *saveptr = str;
    }
    token = *saveptr;

    if (!token) {
        return NULL;
    }

    token += strspn(token, delim);
    *saveptr = strpbrk(token, delim);
    if (*saveptr) {
        *(*saveptr)++ = '\0';
    }

    return *token ? token : NULL;
}

static void list_commands(const shell_cmd_t *scp)
{
    if(scp != NULL) {
        while (scp->sc_name != NULL) {
            if(strcmp(scp->sc_name, SHELL_CMD_SPLIT) == 0) {
                SHELL_LOG_DEBUG("\r\n%s\r\n", scp->description);
            } else {
                SHELL_LOG_DEBUG("%16s\t%s\r\n", scp->sc_name, scp->description);
            }
            scp++;
        }
    }
}

static char cmdexec(const shell_cmd_t *scp, char *name, int argc, char *argv[])
{
    if (scp) {
        while (scp->sc_name != NULL) {
            if (strcasecmp(scp->sc_name, name) == 0) {
                scp->sc_function(argc, argv);
                return 0;
            }
            scp++;
        }
    }

    return 1;
}


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 * @brief   Shell thread function.
 */
void shell_main(char *line, const shell_cmd_t *scp)
{
    int n;
    char *lp, *cmd, *tokp;
    char *args[SHELL_MAX_ARGUMENTS + 1];
    tokp = NULL;
    lp = _strtok(line, " \t", &tokp);
    cmd = lp;
    n = 0;

    while((lp = _strtok(NULL, " \t", &tokp)) != NULL) {
        if(n >= SHELL_MAX_ARGUMENTS) {
            SHELL_LOG_DEBUG("shell: Too many arguments\r\n");
            cmd = NULL;
            return;
        }
        args[n++] = lp;
    }
    args[n] = NULL;
    if(cmd != NULL) {
        if(strcasecmp(cmd, "help") == 0) {
            SHELL_LOG_DEBUG("Commands:\r\n            help\thelp\r\n");
            list_commands(shell_cmd);
            list_commands(scp);
        } else if(cmdexec(scp, cmd, n, args) && cmdexec(shell_cmd, cmd, n, args)) {

      
          // SHELL_LOG_DEBUG("ooooo = %s %s\r\n", cmd,scp->sc_name);
           SHELL_LOG_DEBUG(" ? Use \'help\' to get commands\r\n");
        }
		else
		{
			
		}
    }
}

/**
 * @brief   Reads a whole line from the input channel.
 *
 * @param[in] line        pointer to the line buffer
 * @param[in] line_size   buffer maximum length
 * @return                The operation status.
 * @retval true           the channel was reset or CTRL-D pressed.
 * @retval false          operation successful.
 *
 * @api
 */
bool shell_get_line(char c, char *line, unsigned line_size, unsigned *line_index)
{
    if(c == 4) {
        SHELL_LOG_DEBUG("^D");
        *line_index = 0;
        return true;
    }
    if((c == 8) || (c == 127)) {    //ASCII 8: backspace,  127:DEL
        if(*line_index != 0x00) {
            om_putchar(c);
            om_putchar(0x20);
            om_putchar(c);
            *line_index =  (*line_index) - 1;
        }
        return false;
    }
    if(c == '\r') {
//        om_putchar('\r');
//        om_putchar('\n');
        return true;
    }
    if(c == '\n') {
        if(*line_index == 0x00){
            SHELL_LOG_DEBUG("\n");
        }
        return true;
    }
    if(c < 0x20) {
        return false;
    }
    if((*line_index) < (line_size - 1)) {
        //om_putchar(c);
        line[*line_index] = c;
        (*line_index) ++;
        return false;
    }

    return false;
}

/** @} */
