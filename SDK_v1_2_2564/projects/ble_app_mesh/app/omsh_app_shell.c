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
 * @brief    mesh app shell
 * @details  mesh app shell
 * @version
 *
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

/*******************************************************************************
 * INCLUDES
 */
#include "omsh_app.h"

#if (CONFIG_SHELL)
#include "shell.h"


/*******************************************************************************
 * STATIC FUNCTIONS
 */
static void mesh_shell_dump_info(int argc, char* argv[])
{
    msh_api_info_log();
}

static void mesh_shell_dump_heap(int argc, char* argv[])
{
    extern void msh_heap_dump(void *print);
    msh_heap_dump((void *)om_printf);
}

static void mesh_shell_dump_timer(int argc, char* argv[])
{
    evt_timer_dump((void *)om_printf);
}


/*******************************************************************************
 * CONST & VARIABLES
 */
const shell_cmd_t mesh_shell_cmd[] = {
    {"dump_info",   mesh_shell_dump_info,   "Dump mesh stack information"   },
    {"dump_heap",   mesh_shell_dump_heap,   "Dump mesh stack heap usage"    },
    {"dump_timer",  mesh_shell_dump_timer,  "Dumo mesh stack event timer"   },
    {NULL,          NULL,                   NULL                            },
};

#endif /* CONFIG_SHELL */

/** @} */
