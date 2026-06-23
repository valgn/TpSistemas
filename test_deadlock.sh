echo ">>> Limpiando entorno de ejecuciones previas"

# Elimina cualquier proceso de agente que pueda estar corriendo y borra los logs anteriores
killall agent 2>/dev/null
rm -f log_node*.txt

# Ejecuta el agente C pasándole 6 args (IP local, puerto Erlang, puerto TCP Nodos, CPU, RAM, GPU). 
# > log_nodeA.txt 2>&1 redirige tanto la salida estándar como los errores a ese archivo txt. 
# El & al final envía el proceso a ejecutarse en segundo plano.
# PID_A=$! guarda el PID del último proceso ejecutado en segundo plano en la variable PID_A.
echo ">>> Levantando Nodo A (2 CPU, 8 GB RAM, 0 GPU)"
./C/agent 127.0.0.1 5001 6001 2 8192 0 > log_nodeA.txt 2>&1 &
PID_A=$!

echo ">>> Levantando Nodo B (2 CPU, 4 GB RAM, 1 GPU)"
./C/agent 127.0.0.1 5002 6002 2 4096 1 > log_nodeB.txt 2>&1 &
PID_B=$!

echo ">>> Esperando para el descubrimiento UDP entre nodos"
sleep 10

echo ">>> Simulando peticiones"

# Levanta una máquina virtual de Erlang sin consola interactiva (-noshell), incluyendo los archivos en 
# /Erlang (-pa Erlang). Ejecuta la función dada con los argumentos y luego apaga la máquina virtual (-s init stop).
# El & lo manda a segundo plano para que la siguiente línea se ejecute de forma casi simultánea.
erl -pa Erlang -noshell -eval 'jobs:force_request(5001, "1001", "JOB_REQUEST 1001 @127.0.0.1:cpu:2 @127.0.0.2:gpu:1\n").' -s init stop &

erl -pa Erlang -noshell -eval 'jobs:force_request(5002, "1002", "JOB_REQUEST 1002 @127.0.0.2:gpu:1 @127.0.0.1:cpu:2\n").' -s init stop &

echo ">>> Observar los logs para verificar el escenario de deadlock"

# Mantenemos el script vivo 
wait $PID_A $PID_B

# Matar los procesos de los agentes al finalizar la prueba con $ killall agent