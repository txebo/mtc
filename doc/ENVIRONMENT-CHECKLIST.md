# Checklist de entorno

## Objetivo

Verificar que el entorno Linux quede listo para iniciar la Etapa 1 del proyecto.

## Toolchain

- [ ] `cmake` instalado en version 3.24 o superior
- [ ] compilador C++20 disponible
- [ ] `pkg-config` disponible
- [ ] `ninja` o `make` disponible

## Qt

- [ ] Qt 6 instalado
- [ ] modulo `Qt6::Widgets` disponible
- [ ] herramientas de desarrollo de Qt disponibles

## Dependencias candidatas siguientes

- [ ] TDLib
- [ ] tgcalls
- [ ] lib_webrtc
- [ ] SQLite
- [ ] GStreamer

## Sistema Linux

- [ ] acceso a camara y microfono validado
- [ ] revision del backend multimedia del sistema
- [ ] confirmar entorno objetivo: X11, Wayland o ambos
- [ ] confirmar estrategia de sandboxing: nativo, Flatpak u otra

## Salida esperada

La etapa se considera lista cuando el equipo puede:

- configurar el proyecto con CMake
- compilar la app base
- abrir una ventana Qt
- registrar decisiones de stack pendientes

