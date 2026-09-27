**Ejemplo simple de actualización de firmware OTA**
Entorno ESP32 + IDF
La aplicación se conecta a internet y descarga el binario alojado en este mismo repositorio el cual contiene a la version anterior. En ella la rutina de actualización OTA no está implementada. 

*Importante*
Antes de flashear, verificar las siguientes configuraciones en SDK Configuration Editor:
- Partition Table > Opción "Factory App, two OTA definitions"
- Serial flasher config > Flash size > Opción "4MB" o la que corresponda para el proyecto