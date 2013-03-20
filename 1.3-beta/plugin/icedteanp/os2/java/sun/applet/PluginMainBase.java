/* PluginMainBase -- OS/2 version
   Copyright (C) 2008  Red Hat

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

package sun.applet;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

class PluginMainBase {

    private static int inPipe = -1;
    private static int outPipe = -1;

    static boolean checkArgs(String args[]) {
        if (args.length == 2) {
            inPipe = Integer.valueOf(args[0]).intValue();
            outPipe = Integer.valueOf(args[1]).intValue();
            if (inPipe >= 0 && outPipe >= 0)
                return true;
        }
        System.err.println("Invalid pipe descriptors provided. Refusing to proceed.");
        return false;
    }

    static PluginStreamHandler connect(String args[]) {
        PluginStreamHandler streamHandler = null;
        sun.misc.JavaIOFileDescriptorAccess fdAccess = sun.misc.SharedSecrets.getJavaIOFileDescriptorAccess();
        FileDescriptor inPipeFD = new FileDescriptor();
        fdAccess.set(inPipeFD, inPipe);
        FileDescriptor outPipeFD = new FileDescriptor();
        fdAccess.set(outPipeFD, outPipe);
        try {
            streamHandler = new PluginStreamHandler(new FileInputStream(inPipeFD), new FileOutputStream(outPipeFD));
            PluginDebug.debug("Streams initialized");
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
        return streamHandler;
    }
}
