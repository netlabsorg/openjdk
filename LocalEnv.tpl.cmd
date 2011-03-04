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
G.PATH_TOOL_BOOT_JDK    = 'D:\Dev\openjdk6'

/**
 * Path to GCC 4 environment script (+ possible arguments). If you use
 * the GCC 4 distribution recommended in README, the path will look like:
 *
 *   X:\path_to_gcc4\gcc4xx.cmd
 *
 * Otherwise you will have to play with GCC's gccenv.cmd on your own.
 *
 * Leave this empty if you have GCC 4 already available in your
 * environment.
 */
G.PATH_TOOL_GCC4_ENV    = 'D:\Dev\gcc445\gcc445.cmd'

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
 * Path to the previous Java SDK where components not built from the
 * current source tree will be imported from. Normally, it is not used and
 * should be just the same as G.PATH_TOOL_BOOT_JDK.
 */
G.PATH_JDK_IMPORT       = G.PATH_TOOL_BOOT_JDK

/**
 * Log file to save all console output of the build process (only works when
 * using the SE script). Leave it empty to disable logging.
 */
G.LOG_FILE              = 'se.log' /* ScriptDir'se.log' */

/*
 * Advanced definitions
 * ----------------------------------------------------------------------------
 *
 * Used mostly for developing and debugging.
 */

/**
 * Path to the debug version of kLIBC DLL (used with -l flag). Optional.
 */
G.PATH_TOOL_KLIBC_DEBUG     = ''

/**
 * Path to the log check version of kLIBC DLL (used with -L flag). Optional.
 */
G.PATH_TOOL_KLIBC_LOGCHK    = ''

/**
 * Here you may put any additional environment variable definitions needed for
 * your local environment or for the OpenJDK make files using the form shown
 * below. These variables are passed to the environment without any
 * modifications.
 */
/*
call EnvSet 'ALT_SOME_VAR', 'some value'
*/

/*
 * Production definitions
 * ----------------------------------------------------------------------------
 *
 * Not intended for playing around with.
 */

/**
 * JDK build number in format 'bNN'.
 */
/*
call EnvSet 'BUILD_NUMBER', 'b00'
*/

/**
 * Milestone name. Will appear in the version string after the verison number
 * and before the build number.
 */
/*
call EnvSet 'MILESTONE', 'internal'
*/
