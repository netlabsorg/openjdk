/*REXX*/

say 'Don''t pack ct.sym!'
exit

ver_id = 'openjdk6_b22'

build_id = 'ga-20110627'

src_path = '..'

image_path = '..\openjdk\build-product-release'

out_path = '.'

/*
 * --------------------------------------------------------------------------
 */

/*
'@echo off'
*/

sdk_build_id = ver_id'_sdk_os2_'build_id
jre_build_id = ver_id'_jre_os2_'build_id

/*
 * Create SDK and JRE archives
 * --------------------------------------------------------------------------
 */

rc = CreateImageZIP()
if (rc \= 0) then signal rc_error

rc = CreateImageZIP('JRE')
if (rc \= 0) then signal rc_error

/*
 * Well done!
 * --------------------------------------------------------------------------
 */

say
say 'ALL DONE.'
say

exit 0

/*
 * Local library
 * --------------------------------------------------------------------------
 */

/*
 *
 */
CreateImageZIP:

    parse upper arg aType

    is_jre = aType == 'JRE'

    if (is_jre) then do
        build_id = jre_build_id
        image = 'j2re-image'
    end
    else do
        build_id = sdk_build_id
        image = 'j2sdk-image'
    end

    out = out_path'\'build_id

    'mkdir' out
    if (rc \= 0) then return rc

    'xcopy /s /e /r' image_path'\'image'\*' out'\'
    if (rc \= 0) then return rc

    rc = ProcessMapFiles(out, aType)
    if (rc \= 0) then return rc

    'lxlite /b- /r' out'\*.exe' out'\*.dll'
    if (rc \= 0) then return rc

    'xcopy' src_path'\doc\*.OS2' out'\'
    if (rc \= 0) then return rc

    'zip -SrX9' build_id'.zip' out'/'
    if (rc \= 0) then return rc

    'zip -SrX9' build_id'_map.zip' out'_map/'
    if (rc \= 0) then return rc

    'zip -SrX9' build_id'_sym.zip' out'_sym/'
    if (rc \= 0) then return rc

    return 0

/*
 *
 */
ProcessMapFiles: procedure

    parse arg aOut, aType
    aType = translate(aType)

    is_jre = aType == 'JRE'

    rc = SysFileTree(aOut'\*.map', found, 'FSO')
    if (rc \= 0) then
        return rc
    do i = 1 to found.0
        cwd = directory()
        ncwd = directory(filespec('D', found.i)||,
                         strip(filespec('P', found.i), 'T', '\'))
        'mapsym' filespec('N', found.i)
        rc2 = rc
        call directory cwd
        if (rc2 \= 0) then
            return rc2
    end

    /* copy all .map files to a separate directory */
    'xcopy /s /r' aOut'\*.map' aOut'_map\'
    if (rc \= 0) then
        return rc

    /* copy all .sym files to a separate directory */
    'xcopy /s /r' aOut'\*.sym' aOut'_sym\'
    if (rc \= 0) then
        return rc

    if (\is_jre) then do
        /* remove the mistakenly copied file and its directory */
        'del' aOut'_sym\lib\ct.sym'
        if (rc == 0) then
            'rd' aOut'_sym\lib'
        if (rc \= 0) then
            return rc
    end

    /* delete all .map and .sym files */
    rc = SysFileTree(aOut'\*.map', found, 'FSO')
    if (rc \= 0) then
        return rc

    do i = 1 to found.0
        'del' found.i left(found.i, length(found.i)-4)'.sym'
        if (rc \= 0) then
            return rc
    end

    return 0

/*
 *
 */
rc_error:
    say 'An error has occured during execution of the last command.'
    say 'The error code is' rc'.'
