# Integracion inicial de TDLib

## Estado actual

La base del proyecto ya expone un `TDLibAdapter` con:

- estados de autorizacion
- bootstrap de credenciales
- diagnosticos para UI
- build opcional con `ENABLE_TDLIB`

## Objetivo del siguiente paso

Conectar la libreria real para:

1. inicializar cliente TDLib
2. enviar `setTdlibParameters`
3. manejar `authorizationStateWaitPhoneNumber`
4. manejar `authorizationStateWaitCode`
5. abrir la carga inicial de chats

## Build con TDLib

Si TDLib esta instalada en el sistema:

```bash
cmake -S . -B build -DENABLE_TDLIB=ON
cmake --build build
```

## Descubrimiento por CMake

La configuracion actual intenta localizar:

- header `td/telegram/td_json_client.h`
- biblioteca `tdjson`

Si no se encuentran, la app sigue compilando en modo preparatorio.
