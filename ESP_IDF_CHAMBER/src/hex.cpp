#include <hex.h>

// Function to convert an array of hex values to an ASCII string
void hex_array_to_ascii(const uint8_t *hex_array, size_t len, char *ascii_str) {
    for (size_t i = 0; i < len; i++) {
        ascii_str[i] = (char)hex_array[i];
    }
    ascii_str[len] = '\0'; // Null-terminate the ASCII string
}

// Function to convert a hexadecimal character to its integer value
uint8_t hex_char_to_int(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return 0; // Error case, should not happen for valid input
    }
}

// Function to convert a two-character hexadecimal string to an integer value
uint16_t hex_string_to_int(const char* hex_str) {
    return (hex_char_to_int(hex_str[0]) << 12) |
           (hex_char_to_int(hex_str[1]) << 8)  |
           (hex_char_to_int(hex_str[2]) << 4)  |
           hex_char_to_int(hex_str[3]);
}

void convertData(const uint8_t *hex_array, size_t len, ChamberData *data)
{
    char ascii_str[len + 1]; // +1 for the null-terminator

    hex_array_to_ascii(hex_array, len, ascii_str);

    // Extract the temperature values as hexadecimal strings
    char tempPV_hex[5] = {ascii_str[7], ascii_str[8], ascii_str[9], ascii_str[10], '\0'};
    char tempSP_hex[5] = {ascii_str[11], ascii_str[12], ascii_str[13], ascii_str[14], '\0'};
    char wetPV_hex[5] = {ascii_str[15], ascii_str[16], ascii_str[17], ascii_str[18], '\0'};
    char wetSP_hex[5] = {ascii_str[19], ascii_str[20], ascii_str[21], ascii_str[22], '\0'};
    char humiPV_hex[5] = {ascii_str[23], ascii_str[24], ascii_str[25], ascii_str[26], '\0'};
    char humiSP_hex[5] = {ascii_str[27], ascii_str[28], ascii_str[29], ascii_str[30], '\0'};
    char nowSTS_hex[5] = {ascii_str[43], ascii_str[44], ascii_str[45], ascii_str[46], '\0'};

    // Convert the hexadecimal strings to integer values
    uint16_t tempPV = hex_string_to_int(tempPV_hex);
    uint16_t tempSP = hex_string_to_int(tempSP_hex);
    uint16_t wetPV = hex_string_to_int(wetPV_hex);
    uint16_t wetSP = hex_string_to_int(wetSP_hex);
    uint16_t humiPV = hex_string_to_int(humiPV_hex);
    uint16_t humiSP = hex_string_to_int(humiSP_hex);
    uint16_t nowSTS = hex_string_to_int(nowSTS_hex);

    // Convert the hexadecimal strings to integer values
    data->tempPV = hex_string_to_int(tempPV_hex) / 100.0;
    data->tempSP = hex_string_to_int(tempSP_hex) / 100.0;
    data->wetPV = hex_string_to_int(wetPV_hex) / 100.0;
    data->wetSP = hex_string_to_int(wetSP_hex) / 100.0;
    data->humiPV = hex_string_to_int(humiPV_hex) / 10.0;
    data->humiSP = hex_string_to_int(humiSP_hex) / 10.0;
    data->nowSTS = hex_string_to_int(nowSTS_hex);
}