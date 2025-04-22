import psutil
import subprocess
import re

def info_cpu():
    print("🔧 CPU Info")
    print(f"Uso de CPU: {psutil.cpu_percent(interval=1)}%")
    print(f"Núcleos físicos: {psutil.cpu_count(logical=False)}")
    print(f"Núcleos lógicos: {psutil.cpu_count(logical=True)}")
    freq = psutil.cpu_freq()
    print(f"Frecuencia: actual {freq.current:.2f} MHz, min {freq.min:.2f}, max {freq.max:.2f}")
    print()

def info_ram():
    print("💾 RAM Info")
    ram = psutil.virtual_memory()
    print(f"Total: {ram.total / (1024**3):.2f} GB")
    print(f"Disponible: {ram.available / (1024**3):.2f} GB")
    print(f"Usada: {ram.used / (1024**3):.2f} GB ({ram.percent}%)")
    print()

def info_disco():
    print("📂 Disco Info")
    disco = psutil.disk_usage('/')
    print(f"Total: {disco.total / (1024**3):.2f} GB")
    print(f"Usado: {disco.used / (1024**3):.2f} GB ({disco.percent}%)")
    print(f"Libre: {disco.free / (1024**3):.2f} GB")

    disco = '/dev/sda'
    try:
        resultado = subprocess.run(['sudo', 'smartctl', '-A', disco], capture_output=True, text=True)
        salida = resultado.stdout

        for linea in salida.splitlines():
            if 'Temperature_Celsius' in linea:
                match = re.search(r'(\d+)\s+\(Min/Max', linea)
                if match:
                    print(f"Temperatura: {match.group(1)}°C")
                    print()
                    return
    except Exception as e:
        print(f"❌ Error al obtener temperatura: {e}")

def info_red():
    print("🌐 Red Info")
    net = psutil.net_io_counters()
    print(f"Enviado: {net.bytes_sent / (1024**2):.2f} MB")
    print(f"Recibido: {net.bytes_recv / (1024**2):.2f} MB")
    print()


# Llamada a todas las funciones
info_cpu()
info_ram()
info_disco()
info_red()

