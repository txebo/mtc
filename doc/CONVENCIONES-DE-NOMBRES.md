# Convenciones de nombres

## Reglas generales

- nombres de clases en `PascalCase`
- nombres de funciones y metodos en `camelCase`
- nombres de variables locales en `camelCase`
- miembros privados con sufijo `_`
- constantes de compilacion en `kPascalCase`
- namespaces en minusculas

## Vocabulario canonico

- `TelegramAccount`: cuenta autenticada de Telegram
- `Session`: contexto operativo de una cuenta
- `ActiveSession`: sesion actualmente enfocada en UI
- `SessionStore`: persistencia de sesion
- `TDLibAdapter`: capa de integracion con TDLib
- `CallController`: logica de llamadas
- `DeviceManager`: control centralizado de dispositivos
- `AnalyticsStore`: persistencia analitica local

## Carpetas

- `src/app`: arranque y composicion de aplicacion
- `src/ui`: ventanas, widgets y elementos visuales
- `src/core/session`: gestion de sesiones y cuentas
- `src/core/telegram`: integracion Telegram
- `src/core/calls`: control de llamadas
- `src/core/devices`: dispositivos multimedia
- `src/core/analytics`: almacenamiento y eventos analiticos

## Archivos

- un tipo principal por archivo cuando sea razonable
- nombre de archivo igual al nombre de la clase principal
- headers en `.h`
- implementaciones en `.cpp`

