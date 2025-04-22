
// Núcleo de Arduino para ESP32: v2.0.17
#include <WiFi.h>
#include <HTTPClient.h>
#include <lvgl.h>                // v9.2.0
#include <TFT_eSPI.h>            // v2.5.43
#include <Arduino_JSON.h>        // v0.2.0
#include <XPT2046_Touchscreen.h> // v1.4.0
#include <Preferences.h>
#include "time.h"
#include "ui.h"
#include "images.h"
Preferences memoria;

#define T_CS_PIN 13 // T_CS Touch pin
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define INTERVAL_UPDATING_APIW 300000 // 5 min. Actualizar API - Ajustar segun a sus necesidades
#define INTERVAL_UPDATING_APIC 180000 // 3 min. Actualizar API - Ajustar segun a sus necesidades
const int REST_VALORES = 600000;      // 10 min. Si pasa mas de 10 min sin recibir datos nuevos mostrar 0
const int ACT_SERIAL = 10000;         // 10 seg. Actualizar datos en pantalla
const int SEGUNDERO = 1000;

const byte TOTAL_PERFILES_SERIAL = 10; // Total de datos para los perfiles que llegan desde el monitor Serial sin criptos.
const byte TOTAL_PERFILES = 12;        // Total de los perfiles disponible para mostrar mas criptos.
const byte TOTAL_PUNTOS_X = 19;        // Historial de los datos para el grafico.
const byte TOTAL_CONF_PER = 6;         // Total de perfiles que se muestran en pantalla.

const char *hostname = "Monitor-ESPTFT";
const char *ntpServer = "pool.ntp.org";
const int daylightOffset_sec = 0;
const char *defaultKeyWeather = ""; // API, editable desde la pantalla.
const char *defaultKeyCoin = "";    // API, editable desde la pantalla.
const int pinBrillo = 16;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint8_t *draw_buf;
uint32_t lastTick = 0;

// Pantalla ------------------------------------
struct creden // Estos datos son editables desde la pantalla tactil
{
  char ssid[15];
  char password[20];
  char keyCripto[45];  // API CoinMarketCap --> https://pro.coinmarketcap.com/
  char criptos[2][6];  // ejm: BTC, ETH
  char divisa[6];      // Ejm: USD, EUR, ARS, MXN
  char keyWeather[40]; // API openWeatherMap --> https://home.openweathermap.org/users/sign_in
  char city[20];       // Ejm: New York, London, Madrid --> https://openweathermap.org/
  char countryCode[6]; // Ejm: US, GB, ES
  char utm[4];         // Ejm: -6, +1, +4
};
creden llaves = {"null", "null", "null", {"BTC", "ETH"}, "USD", "null", "London", "GB", "+1"};

const lv_img_dsc_t *logo_perf[12] = {
    // Para obtener los nombres de las imagenes para los LOGOS, consulte el archivo "images.c".
    &img_logo_temp_cpu, // 0
    &img_logo_use_cpu,  // 1
    &img_logo_free_ram, // 2
    &img_logo_rpm_fan,  // 3
    &img_logo_free_hdd, // 4
    &img_logo_temp_gpu, // 5
    &img_logo_procs,    // 6
    &img_logo_use_ram,  // 7
    &img_logo_use_hdd,  // 8
    &img_logo_bitcoin,  // 9
    &img_logo_ethereum, // 10
    &img_logo_temp_hdd  // 11
};

struct datosPer
{
  char nombre[20];           // Nombre del dato.
  char descrip[30];          // Nombre para la grafica.
  char simbolo[6];           // Simbolo para la grafica ymax.
  int ymin;                  // Valor minimo para el eje Y de la grafica.
  int ymax;                  // Valor maxima para el eje Y de la grafica.
  int warn;                  // Advertencia cuando supera el umbral critico.
  bool warnUp;               // Direccion del umbral critico es mayor o menor a los valores normales.
  byte logo;                 // Imagen a mostrar en el monitor.
  int datos[TOTAL_PUNTOS_X]; // Array con los ultimos datos recibidos del monitor serial u obtenidos de la API.
};

// Los datos del arreglo son del monitor serial que recibe desde python excepto los de cripto y deben estar en el mismo orden.
// {[Nombre], [Descripcion], [Simbolo],[ymin], [ymax], [warn], [warnUp], [logo], [datos]}
datosPer perfiles[TOTAL_PERFILES] = {
    {"TempCPU", "TEMPERATURA DEL CPU", "°C", 0, 100, 70, true, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},           // datos_0 Monitor serial
    {"UsoCPU", "USO DEL SISTEMA", "%", 0, 100, 75, true, 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},                 // datos_1 Monitor serial
    {"LibreRAM", "MEMORIA RAM LIBRE (MB)", "MB", 0, 6000, 2000, false, 2, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},   // datos_2 Monitor serial
    {"UsoRAM", "USO DE LA RAM", "%", 0, 100, 70, true, 7, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},                   // datos_3 Monitor serial
    {"LibreHDD", "ESPACIO LIBRE DEL HDD", "GB", 0, 100, 15, false, 4, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},       // datos_4 Monitor serial
    {"TempHDD", "TEMPERATURA DEL DISCO HDD", "°C", 0, 100, 50, true, 11, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},    // datos_5 Monitor serial
    {"UsoHDD", "USO DEL DISCO HDD", "%", 0, 100, 70, true, 4, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},               // datos_6 Monitor serial
    {"TempGPU", "TEMPERATURA DE LA GPU", "°C", 0, 100, 65, true, 5, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},         // datos_7 Monitor serial
    {"VelocFAN", "VELOCIDAD DEL VENTILADOR", "RPM", 0, 5000, 3000, true, 3, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, // datos_8 Monitor serial
    {"Procesos", "NUMERO DE PROCESOS ACTIVOS", "", 0, 500, 300, true, 6, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},    // datos_9 Monitor serial
    {"PrecioBTC", "PRECIO DE BTC", "USD", 0, 120000, 95000, true, 9, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},        // datos_10 API Cripto
    {"PrecioETH", "PRECIO DE ETH", "USD", 0, 8000, 2500, true, 10, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}           // datos_11 API Cripto
};

int verMonitor[TOTAL_CONF_PER] = {0, 1, 4, 7, 3, 10}; //
int newAtajo = -1;
byte nGraf = 0;

bool vistaClima = true;
bool vistaGrafico = false;
bool vistaConfig = false;
bool restartSys = false;
bool conect = true;
int valBrillo = 10;

long tAntActClima = 0;
long tAntActSerial = -10;
long tAntNuevosDat = 0;
long tAntUpdApiW = 0;
long tAntUpdApiC = 0;

// Touch ------------------------------------------------
// Reemplace con los resultados de la calibración de su pantalla táctil TFT.
#define touchscreen_Min_X 3800 // X = 240
#define touchscreen_Max_X 360  // X = 0
#define touchscreen_Min_Y 169
#define touchscreen_Max_Y 3730

XPT2046_Touchscreen touchscreen(T_CS_PIN);
uint16_t x, y, z;

// Clima ----------------------------------------
const char daysOfTheWeek[8][10] = {"Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado", "Error"};
const char namesOfMonths[13][6] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic", "Error"};
String str_Weather_Main, str_Weather_Icon, str_Weather_Description;
String str_Temperature, str_Feels_Like, str_Temp_Max, str_Temp_Min;
String str_Humidity, str_Wind_Speed, str_Pressure, str_Visibility;

int d_year;
byte d_month, d_day, daysOfTheWeek_Val;
byte t_hour, t_minute, t_second;
char ca_Info_Status[50];
String jsonBuffer;

//========================================
void getDatos()
{
  int temp[TOTAL_PERFILES][3];

  memoria.begin("memory", true);
  delay(20);
  memoria.getBytes("datos", verMonitor, sizeof(verMonitor));
  memoria.getBytes("llaves", &llaves, sizeof(llaves));
  valBrillo = memoria.getInt("brillo", 250);

  if (memoria.isKey("ejeY"))
  {
    memoria.getBytes("ejeY", temp, sizeof(temp));
    for (int i = 0; i < TOTAL_PERFILES; i++)
    {
      perfiles[i].ymin = temp[i][0];
      perfiles[i].ymax = temp[i][1];
      perfiles[i].warn = temp[i][2];
    }
  }
  memoria.end();

  if (strlen(llaves.keyCripto) < 15)
  {
    strncpy(llaves.keyCripto, defaultKeyCoin, sizeof(llaves.keyCripto) - 1);
    llaves.keyCripto[sizeof(llaves.keyCripto) - 1] = '\0';
  }

  if (strlen(llaves.keyWeather) < 15)
  {
    strncpy(llaves.keyWeather, defaultKeyWeather, sizeof(llaves.keyWeather) - 1);
    llaves.keyWeather[sizeof(llaves.keyWeather) - 1] = '\0';
  }
}
void setDatos()
{
  int temp[TOTAL_PERFILES][3];
  memoria.begin("memory", false);
  delay(20);
  memoria.putBytes("datos", verMonitor, sizeof(verMonitor));
  memoria.putBytes("llaves", &llaves, sizeof(llaves));
  memoria.putInt("brillo", valBrillo);

  for (int i = 0; i < TOTAL_PERFILES; i++)
  {
    temp[i][0] = perfiles[i].ymin;
    temp[i][1] = perfiles[i].ymax;
    temp[i][2] = perfiles[i].warn;
  }
  memoria.putBytes("ejeY", temp, sizeof(temp));

  memoria.end();
}
void inicializarTexto()
{
  lv_label_set_text(objects.label_city, llaves.city);
  lv_label_set_text(objects.label_date, "--, ----");
  lv_label_set_text(objects.label_time, "--:--:--");
  lv_label_set_text(objects.label_weather_main, "--");
  lv_label_set_text(objects.label_weather_description, "--");
  lv_label_set_text(objects.label_temperature, "--.--°C");
  lv_label_set_text(objects.label_feels_like, "--.-- °C");
  lv_label_set_text(objects.label_temp_max, "--.-- °C");
  lv_label_set_text(objects.label_temp_min, "--.-- °C");
  lv_label_set_text(objects.label_humidity, "--%");
  lv_label_set_text(objects.label_wind, "-- km/h");
  lv_label_set_text(objects.label_pressure, "-- hPa");
  lv_label_set_text(objects.label_visibility, "-- km");
  lv_label_set_text(objects.label_info, "--");

  byte posi1 = TOTAL_PERFILES_SERIAL;
  byte posi2 = TOTAL_PERFILES_SERIAL + 1;
  String texto = "Precio" + String(llaves.criptos[0]);
  strcpy(perfiles[posi1].nombre, texto.c_str());
  texto = "Precio" + String(llaves.criptos[1]);
  strcpy(perfiles[posi2].nombre, texto.c_str());
  texto = "PRECIO DE " + String(llaves.criptos[0]);
  strcpy(perfiles[posi1].descrip, texto.c_str());
  texto = "PRECIO DE " + String(llaves.criptos[1]);
  strcpy(perfiles[posi2].descrip, texto.c_str());
  strcpy(perfiles[posi1].simbolo, llaves.divisa);
  strcpy(perfiles[posi2].simbolo, llaves.divisa);

  update_UI();
}
void conexionWiFi()
{
  lv_label_set_text(objects.label_info, "Conectando al WiFi...");
  Serial.print("\nConectando al WiFi: ");
  update_UI();

  WiFi.setHostname(hostname);
  WiFi.mode(WIFI_STA);
  delay(1000);

  Serial.print(llaves.ssid);
  WiFi.begin(llaves.ssid, llaves.password);

  int connecting_process_timed_out = 20; //--> 20 = 20 segundos para el intento de conexion.
  while (WiFi.status() != WL_CONNECTED)
  {
    connecting_process_timed_out--;
    Serial.print(".");
    delay(500);

    if (connecting_process_timed_out == 0)
    {
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    String x = "Conectado - IP: " + WiFi.localIP().toString();
    Serial.println(x);
    lv_label_set_text(objects.label_info, x.c_str());
    update_UI();
    delay(3000);

    lv_label_set_text(objects.label_info, "");
    update_UI();
  }
  else
  {
    lv_label_set_text(objects.label_info, "No se pudo conectar a WiFi.");
    Serial.println("No se pudo conectar a WiFi.");
    conect = false;
    update_UI();
    delay(2000);
  }
}
void obtenerTiempoNTP()
{
  lv_label_set_text(objects.label_info, "Obteniendo el tiempo y fecha del servidor...");
  update_UI();

  Serial.println();
  Serial.println("Obteniendo datos del servidor NTP. Please wait...");
  delay(500);

  if (WiFi.status() == WL_CONNECTED)
  {
    configTime((atoi(llaves.utm) * 3600), daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      lv_label_set_text(objects.label_info, "No se pudo obtener tiempo.");
      Serial.println("No se pudo obtener tiempo.");
      update_UI();
      delay(1500);
      return;
    }

    t_hour = timeinfo.tm_hour;
    t_minute = timeinfo.tm_min;
    t_second = timeinfo.tm_sec;

    d_day = timeinfo.tm_mday;
    d_month = timeinfo.tm_mon + 1;
    d_year = timeinfo.tm_year + 1900;

    // rtc.adjust(DateTime(d_year, d_month, d_day, t_hour, t_minute, t_second));

    char TimeDate[40];
    sprintf(TimeDate, "Date : %02d-%02d-%d | Time : %02d:%02d:%02d", d_day, d_month, d_year, t_hour, t_minute, t_second);
    update_UI();

    Serial.print("Servidor NTP: ");
    Serial.println(TimeDate);
    delay(1000);

    lv_label_set_text(objects.label_info, "");
    update_UI();
  }
  else
  {
    lv_label_set_text(objects.label_info, "FALLIDO! WiFi desconectado.");
    Serial.println("FAILED! WiFi Desconectado");
    update_UI();
    delay(1500);
  }
}
void leerHora()
{
  struct tm datos;
  if (getLocalTime(&datos))
  {
    d_year = datos.tm_year + 1900;
    d_month = datos.tm_mon + 1;
    if (d_month > 12 || d_month < 1)
      d_month = 13;
    d_day = datos.tm_mday;
    daysOfTheWeek_Val = datos.tm_wday;
    if (daysOfTheWeek_Val > 7 || daysOfTheWeek_Val < 0)
      daysOfTheWeek_Val = 7;
    t_hour = datos.tm_hour;
    t_minute = datos.tm_min;
    t_second = datos.tm_sec;
  }
}
void actualizarHora()
{
  leerHora();
  char str_Date[23];
  lv_snprintf(str_Date, sizeof(str_Date), "%s, %02d-%s-%d", daysOfTheWeek[daysOfTheWeek_Val], d_day, namesOfMonths[d_month - 1], d_year);
  lv_label_set_text(objects.label_date, str_Date);

  char str_Time[9];
  lv_snprintf(str_Time, sizeof(str_Time), "%02d:%02d:%02d", t_hour, t_minute, t_second);
  lv_label_set_text(objects.label_time, str_Time);
}
void monitorSerial()
{
  if (Serial.available())
  {
    for (int i = 0; i < TOTAL_PERFILES_SERIAL; i++)
    {
      if (i != TOTAL_PERFILES_SERIAL - 1)
      {
        String newVal = Serial.readStringUntil(',');
        nuevosDatos(newVal, i);
      }
      else
      {
        String newVal = Serial.readStringUntil('#');
        nuevosDatos(newVal, i);
      }
    }
    tAntNuevosDat = millis();
  }
  else
  {
    if (tAntNuevosDat + REST_VALORES < millis())
    {
      for (int i = 0; i < TOTAL_PERFILES_SERIAL; i++)
      {
        nuevosDatos("0", i);
      }
    }
  }
}

//----------------------------------------
String httpGETRequest(const char *serverName)
{
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName); // Su nombre de dominio con ruta de URL o dirección IP con ruta.

  // Enviar solicitud de publicación HTTP.
  int httpResponseCode = http.GET();
  String payload = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources.
  http.end();
  return payload;
}
String traducirMain(String main)
{
  if (main == "Thunderstorm")
    return "Tormenta eléctrica";
  if (main == "Drizzle")
    return "Llovizna";
  if (main == "Rain")
    return "Lluvia";
  if (main == "Snow")
    return "Nieve";
  if (main == "Atmosphere")
    return "Atmósfera";
  if (main == "Clear")
    return "Despejado";
  if (main == "Clouds")
    return "Nublado";
  return main; // Si no coincide con ninguna, devuelve esto
}

void obtenerDatos_CoinMarketCap(float &crypto1, float &crypto2)
{
  HTTPClient http;

  // Construimos la URL dinámica con los símbolos que pasaste
  String apiUrl = "https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?symbol=" + String(llaves.criptos[0]) + "," + String(llaves.criptos[1]) + "&convert=" + String(llaves.divisa);

  http.begin(apiUrl);
  http.addHeader("Accepts", "application/json");
  http.addHeader("X-CMC_PRO_API_KEY", String(llaves.keyCripto));

  int httpCode = http.GET();

  if (httpCode > 0)
  {
    String payload = http.getString();
    JSONVar data = JSON.parse(payload);

    if (JSON.typeof(data) == "undefined")
    {
      Serial.println("Error al parsear JSON");
      return;
    }
    crypto1 = double(data["data"][String(llaves.criptos[0])]["quote"][String(llaves.divisa)]["price"]);
    crypto2 = double(data["data"][String(llaves.criptos[1])]["quote"][String(llaves.divisa)]["price"]);

    Serial.printf("(%s): $%.2f USD\n", llaves.criptos[0], crypto1);
    Serial.printf("(%s): $%.2f USD\n", llaves.criptos[1], crypto2);
  }
  else
  {
    Serial.printf("Error HTTP: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
void obtenerDatos_OpenWeatherMap()
{
  lv_label_set_text(objects.label_info, "Actualizar datos meteorologicos ...");
  update_UI();

  Serial.println();
  Serial.println("-------------");
  Serial.println("Actualice datos meteorológicos (obteniendo datos meteorológicos de OpenWeathermap).");
  Serial.println("Espere por favor...");
  delay(500);

  // Check WiFi connection status.
  if (WiFi.status() == WL_CONNECTED)
  {
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + String(llaves.city) + "," + String(llaves.countryCode) + "&units=metric&APPID=" + String(llaves.keyWeather) + "&lang=sp";

    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println();
    Serial.println("Datos meteorológicos en forma JSON:");
    Serial.println(jsonBuffer);
    JSONVar myObject = JSON.parse(jsonBuffer);

    // JSON.typeof(jsonVar) can be used to get the type of the var.
    if (JSON.typeof(myObject) == "undefined")
    {
      strcpy(ca_Info_Status, "(Falló: ¡Falló la entrada de análisis!)");
      Serial.println("¡Falló la entrada de análisis!");

      lv_label_set_text(objects.label_info, "");
      update_UI();
      return;
    }

    Serial.println();
    Serial.println("Datos meteorológicos tomados");

    str_Weather_Main = JSON.stringify(myObject["weather"][0]["main"]);
    str_Weather_Main.replace("\"", ""); //--> Removes the Double quotes (") character in the string.
    str_Weather_Main = traducirMain(str_Weather_Main);
    str_Weather_Icon = JSON.stringify(myObject["weather"][0]["icon"]);
    str_Weather_Icon.replace("\"", "");
    str_Weather_Description = JSON.stringify(myObject["weather"][0]["description"]);
    str_Weather_Description.replace("\"", "");

    str_Temperature = JSON.stringify(myObject["main"]["temp"]);
    str_Feels_Like = JSON.stringify(myObject["main"]["feels_like"]);
    str_Temp_Max = JSON.stringify(myObject["main"]["temp_max"]);
    str_Temp_Min = JSON.stringify(myObject["main"]["temp_min"]);

    str_Humidity = JSON.stringify(myObject["main"]["humidity"]);
    str_Wind_Speed = JSON.stringify(myObject["wind"]["speed"]);
    str_Pressure = JSON.stringify(myObject["main"]["pressure"]);
    str_Visibility = JSON.stringify(myObject["visibility"]);

    Serial.print("Weather Main : ");
    Serial.println(str_Weather_Main);
    Serial.print("Weather Icon : ");
    Serial.println(str_Weather_Icon);
    Serial.print("Weather Description : ");
    Serial.println(str_Weather_Description);

    Serial.print("Temperature : ");
    Serial.print(str_Temperature);
    Serial.println(" °C");
    Serial.print("Feels Like : ");
    Serial.print(str_Feels_Like);
    Serial.println(" °C");
    Serial.print("Temp Max : ");
    Serial.print(str_Temp_Max);
    Serial.println(" °C");
    Serial.print("Temp Min : ");
    Serial.print(str_Temp_Min);
    Serial.println(" °C");

    Serial.print("Humidity : ");
    Serial.print(str_Humidity);
    Serial.println(" %");
    Serial.print("Wind Speed : ");
    Serial.print(str_Wind_Speed);
    Serial.println(" m/s");
    Serial.print("Pressure : ");
    Serial.print(str_Pressure);
    Serial.println(" hPa");
    Serial.print("Visibility : ");
    Serial.print(str_Visibility);
    Serial.println(" m");

    Serial.println("-------------");
    Serial.println();

    strcpy(ca_Info_Status, "(Correcto)");

    lv_label_set_text(objects.label_info, "");
    update_UI();
    delay(1000);
  }
  else
  {
    strcpy(ca_Info_Status, "(Error: Sin conexion WiFi!)");
    Serial.println("Sin conexion WiFi!");
    Serial.println("-------------");
    Serial.println();
  }
}

void actualizarCriptos()
{
  byte posi1 = TOTAL_PERFILES_SERIAL;
  byte posi2 = TOTAL_PERFILES_SERIAL + 1;
  float crypt1 = 0;
  float crypt2 = 0;

  obtenerDatos_CoinMarketCap(crypt1, crypt2);
  nuevosDatos(String(crypt1), posi1);
  nuevosDatos(String(crypt2), posi2);
}
void actualizarClima()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    lv_label_set_text(objects.label_info, "No se pudo conectar a WiFi.");
    Serial.println("No se pudo conectar a WiFi.");
    update_UI();
    delay(2000);
    conect = false;
    return;
  }

  actualizarHora();

  char ca_Time[10];
  sprintf(ca_Time, "%02d:%02d:%02d", t_hour, t_minute, t_second);

  obtenerDatos_OpenWeatherMap();                                           // llame a get_data_from_openweathermap() subrutine.
  lv_label_set_text(objects.label_weather_main, str_Weather_Main.c_str()); // Muestra las condiciones climáticas.

  // Muestra imágenes de icono meteorológico.
  //  Para obtener una lista de nombres de variables de icono, consulte el archivo "Images.c".
  if (str_Weather_Icon == "01d")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_01d_72p);
  }
  else if (str_Weather_Icon == "01n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_01n_72p);
  }
  else if (str_Weather_Icon == "02d")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_02d_72p);
  }
  else if (str_Weather_Icon == "02n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_02n_72p);
  }
  else if (str_Weather_Icon == "03d" || str_Weather_Icon == "03n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_03d_03n_72p);
  }
  else if (str_Weather_Icon == "04d" || str_Weather_Icon == "04n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_04d_04n_72p);
  }
  else if (str_Weather_Icon == "09d" || str_Weather_Icon == "09n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_09d_09n_72p);
  }
  else if (str_Weather_Icon == "10d")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_10d_72p);
  }
  else if (str_Weather_Icon == "10n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_10n_72p);
  }
  else if (str_Weather_Icon == "11d" || str_Weather_Icon == "11n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_11d_11n_72p);
  }
  else if (str_Weather_Icon == "13d" || str_Weather_Icon == "13n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_13d_13n_72p);
  }
  else if (str_Weather_Icon == "50d" || str_Weather_Icon == "50n")
  {
    lv_img_set_src(objects.image_icon_weather, &img_icon_50d_50n_72p);
  }
  //----------------------------------------

  // Muestra una descripción de las condiciones climáticas.
  lv_label_set_text(objects.label_weather_description, str_Weather_Description.c_str());

  //----------------------------------------Displays temperature values ​​and other temperature data.
  float f_Temp = str_Temperature.toFloat();
  char ca_Temp[10];
  lv_snprintf(ca_Temp, sizeof(ca_Temp), "%.1f °C", f_Temp);
  lv_label_set_text(objects.label_temperature, ca_Temp);

  float f_Feels_Like = str_Feels_Like.toFloat();
  char ca_Feels_Like[11];
  lv_snprintf(ca_Feels_Like, sizeof(ca_Feels_Like), "%.1f °C", f_Feels_Like);
  lv_label_set_text(objects.label_feels_like, ca_Feels_Like);

  float f_Temp_Max = str_Temp_Max.toFloat();
  char ca_Temp_Max[11];
  lv_snprintf(ca_Temp_Max, sizeof(ca_Temp_Max), "%.1f °C", f_Temp_Max);
  lv_label_set_text(objects.label_temp_max, ca_Temp_Max);

  float f_Temp_Min = str_Temp_Min.toFloat();
  char ca_Temp_Min[11];
  lv_snprintf(ca_Temp_Min, sizeof(ca_Temp_Min), "%.1f °C", f_Temp_Min);
  lv_label_set_text(objects.label_temp_min, ca_Temp_Min);
  //----------------------------------------

  // Muestra valor de humedad.
  str_Humidity += " %";
  lv_label_set_text(objects.label_humidity, str_Humidity.c_str());

  // Displays wind speed values.
  // By default, wind speed values ​​from OpenWeatherMap are in meters per second (m/s).
  // "float f_Wind_Speed ​​= str_Wind_Speed.toFloat() * 3.6;" is to convert from m/s to km/h.
  float f_Wind_Speed = str_Wind_Speed.toFloat() * 3.6;
  char ca_Wind_Speed[12];
  lv_snprintf(ca_Wind_Speed, sizeof(ca_Wind_Speed), "%.1f km/h", f_Wind_Speed);
  lv_label_set_text(objects.label_wind, ca_Wind_Speed);

  // Displays pressure value (atmospheric pressure / air pressure).
  str_Pressure += " hPa";
  lv_label_set_text(objects.label_pressure, str_Pressure.c_str());

  // Displays visibility values.
  // By default, visibility values ​​from OpenWeatherMap are in meters.
  float f_Visibility = str_Visibility.toFloat() / 1000;
  char ca_Visibility[9];
  lv_snprintf(ca_Visibility, sizeof(ca_Visibility), "%.1f km", f_Visibility);
  lv_label_set_text(objects.label_visibility, ca_Visibility);

  // Displays other information.
  char ca_Info[70];
  lv_snprintf(ca_Info, sizeof(ca_Info), "Ultima actualizacion: %s %s", ca_Time, ca_Info_Status);
  lv_label_set_text(objects.label_info, ca_Info);
}

void leerTactil(lv_indev_t *indev, lv_indev_data_t *data)
{
  // Comprueba si se tocó la pantalla táctil e imprime x, y y presión (z)
  if (touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint(); // Obtener puntos de pantalla táctil

    // Calibre los puntos de la pantalla táctil con la función del mapa al ancho y la altura correctos.
    x = map(p.x, touchscreen_Max_X, touchscreen_Min_X, 1, SCREEN_HEIGHT);
    y = map(p.y, touchscreen_Max_Y, touchscreen_Min_Y, 1, SCREEN_WIDTH);
    z = p.z;

    // Establezca las coordenadas.
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;

    // Imprima información táctil sobre X, Y y Presión (Z) en el monitor en serie.
    /*Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();*/
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
void update_UI()
{
  lv_tick_inc(millis() - lastTick); //-> Actualice el temporizador de tick. Tick ​​es nuevo para LVGL 9.
  lastTick = millis();
  lv_timer_handler(); //--> Actualice la interfaz de usuario.
}

// Eventos LVGL --------------------------
static void viewMonitor1_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e); //--> Obtenga el código del evento.
  if (code == LV_EVENT_CLICKED)
  {
    vistaClima = false;
    cargarIconos();
    lv_scr_load_anim(objects.monitor, LV_SCR_LOAD_ANIM_OVER_LEFT, 400, 0, false);
  }
}
static void viewConfig_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e); //--> Obtenga el código del evento.
  if (code == LV_EVENT_CLICKED)
  {
    vistaConfig = true;
    lv_scr_load_anim(objects.config, LV_SCR_LOAD_ANIM_OVER_LEFT, 400, 0, false);
    lv_obj_scroll_to_y(objects.config, 0, LV_ANIM_OFF); // Lleva el scroll al tope (Y=0)

    lv_textarea_set_text(objects.txt_ssid, llaves.ssid);
    lv_textarea_set_text(objects.txt_pass, llaves.password);
    lv_textarea_set_text(objects.txt_api_weather, llaves.keyWeather);
    lv_textarea_set_text(objects.txt_ciudad, llaves.city);
    lv_textarea_set_text(objects.txt_pais, llaves.countryCode);
    lv_textarea_set_text(objects.txt_hora, llaves.utm);
    lv_textarea_set_text(objects.txt_api_coin, llaves.keyCripto);
    lv_textarea_set_text(objects.txt_crypto1, llaves.criptos[0]);
    lv_textarea_set_text(objects.txt_crypto2, llaves.criptos[1]);
    lv_textarea_set_text(objects.txt_divisa, llaves.divisa);

    // debug();
  }
}
static void viewPerfiles_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e); //--> Obtenga el código del evento.
  if (code == LV_EVENT_CLICKED)
  {
    vistaConfig = true;
    lv_scr_load_anim(objects.perfiles, LV_SCR_LOAD_ANIM_OVER_LEFT, 400, 0, false); // Cargar pantalla con animación
    lv_obj_scroll_to_y(objects.perfiles, 0, LV_ANIM_OFF);                          // Lleva el scroll al tope (Y=0)

    lv_dropdown_set_selected(objects.menu_perfiles, 0);
    lv_dropdown_set_options(objects.menu_opciones, String("Seleccionar").c_str());
    lv_label_set_text(objects.text_view, txtEstado().c_str());
    lv_textarea_set_text(objects.txt_ymin, "");
    lv_textarea_set_text(objects.txt_ymax, "");
    lv_textarea_set_text(objects.txt_warn, "");
    newAtajo = -1;
  }
}

static void viewMain1_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e); //--> Obtenga el código del evento.
  if (code == LV_EVENT_CLICKED)
  {
    vistaClima = true;
    tAntUpdApiW = millis() + 3000;
    lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_OVER_RIGHT, 400, 0, false);
  }
}
static void graficar_00_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    nGraf = 0;
    if (verMonitor[nGraf] < 0)
    {
      return;
    }

    vistaGrafico = true;
    lv_obj_add_flag(objects.grafico, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.grafico, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.grafico);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });

    lv_label_set_text(objects.grap_texto, perfiles[verMonitor[nGraf]].descrip);
    lv_label_set_text(objects.label_y, perfiles[verMonitor[nGraf]].simbolo);
    tAntActSerial = 0;
  }
}
static void graficar_01_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    nGraf = 1;
    if (verMonitor[nGraf] < 0)
    {
      return;
    }

    vistaGrafico = true;
    lv_obj_add_flag(objects.grafico, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.grafico, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.grafico);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });

    lv_label_set_text(objects.grap_texto, perfiles[verMonitor[nGraf]].descrip);
    lv_label_set_text(objects.label_y, perfiles[verMonitor[nGraf]].simbolo);
    tAntActSerial = 0;
  }
}
static void graficar_02_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    nGraf = 2;
    if (verMonitor[nGraf] < 0)
    {
      return;
    }

    vistaGrafico = true;
    lv_obj_add_flag(objects.grafico, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.grafico, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.grafico);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });

    lv_label_set_text(objects.grap_texto, perfiles[verMonitor[nGraf]].descrip);
    lv_label_set_text(objects.label_y, perfiles[verMonitor[nGraf]].simbolo);
    tAntActSerial = 0;
  }
}
static void graficar_03_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    nGraf = 3;
    if (verMonitor[nGraf] < 0)
    {
      return;
    }

    vistaGrafico = true;
    lv_obj_add_flag(objects.grafico, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.grafico, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.grafico);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });

    lv_label_set_text(objects.grap_texto, perfiles[verMonitor[nGraf]].descrip);
    lv_label_set_text(objects.label_y, perfiles[verMonitor[nGraf]].simbolo);
    tAntActSerial = 0;
  }
}
static void graficar_04_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    nGraf = 4;
    if (verMonitor[nGraf] < 0)
    {
      return;
    }

    vistaGrafico = true;
    lv_obj_add_flag(objects.grafico, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.grafico, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.grafico);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });

    lv_label_set_text(objects.grap_texto, perfiles[verMonitor[nGraf]].descrip);
    lv_label_set_text(objects.label_y, perfiles[verMonitor[nGraf]].simbolo);
    tAntActSerial = 0;
  }
}
static void graficar_05_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    nGraf = 5;
    if (verMonitor[nGraf] < 0)
    {
      return;
    }

    vistaGrafico = true;
    lv_obj_add_flag(objects.grafico, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.grafico, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.grafico);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });

    lv_label_set_text(objects.grap_texto, perfiles[verMonitor[nGraf]].descrip);
    lv_label_set_text(objects.label_y, perfiles[verMonitor[nGraf]].simbolo);
    tAntActSerial = 0;
  }
}
static void viewMonitor2_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e); //--> Obtenga el código del evento.
  if (code == LV_EVENT_CLICKED)
  {
    vistaGrafico = false;
    lv_obj_add_flag(objects.monitor, LV_OBJ_FLAG_HIDDEN);                    // Ocultar momentáneamente
    lv_scr_load_anim(objects.monitor, LV_SCR_LOAD_ANIM_NONE, 400, 0, false); // Cargar pantalla con animación
    lv_timer_t *t = lv_timer_create_basic();                                 // Mostrar justo después de un pequeño delay
    lv_timer_set_period(t, 10);                                              // espera 10 ms
    lv_timer_set_repeat_count(t, 1);
    lv_timer_set_user_data(t, objects.monitor);
    lv_timer_set_cb(t, [](lv_timer_t *t)
                    { lv_obj_clear_flag((lv_obj_t *)lv_timer_get_user_data(t), LV_OBJ_FLAG_HIDDEN); });
  }
}

static void viewMain3_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    guardDatY();
    lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_OVER_RIGHT, 400, 0, false); // Cargar pantalla con animación
    vistaConfig = false;
    tAntActSerial = 0;
    setDatos();
    // debug();
  }
}
static void box_menuPerfiles_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED)
  {
    guardDatY();
  }

  if (code == LV_EVENT_VALUE_CHANGED)
  {
    newAtajo = lv_dropdown_get_selected(obj) - 1;
    if (newAtajo < 0)
    {
      lv_dropdown_set_options(objects.menu_opciones, "Seleccionar");
      lv_textarea_set_text(objects.txt_ymin, "");
      lv_textarea_set_text(objects.txt_ymax, "");
      lv_textarea_set_text(objects.txt_warn, "");
      return;
    }

    String menuBox = "Seleccionar\n";
    for (int i = 0; i < TOTAL_PERFILES; i++)
    {
      menuBox += String(perfiles[i].nombre);
      if (i != (TOTAL_PERFILES - 1))
      {
        menuBox += "\n";
      }
    }
    lv_dropdown_set_options(objects.menu_opciones, menuBox.c_str());
    lv_dropdown_set_selected(objects.menu_opciones, (verMonitor[newAtajo] + 1));

    if (verMonitor[newAtajo] < 0)
    {
      lv_textarea_set_text(objects.txt_ymin, "");
      lv_textarea_set_text(objects.txt_ymax, "");
      lv_textarea_set_text(objects.txt_warn, "");
      return;
    }
    char buffer0[10];
    sprintf(buffer0, "%d", perfiles[verMonitor[newAtajo]].ymin);
    lv_textarea_set_text(objects.txt_ymin, buffer0);

    char buffer1[10];
    sprintf(buffer1, "%d", perfiles[verMonitor[newAtajo]].ymax);
    lv_textarea_set_text(objects.txt_ymax, buffer1);

    char buffer2[10];
    sprintf(buffer2, "%d", perfiles[verMonitor[newAtajo]].warn);
    lv_textarea_set_text(objects.txt_warn, buffer2);
  }
}
static void box_menuOpciones_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);

  // --------------------------------------
  if (code == LV_EVENT_CLICKED)
  {
    guardDatY();
  }

  // --------------------------------------
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    if (newAtajo < 0)
      return;

    int menuId = lv_dropdown_get_selected(obj) - 1;
    if (menuId < 0)
    {
      lv_label_set_text(objects.label_estado, "Perfil eliminado.");
      lv_textarea_set_text(objects.txt_ymin, "");
      lv_textarea_set_text(objects.txt_ymax, "");
      lv_textarea_set_text(objects.txt_warn, "");
      verMonitor[newAtajo] = -1;
    }
    else
    {
      verMonitor[newAtajo] = menuId;

      char buffer0[10];
      sprintf(buffer0, "%d", perfiles[verMonitor[newAtajo]].ymin);
      lv_textarea_set_text(objects.txt_ymin, buffer0);

      char buffer1[10];
      sprintf(buffer1, "%d", perfiles[verMonitor[newAtajo]].ymax);
      lv_textarea_set_text(objects.txt_ymax, buffer1);

      char buffer2[10];
      sprintf(buffer2, "%d", perfiles[verMonitor[newAtajo]].warn);
      lv_textarea_set_text(objects.txt_warn, buffer2);
    }
    lv_label_set_text(objects.text_view, txtEstado().c_str());
  }
}

static void new_brillo_handler(lv_event_t *e)
{
  lv_obj_t *my_slider = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *my_label_slider = (lv_obj_t *)lv_event_get_user_data(e);

  uint8_t max = 250;
  uint8_t min = 1;

  uint8_t val = lv_slider_get_value(my_slider);
  valBrillo = (max + min) - val;
  analogWrite(pinBrillo, valBrillo);

  // Actualizar texto del label
  char buf[6];
  uint8_t porc = (val - min) * 100 / (max - min);
  lv_snprintf(buf, sizeof(buf), "%d%s", porc, "%");
  lv_label_set_text(my_label_slider, buf);
}
static void btn_guardaCreden_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    const char *txtWifi = lv_textarea_get_text(objects.txt_ssid);
    const char *txtPass = lv_textarea_get_text(objects.txt_pass);
    const char *txtApiW = lv_textarea_get_text(objects.txt_api_weather);
    const char *txtCiud = lv_textarea_get_text(objects.txt_ciudad);
    const char *txtPais = lv_textarea_get_text(objects.txt_pais);
    const char *txtHora = lv_textarea_get_text(objects.txt_hora);
    const char *txtApiC = lv_textarea_get_text(objects.txt_api_coin);
    const char *txtCrypto1 = lv_textarea_get_text(objects.txt_crypto1);
    const char *txtCrypto2 = lv_textarea_get_text(objects.txt_crypto2);
    const char *txtDivisa = lv_textarea_get_text(objects.txt_divisa);

    if (strlen(txtWifi) > 0)
    {
      strncpy(llaves.ssid, txtWifi, sizeof(llaves.ssid) - 1);
      llaves.ssid[sizeof(llaves.ssid) - 1] = '\0';
    }
    if (strlen(txtPass) > 0)
    {
      if (strcmp(txtPass, llaves.password) != 0)
      {
        restartSys = true;
      }
      strncpy(llaves.password, txtPass, sizeof(llaves.password) - 1);
      llaves.password[sizeof(llaves.password) - 1] = '\0';
    }
    if (strlen(txtApiW) > 0)
    {
      strncpy(llaves.keyWeather, txtApiW, sizeof(llaves.keyWeather) - 1);
      llaves.keyWeather[sizeof(llaves.keyWeather) - 1] = '\0';
    }
    if (strlen(txtCiud) > 0)
    {
      strncpy(llaves.city, txtCiud, sizeof(llaves.city) - 1);
      llaves.city[sizeof(llaves.city) - 1] = '\0';
      lv_label_set_text(objects.label_city, llaves.city);
    }
    if (strlen(txtPais) > 0)
    {
      strncpy(llaves.countryCode, txtPais, sizeof(llaves.countryCode) - 1);
      llaves.countryCode[sizeof(llaves.countryCode) - 1] = '\0';
    }
    if (strlen(txtHora) > 0)
    {
      strncpy(llaves.utm, txtHora, sizeof(llaves.utm) - 1);
      llaves.utm[sizeof(llaves.utm) - 1] = '\0';
    }
    if (strlen(txtApiC) > 0)
    {
      strncpy(llaves.keyCripto, txtApiC, sizeof(llaves.keyCripto) - 1);
      llaves.keyCripto[sizeof(llaves.keyCripto) - 1] = '\0';
    }
    if (strlen(txtCrypto1) > 0)
    {
      strncpy(llaves.criptos[0], txtCrypto1, sizeof(llaves.criptos[0]) - 1);
      llaves.criptos[0][sizeof(llaves.criptos[0]) - 1] = '\0';
    }
    if (strlen(txtCrypto2) > 0)
    {
      strncpy(llaves.criptos[1], txtCrypto2, sizeof(llaves.criptos[1]) - 1);
      llaves.criptos[1][sizeof(llaves.criptos[1]) - 1] = '\0';
    }
    if (strlen(txtDivisa) > 0)
    {
      strncpy(llaves.divisa, txtDivisa, sizeof(llaves.divisa) - 1);
      llaves.divisa[sizeof(llaves.divisa) - 1] = '\0';
    }
    lv_label_set_text(objects.label_est_conf, String("Guardado!!").c_str());

    byte posi1 = TOTAL_PERFILES_SERIAL;
    byte posi2 = TOTAL_PERFILES_SERIAL + 1;
    String texto = "Precio" + String(llaves.criptos[0]);
    strcpy(perfiles[posi1].nombre, texto.c_str());
    texto = "Precio" + String(llaves.criptos[1]);
    strcpy(perfiles[posi2].nombre, texto.c_str());
    texto = "PRECIO DE " + String(llaves.criptos[0]);
    strcpy(perfiles[posi1].descrip, texto.c_str());
    texto = "PRECIO DE " + String(llaves.criptos[1]);
    strcpy(perfiles[posi2].descrip, texto.c_str());
    strcpy(perfiles[posi1].simbolo, llaves.divisa);
    strcpy(perfiles[posi2].simbolo, llaves.divisa);

    setDatos();
  }
}
static void viewMain2_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e); //--> Obtenga el código del evento.
  if (code == LV_EVENT_CLICKED)
  {
    if (restartSys)
    {
      lv_label_set_text(objects.label_est_conf, String("Reiniciando...!!").c_str());
      delay(2000);
      ESP.restart();
    }

    obtenerTiempoNTP();
    vistaConfig = false;
    tAntUpdApiW = millis() + 3000;
    lv_label_set_text(objects.label_est_conf, String(" ").c_str());
    lv_scr_load_anim(objects.main, LV_SCR_LOAD_ANIM_OVER_RIGHT, 400, 0, false);
  }
}

static void teclado_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *textarea = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);

  if (code == LV_EVENT_CLICKED)
  {
    lv_obj_align_to(keyboard, lv_scr_act(), LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard, textarea);    // Enlazar el teclado
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Mostrar el teclado
  }
  else if (code == LV_EVENT_DEFOCUSED)
  {
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Ocultar el teclado
    lv_keyboard_set_textarea(keyboard, NULL);      // Desvincularlo si querés
  }
}
static void teclado2_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *textarea = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *keyboard = (lv_obj_t *)lv_event_get_user_data(e);

  if (code == LV_EVENT_CLICKED)
  {
    lv_obj_align_to(keyboard, lv_scr_act(), LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard, textarea);    // Enlazar el teclado
    lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Mostrar el teclado
  }
  else if (code == LV_EVENT_DEFOCUSED)
  {
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Ocultar el teclado
    lv_keyboard_set_textarea(keyboard, NULL);      // Desvincularlo si querés
  }
}
static void teclado_evento(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY)
  {
    lv_keyboard_set_textarea(objects.teclado, NULL);
    lv_obj_add_flag(objects.teclado, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(objects.teclado2, NULL);
    lv_obj_add_flag(objects.teclado2, LV_OBJ_FLAG_HIDDEN);
  }
}

//________________________________________________________________________________
void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Pon tu código de configuración aquí, para ejecutar una vez:
  Serial.begin(115200);
  delay(3000);

  //----------------------------------------
  Serial.println("\n------------");
  Serial.println("ESP32 + TFT LCD Touchscreen ILI9341 + LVGL + EEZ Studio + OpenWeatherMap");
  delay(500);

  touchscreen.begin();        // Comience el SPI para la pantalla táctil e inicie la pantalla táctil.
  touchscreen.setRotation(2); // Establezca la rotación de la pantalla táctil en modo paisajista.

  //----------------------------------------LVGL setup.
  Serial.println("Inicie la configuración de LVGL.");
  delay(500);
  lv_init(); // Start LVGL.

  draw_buf = new uint8_t[DRAW_BUF_SIZE]; // Inicialice la pantalla TFT usando la biblioteca TFT_ESPI.
  lv_display_t *disp = lv_tft_espi_create(SCREEN_HEIGHT, SCREEN_WIDTH, draw_buf, DRAW_BUF_SIZE);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

  // PANTALLA TACTIL - Inicialice un objeto de dispositivo de entrada LVGL (pantalla táctil).
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, leerTactil);

  Serial.println("Configuración de LVGL completada.");
  delay(500);

  //----------------------------------------
  ui_init(); // Integrar EEZ Studio GUI.
  getDatos();
  analogWrite(pinBrillo, valBrillo);

  inicializarTexto();
  delay(1000);

  conexionWiFi();
  delay(1000);

  obtenerTiempoNTP();
  delay(1000);

  actualizarClima();
  delay(1000);
  actualizarCriptos();

  lv_obj_add_event_cb(objects.image_icon_weather, viewMonitor1_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_view_config, viewConfig_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(objects.monitor, viewMain1_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_view_config_per, viewPerfiles_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.grap_line, viewMonitor2_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(objects.menu_perfiles, box_menuPerfiles_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(objects.menu_opciones, box_menuOpciones_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(objects.btn_volver_conf, viewMain3_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(objects.btn_00, graficar_00_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_01, graficar_01_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_02, graficar_02_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_03, graficar_03_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_04, graficar_04_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_05, graficar_05_handler, LV_EVENT_CLICKED, NULL);

  lv_obj_add_event_cb(objects.txt_ssid, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_pass, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_api_weather, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_ciudad, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_pais, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_hora, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_api_coin, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_crypto1, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_crypto2, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_divisa, teclado_handler, LV_EVENT_ALL, objects.teclado);
  lv_obj_add_event_cb(objects.txt_ymin, teclado2_handler, LV_EVENT_ALL, objects.teclado2);
  lv_obj_add_event_cb(objects.txt_ymax, teclado2_handler, LV_EVENT_ALL, objects.teclado2);
  lv_obj_add_event_cb(objects.txt_warn, teclado2_handler, LV_EVENT_ALL, objects.teclado2);
  lv_obj_add_event_cb(objects.teclado, teclado_evento, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(objects.teclado2, teclado_evento, LV_EVENT_ALL, NULL);

  lv_obj_add_event_cb(objects.brillo, new_brillo_handler, LV_EVENT_VALUE_CHANGED, objects.my_label_slider);
  lv_obj_add_event_cb(objects.btn_gurdar_cren, btn_guardaCreden_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(objects.btn_salir_cren, viewMain2_handler, LV_EVENT_CLICKED, NULL);
  delay(1000);

  char buf[6];
  uint8_t max = 250;
  uint8_t min = 1;

  uint8_t val = (max + min) - valBrillo;
  lv_slider_set_value(objects.brillo, val, LV_ANIM_OFF);

  uint8_t porc = (((max + min) - valBrillo) - min) * 100 / (max - min);
  lv_snprintf(buf, sizeof(buf), "%d%s", porc, "%");
  lv_label_set_text(objects.my_label_slider, buf);

  // debug();
}

void loop()
{
  monitorSerial();
  updateApi();

  if (vistaClima) // CLIMA
  {
    if (!vistaConfig)
    {
      unsigned long tClimaMill = millis();
      if (tClimaMill - tAntActClima >= SEGUNDERO && conect)
      {
        actualizarHora();
        tAntActClima = tClimaMill;
      }
    }
  }
  else // MONITOR
  {
    if (tAntActSerial + ACT_SERIAL <= millis())
    {
      if (vistaGrafico)
      {
        actualizarGrafico();
      }
      else
      {
        actualizarMonitor();
      }
      tAntActSerial = millis();
    }
  }
  update_UI();
  delay(1);
}

//________________________________________________________________________________
String txtEstado()
{
  String txt = "";

  for (int i = 0; i < TOTAL_CONF_PER; i++)
  {
    txt += "- Perfil ";
    txt += String(i + 1);
    txt += " --> ";
    if (verMonitor[i] < 0)
    {
      txt += "Vacio";
    }
    else
    {
      txt += String(perfiles[verMonitor[i]].nombre);
    }

    if (i != (TOTAL_CONF_PER - 1))
    {
      txt += "\n";
    }
  }
  return txt;
}

void cargarIconos()
{
  if (verMonitor[0] >= 0)
  {
    lv_img_set_src(objects.btn_00, logo_perf[perfiles[verMonitor[0]].logo]);
  }
  else
  {
    lv_img_set_src(objects.btn_00, &img_logo_lvgl);
  }

  if (verMonitor[1] >= 0)
  {
    lv_img_set_src(objects.btn_01, logo_perf[perfiles[verMonitor[1]].logo]);
  }
  else
  {
    lv_img_set_src(objects.btn_01, &img_logo_lvgl);
  }

  if (verMonitor[2] >= 0)
  {
    lv_img_set_src(objects.btn_02, logo_perf[perfiles[verMonitor[2]].logo]);
  }
  else
  {
    lv_img_set_src(objects.btn_02, &img_logo_lvgl);
  }

  if (verMonitor[3] >= 0)
  {
    lv_img_set_src(objects.btn_03, logo_perf[perfiles[verMonitor[3]].logo]);
  }
  else
  {
    lv_img_set_src(objects.btn_03, &img_logo_lvgl);
  }

  if (verMonitor[4] >= 0)
  {
    lv_img_set_src(objects.btn_04, logo_perf[perfiles[verMonitor[4]].logo]);
  }
  else
  {
    lv_img_set_src(objects.btn_04, &img_logo_lvgl);
  }

  if (verMonitor[5] >= 0)
  {
    lv_img_set_src(objects.btn_05, logo_perf[perfiles[verMonitor[5]].logo]);
  }
  else
  {
    lv_img_set_src(objects.btn_05, &img_logo_lvgl);
  }
}

void nuevosDatos(String valor, int posi)
{
  int val = round(valor.toFloat());

  for (int i = 0; i < TOTAL_PUNTOS_X - 1; i++)
  {
    perfiles[posi].datos[i] = perfiles[posi].datos[i + 1];
  }

  if (val > 0)
  {
    perfiles[posi].datos[TOTAL_PUNTOS_X - 2] = val;
  }
  else
  {
    perfiles[posi].datos[TOTAL_PUNTOS_X - 2] = 0;
  }
}

void actualizarMonitor()
{
  static lv_obj_t *misLabels[TOTAL_CONF_PER] = {
      objects.label_00,
      objects.label_01,
      objects.label_02,
      objects.label_03,
      objects.label_04,
      objects.label_05};

  char miBuffer[15];
  int ultDat = TOTAL_PUNTOS_X - 2; // posicion 17

  for (int i = 0; i < TOTAL_CONF_PER; i++)
  {
    if (verMonitor[i] < 0)
    {
      lv_label_set_text(misLabels[i], String("0").c_str());
      lv_obj_set_style_text_color(misLabels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      continue;
    }

    // --- MODIFICANDO
    String valor;
    if (strcmp(perfiles[verMonitor[i]].simbolo, llaves.divisa) == 0)
    {
      valor = formatMiles(perfiles[verMonitor[i]].datos[ultDat]);
    }
    else
    {
      valor = String(perfiles[verMonitor[i]].datos[ultDat]);
    }
    snprintf(miBuffer, sizeof(miBuffer), "%s %s", valor, perfiles[verMonitor[i]].simbolo);
    // ----

    if (perfiles[verMonitor[i]].warnUp)
    {
      if (perfiles[verMonitor[i]].datos[ultDat] > perfiles[verMonitor[i]].warn)
      {
        lv_obj_set_style_text_color(misLabels[i], lv_color_hex(0xFF0000), LV_PART_MAIN);
      }
      else
      {
        lv_obj_set_style_text_color(misLabels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      }
    }
    else
    {
      if (perfiles[verMonitor[i]].datos[ultDat] < perfiles[verMonitor[i]].warn)
      {
        lv_obj_set_style_text_color(misLabels[i], lv_color_hex(0xFF0000), LV_PART_MAIN);
      }
      else
      {
        lv_obj_set_style_text_color(misLabels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
      }
    }
    lv_label_set_text(misLabels[i], miBuffer);
  }
}

void actualizarGrafico()
{
  char miBuffer[15];
  byte ubic = verMonitor[nGraf];
  int newMaxY = perfiles[ubic].ymax;

  snprintf(miBuffer, sizeof(miBuffer), "%d %s", perfiles[ubic].datos[TOTAL_PUNTOS_X - 2], perfiles[ubic].simbolo);

  if (perfiles[ubic].ymin == 0)
  {
    if (newMaxY > 100)
    {
      int rang = newMaxY / 19;
      newMaxY = round(rang / 50.0) * 50;
      newMaxY = newMaxY * 19;
    }
    else
    {
      newMaxY = 95;
    }
  }

  int alto = lv_obj_get_height(objects.grap_line);
  int valor = perfiles[ubic].warn;
  int rango = newMaxY - perfiles[ubic].ymin;
  int y_pos = alto - ((valor - perfiles[ubic].ymin) * alto) / rango;
  lv_obj_set_pos(objects.grap_alert, 1, y_pos);

  lv_chart_set_type(objects.grap_line, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(objects.grap_line, TOTAL_PUNTOS_X - 1); // Cantidad de puntos
  lv_chart_set_range(objects.grap_line, LV_CHART_AXIS_PRIMARY_Y, perfiles[ubic].ymin, newMaxY);
  lv_chart_set_range(objects.grap_line, LV_CHART_AXIS_PRIMARY_X, 0, TOTAL_PUNTOS_X - 1);
  lv_chart_set_div_line_count(objects.grap_line, 10, TOTAL_PUNTOS_X - 1);
  lv_scale_set_range(objects.grap_scale_y, perfiles[ubic].ymin, newMaxY);

  lv_color_t miColor = lv_palette_main(LV_PALETTE_GREEN);
  static lv_chart_series_t *serie = nullptr;
  if (serie == nullptr)
  {
    serie = lv_chart_add_series(objects.grap_line, miColor, LV_CHART_AXIS_PRIMARY_Y);
  }

  if (perfiles[ubic].warnUp)
  {
    if (perfiles[ubic].datos[TOTAL_PUNTOS_X - 2] > perfiles[ubic].warn)
    {
      miColor = lv_palette_main(LV_PALETTE_RED);
    }
  }
  else
  {
    if (perfiles[ubic].datos[TOTAL_PUNTOS_X - 2] < perfiles[ubic].warn)
    {
      miColor = lv_palette_main(LV_PALETTE_RED);
    }
  }

  lv_label_set_text(objects.label_texto, miBuffer);
  lv_obj_set_style_text_color(objects.label_texto, miColor, LV_PART_MAIN);
  lv_chart_set_series_color(objects.grap_line, serie, miColor);

  // Asignar valores a la serie
  for (int i = 0; i < TOTAL_PUNTOS_X - 1; i++)
  {
    serie->y_points[i] = perfiles[ubic].datos[i];
  }

  // Refrescar el gráfico
  lv_chart_refresh(objects.grap_line);
}

void guardDatY()
{
  lv_label_set_text(objects.label_estado, "");
  if (newAtajo >= 0)
  {

    int menuId = verMonitor[newAtajo];
    if (menuId < 0)
      return;

    const char *txtymin = lv_textarea_get_text(objects.txt_ymin);
    const char *txtymax = lv_textarea_get_text(objects.txt_ymax);
    const char *txtwarn = lv_textarea_get_text(objects.txt_warn);

    if (strlen(txtymin) > 0)
    {
      perfiles[menuId].ymin = atoi(txtymin);
    }
    if (strlen(txtymax) > 0)
    {
      perfiles[menuId].ymax = atoi(txtymax);
    }
    if (strlen(txtwarn) > 0)
    {
      perfiles[menuId].warn = atoi(txtwarn);
    }
    setDatos();
  }
}

void updateApi()
{
  unsigned long tUpdatApi = millis();
  if (vistaClima)
  {
    if (tAntUpdApiW + INTERVAL_UPDATING_APIW < tUpdatApi)
    {
      actualizarClima();
      tAntUpdApiW = tUpdatApi;
    }
  }

  if (tAntUpdApiC + INTERVAL_UPDATING_APIC < tUpdatApi)
  {
    actualizarCriptos();
    tAntUpdApiC = tUpdatApi;
  }
}

String formatMiles(unsigned long num)
{
  String str = String(num);
  int len = str.length();

  // Insertar puntos desde el final
  for (int i = len - 3; i > 0; i -= 3)
  {
    str = str.substring(0, i) + "." + str.substring(i);
  }

  return str;
}

/*
void debug()
{
  Serial.println("Credenciales:");
  Serial.println(String(llaves.ssid));
  Serial.println(String(llaves.password));
  Serial.println(String(llaves.keyCripto));
  Serial.println(String(llaves.criptos[0]));
  Serial.println(String(llaves.criptos[1]));
  Serial.println(String(llaves.divisa));
  Serial.println(String(llaves.keyWeather));
  Serial.println(String(llaves.city));
  Serial.println(String(llaves.countryCode));
  Serial.println(String(llaves.utm));

  Serial.print("verMonitor: [");
  for (int i = 0; i < 6; i++)
  {
    Serial.print(verMonitor[i]);
    Serial.print(", ");
  }
  Serial.println("]");

  Serial.print("Datos[");
  for (int i = 0; i < TOTAL_PERFILES; i++)
  {
    for (int j = 0; j < TOTAL_PUNTOS_X; j++)
    {
      Serial.print(perfiles[i].datos[j]);
      Serial.print(",");
    }
    Serial.println("]");
  }
}
*/