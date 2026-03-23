# Documento rector de arquitectura

## 1. Proposito

Definir los requerimientos arquitectonicos, restricciones y lineamientos de implementacion para una solucion compuesta por:

- un cliente Telegram de escritorio basado en `Qt + TDLib + libtgvoip/WebRTC`
- un subsistema de gestion, aislamiento y virtualizacion de dispositivos multimedia
- una base analitica local para observacion, priorizacion y correlacion de interacciones

## 2. Alcance

La solucion debera permitir:

- operar multiples cuentas de Telegram desde una misma aplicacion
- priorizar mensajes segun relevancia para el usuario
- construir una memoria local analitica de usuarios e interacciones
- controlar camaras y microfonos con aislamiento, seguridad y virtualizacion
- mantener compatibilidad con las funciones de llamadas y videollamadas

Queda fuera del alcance:

- acceder a informacion fuera del subgrafo visible por las cuentas autenticadas
- modificar los protocolos internos de Telegram
- reemplazar la infraestructura de Telegram para mensajeria o group calls

## 3. Principios de diseno

- separacion estricta entre cliente Telegram, analitica, persistencia y gestion de dispositivos
- minimo privilegio para cada componente
- persistencia local y controlada por el usuario
- trazabilidad de decisiones de sesion, acceso y seguridad
- compatibilidad operativa con el flujo multimedia requerido por Telegram
- evolucion incremental, con validacion por etapas

## 4. Vocabulario operativo

Para evitar ambiguedades, este documento utiliza los siguientes terminos:

- `Cuenta`: identidad autenticada de Telegram.
- `Sesion`: contexto operativo asociado a una cuenta autenticada y a sus recursos activos.
- `Sesion activa`: sesion que tiene actualmente el foco principal en la interfaz.
- `Sesion persistida`: estado almacenado que puede restaurarse tras reinicio.
- `Historial unificado`: vista analitica consolidada proveniente de multiples cuentas del usuario.
- `Implementacion`: unidad operativa local asociada a una cuenta o sesion, incluyendo persistencia y configuracion.

## 5. Requerimientos funcionales

### 5.1 Cliente Telegram

#### RF-01. Cliente de escritorio

El sistema debera implementar un cliente Telegram de escritorio con interfaz grafica basada en Qt.

#### RF-02. Integracion con TDLib

El cliente debera utilizar TDLib como motor principal para autenticacion, sincronizacion, almacenamiento local del estado y acceso al modelo de datos de Telegram.

#### RF-03. Integracion de llamadas

El cliente debera integrar una capa de llamadas compatible con libtgvoip/WebRTC para soportar audio y video en escenarios compatibles con Telegram.

### 5.2 Gestion de cuentas y sesiones

#### RF-04. Gestion multi-cuenta

El sistema debera permitir autenticarse con multiples cuentas de Telegram y operar sesiones independientes dentro de una misma interfaz.

#### RF-05. Persistencia de sesiones

El sistema debera mantener persistencia independiente por cuenta o sesion, permitiendo restaurar el estado previamente autenticado cuando corresponda.

#### RF-06. Conmutacion de cuentas

El usuario debera poder cambiar entre cuentas o sesiones de forma controlada, sin perder el aislamiento logico entre ellas.

### 5.3 Priorizacion de mensajes

#### RF-07. Priorizacion de mensajes

El cliente debera incluir un mecanismo de priorizacion de mensajes basado en relevancia para el usuario.

#### RF-08. Criterios de priorizacion

El mecanismo de priorizacion debera considerar al menos:

- mensajes dirigidos exclusivamente al usuario
- mensajes en los que el usuario haya sido mencionado o etiquetado
- mensajes que sean respuesta a mensajes del usuario
- mensajes cuyo contexto conversacional implique al usuario, aun sin mencion explicita

### 5.4 Analitica local

#### RF-09. Directorio local de usuarios observados

El sistema debera mantener un directorio local de usuarios observados desde las cuentas autenticadas.

#### RF-10. Registro de interacciones

El sistema debera registrar eventos e interacciones observables, incluyendo al menos mensajes, respuestas, menciones y participacion en chats compartidos.

#### RF-11. Registro de patrones de comportamiento

El sistema debera mantener informacion agregada sobre patrones de comportamiento e interaccion de usuarios observados.

#### RF-12. Grafo de relaciones

El sistema debera construir y mantener un grafo local de relaciones e interacciones entre usuarios observados dentro del contexto accesible.

#### RF-13. Senales de reputacion

El sistema debera detectar y registrar senales de reputacion observables en usuarios, incluyendo banderas o atributos disponibles en el modelo de Telegram.

#### RF-14. Analisis de coincidencia

El sistema debera incorporar mecanismos de analisis de coincidencia o similitud entre usuarios observados, basados en evidencia local e historica.

#### RF-15. Historial multi-cuenta

El sistema debera permitir preservar y consultar historial propio proveniente de multiples cuentas del usuario, manteniendolo unificado en una base analitica externa.

### 5.5 Dispositivos multimedia

#### RF-16. Deteccion de dispositivos

El subsistema multimedia debera detectar automaticamente camaras y microfonos fisicos disponibles en el sistema.

#### RF-17. Seleccion de dispositivos

El sistema debera permitir seleccionar dispositivos de entrada multimedia por cuenta, sesion o contexto de uso.

#### RF-18. Habilitacion y deshabilitacion de dispositivos

El sistema debera permitir habilitar o deshabilitar temporalmente camaras y microfonos fisicos o virtuales.

#### RF-19. Virtualizacion de dispositivos

El subsistema multimedia debera ser capaz de generar dispositivos virtuales de audio y video utilizables por otras aplicaciones autorizadas.

#### RF-20. Exposicion controlada a otras aplicaciones

El sistema debera poder exponer dispositivos virtuales a otras aplicaciones de manera controlada y auditable.

#### RF-21. Aislamiento entre cuentas y sesiones

El sistema debera impedir que una cuenta o sesion manipule dispositivos asignados a otra cuenta o sesion sin autorizacion explicita.

#### RF-22. Control de acceso a dispositivos

El subsistema debera aplicar politicas de autorizacion para determinar que cuenta, sesion o aplicacion puede acceder a cada dispositivo.

#### RF-23. Monitoreo de uso de dispositivos

El sistema debera registrar el estado de uso de cada dispositivo, incluyendo asignacion, activacion, desactivacion y reasignacion.

#### RF-24. Auditoria

El sistema debera mantener un registro auditable de eventos de seguridad y control relacionados con dispositivos, sesiones y accesos.

#### RF-25. Compatibilidad operativa

La solucion debera conservar compatibilidad con el funcionamiento esperado de llamadas y videollamadas basadas en libtgvoip/WebRTC.

## 6. Requerimientos no funcionales

#### RNF-01. Aislamiento del sistema

La aplicacion debera ejecutarse en un entorno aislado, logico o fisico, que limite su acceso al resto del sistema operativo sin comprometer las funciones explicitamente permitidas.

#### RNF-02. Seguridad de dispositivos

El acceso a camaras y microfonos debera estar protegido por una capa de control central que evite manipulaciones no autorizadas.

#### RNF-03. Principio de minimo privilegio

Cada componente debera operar con el minimo conjunto de permisos necesarios para cumplir su funcion.

#### RNF-04. Separacion de responsabilidades

La arquitectura debera separar claramente:

- cliente Telegram
- logica analitica
- persistencia
- gestion de sesiones
- gestion y virtualizacion de dispositivos

#### RNF-05. Trazabilidad

Las decisiones relevantes de sesion, priorizacion, acceso a dispositivos, coincidencia de usuarios y eventos de seguridad deberan poder rastrearse.

#### RNF-06. Persistencia robusta

La base analitica local y la persistencia de sesiones deberan disenarse para tolerar reinicios, cierres inesperados y restauracion controlada.

#### RNF-07. Escalabilidad local

La arquitectura debera soportar crecimiento progresivo en numero de cuentas, usuarios observados, interacciones y relaciones sin degradacion severa de operacion.

#### RNF-08. Consistencia de datos

Los componentes de persistencia y analisis deberan mantener consistencia entre eventos sincronizados, entidades observadas y modelos derivados.

#### RNF-09. Desempeno interactivo

La interfaz debera responder de forma fluida a navegacion, cambio de cuenta, apertura de chats y consulta de datos analiticos.

#### RNF-10. Compatibilidad multimedia

La capa de gestion de dispositivos debera mantener compatibilidad con el flujo de audio y video requerido por Telegram y otras aplicaciones autorizadas.

#### RNF-11. No interferencia con media

Los mecanismos de aislamiento, virtualizacion y control de dispositivos no deberan romper la captura, transmision ni reproduccion de audio y video.

#### RNF-12. Auditabilidad

Los eventos de seguridad y control deberan quedar registrados de forma suficiente para diagnostico, revision y analisis posterior.

#### RNF-13. Extensibilidad

La solucion debera permitir agregar nuevos criterios de priorizacion, nuevas metricas analiticas y nuevas politicas de control de dispositivos sin rediseno total.

#### RNF-14. Mantenibilidad

Los modulos deberan definirse con interfaces claras, responsabilidades acotadas y bajo acoplamiento.

#### RNF-15. Portabilidad controlada

La solucion debera disenarse con preferencia por compatibilidad en Linux de escritorio, dejando abierta la posibilidad de portabilidad futura a otras plataformas si la capa de integracion lo permite.

#### RNF-16. Observabilidad interna

La arquitectura debera proveer mecanismos de logging, metricas y diagnostico suficientes para depurar sesiones, sincronizacion, priorizacion y uso de dispositivos.

#### RNF-17. Fail-safe en dispositivos

Ante error, conflicto o perdida de control, el sistema debera favorecer estados seguros, especialmente respecto a camara y microfono.

#### RNF-18. Privacidad local

La base analitica debera tratar el historial y las huellas observadas como informacion sensible del usuario, manteniendola bajo control local y evitando exposicion innecesaria.

## 7. Decisiones arquitectonicas iniciales

#### DA-01. Cliente principal

Se utilizara Qt como framework de GUI para el cliente de escritorio.

#### DA-02. Motor de cliente

Se utilizara TDLib como motor principal de integracion con Telegram.

#### DA-03. Capa de llamadas

La funcionalidad de llamadas se apoyara en libtgvoip/WebRTC.

#### DA-04. Persistencia analitica separada

La base analitica local sera independiente de la persistencia interna de TDLib.

#### DA-05. Subsistema de dispositivos separado

La gestion y virtualizacion de dispositivos multimedia se modelara como un subsistema separado, aunque integrado funcionalmente con el cliente principal.

#### DA-06. Aislamiento por sesion

El uso de recursos multimedia debera poder asignarse y auditarse por sesion o cuenta.

## 8. Descomposicion de modulos propuesta

```text
ClientePrincipal
|- GUI Qt
|- Session Manager
|- TDLib Adapter
|- Message Prioritizer
|- Local Analytics Engine
`- Media Session Controller

SubsistemaDeDispositivos
|- Device Discovery
|- Device Policy Engine
|- Virtual Device Layer
|- Access Control
`- Audit Log
```

## 9. Riesgos arquitectonicos

- complejidad de integrar multi-cuenta con llamadas
- complejidad de virtualizacion estable de audio y video
- conflictos entre aislamiento y compatibilidad multimedia
- crecimiento del grafo analitico y costo de correlacion
- sobrecarga semantica entre sesion activa, sesion persistida e historial de sesion

## 10. Criterios de validacion

La arquitectura se considerara alineada si permite demostrar:

- autenticacion y operacion de multiples cuentas
- priorizacion funcional de mensajes segun reglas definidas
- almacenamiento local de usuarios, interacciones y relaciones
- consulta de senales de reputacion y coincidencia
- control seguro de camara y microfono
- aislamiento entre sesiones en el uso de dispositivos
- compatibilidad con llamadas y videollamadas

## 11. Roadmap de implementacion

### 11.1 Criterio de ejecucion

Cada etapa debe producir un entregable verificable y cumplir su criterio de salida antes de continuar.

Principio operativo:

```text
una etapa = un entregable = una validacion
```

### 11.2 Etapas

| Etapa | Objetivo | Entregables principales | Dependencias | Prioridad | Riesgo |
| --- | --- | --- | --- | --- | --- |
| 0. Preparacion | Dejar listo el entorno, el repositorio y los contratos base | repo base, documento rector integrado, checklist de entorno, convencion de nombres | ninguna | alta | bajo |
| 1. Core Telegram | Arranque, autenticacion y navegacion basica | cliente funcional con login, listado de chats y apertura de conversacion | 0 | alta | medio |
| 2. PoC de mensajeria | Validar recepcion y render de contenido V1 | PoC de texto, imagen, PDF y video | 1 | alta | bajo |
| 3. PoC de media | Validar el stack real de llamadas | PoC de media, matriz comparativa, decision tecnica del stack | 1 | critica | alto |
| 4. CallController base | Modelar y reflejar estados de llamada | `CallController`, `CallRegistry`, `CallStateMachine`, `CallPanel` | 3 | alta | medio |
| 5. CallsAdapter | Integrar la logica de llamadas con el stack validado | `CallsAdapter` funcional conectado a `CallController` | 3, 4 | alta | alto |
| 6. Videollamada 1:1 | Validar el flujo completo de llamada individual | PoC de videollamada 1:1 | 5 | critica | alto |
| 7. Group calls | Validar soporte real de llamadas grupales | PoC de group call y limites documentados | 5 | critica | alto |
| 8. Concurrencia de llamadas | Resolver llamada activa mas llamada entrante | flujo concurrente estable en UI y logica | 6 | critica | alto |
| 9. Multi-cuenta V1 | Soportar multiples cuentas persistidas con foco unico operativo | panel de cuentas funcional y cambio estable | 1 | media | medio |
| 10. DeviceManager basico | Centralizar el control de camara y microfono | `DeviceManager` y seleccion de dispositivos desde UI | 6 | media | medio |
| 11. Analitica basica V1 | Construir memoria operativa minima | DB analitica separada, usuarios observados e interacciones persistidas | 2, 6 | media | bajo |
| 12. Seguridad basica V1 | Agregar politicas minimas y auditoria | `SecurityEngine`, policy basica y audit log | 10 | media | bajo |

### 11.3 Hitos

#### Hito A. Cliente Telegram funcional

Incluye etapas 1 y 2.

#### Hito B. Media validada

Incluye etapa 3.

#### Hito C. Llamadas reales funcionales

Incluye etapas 4, 5 y 6.

#### Hito D. Group call y concurrencia

Incluye etapas 7 y 8.

#### Hito E. Base operativa completa V1

Incluye etapas 9, 10, 11 y 12.

## 12. Criterio de cierre de V1

La V1 se considera lograda cuando existe evidencia real de:

- login funcional
- mensajeria con texto, imagen, PDF y video
- videollamada 1 a 1
- group call
- llamada entrante durante llamada activa
- cambio de cuenta
- control basico de dispositivos
- base analitica separada
- auditoria basica

## 13. Recomendacion de ejecucion

No implementar todas las areas en paralelo. El camino recomendado es:

```text
Core -> Mensajeria -> Media -> Llamadas -> Concurrencia -> Multi-cuenta -> Dispositivos -> Analitica -> Seguridad
```

Las etapas mas criticas para la viabilidad del proyecto son:

- Etapa 3: PoC de media
- Etapa 6: Videollamada 1 a 1
- Etapa 7: Group calls
- Etapa 8: Concurrencia de llamadas

Si alguna de estas etapas falla, debera revisarse la arquitectura, el stack seleccionado o el alcance de V1 antes de continuar.

## 14. Anexo tecnico de integracion

Este anexo resume decisiones y hallazgos tecnicos extraidos de la investigacion contenida en `doc/deep-research-report(2).md`. Su objetivo es complementar este documento rector sin convertir la investigacion exploratoria en requisito normativo.

### 14.1 Separacion de dominios tecnicos

Para mantener viabilidad y claridad de implementacion, la solucion debe separar tres dominios:

- senalizacion, estado y datos de Telegram
- motor de llamada en tiempo real
- composicion y ruteo multimedia

Asignacion recomendada:

- `TDLib`: autenticacion, estado, chats, updates y metadatos de videochat
- `tgcalls/lib_webrtc`: transporte y sesion de llamada grupal
- `GStreamer` o grafo multimedia equivalente: captura multi-camara, composicion, duplicacion de salidas y previsualizacion
- `Janus` o capa WebRTC equivalente: exposicion del feed local hacia navegador
- `Qt`: interfaz de usuario, layouts, ventanas y preview multi-monitor

### 14.2 Arquitectura de referencia ampliada

La arquitectura de referencia para V1 ampliada es:

```text
UI Qt
|- Login + chats + controles de llamada
|- Gestor de layouts de video
`- Preview local y multi-monitor

Capa Telegram
|- TDLib
`- Join a videochat y estado del group call

Motor de llamada
|- tgcalls GroupInstance
`- lib_webrtc

Grafo multimedia
|- Captura multi-camara
|- Compositor de video
|- Salida hacia tgcalls
|- Salida hacia preview Qt
`- Salida hacia streaming local

Streaming local
`- Janus Streaming o Janus VideoRoom

Control del sistema
`- PipeWire + xdg-desktop-portal
```

### 14.3 Decision tecnica recomendada para group calls

Para llamadas grupales y videochat, la ruta preferente de integracion es:

- `TDLib` para descubrir estado, `group_call_id` y ejecutar el join
- `tgcalls/lib_webrtc` como stack principal de media

Justificacion tecnica:

- se alinea mejor con la integracion usada como referencia por Telegram Desktop
- la mecanica `joinVideoChat` y los parametros de join encajan conceptualmente con tgcalls
- habilita una fuente de video custom, lo que favorece composicion multi-camara en un solo feed saliente

Conclusiones operativas:

- `libtgvoip` no debe asumirse como ruta principal para group calls V1
- la PoC de media debe validar desde temprano la integracion `TDLib <-> tgcalls`

### 14.4 Estrategia para multi-camara y composicion

La estrategia recomendada no es enviar varias pistas independientes al motor de llamada, sino:

- capturar N camaras
- normalizar resolucion y framerate
- componer en un solo canvas
- inyectar un feed compuesto al stack de llamada

Ventajas:

- reduce complejidad de integracion con Telegram
- permite layouts `grid`, `speaker`, `PiP` y reordenamiento dinamico
- simplifica preview local y retransmision a navegador

Implicaciones de implementacion:

- el compositor debe exponer al menos una salida para tgcalls
- conviene duplicar la salida hacia preview local
- conviene dejar una tercera salida para streaming local o grabacion futura

### 14.5 Estrategia para streaming local a navegador

Se recomiendan dos caminos, con distinta prioridad:

#### Opcion A. MVP rapido

- composicion local en `GStreamer`
- salida RTP local
- `Janus Streaming plugin` para convertir a consumo WebRTC en navegador

Esta opcion es la recomendada para una primera version porque desacopla la composicion del signaling WebRTC del navegador.

#### Opcion B. Ruta extensible

- composicion local en `GStreamer`
- publicacion a `Janus VideoRoom` como publisher WebRTC
- navegadores suscritos como viewers

Esta opcion es mejor si el producto necesita multiples viewers, control mas fino del flujo WebRTC o evolucion hacia un modo broadcast mas completo.

### 14.6 Preview local y multi-monitor

La solucion debe contemplar explicitamente:

- preview embebido en UI principal
- ventana secundaria opcional
- asignacion de preview a monitor especifico

Lineamiento tecnico:

- la salida de preview debe desacoplarse del feed enviado a Telegram
- el manejo multi-monitor debe resolverse desde Qt
- los layouts deben poder cambiar sin reiniciar la llamada

### 14.7 Seguridad y sandboxing ampliados

Ademas de los requerimientos base de seguridad, la investigacion sugiere:

- separar en procesos la UI, la capa Telegram y el pipeline de media cuando sea viable
- minimizar privilegios del proceso de media
- preferir `PipeWire + xdg-desktop-portal` para permisos granulares de captura
- aislar cualquier servidor local de streaming a `127.0.0.1`
- evitar logs persistentes con SDP, ICE, tokens o datos sensibles de sesion

### 14.8 Riesgos tecnicos adicionales

Se agregan los siguientes riesgos a los ya definidos:

- complejidad de integrar `TDLib`, `tgcalls` y callbacks de join sin errores de sincronizacion
- conversion de formatos de video con demasiadas copias CPU
- degradacion de latencia por composicion, escalado o colas largas
- complejidad de signaling WebRTC si se evita Janus
- diferencias operativas entre entornos Linux con X11, Wayland, sandbox y permisos de dispositivos

### 14.9 Recomendaciones de PoC previas a V1

Antes de consolidar la arquitectura, se recomienda validar estas PoC tecnicas:

- PoC de join real a group call con `TDLib + tgcalls`
- PoC de composicion de dos camaras en un solo feed
- PoC de preview secundario en otro monitor
- PoC de streaming local a navegador mediante `Janus Streaming`

Estas validaciones pueden ejecutarse antes o durante la Etapa 3, porque reducen riesgo sobre media, group calls y composicion.

## 15. Fuentes y referencias internas

Los lineamientos del anexo tecnico se derivan de la investigacion consolidada en:

- `doc/deep-research-report(2).md`

Ese reporte debe considerarse una referencia tecnica de apoyo, especialmente para:

- detalles de integracion `TDLib`, `tgcalls`, `GStreamer` y `Janus`
- enlaces a documentacion primaria y repositorios upstream
- ejemplos de integracion y criterios comparativos de stack
