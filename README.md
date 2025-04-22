# Monitor de datos con Pantalla TFT ILI9341 2.4" con LVGL y ESP32

Monitoreo de datos con una **ESP32** y **pantalla táctil TFT 2.4" SPI** con el controlador **ILI9341** y la biblioteca gráfica **LVGL**. El sistema muestra datos **meteorológicos**, precios de **criptomonedas** y **métricas del sistema** en una interfaz gráfica interactiva. Además, incluye un script en Python para enviar datos del sistema host al **ESP32** a través de comunicación serial.

> - 🌐 Este proyecto está basado e inspirado en el trabajo de [Uteh Str](https://www.youtube.com/watch?v=bv47wcvvn9s) y [DustinWatts](https://github.com/DustinWatts/Bluetooth-System-Monitor).

## 📜 Características

- **Datos meteorológicos**: Obtiene información del clima desde la API de [OpenWeatherMap](https://home.openweathermap.org/users/sign_in).
- **Precios de criptomonedas**: Consulta precios en tiempo real desde la API de [CoinMarketCap](https://pro.coinmarketcap.com/).
- **Interfaz gráfica**: Utiliza LVGL para crear una interfaz gráfica interactiva.
- **Pantalla táctil**: Soporte para interacción táctil con calibración personalizada.
- **Conexión WiFi**: Configuración y conexión a redes WiFi.
- **Persistencia de datos**: Guarda configuraciones en la memoria.

## 🖼️ Capturas
<p align="center">
  <img src="/imagenes/animation.gif" alt="Interfaz" width="45%">
  <img src="/imagenes/img0.png" alt="Interfaz 0" width="45%">
</p>

<p align="center">
  <img src="/imagenes/img1.png" alt="Interfaz 1" width="45%">
  <img src="/imagenes/img2.png" alt="Interfaz 2" width="45%">
</p>

## 🔌 Conexiones
![alt text](/imagenes/connecting.jpg "Conexiones")

## 🖥️ Hardware
- **Placa:** ESP32
- **Pantalla táctil:** TFT ILI9341 2.4 SPI"

## 🛠️ Configuracion del Entorno
### Software
- **IDE**: Arduino IDE `v1.8.x` o `2.3.x`
- **Framework**: Arduino ESP32 `v2.0.17`
- **Board**: `ESP32 Dev Module`
- **Partition:**: `HUGO APP (3MB No OTA/1MB SPIFFS)`
- **Archivos de configuracion**:
  - Archivo `User_Setup.h` copiar al directorio de la libreria `TFT_eSPI`
  - Archivo `lv_conf.h` copiar al directorio de las librerias de arduino _(fuera del directorio de la libreria de `lvgl`)_
- **Linux**:
    - **Python**: `v3.10`

### Librerias
- **LVGL**: `v9.2.0`
- **TFT_eSPI**: `v2.5.43`
- **Arduino_JSON**: `v0.2.0`
- **XPT2046_Touchscreen**: `v1.4.0`

### EZZ-STUDIO
Si va realizar modificaciones a las imagenes o al diseño en el archivo `Diseño_Monitor_SystemWeather_ESP32TFT.eez-project` necesitara **ezz-studio**
- **EEZ Studio**: `v0.20.0` --> https://github.com/eez-open/studio/releases/tag/v0.20.0
- **Python**: `v3.10.2`
- **Paquetes/Módulos de Python**:
  - **pypng**: `v0.20220715.0` --> `python3 -m pip install pypng==0.20220715.0`
  - **lz4**: `v4.3.3` --> `python3 -m pip install lz4`

**Configuracion de ezz-studio:** Asegúrese de que cuente con las siguientes configuraciones.

  Modificar en `Setting/General`:
  - `Display width: 320`
  - `Display height: 240`

  Modificar en `Setting/build`:
  - `LVGL include: lvgl.h` 
  <br>
  
  > ⚠️ Una vez finalizado el diseño y la compilación (`build`), copia los archivos generados en el `directorio ui` al `directorio del código`. Alternativamente, puedes utilizar el script `integrar_diseño_al_codigo.py` para automatizar este proceso.

## 🚀 Script Python (`linux_host_UART.py`)

El script `linux_host_UART.py` recopila métricas del sistema host, como temperatura del CPU/GPU, uso de RAM, velocidad del ventilador, entre otros, y las envía al ESP32 a través de un puerto serial cada **10 segundos**. 

- **Reconexión automática**: Detecta y reconecta automáticamente el puerto serial si se pierde la conexión en caso de estar trabajando con otrar placa de desarrollo.

### Requisitos de software para el script:

1. Python3
2. Paquetes Python:
   - **psutil** -> Instalar: `pip3 install psutil`
   - **pyserial** -> Instalar: `pip3 install pyserial` 
  Algunas veces requiere **reiniciar** o **cerrar sesion**.

3. Herramientas del sistema:
   - **smartmontools** -> Instalar: `sudo apt install smartmontools`.
    Requiere permisos de superusuario para usar `smartctl` para obtener la temperatura del disco. O bien, añadir permisos al ejecutable de smartctl con:`sudo chmod +s /usr/sbin/smartctl`, si no requiere esta informacion en pantalla puede hacer que ignore esa funcion.

   - **lm-sensors** -> `sudo apt install lm-sensors`.
    Ejecutar `sudo sensors-detect` una sola vez.

   - **nvidia-smi** (para temperatura de GPU NVIDIA, ya incluido en drivers propietarios)
  <br>

  > ⚠️ Estos requisitos están basados en una configuración común de sistemas Linux como Debian, Ubuntu o derivados, y pueden no funcionar de forma idéntica en todas las distribuciones, versiones o **configuraciones de hardware**. Algunas funciones del script, como la lectura de temperatura del disco, la velocidad del ventilador o la temperatura de la GPU, dependen del **hardware disponible** y de cómo el sistema **expone esa información**. Es posible que necesites adaptar o personalizar las funciones del script según tu entorno.

### Uso
- El script enviara laetricas al ESP32 cada 10 segundos (configurable mediante la variable `ESPERA`).
- Reemplaza el valor de `serialEsp32` en el script `linux_host_UART.py` con el número de serie de su **ESP32**. Puedes obtener este número ejecutando el script `id_ESP32.py`.

  ![alt text](/imagenes/code1.png "Numero de serie")

  **Ejemplo de salida del script:** `b'40.0,21.7,2905.45,62.8,50,34,24.5,40.0,n/a,285#'`
- Si tiene `error de permisos de puerto`, dar permiso a tu usuario para acceder a los puertos seriales:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
  Ejecutar: `python3 linux_host_UART.py`

## 📺️ Calibracion de la pantalla
Sustituye los valores obtenidos del monitor serial durante la calibración realizada con `Calibrar_Pantalla.ino` en el código principal.
![alt text](/imagenes/calibracion.png "Calibracion")
> ⚠️ Revisa la calibración en el monitor serial si las pulsaciones táctiles no funcionan correctamente. Si es necesario, puedes ajustar manualmente los valores de calibración en el código principal para mejorar la precisión de la pantalla táctil.

## 📄 Descripción de la interfaces de LVGL
### 1. Ventana de Meteorología
Esta ventana muestra información detallada sobre el clima actual obtenida de la API de **OpenWeatherMap**. Incluye datos como la **temperatura** actual, máxima, mínima y **sensación térmica**, así como la **humedad**, **velocidad del viento**, **presión atmosférica** y **visibilidad**. También se presenta un icono gráfico que representa las condiciones climáticas (por ejemplo, soleado, nublado, lluvia). La información se actualiza automáticamente cada **5 minutos**.

### 2. Ventana de Monitor
En esta ventana se visualizan las métricas del sistema host que se reciben a través del puerto serial y de la api de criptomonedas. Estas métricas incluyen:
- **Temperatura del CPU**: En grados Celsius.
- **Uso del CPU**: Porcentaje de uso actual.
- **Memoria RAM libre**: En MB.
- **Uso de la RAM**: Porcentaje de uso actual.
- **Espacio libre en disco**: En GB.
- **Uso del disco**: Porcentaje de uso actual.
- **Temperatura de la GPU**: En grados Celsius.
- **Velocidad del ventilador**: En RPM.
- **Número de procesos activos**: Cantidad de procesos en ejecución.
- **Precio de criptomenedas BTC y ETH**

Cada métrica se muestra con un valor actualizado cada **10 segundos** excepto el de criptomonedas es cada **3 minutos** y un indicador que alerta si el valor supera un umbral crítico.

### 3. Ventana Gráfica
Esta ventana permite visualizar un gráfico de los últimos datos recibidos sobre una métrica seleccionada. El gráfico incluye:
- **Eje X**: Representa el tiempo o el número de muestras.
- **Eje Y**: Representa los valores de la métrica seleccionada, con su unidad.
- **Línea de advertencia**: Indica un umbral crítico para la métrica, como un límite de temperatura o uso.

El gráfico se actualiza dinámicamente con los nuevos datos recibidos.

### 4. Ventana Configurar Credenciales
Esta ventana permite al usuario ingresar y guardar las credenciales necesarias para la conexión **WiFi** y las **APIs** utilizadas en el sistema. Los campos configurables incluyen:
- **WiFi**:
  - **SSID**: Nombre de la red WiFi.
  - **Contraseña**: Clave de la red WiFi.
- **API del clima**:
  - **Clave API**: Clave de OpenWeatherMap.
  - **Ciudad**: Nombre de la ciudad (ejemplo: `New York`, `London`, `Madrid`).
  - **País**: Código del país ISO 3166-1 alfa-2 (ejemplo: `US`, `GB`, `ES`).
  - **Zona horaria**: Desplazamiento UTC (ejemplo: `-6`, `+1`, `-6`).
- **API de criptomonedas**:
  - **Clave API**: Clave de CoinMarketCap.
  - **Monedas**: Símbolos de las criptomonedas (ejemplo: `BTC`, `ETH`).
  - **Divisa**: Moneda de conversión (ejemplo: `USD`, `EUR`).

### 5. Ventana Configuración del Monitor
En esta ventana, el usuario puede personalizar qué métricas se mostrarán en la ventana de monitor. Las opciones incluyen:
- Selección de métricas disponibles, como temperatura del CPU, uso de RAM, velocidad del ventilador, etc.
- Visualización del estado actual de las métricas configuradas.
- Modificar los datos `yMin`, `yMax` y `warn` de las metricas.

Esta ventana permite al usuario adaptar la interfaz del monitor.

