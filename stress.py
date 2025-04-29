#!/usr/bin/env python3

import argparse
import subprocess
import threading

# Funcion encargada de ejecutar el cliente HTTP en un hilo
def run_client(command, results, idx):
    try:
        # Ejecuta el comando HTTP client y captura la salida
        result = subprocess.run(' '.join(command), shell=True, capture_output=True, text=True)
        output = result.stdout + result.stderr
        print(f"Thread {idx}: {output}")

        # Clasifica la salida
        if "503" in output:
            results[idx] = "503"
        elif "200" in output or "201" in output:
            results[idx] = "OK"
        else:
            results[idx] = "ERROR"

    # Excepción en caso de error al ejecutar el comando
    except Exception as e:
        results[idx] = "ERROR"

def main():
    # Configuración del parser de argumentos
    parser = argparse.ArgumentParser(description="Stress test fot HTTPClient")
    parser.add_argument("-n", type=int, required=True, help="Number of threads")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="HTTP client command to execute")

    args = parser.parse_args()

    # Validación de argumentos
    if not args.command:
        print("You must specify a command to run with the HTTP client.")
        return

    print(f"Launching {args.n} threads for: {' '.join(args.command)}\n")

    threads = []
    results = [None] * args.n

    # Crea y lanza los hilos
    for i in range(args.n):
        thread = threading.Thread(target=run_client, args=(args.command, results, i))
        thread.start()
        threads.append(thread)

    # Espera a que todos los hilos terminen
    for thread in threads:
        thread.join()

    # Clasifica y cuenta los resultados
    ok_count = results.count("OK")
    rej_count = results.count("503")
    err_count = results.count("ERROR")

    # Imprime los resultados
    print("\nStress test completed.")
    print(f"Successfully served: {ok_count}")
    print(f"Rejected (503): {rej_count}")
    print(f"Killed: {err_count}")

if __name__ == "__main__":
    main()
