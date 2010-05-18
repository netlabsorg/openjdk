/*
 * Copyright 2004-2005 Sun Microsystems, Inc.  All Rights Reserved.
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
#include <stdlib.h>

#include "jni.h"
#include "manifest_info.h"
#include "JarFacade.h"

typedef struct {
    jarAttribute* head;
    jarAttribute* tail;
} iterationContext;

static void
doAttribute(const char* name, const char* value, void* user_data) {
    iterationContext* context = (iterationContext*) user_data;

    jarAttribute* attribute = (jarAttribute*)malloc(sizeof(jarAttribute));
    if (attribute != NULL) {
        attribute->name = strdup(name);
        if (attribute->name == NULL) {
            free(attribute);
        } else {
            attribute->value = strdup(value);
            if (attribute->value == NULL) {
                free(attribute->name);
                free(attribute);
            } else {
                attribute->next = NULL;
                if (context->head == NULL) {
                    context->head = attribute;
                } else {
                    context->tail->next = attribute;
                }
                context->tail = attribute;
            }
        }

    }
}

/*
 * Return a list of attributes from the main section of the given JAR
 * file. Returns NULL if there is an error or there aren't any attributes.
 */
jarAttribute*
readAttributes(const char* jarfile)
{
    int rc;
    iterationContext context = { NULL, NULL };

    rc = JLI_ManifestIterate(jarfile, doAttribute, (void*)&context);

    if (rc == 0) {
        return context.head;
    } else {
        freeAttributes(context.head);
        return NULL;
    }
}


/*
 * Free a list of attributes
 */
void
freeAttributes(jarAttribute* head) {
    while (head != NULL) {
        jarAttribute* next = (jarAttribute*)head->next;
        free(head->name);
        free(head->value);
        free(head);
        head = next;
    }
}

/*
 * Get the value of an attribute in an attribute list. Returns NULL
 * if attribute not found.
 */
char*
getAttribute(const jarAttribute* attributes, const char* name) {
    while (attributes != NULL) {
        if (strcasecmp(attributes->name, name) == 0) {
            return attributes->value;
        }
        attributes = (jarAttribute*)attributes->next;
    }
    return NULL;
}
