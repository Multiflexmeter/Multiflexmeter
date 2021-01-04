#if !defined(_CONFIG_H_)
#define _CONFIG_H_

/*
  Generic configuration
*/
// Whether to output debug information to the serial output
// #define DEBUG
// Sleep duration (-> measurement interval) in seconds
#define SLEEP_DURATION 15000
// The lowest DR (thus highest SF) the device will join and TX at
#define MIN_LORA_DR LORAWAN_DR3
// Whether to print the firmware its build date/time to the serial on power on
#define PRINT_BUILD_DATE_TIME
// Milliseconds before measurement times out to prevent hanging
#define SENSOR_JSN_TIMEOUT 1000

/*
  Board configuration (pins)
*/
#define PIN_JSN_SDN 2
#define PIN_JSN_TX 3
#define PIN_JSN_RX 4
#define PIN_ONE_WIRE 5
#define PIN_NSS 10
#define PIN_RST 9
#define PIN_DIO_0 8
#define PIN_DIO_1 7
#define PIN_DIO_2 6

#endif // _CONFIG_H_