/*
 * Local environment settings for OpenJDK (Template)
 *
 * This file is used to specify local paths to external tools and libraries
 * and to perform other site-specific project environment setup.
 *
 * NOTES:
 *
 *   This file is a template! Copy it to a file named LocalEnv.cmd in
 *   the same directory and modify the copy to fit your local environment.
 *
 *   All paths in this file are specified using back slashes unless specified
 *   otherwise.
 */

/**
 * Path to JDK used for bootstrapping. Must be set.
 */
G.PATH_TOOL_BOOT_JDK    = 'D:\Dev\java150'

/**
 * Path to GCC 4.4.2. Leave empty if you have it already available
 * in your environment.
 */
G.PATH_TOOL_GCC442      = 'D:\Dev\gcc442'

/**
 * Path to Apache Ant. Leave empty if you have it already available
 * in your environment.
 */
G.PATH_TOOL_ANT         = 'D:\Dev\java\apache-ant-1.8.1'

/**
 * Path to Odin32 SDK. Must be set.
 */
G.PATH_LIB_ODIN32       = 'D:\Coding\odin32'

/**
 * Log file to save all console output of the build process (only works when
 * using the SE script). Leave it empty to disable logging.
 */
G.LOG_FILE              = 'se.log' /* ScriptDir'se.log' */

/**
 * Here you may put any additional environment variable definitions needed for
 * your local environment or for the OpenJDK make files using the form shown
 * below. These variables are passed to the environment without any
 * modifications.
 */
/*
call EnvSet 'ALT_SOME_VAR', 'some value'
*/

