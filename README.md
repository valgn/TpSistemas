
# Trabajo Práctico Sistemas Operativos I

Este repositorio contiene la implementación de un sistema distribuido de asignación de recursos y prevención de deadlocks. El sistema está compuesto por una red de agentes desarrollados en **C** y un planificador de trabajos implementado en **Erlang**.



## Estructura del Proyecto

* `/C`: Contiene el código fuente del agente C.
* `/C/structures`: Implementaciones de estructuras de datos (Listas enlazadas, Tablas Hash, Colas) y funciones auxiliares.
* `/Erlang`: Contiene los módulos del cliente planificador que simula los trabajos y maneja la resolución de deadlocks.



## Compilación y Ejecución


#### Compilar todo el proyecto:

Esto generará el ejecutable `agent` dentro de la carpeta `/C` y los `.beam` en `/Erlang`.

```bash
make
```


#### Limpiar los archivos compilados:

```bash
make clean
```


#### Levantar un nodo en C y el cliente Erlang:

Una vez compilado el proyecto, abrir dos terminales en la raíz y ejecutar:

**Terminal 1: Levantar el agente C**
El agente en C requiere 6 parámetros obligatorios:
`./C/agent <IP> <Erlang_Port> <Node_Port> <CPU> <MEM> <GPU>` donde la ip corresponde a la respectiva maquina. El enunciado propone 5678 para el Erlang_port y 5679 el Node_port.

```bash
./C/agent <IP> 5678 5679 2 8192 0
```

**Terminal 2: Iniciar el cliente Erlang**
Una vez que el agente esté corriendo, abrir una tercera terminal, ingresar a la carpeta de Erlang e iniciar la consola interactiva. Para comenzar a simular los trabajos:

```erlang
1> client:init().
```


#### Test de deadlock:

`test_deadlock.sh` levanta dos nodos en C en segundo plano y fuerza desde Erlang el escenario de deadlock planteado en el enunciado. Antes de ejecutar el script, asegúrarse de compilar el proyecto con `make` para generar el ejecutable necesario.

```bash
./test_deadlock.sh
```

Luego de ejecutar el script, recordar matar los procesos de C levantados en segundo plano con `killall agent`.