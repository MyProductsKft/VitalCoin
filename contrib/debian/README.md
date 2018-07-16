
Debian
====================
This directory contains files used to package vitalcoind/vitalcoin-qt
for Debian-based Linux systems. If you compile vitalcoind/vitalcoin-qt yourself, there are some useful files here.

## vitalcoin: URI support ##


vitalcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install vitalcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your vitalcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/vitalcoin128.png` to `/usr/share/pixmaps`

vitalcoin-qt.protocol (KDE)

