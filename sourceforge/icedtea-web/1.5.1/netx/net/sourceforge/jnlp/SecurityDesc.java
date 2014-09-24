// Copyright (C) 2001-2003 Jon A. Maxwell (JAM)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

package net.sourceforge.jnlp;

import java.awt.AWTPermission;
import java.io.FilePermission;
import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.net.SocketPermission;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.AllPermission;
import java.security.CodeSource;
import java.security.Permission;
import java.security.PermissionCollection;
import java.security.Permissions;
import java.security.Policy;
import java.security.URIParameter;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.PropertyPermission;
import java.util.Set;

import net.sourceforge.jnlp.config.DeploymentConfiguration;
import net.sourceforge.jnlp.runtime.JNLPRuntime;
import net.sourceforge.jnlp.util.logging.OutputController;

/**
 * The security element.
 *
 * @author <a href="mailto:jmaxwell@users.sourceforge.net">Jon A. Maxwell (JAM)</a> - initial author
 * @version $Revision: 1.7 $
 */
public class SecurityDesc {

    /**
     * Represents the security level requested by an applet/application, as specified in its JNLP or HTML.
     */
    public enum RequestedPermissionLevel {
        NONE(null, null),
        DEFAULT(null, "default"),
        SANDBOX(null, "sandbox"),
        J2EE("j2ee-application-client-permissions", null),
        ALL("all-permissions", "all-permissions");

        public static final String PERMISSIONS_NAME = "permissions";
        private final String jnlpString, htmlString;

        private RequestedPermissionLevel(final String jnlpString, final String htmlString) {
            this.jnlpString = jnlpString;
            this.htmlString = htmlString;
        }

        /**
         * This permission level, as it would appear requested in a JNLP file. null if this level
         * is NONE (unspecified) or cannot be requested in a JNLP file.
         * @return the String level
         */
        public String toJnlpString() {
            return this.jnlpString;
        }

        /**
         * This permission level, as it would appear requested in an HTML file. null if this level
         * is NONE (unspecified) or cannot be requested in an HTML file.
         * @return the String level
         */
        public String toHtmlString() {
            return this.htmlString;
        }

        /**
         * The JNLP permission level corresponding to the given String. If null is given, null comes
         * back. If there is no permission level that can be granted in JNLP matching the given String,
         * null is also returned.
         * @param jnlpString the JNLP permission String
         * @return the matching RequestedPermissionLevel
         */
        public RequestedPermissionLevel fromJnlpString(final String jnlpString) {
            for (final RequestedPermissionLevel level : RequestedPermissionLevel.values()) {
                if (level.jnlpString != null && level.jnlpString.equals(jnlpString)) {
                    return level;
                }
            }
            return null;
        }

        /**
         * The HTML permission level corresponding to the given String. If null is given, null comes
         * back. If there is no permission level that can be granted in HTML matching the given String,
         * null is also returned.
         * @param htmlString the JNLP permission String
         * @return the matching RequestedPermissionLevel
         */
        public RequestedPermissionLevel fromHtmlString(final String htmlString) {
            for (final RequestedPermissionLevel level : RequestedPermissionLevel.values()) {
                if (level.htmlString != null && level.htmlString.equals(htmlString)) {
                    return level;
                }
            }
            return null;
        }
    }

    /*
     * We do not verify security here, the classloader deals with security
     */

    /** All permissions. */
    public static final Object ALL_PERMISSIONS = "All";

    /** Applet permissions. */
    public static final Object SANDBOX_PERMISSIONS = "Sandbox";

    /** J2EE permissions. */
    public static final Object J2EE_PERMISSIONS = "J2SE";

    /** requested permissions type according to HTML or JNLP */
    private final RequestedPermissionLevel requestedPermissionLevel;

    /** permissions type */
    private Object type;

    /** the download host */
    private String downloadHost;

    /** whether sandbox applications should get the show window without banner permission */
    private final boolean grantAwtPermissions;

    /** the JNLP file */
    private final JNLPFile file;

    private final Policy customTrustedPolicy;

    /**
     * URLPermission is new in Java 8, so we use reflection to check for it to keep compatibility
     * with Java 6/7. If we can't find the class or fail to construct it then we continue as usual
     * without.
     * 
     * These are saved as fields so that the reflective lookup only needs to be performed once
     * when the SecurityDesc is constructed, rather than every time a call is made to
     * {@link SecurityDesc#getSandBoxPermissions()}, which is called frequently.
     */
    private static Class<Permission> urlPermissionClass = null;
    private static Constructor<Permission> urlPermissionConstructor = null;

    static {
        try {
            urlPermissionClass = (Class<Permission>) Class.forName("java.net.URLPermission");
            urlPermissionConstructor = urlPermissionClass.getDeclaredConstructor(new Class[] { String.class });
        } catch (final SecurityException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while reflectively finding URLPermission - host is probably not running Java 8+");
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
            urlPermissionClass = null;
            urlPermissionConstructor = null;
        } catch (final ClassNotFoundException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while reflectively finding URLPermission - host is probably not running Java 8+");
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
            urlPermissionClass = null;
            urlPermissionConstructor = null;
        } catch (final NoSuchMethodException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while reflectively finding URLPermission - host is probably not running Java 8+");
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
            urlPermissionClass = null;
            urlPermissionConstructor = null;
        }
    }

    // We go by the rules here:
    // http://java.sun.com/docs/books/tutorial/deployment/doingMoreWithRIA/properties.html

    // Since this is security sensitive, take a conservative approach:
    // Allow only what is specifically allowed, and deny everything else

    /** basic permissions for restricted mode */
    private static Permission j2eePermissions[] = {
            new AWTPermission("accessClipboard"),
            // disabled because we can't at this time prevent an
            // application from accessing other applications' event
            // queues, or even prevent access to security dialog queues.
            //
            // new AWTPermission("accessEventQueue"),
            new RuntimePermission("exitVM"),
            new RuntimePermission("loadLibrary"),
            new RuntimePermission("queuePrintJob"),
            new SocketPermission("*", "connect"),
            new SocketPermission("localhost:1024-", "accept, listen"),
            new FilePermission("*", "read, write"),
            new PropertyPermission("*", "read"),
    };

    /** basic permissions for restricted mode */
    private static Permission sandboxPermissions[] = {
            new SocketPermission("localhost:1024-", "listen"),
            // new SocketPermission("<DownloadHost>", "connect, accept"), // added by code
            new PropertyPermission("java.util.Arrays.useLegacyMergeSort", "read,write"),
            new PropertyPermission("java.version", "read"),
            new PropertyPermission("java.vendor", "read"),
            new PropertyPermission("java.vendor.url", "read"),
            new PropertyPermission("java.class.version", "read"),
            new PropertyPermission("os.name", "read"),
            new PropertyPermission("os.version", "read"),
            new PropertyPermission("os.arch", "read"),
            new PropertyPermission("file.separator", "read"),
            new PropertyPermission("path.separator", "read"),
            new PropertyPermission("line.separator", "read"),
            new PropertyPermission("java.specification.version", "read"),
            new PropertyPermission("java.specification.vendor", "read"),
            new PropertyPermission("java.specification.name", "read"),
            new PropertyPermission("java.vm.specification.vendor", "read"),
            new PropertyPermission("java.vm.specification.name", "read"),
            new PropertyPermission("java.vm.version", "read"),
            new PropertyPermission("java.vm.vendor", "read"),
            new PropertyPermission("java.vm.name", "read"),
            new PropertyPermission("javawebstart.version", "read"),
            new PropertyPermission("javaplugin.*", "read"),
            new PropertyPermission("jnlp.*", "read,write"),
            new PropertyPermission("javaws.*", "read,write"),
            new PropertyPermission("browser", "read"),
            new PropertyPermission("browser.*", "read"),
            new RuntimePermission("exitVM"),
            new RuntimePermission("stopThread"),
        // disabled because we can't at this time prevent an
        // application from accessing other applications' event
        // queues, or even prevent access to security dialog queues.
        //
        // new AWTPermission("accessEventQueue"),
        };

    /** basic permissions for restricted mode */
    private static Permission jnlpRIAPermissions[] = {
            new PropertyPermission("awt.useSystemAAFontSettings", "read,write"),
            new PropertyPermission("http.agent", "read,write"),
            new PropertyPermission("http.keepAlive", "read,write"),
            new PropertyPermission("java.awt.syncLWRequests", "read,write"),
            new PropertyPermission("java.awt.Window.locationByPlatform", "read,write"),
            new PropertyPermission("javaws.cfg.jauthenticator", "read,write"),
            new PropertyPermission("javax.swing.defaultlf", "read,write"),
            new PropertyPermission("sun.awt.noerasebackground", "read,write"),
            new PropertyPermission("sun.awt.erasebackgroundonresize", "read,write"),
            new PropertyPermission("sun.java2d.d3d", "read,write"),
            new PropertyPermission("sun.java2d.dpiaware", "read,write"),
            new PropertyPermission("sun.java2d.noddraw", "read,write"),
            new PropertyPermission("sun.java2d.opengl", "read,write"),
            new PropertyPermission("swing.boldMetal", "read,write"),
            new PropertyPermission("swing.metalTheme", "read,write"),
            new PropertyPermission("swing.noxp", "read,write"),
            new PropertyPermission("swing.useSystemFontSettings", "read,write"),
    };

    /**
     * Create a security descriptor.
     *
     * @param file the JNLP file
     * @param requestedPermissionLevel the permission level specified in the JNLP
     * @param type the type of security
     * @param downloadHost the download host (can always connect to)
     */
    public SecurityDesc(JNLPFile file, RequestedPermissionLevel requestedPermissionLevel, Object type, String downloadHost) {
        if (file == null) {
            throw new NullJnlpFileException();
        }
        this.file = file;
        this.requestedPermissionLevel = requestedPermissionLevel;
        this.type = type;
        this.downloadHost = downloadHost;

        String key = DeploymentConfiguration.KEY_SECURITY_ALLOW_HIDE_WINDOW_WARNING;
        grantAwtPermissions = Boolean.valueOf(JNLPRuntime.getConfiguration().getProperty(key));

        customTrustedPolicy = getCustomTrustedPolicy();
    }

    /**
     * Create a security descriptor.
     *
     * @param file the JNLP file
     * @param type the type of security
     * @param downloadHost the download host (can always connect to)
     */
    public SecurityDesc(JNLPFile file, Object type, String downloadHost) {
        this(file, RequestedPermissionLevel.NONE, type, downloadHost);
    }

    /**
     * Returns a Policy object that represents a custom policy to use instead
     * of granting {@link AllPermission} to a {@link CodeSource}
     *
     * @return a {@link Policy} object to delegate to. May be null, which
     * indicates that no policy exists and AllPermissions should be granted
     * instead.
     */
    private Policy getCustomTrustedPolicy() {
        String key = DeploymentConfiguration.KEY_SECURITY_TRUSTED_POLICY;
        String policyLocation = JNLPRuntime.getConfiguration().getProperty(key);

        Policy policy = null;
        if (policyLocation != null) {
            try {
                URI policyUri = new URI("file://" + policyLocation);
                policy = Policy.getInstance("JavaPolicy", new URIParameter(policyUri));
            } catch (Exception e) {
                OutputController.getLogger().log(OutputController.Level.ERROR_ALL, e);
            }
        }
        // return the appropriate policy, or null
        return policy;
    }

    /**
     * Returns the permissions type, one of: ALL_PERMISSIONS,
     * SANDBOX_PERMISSIONS, J2EE_PERMISSIONS.
     */
    public Object getSecurityType() {
        return type;
    }

    /**
     * Returns a PermissionCollection containing the basic
     * permissions granted depending on the security type.
     *
     * @param cs the CodeSource to get permissions for
     */
    public PermissionCollection getPermissions(CodeSource cs) {
        PermissionCollection permissions = getSandBoxPermissions();

        // discard sandbox, give all
        if (ALL_PERMISSIONS.equals(type)) {
            permissions = new Permissions();
            if (customTrustedPolicy == null) {
                permissions.add(new AllPermission());
                return permissions;
            } else {
                return customTrustedPolicy.getPermissions(cs);
            }
        }

        // add j2ee to sandbox if needed
        if (J2EE_PERMISSIONS.equals(type))
            for (int i = 0; i < j2eePermissions.length; i++)
                permissions.add(j2eePermissions[i]);

        return permissions;
    }

    /**
     * @return the permission level requested in the JNLP
     */
    public RequestedPermissionLevel getRequestedPermissionLevel() {
        return requestedPermissionLevel;
    }

    /**
     * Returns a PermissionCollection containing the sandbox permissions
     */
    public PermissionCollection getSandBoxPermissions() {
        final Permissions permissions = new Permissions();

        for (int i = 0; i < sandboxPermissions.length; i++)
            permissions.add(sandboxPermissions[i]);

        if (grantAwtPermissions) {
            permissions.add(new AWTPermission("showWindowWithoutWarningBanner"));
        }
        if (JNLPRuntime.isWebstartApplication()) {
            if (file == null) {
                throw new NullJnlpFileException("Can not return sandbox permissions, file is null");
            }
            if (file.isApplication()) {
                for (int i = 0; i < jnlpRIAPermissions.length; i++) {
                    permissions.add(jnlpRIAPermissions[i]);
                }
            }
        }

        if (downloadHost != null && downloadHost.length() > 0)
            permissions.add(new SocketPermission(downloadHost,
                                                 "connect, accept"));

        final Collection<Permission> urlPermissions = getUrlPermissions();
        for (final Permission permission : urlPermissions) {
            permissions.add(permission);
        }

        return permissions;
    }

    private Set<Permission> getUrlPermissions() {
        if (urlPermissionClass == null || urlPermissionConstructor == null) {
            return Collections.emptySet();
        }
        final Set<Permission> permissions = new HashSet<Permission>();
        for (final JARDesc jar : file.getResources().getJARs()) {
            try {
                // Allow applets all HTTP methods (ex POST, GET) with any request headers
                // on resources anywhere recursively in or below the applet codebase, only on
                // default ports and ports explicitly specified in resource locations
                final URI resourceLocation = jar.getLocation().toURI().normalize();
                final URI host = getHost(resourceLocation);
                final String hostUriString = host.toString();
                final String urlPermissionUrlString = appendRecursiveSubdirToCodebaseHostString(hostUriString);
                final Permission p = urlPermissionConstructor.newInstance(urlPermissionUrlString);
                permissions.add(p);
            } catch (final URISyntaxException e) {
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Could not determine codebase host for resource at " + jar.getLocation() +  " while generating URLPermissions");
                OutputController.getLogger().log(e);
            } catch (final InvocationTargetException e) {
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while attempting to reflectively generate a URLPermission, probably not running on Java 8+?");
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
            } catch (final InstantiationException e) {
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while attempting to reflectively generate a URLPermission, probably not running on Java 8+?");
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
            } catch (final IllegalAccessException e) {
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while attempting to reflectively generate a URLPermission, probably not running on Java 8+?");
                OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
            }
        }
        try {
            final URI codebase = file.getCodeBase().toURI().normalize();
            final URI host = getHost(codebase);
            final String codebaseHostUriString = host.toString();
            final String urlPermissionUrlString = appendRecursiveSubdirToCodebaseHostString(codebaseHostUriString);
            final Permission p = urlPermissionConstructor.newInstance(urlPermissionUrlString);
            permissions.add(p);
        } catch (final URISyntaxException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Could not determine codebase host for codebase " + file.getCodeBase() +  "  while generating URLPermissions");
            OutputController.getLogger().log(e);
        } catch (final InvocationTargetException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while attempting to reflectively generate a URLPermission, probably not running on Java 8+?");
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
        } catch (final InstantiationException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while attempting to reflectively generate a URLPermission, probably not running on Java 8+?");
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
        } catch (final IllegalAccessException e) {
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, "Exception while attempting to reflectively generate a URLPermission, probably not running on Java 8+?");
            OutputController.getLogger().log(OutputController.Level.WARNING_DEBUG, e);
        }
        return permissions;
    }

    /**
     * Gets the host domain part of an applet's codebase. Removes path, query, and fragment, but preserves scheme,
     * user info, and host. The port used is overridden with the specified port.
     * @param codebase the applet codebase URL
     * @param port
     * @return the host domain of the codebase
     * @throws URISyntaxException
     */
    static URI getHostWithSpecifiedPort(final URI codebase, final int port) throws URISyntaxException {
        requireNonNull(codebase);
        return new URI(codebase.getScheme(), codebase.getUserInfo(), codebase.getHost(), port, null, null, null);
    }

    /**
     * Gets the host domain part of an applet's codebase. Removes path, query, and fragment, but preserves scheme,
     * user info, host, and port.
     * @param codebase the applet codebase URL
     * @return the host domain of the codebase
     * @throws URISyntaxException
     */
    static URI getHost(final URI codebase) throws URISyntaxException {
        requireNonNull(codebase);
        return getHostWithSpecifiedPort(codebase, codebase.getPort());
    }

    /**
     * Appends a recursive access marker to a codebase host, for granting Java 8 URLPermissions which are no
     * more restrictive than the existing SocketPermissions
     * See http://docs.oracle.com/javase/8/docs/api/java/net/URLPermission.html
     * @param codebaseHost the applet's codebase's host domain URL as a String. Expected to be formatted as eg
     *                     "http://example.com:8080" or "http://example.com/"
     * @return the resulting String eg "http://example.com:8080/-
     */
    static String appendRecursiveSubdirToCodebaseHostString(final String codebaseHost) {
        requireNonNull(codebaseHost);
        String result = codebaseHost;
        while (result.endsWith("/")) {
            result = result.substring(0, result.length() - 1);
        }
        // See http://docs.oracle.com/javase/8/docs/api/java/net/URLPermission.html
        result = result + "/-"; // allow access to any resources recursively on the host domain
        return result;
    }

    private static void requireNonNull(final Object arg) {
        if (arg == null) {
            throw new NullPointerException();
        }
    }

    /**
     * Returns all the names of the basic JNLP system properties accessible by RIAs
     */
    public static String[] getJnlpRIAPermissions() {
        String[] jnlpPermissions = new String[jnlpRIAPermissions.length];

        for (int i = 0; i < jnlpRIAPermissions.length; i++)
            jnlpPermissions[i] = jnlpRIAPermissions[i].getName();

        return jnlpPermissions;
    }

}
