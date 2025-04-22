#   Obtener numero de serie de la ESP32
import serial.tools.list_ports

puertos = serial.tools.list_ports.comports()

for puerto in puertos:
    print(f"Dispositivo: {puerto.device}")
    print(f"  - Descripción: {puerto.description}")
    print(f"  - Fabricante: {puerto.manufacturer}")
    print(f"  - Serial: {puerto.serial_number}\n")
