/*
 * Copyright 2000 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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

/*
 *
 */

package bench.rmi;

import bench.Benchmark;
import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;

/**
 * Benchmark for testing speed of null calls.
 */
public class NullCalls implements Benchmark {

    interface Server extends Remote {
        public void call() throws RemoteException;
    }

    static class ServerImpl extends UnicastRemoteObject implements Server {
        public ServerImpl() throws RemoteException {
        }

        public void call() throws RemoteException {
        }
    }

    static class ServerFactory implements BenchServer.RemoteObjectFactory {
        public Remote create() throws RemoteException {
            return new ServerImpl();
        }
    }

    /**
     * Issue null calls.
     * Arguments: <# calls>
     */
    public long run(String[] args) throws Exception {
        int reps = Integer.parseInt(args[0]);
        BenchServer bsrv = Main.getBenchServer();
        Server stub = (Server) bsrv.create(new ServerFactory());

        long start = System.currentTimeMillis();
        for (int i = 0; i < reps; i++)
            stub.call();
        long time = System.currentTimeMillis() - start;

        bsrv.unexport(stub, true);
        return time;
    }
}
