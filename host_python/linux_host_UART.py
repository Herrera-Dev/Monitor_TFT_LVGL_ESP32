import serial.tools.list_ports
import os, time, re, glob
import serial, psutil, subprocess

ESPERA = 10 # Tiempo de espera entre envios de datos
serialEsp32 = "0001" # REMPLAZAR POR EL NUMERO DE SERIE DE SU ESP32 --> id_ESP32.py
portEsp32 = None
con = serial.Serial()

#---------------
def encontrarPuerto():
    puertos = serial.tools.list_ports.comports()
    dispositivos = {}
    for puerto in puertos:
        dispositivos[puerto.serial_number] = puerto.device

    if dispositivos:
        for id, puerto in dispositivos.items():
            print(f" - {id} en: {puerto}")
            if serialEsp32 == id:
                return puerto
    else:
        return None
    return None
def newConexion():
    global portEsp32, con
    con.close()

    portEsp32 = encontrarPuerto()
    while portEsp32 == None:
        print("Dispositivo ESP32 no encontrado.")
        portEsp32 = encontrarPuerto()
        time.sleep(3)

#---------------
def GPU_Temp():
    # Intentar obtener la temperatura de la GPU (si es una NVIDIA)
    gpu_temp = os.popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader").read().strip()
    return gpu_temp if gpu_temp else "n/a"
def CPU_Temp():
    temperatures = {}
    
    # Buscar en todas los sensores de temperatura
    for path in glob.glob("/sys/class/thermal/thermal_zone*/temp"):
        try:
            # Obtener el tipo de sensor
            sensor_path = path.replace("/temp", "/type")
            with open(sensor_path, "r") as f:
                sensor_name = f.read().strip()

            # Leer la temperatura
            with open(path, "r") as f:
                temp = int(f.read().strip()) / 1000  # Convertir a grados Celsius

            temperatures[sensor_name] = temp
        except:
            return "n/a"
        
    #print(temperatures) # Ver todas las temeperaturas
    return temperatures.get("x86_pkg_temp", "n/a") # ELEGIR QUE TEMPERATURA MOSTRAR, x86_pkg_temp es del CPU
def FAN_Speed():
    # Obtener la velocidad del ventilador usando lm-sensors
    fan_speed = os.popen("sensors | grep 'fan'").read().strip()

    # Buscar todos los números en la salida
    numbers = re.findall(r"\b\d+\b", fan_speed)

    # Filtrar los valores mayores a 0 y devolver el primero encontrado
    valid_numbers = [int(num) for num in numbers if int(num) > 0]

    return str(valid_numbers[0]) if valid_numbers else "n/a"
def free_Disk():
    obj_Disk = psutil.disk_usage('/') # Rutas de montaje /, /home, /mnt/data
    free_disk = int(obj_Disk.free / (1000.0 ** 3))  # GB
    return free_disk
def used_Disk():
    disco = psutil.disk_usage('/')
    return disco.percent
def temp_Disk():
    disco = '/dev/sda'
    try:
        resultado = subprocess.run(['smartctl', '-A', disco], capture_output=True, text=True)
#        resultado = subprocess.run(['sudo', 'smartctl', '-A', disco], capture_output=True, text=True)
        salida = resultado.stdout

        for linea in salida.splitlines():
            if 'Temperature_Celsius' in linea:
                match = re.search(r'(\d+)\s+\(Min/Max', linea)
                if match:
                    return match.group(1)
                return 0
    except Exception as e:
        print(f"❌ Error al obtener temperatura: {e}")
        return 0
def free_Memory():
    ram = psutil.virtual_memory()
    disponible_mb = ram.available / (1024 ** 2)  # Convertir a MB
    return round(disponible_mb, 2)
def used_memory():
    ram = psutil.virtual_memory()
    return ram.percent
def proc_count():
    return len(list(psutil.process_iter()))
def system_use():
    return psutil.cpu_percent(interval=1)

def enviarDatos(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9): # Enviando a la ESP32
    global con
    try:
        if portEsp32 is not None:
            data = f"{d0},{d1},{d2},{d3},{d4},{d5},{d6},{d7},{d8},{d9}#"
            con = serial.Serial(portEsp32, baudrate=115200) # Port USB
            con.rts = False # Evitar que el ESP32 se reinicie
            con.dtr = False # Evitar que el ESP32 se reinicie
            con.write(data.encode())
            print("Enviado:", data.encode())  
            con.close()
        else:
            print("Dispositivo NO encontrado.")
            newConexion() 

    except Exception as e:        
        con.close()
        print(f"ERROR:", {e})
        newConexion()

#----------------
newConexion()
print("Enviando datos a ESP32...")

while True:
    try:
        dat0 = CPU_Temp()    # °C
        dat1 = system_use()  # %
        dat2 = free_Memory() # MBs
        dat3 = used_memory() # %
        dat4 = free_Disk()   # GB
        dat5 = temp_Disk()   # °C
        dat6 = used_Disk()   # %
        dat7 = CPU_Temp()    # °C -> En mi caso muestro en la GPU el de la CPU
        dat8 = FAN_Speed()   # RPM
        dat9 = proc_count()  # count

        enviarDatos(dat0, dat1, dat2, dat3, dat4, dat5, dat6, dat7, dat8, dat9) # Enviar datos a ESP32
        time.sleep(ESPERA)

    except Exception as error:
        print(error)
        print("🔴 Sin comunicación.\n")
        con.close()
        newConexion()
