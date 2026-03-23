# Decisiones tecnicas iniciales

## Alcance de esta etapa

Estas decisiones fijan la base de preparacion del repositorio. No reemplazan decisiones futuras de integracion profunda.

## Decisiones confirmadas

### DT-01. Plataforma objetivo inicial

La plataforma objetivo inicial es Linux de escritorio.

### DT-02. Lenguaje base

El codigo principal del cliente se implementara en C++20.

### DT-03. Framework de interfaz

La interfaz de escritorio se construira sobre Qt 6.

### DT-04. Sistema de build

El proyecto utilizara CMake como sistema de build principal.

### DT-05. Organizacion modular

El codigo se organizara por dominios tecnicos:

- aplicacion
- interfaz
- sesiones
- telegram
- llamadas
- dispositivos
- analitica

### DT-06. Integracion progresiva

La base del repositorio se preparara primero con esqueletos compilables antes de integrar TDLib, tgcalls y GStreamer.

## Decisiones pendientes

- estrategia exacta de adquisicion de TDLib
- estrategia exacta de adquisicion de tgcalls y lib_webrtc
- modelo de empaquetado para Linux
- estrategia de sandboxing
- backend multimedia principal para captura y composicion

