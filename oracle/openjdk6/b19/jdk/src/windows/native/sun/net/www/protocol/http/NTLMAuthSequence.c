/*
 * Copyright 2002-2005 Sun Microsystems, Inc.  All Rights Reserved.
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

#include <jni.h>
#include <windows.h>
#include <rpc.h>
#include <winsock.h>
#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <fcntl.h>

#define SECURITY_WIN32
#include "sspi.h"
#include "issperr.h"


/*
 * OS calls loaded from DLL on intialization
 */

static FREE_CREDENTIALS_HANDLE_FN pFreeCredentialsHandle;
static ACQUIRE_CREDENTIALS_HANDLE_FN pAcquireCredentialsHandle;
static FREE_CONTEXT_BUFFER_FN pFreeContextBuffer;
static INITIALIZE_SECURITY_CONTEXT_FN pInitializeSecurityContext;
static COMPLETE_AUTH_TOKEN_FN pCompleteAuthToken;
static DELETE_SECURITY_CONTEXT_FN pDeleteSecurityContext;

static void endSequence (PCredHandle credHand, PCtxtHandle ctxHandle, SecBufferDesc OutBuffDesc);

static jfieldID ntlm_ctxHandleID;
static jfieldID ntlm_crdHandleID;

static HINSTANCE lib = NULL;

JNIEXPORT void JNICALL Java_sun_net_www_protocol_http_NTLMAuthSequence_initFirst
(JNIEnv *env, jclass clazz)
{
    OSVERSIONINFO   version;
    UCHAR libName[MAX_PATH];

    ntlm_ctxHandleID = (*env)->GetFieldID(env, clazz, "ctxHandle", "J");
    ntlm_crdHandleID = (*env)->GetFieldID(env, clazz, "crdHandle", "J");

    version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&version);

    if (version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        strcpy (libName, "security.dll" );
    }
    else if (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        strcpy (libName, "secur32.dll" );
    }

    lib = LoadLibrary (libName);

    pFreeCredentialsHandle
        = (FREE_CREDENTIALS_HANDLE_FN) GetProcAddress(
        lib, "FreeCredentialsHandle" );

    pAcquireCredentialsHandle
        = (ACQUIRE_CREDENTIALS_HANDLE_FN) GetProcAddress(
        lib, "AcquireCredentialsHandleA" );

    pFreeContextBuffer
        = (FREE_CONTEXT_BUFFER_FN) GetProcAddress(
        lib, "FreeContextBuffer" );

    pInitializeSecurityContext
        = (INITIALIZE_SECURITY_CONTEXT_FN) GetProcAddress(
        lib, "InitializeSecurityContextA" );

    pCompleteAuthToken
        = (COMPLETE_AUTH_TOKEN_FN) GetProcAddress(
        lib, "CompleteAuthToken" );

    pDeleteSecurityContext
        = (DELETE_SECURITY_CONTEXT_FN) GetProcAddress(
        lib, "DeleteSecurityContext" );

}

/*
 * Class:     sun_net_www_protocol_http_NTLMAuthSequence
 * Method:    getCredentialsHandle
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)J
 */

JNIEXPORT jlong JNICALL Java_sun_net_www_protocol_http_NTLMAuthSequence_getCredentialsHandle
(JNIEnv *env, jobject this, jstring user, jstring domain, jstring password)
{
    SEC_WINNT_AUTH_IDENTITY   AuthId;
    SEC_WINNT_AUTH_IDENTITY * pAuthId;
    CHAR        *pUser = 0;
    CHAR        *pDomain = 0;
    CHAR        *pPassword = 0;
    CredHandle      *pCred;
    TimeStamp            ltime;
    jboolean         isCopy;
    SECURITY_STATUS      ss;

    if (user != 0) {
        pUser = (CHAR *)(*env)->GetStringUTFChars(env, user, &isCopy);
    }
    if (domain != 0) {
        pDomain = (CHAR *)(*env)->GetStringUTFChars(env, domain, &isCopy);
    }
    if (password != 0) {
        pPassword = (CHAR *)(*env)->GetStringUTFChars(env, password, &isCopy);
    }
    pCred = (CredHandle *)malloc(sizeof (CredHandle));

    if ( ((pUser != NULL) || (pPassword != NULL)) || (pDomain != NULL)) {
        pAuthId = &AuthId;

        memset( &AuthId, 0, sizeof( AuthId ));

        if ( pUser != NULL ) {
            AuthId.User       = (unsigned char *) pUser;
            AuthId.UserLength = strlen( pUser );
        }

        if ( pPassword != NULL ) {
            AuthId.Password       = (unsigned char *) pPassword;
            AuthId.PasswordLength = strlen( pPassword );
        }

        if ( pDomain != NULL ) {
            AuthId.Domain       = (unsigned char *) pDomain;
            AuthId.DomainLength = strlen( pDomain );
        }

        AuthId.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    } else {
        pAuthId = NULL;
    }

    ss = pAcquireCredentialsHandle(
        NULL, "NTLM", SECPKG_CRED_OUTBOUND,
        NULL, pAuthId, NULL, NULL,
        pCred, &ltime
        );

    if (ss == 0) {
        return (jlong) pCred;
    } else {
        return 0;
    }
}

JNIEXPORT jbyteArray JNICALL Java_sun_net_www_protocol_http_NTLMAuthSequence_getNextToken
(JNIEnv *env, jobject this, jlong crdHandle, jbyteArray lastToken)
{

    VOID        *pInput = 0;
    DWORD            inputLen;
    CHAR         buffOut[512];
    DWORD        pcbBuffOut;
    jboolean         isCopy;
    SECURITY_STATUS      ss;
    SecBufferDesc        OutBuffDesc;
    SecBuffer            OutSecBuff;
    SecBufferDesc        InBuffDesc;
    SecBuffer            InSecBuff;
    ULONG                ContextAttributes;
    CredHandle      *pCred = (CredHandle *)crdHandle;
    CtxtHandle      *pCtx;
    CtxtHandle      *newContext;
    TimeStamp            ltime;
    jbyteArray       result;


    pCtx = (CtxtHandle *) (*env)->GetLongField (env, this, ntlm_ctxHandleID);
    if (pCtx == 0) { /* first call */
        newContext = (CtxtHandle *)malloc(sizeof(CtxtHandle));
        (*env)->SetLongField (env, this, ntlm_ctxHandleID, (jlong)newContext);
    } else {
        newContext = pCtx;
    }

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = 512;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = buffOut;

    /*
     *  Prepare our Input buffer - Note the server is expecting the client's
     *  negotiation packet on the first call
     */

    if (lastToken != 0)
    {
        pInput = (VOID *)(*env)->GetByteArrayElements(env, lastToken, &isCopy);
        inputLen = (*env)->GetArrayLength(env, lastToken);

        InBuffDesc.ulVersion = 0;
        InBuffDesc.cBuffers  = 1;
        InBuffDesc.pBuffers  = &InSecBuff;

        InSecBuff.cbBuffer       = inputLen;
        InSecBuff.BufferType = SECBUFFER_TOKEN;
        InSecBuff.pvBuffer       = pInput;
    }

    /*
     *  will return success when its done but we still
     *  need to send the out buffer if there are bytes to send
     */

    ss = pInitializeSecurityContext(
        pCred, pCtx, NULL, 0, 0, SECURITY_NATIVE_DREP,
        lastToken ? &InBuffDesc : NULL, 0, newContext, &OutBuffDesc,
        &ContextAttributes, &ltime
    );

    if (pInput != 0) {
        (*env)->ReleaseByteArrayElements(env, lastToken, pInput, JNI_ABORT);
    }

    if (ss < 0) {
        endSequence (pCred, pCtx, OutBuffDesc);
        return 0;
    }

    if ((ss == SEC_I_COMPLETE_NEEDED) || (ss == SEC_I_COMPLETE_AND_CONTINUE) ) {
        ss = pCompleteAuthToken( pCtx, &OutBuffDesc );

        if (ss < 0) {
            endSequence (pCred, pCtx, OutBuffDesc);
            return 0;
        }
    }

    if ( OutSecBuff.cbBuffer > 0 ) {
        jbyteArray ret = (*env)->NewByteArray(env, OutSecBuff.cbBuffer);
        (*env)->SetByteArrayRegion(env, ret, 0, OutSecBuff.cbBuffer,
                OutSecBuff.pvBuffer);
        if (lastToken != 0) // 2nd stage
            endSequence (pCred, pCtx, OutBuffDesc);
        result = ret;
    }

    if ((ss != SEC_I_CONTINUE_NEEDED) && (ss == SEC_I_COMPLETE_AND_CONTINUE)) {
        endSequence (pCred, pCtx, OutBuffDesc);
    }

    return result;
}

static void endSequence (PCredHandle credHand, PCtxtHandle ctxHandle, SecBufferDesc OutBuffDesc) {
    if (credHand != 0) {
        pFreeCredentialsHandle (credHand);
        free (credHand);
    }

    pFreeContextBuffer (&OutBuffDesc);

    if (ctxHandle != 0) {
        pDeleteSecurityContext(ctxHandle);
        free (ctxHandle);
    }
}
