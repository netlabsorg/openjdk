/*
 * Ant setup script. This must be a searate file because JAVA_HOME
 * is needed by Ant but must not be set in the OpenJDK build env.
 */
call do_the_job
exit

do_the_job: procedure

    env = 'OS2ENVIRONMENT'
    JAVA_HOME = translate(value('ALT_BOOTDIR',, env), '\', '/')
    PATH = value('PATH',, env)
    call value 'JAVA_HOME', JAVA_HOME, env
    call value 'PATH', JAVA_HOME'\JRE\BIN;'JAVA_HOME'\BIN;'PATH, env
    call value 'JAVA2_FILEENCODING', 'CP866', env
    return

