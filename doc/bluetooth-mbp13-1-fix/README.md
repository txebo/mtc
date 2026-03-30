# MacBookPro13,1 Bluetooth Fix

Bundle de trabajo para recuperar Bluetooth en un `MacBookPro13,1` con:

- Linux Mint `22.3`
- kernel `6.8.0-106-generic`
- chip Broadcom `BCM4350`

## Contexto

En este equipo el Bluetooth quedaba detectado pero no inicializaba bien con el
`hci_uart` stock. Los síntomas observados fueron:

- `hci0` presente pero `DOWN`
- errores ACPI/GPIO del `hci_bcm`
- timeouts al cargar firmware Broadcom

Se validó una variante de `hci_uart` compilada desde el trabajo de:

- `https://github.com/leifliddy/macbook12-bluetooth-driver`

La compilación se ajustó para Ubuntu/Mint con kernel `6.8.0-106-generic`,
limitando el módulo al camino que este equipo usa realmente:

- `H4`
- `serdev`
- `Broadcom BCM`

## Archivos

- `hci_uart.ko`: módulo compilado y validado localmente
- `install-built-hci_uart.sh`: instala el módulo, recrea el alias de firmware y
  recarga `bluetooth`

## Instalación

```bash
sudo ./install-built-hci_uart.sh
```

O desde la raíz del repo:

```bash
sudo /home/tx/dev/MTC/doc/bluetooth-mbp13-1-fix/install-built-hci_uart.sh
```

## Verificación esperada

```bash
bluetoothctl show
hciconfig -a
```

Salida esperada:

- `Powered: yes`
- `UP RUNNING`

## Notas

- Este bundle quedó validado en `MacBookPro13,1` con kernel
  `6.8.0-106-generic`.
- Si cambias de kernel, probablemente habrá que recompilar `hci_uart.ko`.
- SHA-256 del módulo validado:
  `284c6f057d22e94d54f4d4aab3bfa94eeacdd93badf35a62ebb31fc6cfaa737e`

## Referencias

- `https://github.com/Dunedan/mbp-2016-linux/issues/29`
- `https://github.com/leifliddy/macbook12-bluetooth-driver`
