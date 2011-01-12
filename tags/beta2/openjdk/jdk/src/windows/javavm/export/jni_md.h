/*
 * Copyright 1996-1998 Sun Microsystems, Inc.  All Rights Reserved.
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

#ifndef _JAVASOFT_JNI_MD_H_
#define _JAVASOFT_JNI_MD_H_

#define JNIEXPORT __declspec(dllexport)
#define JNIIMPORT __declspec(dllimport)
#define JNICALL __stdcall

typedef long jint;
#ifdef __EMX__
typedef __int64_t jlong;
#else
typedef __int64 jlong;
#endif
typedef signed char jbyte;

#ifdef __cplusplus

/* template for safe type casting: the generic version lets the compiler
 * decide (as if no template was used); specific instantiations deal with
 * special cases which are guaranteed to be safe */
template<typename TR, typename TS>
inline TR jsafe_cast(TS ts) { return ts; }

#ifdef __EMX__
/* sizeof(jchar) = sizeof(wchar_t) in GCC but the types are not relative
 * (as opposed to MSVC) so an explicit cast is required */
typedef unsigned short jchar;
template<>
inline jchar *jsafe_cast<jchar *, wchar_t *>(wchar_t *ts) { return reinterpret_cast<jchar*>(ts); }
template<>
inline const jchar *jsafe_cast<const jchar *, wchar_t *>(wchar_t *ts) { return reinterpret_cast<const jchar*>(ts); }
template<>
inline const jchar *jsafe_cast<const jchar *, const wchar_t *>(const wchar_t *ts) { return reinterpret_cast<const jchar*>(ts); }
template<>
inline wchar_t *jsafe_cast<wchar_t *, jchar *>(jchar *ts) { return reinterpret_cast<wchar_t*>(ts); }
template<>
inline const wchar_t *jsafe_cast<const wchar_t *, jchar *>(jchar *ts) { return reinterpret_cast<const wchar_t*>(ts); }
template<>
inline const wchar_t *jsafe_cast<const wchar_t *, const jchar *>(const jchar *ts) { return reinterpret_cast<const wchar_t*>(ts); }
#endif

#endif /* __cplusplus */

#endif /* !_JAVASOFT_JNI_MD_H_ */
