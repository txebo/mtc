#!/usr/bin/env bash
set -euo pipefail

KVER="${KVER:-$(uname -r)}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODULE_SRC="$SCRIPT_DIR/hci_uart.ko"
MODULE_DST="/lib/modules/$KVER/updates/hci_uart.ko"
FW_DIR="/usr/lib/firmware/brcm"
FW_TARGET="$FW_DIR/BCM-0a5c-6410.hcd.zst"
FW_ALIAS="$FW_DIR/BCM.hcd.zst"

if [[ $EUID -ne 0 ]]; then
  echo "Run this script with sudo or as root."
  exit 1
fi

if [[ ! -f "$MODULE_SRC" ]]; then
  echo "Built module not found: $MODULE_SRC"
  exit 1
fi

if [[ -f "$FW_TARGET" ]]; then
  ln -sfn "$(basename "$FW_TARGET")" "$FW_ALIAS"
fi

install -D -m 0644 "$MODULE_SRC" "$MODULE_DST"
depmod -a "$KVER"

systemctl stop bluetooth || true

if lsmod | grep -q '^hci_uart'; then
  modprobe -r hci_uart || true
fi

modprobe hci_uart
systemctl start bluetooth || true

echo
echo "Module installed to: $MODULE_DST"
echo "Firmware alias: $FW_ALIAS"
echo
echo "Verification:"
systemctl is-active bluetooth || true
bluetoothctl show || true
hciconfig -a || true
dmesg | tail -n 80 | grep -Ei 'bluetooth|hci|bcm|firmware' || true
