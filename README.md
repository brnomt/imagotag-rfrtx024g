# Panel E-ink VUSION 2.66" (SE2266JS0C1) en ESP32-C3 Super Mini

Firmware mínimo de prueba para un panel EPD rescatado de una etiqueta
Imagotag / SES-imagotag VUSION, manejado por SPI por hardware desde un
ESP32-C3 Super Mini.

## Qué panel es

`SE226JS0C1` es casi seguro **`SE2266JS0C1`** (falta un dígito). La nomenclatura
de Pervasive Displays es `[S]E[2][tamaño 3 dígitos][film+driver][serie][x]`, así
que `SE 2 266 JS0 C 1` = panel de **2.66"**, film **J** (Spectra BWR),
driver **S0** (iTC), revisión C1.

| Dato | Valor |
|---|---|
| Resolución | 296 × 152 (125 dpi) |
| Colores | negro / blanco / rojo (Spectra Red R2.0) |
| Área activa | 60.09 × 30.70 mm |
| Contorno | 71.82 × 36.3 × ~0.9 mm |
| Controlador | **Pervasive iTC** (no es SSD16xx ni UC8151) |
| Temperatura | 0…40 °C |
| Equivalente comercial | E2266JS0C1 (Pervasive Displays) |

**El PDF que enviaste no es el datasheet del panel**: es el datasheet del
EFR32FG22E (Wireless Gecko), el SoC de radio que va dentro de la etiqueta. El
datasheet real del panel está detrás de un formulario en el sitio de Pervasive.

## Librería: cuál sirve y cuál no

- **GxEPD2 no sirve** para este panel: solo soporta controladores tipo
  SSD/UC/IL. No hay driver para iTC.
- La librería oficial es **PDLS_Basic** de Pervasive Displays
  (`PervasiveDisplays/PDLS_Basic`), pensada para pantallas iTC, más las
  dependencias `PDLS_Common` y `Pervasive_BWRY_Small`. Es la referencia que usé
  para escribir el driver de este repo; su HAL asume sus propias placas, así que
  integrarla en un ESP32 custom requiere definir un `pins_t`.
- Este repo implementa el subconjunto necesario del protocolo iTC directamente,
  sin dependencias, para que puedas validar el panel en minutos.

## Protocolo iTC (lo que hace el firmware)

| Paso | Comando | Detalle |
|---|---|---|
| Soft reset | `0x00` ← `0x0E` | esperar 5 ms |
| Temperatura | `0xE5` ← `0x19` | 0 °C = `0x00`, 22 °C = `0x16`, 25 °C = `0x19` |
| Modo temperatura | `0xE0` ← `0x02` | usar la anterior |
| Panel setting (PSR) | `0x00` ← `0xCF 0x8D` | valores de Pervasive para C/J small |
| Plano negro | `0x10` | 5624 bytes |
| Plano rojo | `0x13` | 5624 bytes |
| DC/DC on | `0x04` | esperar BUSY |
| Refresco | `0x12` | ~15 s en BWR, esperar BUSY |
| DC/DC off | `0x02` | la imagen queda retenida |

Detalles que suelen romper la implementación:

- **BUSY es activo a nivel bajo** en iTC (LOW = ocupado), al revés que SSD/UC.
- Comando y sus datos van en **un solo frame de CS** (D/C baja para el comando,
  sube para los datos). Si los separas en dos transacciones, el panel ignora la
  escritura.
- El buffer es **column-major**: 296 columnas × 19 bytes (152 bits), MSB = línea
  superior. `index = x*19 + (y>>3)`, bit `7-(y&7)`. Un buffer row-major sale
  rotado 90°.
- Codificación: plano negro `1 = blanco`, `0 = negro`; plano rojo `1 = rojo`.
  Blanco = `1,0`; negro = `0,0`; rojo = `0,1`.
- El OTP se lee por SPI de 3 hilos (SDA bidireccional). No hace falta para
  arrancar: el firmware usa los valores fijos de PSR.

## Cableado ESP32-C3 Super Mini → panel

| Señal del panel | ESP32-C3 | Notas |
|---|---|---|
| SCL | GPIO 4 | SCK (SPI2/FSPI por matrix) |
| SDA | GPIO 6 | MOSI; el panel solo escucha salvo la lectura de OTP |
| CS  | GPIO 7 | chip select |
| D/C | GPIO 5 | bajo = comando, alto = dato |
| RES | GPIO 1 | reset activo bajo (pulso de 10 ms) |
| BUSY| GPIO 0 | entrada; **bajo = ocupado** |
| VDD / VDDIO | 3V3 | 100 nF + 10 µF junto al flex |
| GND | GND | — |
| VPP, C1…C4 | — | condensadores del booster: van en la PCB de la etiqueta |

GPIO 2/8/9 son los strapping pins del C3 y se evitan. GPIO 18/19 son USB y
20/21 UART0, así que el mapeo está libre. El SPI2 por defecto del C3 es
CLK=6/MOSI=7/MISO=2, por eso `SPI.begin(SCK, -1, MOSI, -1)` pasa los pines
explícitos: nunca llames `SPI.begin()` sin argumentos.

**Ojo con los pines del flex que no son señal.** El FPC de 24 contactos lleva
además alimentación y los condensadores del charge pump. Si arrancaste el panel
de la etiqueta, esos condensadores estaban en la PCB del label (cerca del
conector), no en el panel: sin ellos el booster no arranca y no hay refresco
aunque el SPI esté perfecto. Revísalos y replica 1 µF por pin de bomba según el
*Application Note* de Pervasive para paneles small-size.

## Compilar y usar

```bash
pio run -t upload
pio device monitor        # 115200
```

Al arrancar: init → limpieza a blanco → patrón de prueba (marco negro, damero,
banda roja, barras). El primer refresco tarda ~15 s; es normal.

Consola serie:

| Tecla | Acción |
|---|---|
| `w` | pantalla blanca |
| `k` | pantalla negra |
| `r` | pantalla roja |
| `p` | patrón de prueba |
| `i` | re-inicializar el panel |
| `s` | DC/DC off (imagen retenida) |

## `platformio.ini`

- `board = esp32-c3-devkitm-1`: 4 MB de flash, RISC-V. No hay board oficial
  "super mini"; este es el devkit equivalente.
- `ARDUINO_USB_CDC_ON_BOOT=1` + `ARDUINO_USB_MODE=1`: Serial por el USB-C
  nativo de la Super Mini (no hay USB-UART en la placa).
- `EPD_CONTROLLER`, `EPD_WIDTH`, `EPD_HEIGHT`: eligen protocolo y geometría sin
  tocar código; se leen en `include/epd_config.h`.
- `upload_speed = 921600`: flasheo rápido. Si falla, baja a 115200.

## Otros controladores

Si cambias de panel, `-D EPD_CONTROLLER=1` (SSD1619/SSD1680) y `=2`
(UC8151/IL0373) siguen implementados en `src/main.cpp`, con BUSY activo alto y
buffer row-major. Ajusta `EPD_WIDTH`/`EPD_HEIGHT` según el panel.

## Diagnóstico

- **BUSY no cambia nunca**: en iTC debe estar en alto al reposo y bajar durante
  el refresco. Si está siempre bajo, revisa RES, alimentación y que el booster
  tenga sus condensadores.
- **No refresca pero el SPI se ve bien**: falta el frame CS único de
  comando+datos, o el PSR no es el correcto para el film.
- **Imagen rotada 90° o en franjas verticales**: buffer row-major en un panel
  iTC (o al revés).
- **Colores cambiados**: recuerda `1,0` = blanco, `0,0` = negro, `0,1` = rojo.
- **Refresco parcial / fantasmas**: sube el tiempo de espera y haz dos refrescos
  seguidos (`epdShowFrame(true)`).
- **Corriente**: el refresco BWR pide picos de decenas de mA; 100 µF + 10 µF
  junto al conector y una fuente USB decente.

## Advertencias

- No conectes ni desconectes el flex con alimentación puesta.
- No toques los voltajes de waveform del PSR "a ver qué pasa": se degrada el
  panel de forma irreversible.
- Cada refresco completo consume ciclos del panel; para pruebas usa el patrón,
  no un bucle continuo.

## Fuentes

- [Pervasive Displays — guía de nomenclatura de modelos](https://www.pervasivedisplays.com/product/model-name-explanation/)
- [Pervasive Displays — 2.66" E-Ink Displays](https://www.pervasivedisplays.com/products/2-66-e-ink-displays/)
- [E2266JS0C1 en DigiKey (296×152, BWR)](https://www.digikey.com/en/products/detail/pervasive-displays/E2266JS0C1/13572401)
- [PDLS_Basic — librería para pantallas iTC](https://github.com/PervasiveDisplays/PDLS_Basic)
- [PDLS_EXT3_Basic_Global — driver iTC C/J (secuencias y formato de buffer)](https://github.com/PervasiveDisplays/PDLS_EXT3_Basic_Global)
