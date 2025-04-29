#define MAX_ARCHIVOS 20
#define MAX_NOMBRE_ARCHIVO 50
char listaNombresArchivos[MAX_ARCHIVOS][MAX_NOMBRE_ARCHIVO];
uint8_t cantidadArchivos = 0;
char mensajeUART[100];

void transmitir_uart(char *texto) {
    HAL_UART_Transmit(&huart2, (uint8_t*)texto, strlen(texto), HAL_MAX_DELAY);
}

void leer_linea_uart(char *entrada, uint16_t max_largo) {
    char caracter;
    uint16_t i = 0;
    while (i < max_largo - 1) {
        HAL_UART_Receive(&huart2, (uint8_t*)&caracter, 1, HAL_MAX_DELAY);
        if (caracter == '\r' || caracter == '\n') break;
        entrada[i++] = caracter;
        HAL_UART_Transmit(&huart2, (uint8_t*)&caracter, 1, HAL_MAX_DELAY);
    }
    entrada[i] = '\0';
    transmitir_uart("\r\n");
}

void leer_y_mostrar_archivo(char *nombreArchivo) {
    FIL archivoSD;
    FRESULT resultado = f_open(&archivoSD, nombreArchivo, FA_READ);
    if (resultado != FR_OK) {
        transmitir_uart("Error: No se pudo abrir el archivo\r\n");
        return;
    }

    char lineaArchivo[100];
    while (f_gets(lineaArchivo, sizeof(lineaArchivo), &archivoSD)) {
        snprintf(mensajeUART, sizeof(mensajeUART), "%s\r\n", lineaArchivo);
        transmitir_uart(mensajeUART);
    }
    f_close(&archivoSD);
    transmitir_uart("Archivo cerrado.\r\n");
}

void menu_tarjeta_sd(void) {
    if (f_mount(&fs, "", 1) != FR_OK) {
        transmitir_uart("No se pudo montar la SD\r\n");
        return;
    }

    transmitir_uart("SD montada correctamente\r\n");
    DIR directorioSD;
    FILINFO infoArchivo;
    cantidadArchivos = 0;

    if (f_opendir(&directorioSD, "/") == FR_OK) {
        transmitir_uart("Archivos encontrados:\r\n");

        while (cantidadArchivos < MAX_ARCHIVOS && f_readdir(&directorioSD, &infoArchivo) == FR_OK && infoArchivo.fname[0]) {
            if (!(infoArchivo.fattrib & AM_DIR)) {
                snprintf(listaNombresArchivos[cantidadArchivos], MAX_NOMBRE_ARCHIVO, "%s", infoArchivo.fname);
                snprintf(mensajeUART, sizeof(mensajeUART), "%d. %s (%lu bytes)\r\n", cantidadArchivos + 1, infoArchivo.fname, (uint32_t)infoArchivo.fsize);
                transmitir_uart(mensajeUART);
                cantidadArchivos++;
            }
        }
        f_closedir(&directorioSD);
    } else {
        transmitir_uart("No se pudo abrir el directorio\r\n");
    }

    if (cantidadArchivos == 0) {
        transmitir_uart("La SD está vacía\r\n");
    } else {
        transmitir_uart("Ingrese el número del archivo que desea leer:\r\n");
        char entradaUsuario[10];
        leer_linea_uart(entradaUsuario, sizeof(entradaUsuario));
        uint8_t seleccion = atoi(entradaUsuario);

        if (seleccion > 0 && seleccion <= cantidadArchivos) {
            leer_y_mostrar_archivo(listaNombresArchivos[seleccion - 1]);
        } else {
            transmitir_uart("Número inválido\r\n");
        }
    }

    f_mount(NULL, "", 1);  // desmontar
    transmitir_uart("SD desmontada\r\n");
}
