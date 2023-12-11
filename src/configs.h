extern int interval;

#define PIN_LUZ 19
#define PIN_COCINA 21

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