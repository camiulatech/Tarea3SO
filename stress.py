#!/usr/bin/env python3

import argparse
import subprocess
import threading

def ejecutar_cliente(comando, resultados, idx):
    try:
        result = subprocess.run(' '.join(comando), shell=True, capture_output=True, text=True)
        salida = result.stdout + result.stderr
        print(f"Hilo {idx}: {salida}")
        if "503" in salida:
            resultados[idx] = "503"
        elif "200" in salida or "201" in salida:
            resultados[idx] = "OK"
        else:
            resultados[idx] = "ERROR"

    except Exception as e:
        resultados[idx] = "ERROR"

def main():
    parser = argparse.ArgumentParser(description="Stress test para HTTPClient")
    parser.add_argument("-n", type=int, required=True, help="Cantidad de hilos")
    parser.add_argument("comando", nargs=argparse.REMAINDER, help="Comando del cliente HTTP")

    args = parser.parse_args()

    if not args.comando:
        print("Debes especificar un comando para ejecutar con el cliente HTTP.")
        return

    print(f"Lanzando {args.n} hilos para: {' '.join(args.comando)}\n")

    hilos = []
    resultados = [None] * args.n

    for i in range(args.n):
        hilo = threading.Thread(target=ejecutar_cliente, args=(args.comando, resultados, i))
        hilo.start()
        hilos.append(hilo)

    for hilo in hilos:
        hilo.join()

    ok_count = resultados.count("OK")
    rej_count = resultados.count("503")
    err_count = resultados.count("ERROR")

    print("\nFinalizó el stress test.")
    print(f"Atendidos correctamente: {ok_count}")
    print(f"Rechazados (503): {rej_count}")
    print(f"Otros errores: {err_count}")

if __name__ == "__main__":
    main()
