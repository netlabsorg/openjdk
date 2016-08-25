/*
 * Script to set up the build environment for OpenJDK
 *
 * Best used with the SE script (see README for details).
 *
 * NOTE:
 *
 *   Do not modify this script to tailor for your local environment,
 *   copy LocalEnv.tpl.cmd to LocalEnv.cmd and modify that copy instead!
 */

'@echo off'
trace off
signal on error name OnError

parse source . . ScriptFile
ScriptDir = filespec('D', ScriptFile)||filespec('P', ScriptFile)

parse arg aArgs

rc = rxFuncAdd('SysLoadFuncs', 'REXXUTIL', 'SysLoadFuncs')
rc = SysLoadFuncs()

/* running under se.cmd? */
UnderSE = EnvGet('SE_CMD_RUNNING') \== ''

if (\UnderSE) then do
    if (EnvGet('OPENJDK_ENV_CMD_DONE') == 1) then do
        say
        say 'OpenJDK build environment is already set up.'
        exit 0
    end
end
else do
    aArgs = EnvGet('SE_CMD_ARGS')
end

/*
 * get options
 */

aFlags = ''
fTargets = ''

do i = 1 to words(aArgs)
    a = word(aArgs, i)
    if (left(a, 1) == '-') then do
        f = substr(a, 2, 1)
        select
            when f == 't' then do
                fTargets = substr(a, 3)
            end
            otherwise do
                aFlags = aFlags || substr(a, 2)
            end
        end
    end
    else do
        leave
    end
end
aArgs = strip(subword(aArgs, i))
drop a

if (verify(aFlags, 'Hh?', 'M')) then do
    say 'Flags for 'ScriptFile':'
    say ' -r        Start commands in release environment'
    say ' -l        Use debug version of LIBC DLL'
    say ' -L        Use log check version of LIBC DLL'
    say ' -j        Enable java launcher debug output'
    say ' -o        Enable Odin extended logging'
    say ' -R        Start commands in PRODUCT RELEASE environment'
    say ' -t<list>  Narrow down the list of targets (possble values are'
    say '           jdk,hotspot,jaxp,jaxws,corba,langtools)'

    if (UnderSE) then call EnvSet 'SE_CMD_ARGS', 'exit'
    exit
end

fRelease        = pos('r', aFlags) \= 0
fLibcDebug      = pos('l', aFlags) \= 0
fLibcLogChk     = pos('L', aFlags) \= 0
fJavaDebug      = pos('j', aFlags) \= 0
fOdinLog        = pos('o', aFlags) \= 0
fProductRelease = pos('R', aFlags) \= 0

fTargets = translate(fTargets, ' ', ',')

if (fProductRelease) then fRelease = 1

/* running make? */
fMake = word(aArgs, 1) == 'make'

/*
 * empty some variables for detecting if LocalEnv.cmd sets them
 */
VarsToEmpty = 'ALT_BOOTDIR ALT_FREETYPE_HEADERS_PATH ALT_FREETYPE_LIB_PATH'
do i = 1 to words(VarsToEmpty)
    call EnvSet word(VarsToEmpty, i), ''
end

/*
 * include LocalEnv.cmd
 */
LocalEnv_cmd = ScriptDir'LocalEnv.cmd'
LocalEnv = charin(LocalEnv_cmd,, chars(LocalEnv_cmd))
LocalEnv = translate(LocalEnv, ';;', '0D0A'x)
interpret LocalEnv

/*
 * setup GCC
 */
if (symbol('G.PATH_TOOL_GCC4_ENV') == 'VAR') then do
    if (G.PATH_TOOL_GCC4_ENV \== '') then do
        cmdline = 'call' G.PATH_TOOL_GCC4_ENV
        cmdline
        drop cmdline
    end
end

if (\fMake) then do
    if (G.PATH_TOOL_KLIBC_DEBUG \== '' & fLibcDebug) then do
        address 'cmd' 'set LIBPATHSTRICT=T'
        call EnvAddFront 'BEGINLIBPATH', G.PATH_TOOL_KLIBC_DEBUG
        call EnvSet 'LIBC_STRICT_DISABLED', '1'
    end
    else
    if (G.PATH_TOOL_KLIBC_LOGCHK \== '' & fLibcLogChk) then do
        address 'cmd' 'set LIBPATHSTRICT=T'
        call EnvAddFront 'BEGINLIBPATH', G.PATH_TOOL_KLIBC_LOGCHK
        call EnvSet 'LIBC_STRICT_DISABLED', '1'
    end
end

/*
 * setup Ant
 */
if (G.PATH_TOOL_ANT \== '') then do
    call EnvSet 'ANT_HOME', UnixSlashes(G.PATH_TOOL_ANT)
    call EnvSet 'antenv', G.PATH_TOOL_ANT'\bin\antenv.cmd'
    call EnvSet 'runrc', ScriptDir'antrc.cmd'
    call EnvAddFront 'PATH', G.PATH_TOOL_ANT'\bin'
end

/*
 * add boot JDK tools to BEGINLIBPATH
 */
call EnvAddFront 'BEGINLIBPATH', G.PATH_TOOL_BOOT_JDK'\bin'

/*
 * POSIX shell is required for the build (dash is recommended).
 */
call EnvSet 'MAKESHELL', 'sh.exe'

/*
 * Force English messages.
 */
call EnvSet 'LANG', 'en_US'

/*
 * these must be always unset
 */
call EnvSet 'CLASSPATH', ''
call EnvSet 'JAVA_HOME', ''

/*
 * various variables for OpenJDK make files
 */
call EnvSetIfEmpty 'ALT_BOOTDIR', UnixSlashes(G.PATH_TOOL_BOOT_JDK)
call EnvSetIfEmpty 'ALT_ODINSDK_HEADERS_PATH', UnixSlashes(G.PATH_SDK_ODIN32_HEADERS)
call EnvSetIfEmpty 'ALT_ODINSDK_LIB_PATH', UnixSlashes(G.PATH_SDK_ODIN32_LIBS)
call EnvSetIfEmpty 'ALT_ODINSDK_DBGLIB_PATH', UnixSlashes(G.PATH_SDK_ODIN32_DBGLIBS)
call EnvSetIfEmpty 'ALT_FREETYPE_HEADERS_PATH', UnixSlashes(ScriptDir'libs\freetype\include')
call EnvSetIfEmpty 'ALT_FREETYPE_LIB_PATH', UnixSlashes(ScriptDir'libs\freetype\lib')
call EnvSetIfEmpty 'ALT_JDK_IMPORT_PATH', UnixSlashes(G.PATH_JDK_IMPORT)

if (fTargets \== '') then do
    call EnvSet 'SKIP_BUILD_JDK', 'true'
    call EnvSet 'SKIP_BUILD_HOTSPOT', 'true'
    call EnvSet 'SKIP_BUILD_JAXP', 'true'
    call EnvSet 'SKIP_BUILD_JAXWS', 'true'
    call EnvSet 'SKIP_BUILD_CORBA', 'true'
    call EnvSet 'SKIP_BUILD_LANGTOOLS', 'true'
    do i = 1 to words(fTargets)
        call EnvSet 'SKIP_BUILD_'translate(word(fTargets, i))
    end
end

if (G.MAKE_JOBS \= '' && G.MAKE_JOBS \= '1') then do
    call EnvSet 'COMPILE_APPROACH', 'parallel'
    call EnvSet 'ALT_PARALLEL_COMPILE_JOBS', G.MAKE_JOBS
    call EnvSet 'HOTSPOT_BUILD_JOBS', G.MAKE_JOBS
end

/**
 * generate include file dependencies for C/C++ sources.
 */
call EnvSetIfEmpty 'INCREMENTAL_BUILD', 'true'

if (\fRelease) then do
    /* disable generation of installation bundles and similar stuff */
    call EnvSet 'DEV_ONLY', 'true'
    /* cause the optimized build of hotspot JVM to contain assert() calls */
    call EnvSet 'DEVELOP', '1'
end
else do
    call EnvSet 'DEV_ONLY'
    call EnvSet 'DEVELOP'
end

if (fProductRelease) then do
    call EnvSet 'ALT_OUTPUTDIR', UnixSlashes(ScriptDir'openjdk\build-product-release')
    if (EnvGet('MILESTONE') == '') then do
        call EnvSet 'MILESTONE', 'fcs' /* avoid appearing milestone in version string */
    end
    if (EnvGet('BUILD_NUMBER') == '') then do
        say 'ERROR: BUILD_NUMBER must be set in -R mode!'
        exit 1
    end
end

if (fJavaDebug & \fMake) then call EnvSet '_JAVA_LAUNCHER_DEBUG', '1'

/*
 * @todo temporarily disable some things we don't support
 */
call EnvSet 'OS2_TEMP', 'true'
/* precompiled support is broken in GCC4 */
call EnvSet 'USE_PRECOMPILED_HEADER', '0'

/*
 * set up Odin32 runtime
 */
/*
if (\fMake) then do
    if (fRelease) then do
        call EnvAddFront 'PATH', G.PATH_SDK_ODIN32_BIN';'G.PATH_LIB_ODIN32_SRCTREE'\bin'
        call EnvAddFront 'BEGINLIBPATH', G.PATH_SDK_ODIN32_BIN';'G.PATH_LIB_ODIN32_SRCTREE'\bin'
    end
    else do
        call EnvAddFront 'PATH', G.PATH_SDK_ODIN32_DBGBIN';'G.PATH_LIB_ODIN32_SRCTREE'\bin'
        call EnvAddFront 'BEGINLIBPATH', G.PATH_SDK_ODIN32_DBGBIN';'G.PATH_LIB_ODIN32_SRCTREE'\bin'
        call EnvSet 'WIN32.DEBUGBREAK', '1'
    end

    if (fOdinLog) then call EnvSet 'WIN32LOG_ENABLED', '1'
    else call EnvSet 'WIN32LOG_ENABLED', ''
end
*/

/*
 * Various Java runtime settings
 */
if (\fRelease & \fMake) then do
    call EnvSet 'JAVA_TOOL_OPTIONS', '-XX:+UseOSErrorReporting'
end

/* Special hotspot build setup */
if (fMake & wordpos('hotspot', fTargets) > 0) then do

    /* Only build the clent JVM to speed up the rebuild cycle */
    call EnvSet 'BUILD_CLIENT_ONLY', '1'

    /* Also manually copy JVM.DLL to the right location as the hotspot target doesn't do so */
    if (fProductRelease) then
        build_path = DosSlashes(EnvGet('ALT_OUTPUTDIR'))
    else do
        build_path = ScriptDir'openjdk\build\os2-i586'
        if (wordpos('debug_build', aArgs) > 0) then build_path = build_path'-debug'
        else if (wordpos('fastdebug_build', aArgs) > 0) then build_path = build_path'-fastdebug'
    end
    ExtraArgs = 'xcopy' build_path'\hotspot\import\jre\bin\client\*' build_path'\bin\client\'
    aArgs = aArgs '&&' ExtraArgs
end

/*
 * finally, start the command
 */
if (\UnderSE) then do
    /* final mark */
    call EnvSet 'OPENJDK_ENV_CMD_DONE', 1

    if (aArgs \== '') then do
        /* Start the program */
        prg = translate(word(aArgs, 1))
        isCmd = 0
        realPrg = SysSearchPath('PATH', prg)
        if (realPrg == '') then realPrg = SysSearchPath('PATH', prg'.EXE')
        if (realPrg == '') then realPrg = SysSearchPath('PATH', prg'.COM')
        if (realPrg == '') then realPrg = SysSearchPath('PATH', prg'.CMD')
        if (realPrg \== '') then do
            if (right(realPrg, 4) == '.CMD') then isCmd = 1
        end
        if (isCmd) then 'call' aArgs
        else aArgs
    end
    else do
        /* Print some information about the environment */
        say
        say 'OpenJDK build environment is set up.'
    end
end
else do
    if (G.LOG_FILE \== '' & aArgs \= '') then do
        /* copy all output to the log file */
        aArgs = '('aArgs') 2>&1 | tee' G.LOG_FILE
    end
    call EnvSet 'SE_CMD_ARGS', aArgs
end

exit

OnError:
    if (rc \= 0 & symbol('cmdline') == 'VAR') then
        say 'ERROR: Executing "'cmdline'" failed with code' rc'.'
	exit rc

/*----------------------------------------------------------------------------
 * function library
 *----------------------------------------------------------------------------*/

UnixSlashes: procedure
    parse arg aPath
    return translate(aPath, '/', '\')

DosSlashes: procedure
    parse arg aPath
    return translate(aPath, '\', '/')

/**
 * Add sToAdd in front of sEnvVar. sToAdd may be a list.
 *
 * See EnvAddFrontOrBack for details.
 */
EnvAddFront: procedure
    parse arg sEnvVar, sToAdd, sSeparator
    return EnvAddFrontOrBack(1, 0, sEnvVar, sToAdd, sSeparator);

/**
 * Add sToAdd to the end of sEnvVar. sToAdd may be a list.
 *
 * See EnvAddFrontOrBack for details.
 */
EnvAddEnd: procedure
    parse arg sEnvVar, sToAdd, sSeparator
    return EnvAddFrontOrBack(0, 0, sEnvVar, sToAdd, sSeparator);

/**
 * Remove all occurences of sToRemove from sEnvVar. sToRemove may be a list.
 *
 * See EnvAddFrontOrBack for details.
 */
EnvRemove: procedure
    parse arg sEnvVar, sToRemove, sSeparator
    return EnvAddFrontOrBack(0, 1, sEnvVar, sToRemove, sSeparator);

/**
 * Add sToAdd in front or at the end of sEnvVar. sToAdd may be a list.
 *
 * Note: This function will remove leading and trailing semicolons, as well
 * as duplcate semicolons somewhere in the middle. See
 *
 *   http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Environment-Variables.html
 *
 * (CPLUS_INCLUDE_PATH and friends) for the explaination why it is necessary to
 * do. If you want to specifially mean the current working directory, use "."
 * instead of an empty path element.
 */
EnvAddFrontOrBack: procedure
    parse arg fFront, fRM, sEnvVar, sToAdd, sSeparator

    /* sets default separator if not specified. */
    if (sSeparator = '') then sSeparator = ';';

    /* get rid of extra ';' */
    sToAdd = strip(sToAdd,, sSeparator);
    sToAddClean = ''
    do i = 1 to length(sToAdd)
        ch = substr(sToAdd, i, 1);
        if (ch == sSeparator & right(sToAddClean, 1) == sSeparator) then
            iterate
        sToAddClean = sToAddClean||ch
    end

    /* Get original variable value */
    sOrgEnvVar = EnvGet(sEnvVar);

    /* loop thru sToAddClean */
    rc = 0;
    i = length(sToAddClean);
    do while (i > 0 & rc = 0)
        j = lastpos(sSeparator, sToAddClean, i);
        sOne = substr(sToAddClean, j+1, i - j);
        cbOne = length(sOne);
        /* Remove previous sOne if exists. (Changing sOrgEnvVar). */
        s1 = 1;
        do while (s1 <= length(sOrgEnvVar))
            s2 = pos(sSeparator, sOrgEnvVar, s1);
            if (s2 == 0) then
                s2 = length(sOrgEnvVar) + 1;
            if (translate(substr(sOrgEnvVar, s1, s2 - s1)) == translate(sOne)) then do
                sOrgEnvVar = delstr(sOrgEnvVar, s1, s2 - s1 + 1 /*+sep*/);
                s1 = s1 + 1;
            end
            else do
                s1 = s2 + 1;
            end
        end
        sOrgEnvVar = strip(sOrgEnvVar,, sSeparator);

        i = j - 1;
    end

    /* set environment */
    if (fRM) then
        return EnvSet(sEnvVar, sOrgEnvVar);

    /* add sToAddClean in necessary mode */
    if (fFront) then
    do
        if (sOrgEnvVar \== '' & left(sOrgEnvVar, 1) \== sSeparator) then
            sOrgEnvVar = sSeparator||sOrgEnvVar;
        sOrgEnvVar = sToAddClean||sOrgEnvVar;
    end
    else
    do
        if (sOrgEnvVar \== '' & right(sOrgEnvVar, 1) \== sSeparator) then
            sOrgEnvVar = sOrgEnvVar||sSeparator;
        sOrgEnvVar = sOrgEnvVar||sToAddClean;
    end

    return EnvSet(sEnvVar, sOrgEnvVar);

/**
 * Sets sEnvVar to sValue.
 */
EnvSet: procedure
    parse arg sEnvVar, sValue

    sEnvVar = translate(sEnvVar);

    /*
     * Begin/EndLibpath fix:
     *      We'll have to set internal these using both commandline 'SET'
     *      and internal VALUE in order to export it and to be able to
     *      get it (with EnvGet) again.
     */
    if ((sEnvVar = 'BEGINLIBPATH') | (sEnvVar = 'ENDLIBPATH')) then
    do
        if (length(sValue) >= 1024) then
            say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
        return SysSetExtLibPath(sValue, substr(sEnvVar, 1, 1));
    end

    if (length(sValue) >= 1024) then
    do
        say 'Warning: 'sEnvVar' is too long,' length(sValue)' char.';
        say '    This may make CMD.EXE unstable after a SET operation to print the environment.';
    end
    sRc = VALUE(sEnvVar, sValue, 'OS2ENVIRONMENT');
    return 0;

/**
 * Gets the value of sEnvVar.
 */
EnvGet: procedure
    parse arg sEnvVar
    if ((translate(sEnvVar) = 'BEGINLIBPATH') | (translate(sEnvVar) = 'ENDLIBPATH')) then
        return SysQueryExtLibPath(substr(sEnvVar, 1, 1));
    return value(sEnvVar,, 'OS2ENVIRONMENT');

/**
 * Sets sEnvVar to sValue if sEnvVar is not currently set (empty), otherwise
 * leaves it as is.
 */
EnvSetIfEmpty: procedure
    parse arg sEnvVar, sValue
    if (EnvGet(sEnvVar) == '') then call EnvSet sEnvVar, sValue
    return
