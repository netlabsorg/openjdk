/*
 * Copyright 1994-2005 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Sun designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Sun in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 */

#include <string.h>

#include "jni.h"
#include "jni_util.h"
#include "jvm.h"
#include "java_props.h"

#include "java_lang_System.h"

#define OBJ "Ljava/lang/Object;"

/* Only register the performance-critical methods */
static JNINativeMethod methods[] = {
    {"currentTimeMillis", "()J",              (void *)&JVM_CurrentTimeMillis},
    {"nanoTime",          "()J",              (void *)&JVM_NanoTime},
    {"arraycopy",     "(" OBJ "I" OBJ "II)V", (void *)&JVM_ArrayCopy},
};

#undef OBJ

JNIEXPORT void JNICALL
Java_java_lang_System_registerNatives(JNIEnv *env, jclass cls)
{
    (*env)->RegisterNatives(env, cls,
                            methods, sizeof(methods)/sizeof(methods[0]));
}

JNIEXPORT jint JNICALL
Java_java_lang_System_identityHashCode(JNIEnv *env, jobject this, jobject x)
{
    return JVM_IHashCode(env, x);
}

#define PUTPROP(props, key, val) \
    if (1) { \
        jstring jkey = (*env)->NewStringUTF(env, key); \
        jstring jval = (*env)->NewStringUTF(env, val); \
        jobject r = (*env)->CallObjectMethod(env, props, putID, jkey, jval); \
        if ((*env)->ExceptionOccurred(env)) return NULL; \
        (*env)->DeleteLocalRef(env, jkey); \
        (*env)->DeleteLocalRef(env, jval); \
        (*env)->DeleteLocalRef(env, r); \
    } else ((void) 0)

#define PUTPROP_ForPlatformCString(props, key, val) \
    if (1) { \
        jstring jkey = JNU_NewStringPlatform(env, key); \
        jstring jval = JNU_NewStringPlatform(env, val); \
        jobject r = (*env)->CallObjectMethod(env, props, putID, jkey, jval); \
        if ((*env)->ExceptionOccurred(env)) return NULL; \
        (*env)->DeleteLocalRef(env, jkey); \
        (*env)->DeleteLocalRef(env, jval); \
        (*env)->DeleteLocalRef(env, r); \
    } else ((void) 0)

#ifndef VENDOR /* Third party may overwrite this. */
#define VENDOR "Sun Microsystems Inc."
#define VENDOR_URL "http://java.sun.com/"
#define VENDOR_URL_BUG "http://java.sun.com/cgi-bin/bugreport.cgi"
#endif

#define JAVA_MAX_SUPPORTED_VERSION 50
#define JAVA_MAX_SUPPORTED_MINOR_VERSION 0

JNIEXPORT jobject JNICALL
Java_java_lang_System_initProperties(JNIEnv *env, jclass cla, jobject props)
{
    char buf[128];
    java_props_t *sprops = GetJavaProperties(env);
    jmethodID putID = (*env)->GetMethodID(env,
                                          (*env)->GetObjectClass(env, props),
                                          "put",
            "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    if (sprops == NULL || putID == NULL ) return NULL;

    PUTPROP(props, "java.specification.version",
            JDK_MAJOR_VERSION "." JDK_MINOR_VERSION);
    PUTPROP(props, "java.specification.name",
            "Java Platform API Specification");
    PUTPROP(props, "java.specification.vendor", "Sun Microsystems Inc.");

    PUTPROP(props, "java.version", RELEASE);
    PUTPROP(props, "java.vendor", VENDOR);
    PUTPROP(props, "java.vendor.url", VENDOR_URL);
    PUTPROP(props, "java.vendor.url.bug", VENDOR_URL_BUG);

    jio_snprintf(buf, sizeof(buf), "%d.%d", JAVA_MAX_SUPPORTED_VERSION,
                                            JAVA_MAX_SUPPORTED_MINOR_VERSION);
    PUTPROP(props, "java.class.version", buf);

    if (sprops->awt_toolkit) {
        PUTPROP(props, "awt.toolkit", sprops->awt_toolkit);
    }

    /* os properties */
    PUTPROP(props, "os.name", sprops->os_name);
    PUTPROP(props, "os.version", sprops->os_version);
    PUTPROP(props, "os.arch", sprops->os_arch);

    /* file system properties */
    PUTPROP(props, "file.separator", sprops->file_separator);
    PUTPROP(props, "path.separator", sprops->path_separator);
    PUTPROP(props, "line.separator", sprops->line_separator);

    /*
     *  user.language
     *  user.country, user.variant (if user's environment specifies them)
     *  file.encoding
     *  file.encoding.pkg
     */
    PUTPROP(props, "user.language", sprops->language);
    if (sprops->country) {
        PUTPROP(props, "user.country", sprops->country);
    }
    if (sprops->variant) {
        PUTPROP(props, "user.variant", sprops->variant);
    }
    PUTPROP(props, "file.encoding", sprops->encoding);
    PUTPROP(props, "sun.jnu.encoding", sprops->sun_jnu_encoding);
    PUTPROP(props, "file.encoding.pkg", "sun.io");
    /* unicode_encoding specifies the default endianness */
    PUTPROP(props, "sun.io.unicode.encoding", sprops->unicode_encoding);
    PUTPROP(props, "sun.cpu.isalist",
            (sprops->cpu_isalist ? sprops->cpu_isalist : ""));
    PUTPROP(props, "sun.cpu.endian",  sprops->cpu_endian);

    /* !!! DO NOT call PUTPROP_ForPlatformCString before this line !!!
     * !!! I18n properties have not been set up yet !!!
     */

    /* Printing properties */
    /* Note: java.awt.printerjob is an implementation private property which
     * just happens to have a java.* name because it is referenced in
     * a java.awt class. It is the mechanism by which the Sun implementation
     * finds the appropriate class in the JRE for the platform.
     * It is explicitly not designed to be overridden by clients as
     * a way of replacing the implementation class, and in any case
     * the mechanism by which the class is loaded is constrained to only
     * find and load classes that are part of the JRE.
     * This property may be removed if that mechanism is redesigned
     */
    PUTPROP(props, "java.awt.printerjob", sprops->printerJob);

    /* data model */
    if (sizeof(sprops) == 4) {
        sprops->data_model = "32";
    } else if (sizeof(sprops) == 8) {
        sprops->data_model = "64";
    } else {
        sprops->data_model = "unknown";
    }
    PUTPROP(props, "sun.arch.data.model",  \
                    sprops->data_model);

    /* patch level */
    PUTPROP(props, "sun.os.patch.level",  \
                    sprops->patch_level);

    /* Java2D properties */
    /* Note: java.awt.graphicsenv is an implementation private property which
     * just happens to have a java.* name because it is referenced in
     * a java.awt class. It is the mechanism by which the Sun implementation
     * finds the appropriate class in the JRE for the platform.
     * It is explicitly not designed to be overridden by clients as
     * a way of replacing the implementation class, and in any case
     * the mechanism by which the class is loaded is constrained to only
     * find and load classes that are part of the JRE.
     * This property may be removed if that mechanism is redesigned
     */
    PUTPROP(props, "java.awt.graphicsenv", sprops->graphics_env);
    if (sprops->font_dir != NULL) {
        PUTPROP_ForPlatformCString(props,
                                   "sun.java2d.fontpath", sprops->font_dir);
    }

    PUTPROP_ForPlatformCString(props, "java.io.tmpdir", sprops->tmp_dir);

    PUTPROP_ForPlatformCString(props, "user.name", sprops->user_name);
    PUTPROP_ForPlatformCString(props, "user.home", sprops->user_home);

    PUTPROP(props, "user.timezone", sprops->timezone);

    PUTPROP_ForPlatformCString(props, "user.dir", sprops->user_dir);

    /* This is a sun. property as it is currently only set for Gnome and
     * Windows desktops.
     */
    if (sprops->desktop != NULL) {
        PUTPROP(props, "sun.desktop", sprops->desktop);
    }

    return JVM_InitProperties(env, props);
}

/*
 * The following three functions implement setter methods for
 * java.lang.System.{in, out, err}. They are natively implemented
 * because they violate the semantics of the language (i.e. set final
 * variable).
 */
JNIEXPORT void JNICALL
Java_java_lang_System_setIn0(JNIEnv *env, jclass cla, jobject stream)
{
    jfieldID fid =
        (*env)->GetStaticFieldID(env,cla,"in","Ljava/io/InputStream;");
    if (fid == 0)
        return;
    (*env)->SetStaticObjectField(env,cla,fid,stream);
}

JNIEXPORT void JNICALL
Java_java_lang_System_setOut0(JNIEnv *env, jclass cla, jobject stream)
{
    jfieldID fid =
        (*env)->GetStaticFieldID(env,cla,"out","Ljava/io/PrintStream;");
    if (fid == 0)
        return;
    (*env)->SetStaticObjectField(env,cla,fid,stream);
}

JNIEXPORT void JNICALL
Java_java_lang_System_setErr0(JNIEnv *env, jclass cla, jobject stream)
{
    jfieldID fid =
        (*env)->GetStaticFieldID(env,cla,"err","Ljava/io/PrintStream;");
    if (fid == 0)
        return;
    (*env)->SetStaticObjectField(env,cla,fid,stream);
}

static void cpchars(jchar *dst, char *src, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

JNIEXPORT jstring JNICALL
Java_java_lang_System_mapLibraryName(JNIEnv *env, jclass ign, jstring libname)
{
    int len;
    int prefix_len = (int) strlen(JNI_LIB_PREFIX);
    int suffix_len = (int) strlen(JNI_LIB_SUFFIX);

    jchar chars[256];
    if (libname == NULL) {
        JNU_ThrowNullPointerException(env, 0);
        return NULL;
    }
    len = (*env)->GetStringLength(env, libname);
    if (len > 240) {
        JNU_ThrowIllegalArgumentException(env, "name too long");
        return NULL;
    }
    cpchars(chars, JNI_LIB_PREFIX, prefix_len);
    (*env)->GetStringRegion(env, libname, 0, len, chars + prefix_len);
    len += prefix_len;
    cpchars(chars + len, JNI_LIB_SUFFIX, suffix_len);
    len += suffix_len;

    return (*env)->NewString(env, chars, len);
}
