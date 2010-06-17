/*
 * Script to set up the build environment for OpenJDK
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

if (\UnderSE & EnvGet('OPENJDK_ENV_CMD_DONE') == 1) then do
    say
    say 'OpenJDK build environment is already set up.'
    exit 0
end

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
if (G.PATH_TOOL_GCC442 \== '') then do
    cmdline = 'call' G.PATH_TOOL_GCC442'\gcc442.cmd @'
    cmdline
    drop cmdline
end

/*
 * setup Ant
 */
if (G.PATH_TOOL_ANT \== '') then do
    call EnvSet 'ANT_HOME', G.PATH_TOOL_ANT
    call EnvSet 'antenv', G.PATH_TOOL_ANT'\bin\antenv.cmd'
    call EnvSet 'runrc', ScriptDir'antrc.cmd'
    call EnvAddFront 'PATH', G.PATH_TOOL_ANT'\bin'
end

/*
 * add SVN tools to PATH
 */
call EnvAddFront 'PATH', ScriptDir'tools\bin'
call EnvAddFront 'BEGINLIBPATH', ScriptDir'tools\bin'

/*
 * bash is required for the build
 */
call EnvSet 'MAKESHELL', 'bash.exe'

/*
 * these must be always unset
 */
call EnvSet 'CLASSPATH', ''
call EnvSet 'JAVA_HOME', ''

/*
 * various variables for OpenJDK make files
 */
call EnvSetIfEmtpy 'ALT_BOOTDIR', UnixSlashes(G.PATH_TOOL_BOOT_JDK)
call EnvSetIfEmtpy 'ALT_ODINSDK_PATH', UnixSlashes(G.PATH_LIB_ODIN32)
call EnvSetIfEmtpy 'ALT_FREETYPE_HEADERS_PATH', UnixSlashes(ScriptDir'libs\freetype\include')
call EnvSetIfEmtpy 'ALT_FREETYPE_LIB_PATH', UnixSlashes(ScriptDir'libs\freetype\lib')

/*
 * @todo temporarily disable some components
 */
call EnvSet 'BUILD_CORBA', 'false'
call EnvSet 'BUILD_JAXP', 'false'
call EnvSet 'BUILD_JAXWS', 'false'

if (\UnderSE) then do
    /* final mark */
    call EnvSet 'OPENJDK_ENV_CMD_DONE', 1

    if (aArgs \== '') then do
        /* Start the program */
        prg = translate(word(aArgs, 1))
        isCmd = 0
        realPrg = SysSearchPath(PATH, prg)
        if (realPrg == '') then realPrg = SysSearchPath(PATH, prg'.EXE')
        if (realPrg == '') then realPrg = SysSearchPath(PATH, prg'.COM')
        if (realPrg == '') then realPrg = SysSearchPath(PATH, prg'.CMD')
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
    if (G.LOG_FILE \== '') then do
        /* instruct to copy all output to the log file */
        args = EnvGet('SE_CMD_ARGS')' 2>&1 | tee' G.LOG_FILE
        call EnvSet 'SE_CMD_ARGS', args
    end
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
EnvSetIfEmtpy: procedure
    parse arg sEnvVar, sValue
    if (EnvGet(sEnvVar) == '') then call EnvSet sEnvVar, sValue
    return
