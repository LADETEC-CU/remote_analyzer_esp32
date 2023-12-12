extern int interval;

#define PIN_LUZ 19
#define PIN_COCINA 21

/* Modbus stuff */
#define MODBUS_DIR_PIN 4        // connect DR, RE pin of MAX485 to gpio 4
#define MODBUS_RX_PIN 18        // Rx pin
#define MODBUS_TX_PIN 5         // Tx pin
#define MODBUS_SERIAL_BAUD 9600 // Baud rate for esp32 and max485 communication
#define START_ADDRESS 0x280     // Define the starting address of the holding registers
#define NUM_WORDS 12            // Registers to be read in a master transaction
#define SLAVE_ID 1              // modbus slave ID 1

#define TC_RATIO 50

#define DEBUG true
#define DEBUG_PRINT(...)                \
    do                                  \
    {                                   \
        if (DEBUG)                      \
        {                               \
            Serial.printf(__VA_ARGS__); \
        }                               \
    } while (0)
#define DEBUG_PRINTLN(...)               \
    do                                   \
    {                                    \
        if (DEBUG)                       \
        {                                \
            Serial.println(__VA_ARGS__); \
        }                                \
    } while (0)