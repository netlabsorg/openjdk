/* OS_OS2_WinOS2.cc

   Copyright (C) 2009, 2010  Red Hat

This file is part of IcedTea.

IcedTea is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

IcedTea is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with IcedTea; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version. */

#include <windows.h>

#include <winuser32.h> // CreateFakeWindowEx and friends

#include "IcedTeaPluginUtils.h"

static ATOM WrapperClass;

LRESULT WIN32API WrapperWndProc (HWND hwnd, UINT msg, WPARAM mp1, LPARAM mp2)
{
    return DefWindowProc (hwnd, msg, mp1, mp2);
}

bool init_os_winos2()
{
    WNDCLASSA WndClass;
    memset (&WndClass, 0, sizeof(WndClass));
    WndClass.lpfnWndProc = WrapperWndProc;
    WndClass.lpszClassName = "WC_ICEDTEANP_WRAPPER";
    WndClass.cbWndExtra = 0;
    WrapperClass = RegisterClass (&WndClass);
    if (!WrapperClass)
    {
        PLUGIN_DEBUG ("RegisterClass failed.");
        return FALSE;
    }

    return TRUE;
}

void *wrap_window_handle (void *handle)
{
    HWND hwnd = CreateFakeWindowEx ((HWND) handle, WrapperClass);
    return (void *) hwnd;
}
