Sample init scripts and service configuration for vitalcoind
==========================================================

Sample scripts and configuration files for systemd, Upstart and OpenRC
can be found in the contrib/init folder.

    contrib/init/vitalcoind.service:    systemd service unit configuration
    contrib/init/vitalcoind.openrc:     OpenRC compatible SysV style init script
    contrib/init/vitalcoind.openrcconf: OpenRC conf.d file
    contrib/init/vitalcoind.conf:       Upstart service configuration file
    contrib/init/vitalcoind.init:       CentOS compatible SysV style init script

Service User
---------------------------------

All three Linux startup configurations assume the existence of a "vitalcoin" user
and group.  They must be created before attempting to use these scripts.
The macOS configuration assumes vitalcoind will be set up for the current user.

Configuration
---------------------------------

At a bare minimum, vitalcoind requires that the rpcpassword setting be set
when running as a daemon.  If the configuration file does not exist or this
setting is not set, vitalcoind will shutdown promptly after startup.

This password does not have to be remembered or typed as it is mostly used
as a fixed token that vitalcoind and client programs read from the configuration
file, however it is recommended that a strong and secure password be used
as this password is security critical to securing the wallet should the
wallet be enabled.

If vitalcoind is run with the "-server" flag (set by default), and no rpcpassword is set,
it will use a special cookie file for authentication. The cookie is generated with random
content when the daemon starts, and deleted when it exits. Read access to this file
controls who can access it through RPC.

By default the cookie is stored in the data directory, but it's location can be overridden
with the option '-rpccookiefile'.

This allows for running vitalcoind without having to do any manual configuration.

`conf`, `pid`, and `wallet` accept relative paths which are interpreted as
relative to the data directory. `wallet` *only* supports relative paths.

For an example configuration file that describes the configuration settings,
see `share/examples/vitalcoin.conf`.

Paths
---------------------------------

### Linux

All three configurations assume several paths that might need to be adjusted.

Binary:              `/usr/bin/vitalcoind`  
Configuration file:  `/etc/vitalcoin/vitalcoin.conf`  
Data directory:      `/var/lib/vitalcoind`  
PID file:            `/var/run/vitalcoind/vitalcoind.pid` (OpenRC and Upstart) or `/var/lib/vitalcoind/vitalcoind.pid` (systemd)  
Lock file:           `/var/lock/subsys/vitalcoind` (CentOS)  

The configuration file, PID directory (if applicable) and data directory
should all be owned by the vitalcoin user and group.  It is advised for security
reasons to make the configuration file and data directory only readable by the
vitalcoin user and group.  Access to vitalcoin-cli and other vitalcoind rpc clients
can then be controlled by group membership.

### macOS

Binary:              `/usr/local/bin/vitalcoind`  
Configuration file:  `~/Library/Application Support/Vitalcoin/vitalcoin.conf`  
Data directory:      `~/Library/Application Support/Vitalcoin`  
Lock file:           `~/Library/Application Support/Vitalcoin/.lock`  

Installing Service Configuration
-----------------------------------

### systemd

Installing this .service file consists of just copying it to
/usr/lib/systemd/system directory, followed by the command
`systemctl daemon-reload` in order to update running systemd configuration.

To test, run `systemctl start vitalcoind` and to enable for system startup run
`systemctl enable vitalcoind`

NOTE: When installing for systemd in Debian/Ubuntu the .service file needs to be copied to the /lib/systemd/system directory instead.

### OpenRC

Rename vitalcoind.openrc to vitalcoind and drop it in /etc/init.d.  Double
check ownership and permissions and make it executable.  Test it with
`/etc/init.d/vitalcoind start` and configure it to run on startup with
`rc-update add vitalcoind`

### Upstart (for Debian/Ubuntu based distributions)

Upstart is the default init system for Debian/Ubuntu versions older than 15.04. If you are using version 15.04 or newer and haven't manually configured upstart you should follow the systemd instructions instead.

Drop vitalcoind.conf in /etc/init.  Test by running `service vitalcoind start`
it will automatically start on reboot.

NOTE: This script is incompatible with CentOS 5 and Amazon Linux 2014 as they
use old versions of Upstart and do not supply the start-stop-daemon utility.

### CentOS

Copy vitalcoind.init to /etc/init.d/vitalcoind. Test by running `service vitalcoind start`.

Using this script, you can adjust the path and flags to the vitalcoind program by
setting the VITALCOIND and FLAGS environment variables in the file
/etc/sysconfig/vitalcoind. You can also use the DAEMONOPTS environment variable here.

### macOS

Copy org.vitalcoin.vitalcoind.plist into ~/Library/LaunchAgents. Load the launch agent by
running `launchctl load ~/Library/LaunchAgents/org.vitalcoin.vitalcoind.plist`.

This Launch Agent will cause vitalcoind to start whenever the user logs in.

NOTE: This approach is intended for those wanting to run vitalcoind as the current user.
You will need to modify org.vitalcoin.vitalcoind.plist if you intend to use it as a
Launch Daemon with a dedicated vitalcoin user.

Auto-respawn
-----------------------------------

Auto respawning is currently only configured for Upstart and systemd.
Reasonable defaults have been chosen but YMMV.
