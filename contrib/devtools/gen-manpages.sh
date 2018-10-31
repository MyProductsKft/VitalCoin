#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

VITALCOIND=${VITALCOIND:-$SRCDIR/vitalcoind}
VITALCOINCLI=${VITALCOINCLI:-$SRCDIR/vitalcoin-cli}
VITALCOINTX=${VITALCOINTX:-$SRCDIR/vitalcoin-tx}
VITALCOINQT=${VITALCOINQT:-$SRCDIR/qt/vitalcoin-qt}

[ ! -x $VITALCOIND ] && echo "$VITALCOIND not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
VTCVER=($($VITALCOINCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for vitalcoind if --version-string is not set,
# but has different outcomes for vitalcoin-qt and vitalcoin-cli.
echo "[COPYRIGHT]" > footer.h2m
$VITALCOIND --version | sed -n '1!p' >> footer.h2m

for cmd in $VITALCOIND $VITALCOINCLI $VITALCOINTX $VITALCOINQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${VTCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${VTCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
