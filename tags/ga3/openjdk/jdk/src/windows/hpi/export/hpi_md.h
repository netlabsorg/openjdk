/*
 * Copyright (c) 1998, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#ifndef _JAVASOFT_HPI_MD_H_
#define _JAVASOFT_HPI_MD_H_

#include "timeval_md.h"
#include "io_md.h"
#include "path_md.h"
#include "byteorder_md.h"

#ifdef __EMX__
#define EMXNOP(expr)
#define EMXONLY(expr) expr
#define EMXNOEMX(expr_emx,expr_otherwise) expr_emx
#else /* __EMX__ */
#define EMXNOP(expr) expr
#define EMXONLY(expr)
#define EMXNOEMX(expr_emx,expr_otherwise) expr_otherwise
#endif /* __EMX__ */

#define HPI_TIMEOUT_INFINITY ((jlong)(-1))

#endif /* !_JAVASOFT_HPI_MD_H_ */
