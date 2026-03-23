# MTC

Cliente Telegram de escritorio para Linux con soporte multi-cuenta, base analitica local y subsistema de control multimedia.

## Estado

El repositorio se encuentra en preparacion de base arquitectonica.

En esta etapa el proyecto incluye:

- documento rector de arquitectura
- investigacion tecnica de referencia
- estructura inicial del repositorio
- build base con Qt
- esqueletos de modulos principales

## Objetivo V1

La V1 buscara demostrar:

- login funcional
- mensajeria con texto, imagen, PDF y video
- videollamada 1 a 1
- group call
- llamada entrante durante llamada activa
- cambio de cuenta
- control basico de dispositivos
- base analitica separada
- auditoria basica

## Estructura del repositorio

```text
.
|- doc/
|- src/
|  |- app/
|  |- core/
|  |  |- analytics/
|  |  |- calls/
|  |  |- devices/
|  |  `- session/
|  `- ui/
`- tests/
```

## Requisitos iniciales

- Linux de escritorio
- CMake 3.24 o superior
- compilador con soporte C++20
- Qt 6 con modulo Widgets

## Build local

```bash
cmake -S . -B build
cmake --build build
```

## Proximos pasos

1. cerrar la preparacion del entorno Linux
2. confirmar estrategia de dependencias para TDLib y tgcalls
3. implementar la PoC del Core Telegram
4. validar la PoC de media como riesgo principal

