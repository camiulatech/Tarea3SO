#!/usr/bin/env python3

import argparse
import subprocess
import threading

def ejecutar_cliente(comando):
    try:
        print(f"[DEBUG] Ejecutando: {' '.join(comando)}")
        result = subprocess.run(' '.join(comando), shell=True)
        print(f"[DEBUG] Proceso finalizó con código: {result.returncode}")
    except Exception as e:
        print(f"[ERROR] al ejecutar: {e}")

def main():
    parser = argparse.ArgumentParser(description="Stress test para HTTPClient")
    parser.add_argument("-n", type=int, required=True, help="Cantidad de hilos")
    parser.add_argument("comando", nargs=argparse.REMAINDER, help="Comando del cliente HTTP")

    args = parser.parse_args()

    if not args.comando:
        print("Debes especificar un comando para ejecutar con el cliente HTTP.")
        return

    print(f"🔥 Lanzando {args.n} hilos para: {' '.join(args.comando)}\n")

    hilos = []
    for _ in range(args.n):
        hilo = threading.Thread(target=ejecutar_cliente, args=(args.comando,))
        hilo.start()
        hilos.append(hilo)

    for hilo in hilos:
        hilo.join()

    print("\n✅ Finalizó el stress test.")

if __name__ == "__main__":
    main()
