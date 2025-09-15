#include "gps-neo6m.h"

String LATval = "######";
String LNGval = "######";
char inChar;
String gpsData;
String latt;
String la;
String lonn;
String lo;
float lattt;
float lonnn;
float latitude;
float longitude;
int latDeg;
int lonDeg;
float latMin;
float lonMin;
float latttt;
float lonnnn;
String sGPRMC;

String tipo_mensaje;
String hora_utc;
char valido;
float f_latitud;
char hemisferio_latitud;
float f_longitud;
char hemisferio_longitud;
float velocidad;
float rumbo;
String fecha;
float variacion_magnetica;
char direccion_variacion_magnetica;

float latitud = 0;
float longitud = 0;

extern uint32_t stateVariables;

float nmeaToSingleFloat(float decimalCoordinate, char hemisphere)
{
    // Calcular los grados
    int degrees = (int)(decimalCoordinate / 100);

    // Calcular los minutos (parte decimal de la coordenada)
    float minutes = (decimalCoordinate / 100 - degrees) * 100;

    // Aplicar signo negativo si es necesario
    if (hemisphere == 'S' || hemisphere == 'W')
    {
        degrees *= -1;
        minutes *= -1;
    }

    // Combinar los grados y los minutos
    float result = degrees + (minutes / 60);

    return result;
}

void gpsdata(void)
{
    /* ejemplo de respuesta gps
    $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a\*hh
    $GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A,V*57
    */
//    DEBUG_NL("[gpsData]");
    String dataTosend;
    while (GPS_SERIAL.available())
    {
        inChar = GPS_SERIAL.read();
        gpsData += inChar;
        dataTosend += inChar;
        if (inChar == '$')
        {
            gpsData = GPS_SERIAL.readStringUntil('\n');
            dataTosend = gpsData;
            // DEBUG_NL("%s", gpsData);
            break;
        }
    }

    sGPRMC = gpsData.substring(0, 5);
    if (sGPRMC == "GPRMC")
    {
        GPS_SERIAL.flush();
    }
    else
    {
        return;
    }
    char mensajeChars[256];
    memset(mensajeChars, 0, sizeof(mensajeChars));
    gpsData.toCharArray(mensajeChars, gpsData.length() + 1);
    char *token = strtok(mensajeChars, ",");

    int data_position = 0;
    bool err = false;
    while (token != NULL)
    {
        switch (data_position)
        {
        case 0:
        {
            tipo_mensaje = String(token);
        }
        break;
        case 1:
            hora_utc = String(token);
            break;
        case 2:
        {
            valido = token[0];
            stateVariables |= GPS_DATA_VALID;
            if (valido == 'V')
            {
                err = true;
                stateVariables &= ~GPS_DATA_VALID;
            }
        }
        break;
        case 3:
            f_latitud = atof(token);
            break;
        case 4:
            hemisferio_latitud = token[0];
            break;
        case 5:
            f_longitud = atof(token);
            break;
        case 6:
            hemisferio_longitud = token[0];
            break;
        case 7:
            velocidad = atof(token);
            break;
        case 8:
            rumbo = atof(token);
            break;
        case 9:
            fecha = String(token);
            break;
        case 10:
            variacion_magnetica = atof(token);
            break;
        case 11:
            direccion_variacion_magnetica = token[0];
            break;
        }
        if (err && data_position == 2)
        {
            // DEBUG_NL("Data invalid");
            return;
        }
        token = strtok(NULL, ",");
        data_position++;
    }
    latitud = nmeaToSingleFloat(f_latitud, hemisferio_latitud);
    longitud = nmeaToSingleFloat(f_longitud, hemisferio_longitud);
}