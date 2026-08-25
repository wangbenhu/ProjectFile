#include "wifi_config_codec.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIFI_LAT_TYPE_HEADER "[WIFI_LAT_TYPE]"
#define WIFI_LON_TYPE_HEADER "[WIFI_LON_TYPE]"

#define WIFI_SSID_HEADER "[WIFI_SSID]"//WIFI SSID
#define WIFI_MAC_HEADER  "[WIFI_MAC]"//WIFI MAC地址
#define WIFI_LAT_HEADER  "[WIFI_LAT]"//纬度
#define WIFI_LON_HEADER  "[WIFI_LON]"//经度

enum {
    WIFI_FIELD_SSID = 1U << 0,
    WIFI_FIELD_MAC  = 1U << 1,
    WIFI_FIELD_LAT  = 1U << 2,
    WIFI_FIELD_LON  = 1U << 3,
    WIFI_FIELD_LAT_TYPE = 1U << 4,
    WIFI_FIELD_LON_TYPE = 1U << 5
};

static size_t bounded_string_length(const char *text, size_t capacity)
{
    size_t length;

    for (length = 0; length < capacity && text[length] != '\0'; ++length) {
    }
    return length;
}

static int contains_line_break(const char *text, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i) {
        if (text[i] == '\r' || text[i] == '\n') {
            return 1;
        }
    }
    return 0;
}

static int valid_mac(const char *mac, size_t length)
{
    size_t i;

    if (length != WIFI_MAC_STR_LEN) {
        return 0;
    }
    for (i = 0; i < length; ++i) {
        if (!isxdigit((unsigned char)mac[i])) {
            return 0;
        }
    }
    return 1;
}

static int number_is_finite(double value)
{
    return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

static int valid_fence_count(uint8_t count)
{
    return count == 0U || (count >= 3U && count <= FENCE_MAX_POINTS);
}

static int valid_polygon(const GeoPolygon_t *polygon)
{
    uint8_t i;

    if (!valid_fence_count(polygon->count)) {
        return 0;
    }
    for (i = 0U; i < polygon->count; ++i) {
        size_t lat_len = bounded_string_length(polygon->points[i].latitude,
                                               sizeof(polygon->points[i].latitude));
        size_t lon_len = bounded_string_length(polygon->points[i].longitude,
                                               sizeof(polygon->points[i].longitude));
        if (lat_len == 0U || lat_len >= sizeof(polygon->points[i].latitude) ||
            lon_len == 0U || lon_len >= sizeof(polygon->points[i].longitude) ||
            contains_line_break(polygon->points[i].latitude, lat_len) ||
            contains_line_break(polygon->points[i].longitude, lon_len)) {
            return 0;
        }
    }
    return 1;
}

static int append_fence_line(char *output,
                             size_t output_size,
                             size_t *written,
                             const char *format,
                             const char *name,
                             unsigned int index,
                             const char *value)
{
    int result;

    result = snprintf(output + *written, output_size - *written,
                      format, name, index, value);
    if (result < 0 || (size_t)result >= output_size - *written) {
        return -1;
    }
    *written += (size_t)result;
    return 0;
}

static int append_fence_polygon(const GeoPolygon_t *polygon,
                                const char *name,
                                char *output,
                                size_t output_size,
                                size_t *written)
{
    uint8_t i;
    int result;

    result = snprintf(output + *written, output_size - *written,
                      "[%s_COUNT]%u\r\n", name, (unsigned int)polygon->count);
    if (result < 0 || (size_t)result >= output_size - *written) {
        return -1;
    }
    *written += (size_t)result;
    for (i = 0U; i < polygon->count; ++i) {
        if (append_fence_line(output, output_size, written,
                              "[%s_%u_LAT]%s\r\n", name, (unsigned int)i,
                              polygon->points[i].latitude) < 0 ||
            append_fence_line(output, output_size, written,
                              "[%s_%u_LON]%s\r\n", name, (unsigned int)i,
                              polygon->points[i].longitude) < 0) {
            return -1;
        }
    }
    return 0;
}

int fence_config_encode(const FenceConfig_t *config,
                        char *output,
                        size_t output_size)
{
    size_t written = 0U;

    if (!output || output_size == 0U) {
        return -1;
    }
    output[0] = '\0';
    if (!config || config->fence_id == 0U || config->fence_id > 10U ||
        !valid_polygon(&config->fence) ||
        !valid_polygon(&config->safe_zone)) {
        return -2;
    }
    if (snprintf(output, output_size, "[FENCE_ID]%u\r\n",
                 (unsigned int)config->fence_id) < 0) {
        return -3;
    }
    written = strlen(output);
    if (append_fence_polygon(&config->fence, "FENCE", output, output_size,
                             &written) < 0 ||
        append_fence_polygon(&config->safe_zone, "SAFE", output, output_size,
                             &written) < 0) {
        output[0] = '\0';
        return -3;
    }
    return (int)written;
}

static int parse_fence_count(const char *value, size_t value_len,
                             uint8_t *count)
{
    if (value_len == 1U && value[0] >= '0' && value[0] <= '9') {
        *count = (uint8_t)(value[0] - '0');
    } else if (value_len == 2U && value[0] == '1' && value[1] == '0') {
        *count = 10U;
    } else {
        return -1;
    }
    return valid_fence_count(*count) ? 0 : -1;
}

static int parse_fence_coordinate(const char *value, size_t value_len,
                                  char *coordinate, size_t coordinate_size)
{
    if (value_len == 0U || value_len >= coordinate_size ||
        contains_line_break(value, value_len)) {
        return -1;
    }
    memcpy(coordinate, value, value_len);
    coordinate[value_len] = '\0';
    return 0;
}

static int parse_fence_header(const char *line, size_t line_len,
                              const char **value, size_t *value_len,
                              int *is_safe, int *is_count,
                              uint8_t *index, int *is_longitude)
{
    static const char fence_id[] = "[FENCE_ID]";
    static const char fence_count[] = "[FENCE_COUNT]";
    static const char safe_count[] = "[SAFE_COUNT]";
    size_t name_len;
    size_t header_len;

    if (line_len >= sizeof(fence_id) - 1U &&
        memcmp(line, fence_id, sizeof(fence_id) - 1U) == 0) {
        *is_safe = 0;
        *is_count = 2;
        header_len = sizeof(fence_id) - 1U;
    } else if (line_len >= sizeof(fence_count) - 1U &&
        memcmp(line, fence_count, sizeof(fence_count) - 1U) == 0) {
        *is_safe = 0;
        *is_count = 1;
        header_len = sizeof(fence_count) - 1U;
    } else if (line_len >= sizeof(safe_count) - 1U &&
               memcmp(line, safe_count, sizeof(safe_count) - 1U) == 0) {
        *is_safe = 1;
        *is_count = 1;
        header_len = sizeof(safe_count) - 1U;
    } else {
        if (line_len < 13U || line[0] != '[') {
            return -1;
        }
        if (memcmp(line, "[FENCE_", 7U) == 0) {
            name_len = 5U;
            *is_safe = 0;
        } else if (memcmp(line, "[SAFE_", 6U) == 0) {
            name_len = 4U;
            *is_safe = 1;
        } else {
            return -1;
        }
        header_len = name_len + 8U;
        if (line_len < header_len || line[name_len + 2U] < '0' ||
            line[name_len + 2U] > '9' || line[name_len + 3U] != '_' ||
            line[header_len - 1U] != ']') {
            return -1;
        }
        *index = (uint8_t)(line[name_len + 2U] - '0');
        if (memcmp(line + name_len + 4U, "LAT]", 4U) == 0) {
            *is_longitude = 0;
        } else if (memcmp(line + name_len + 4U, "LON]", 4U) == 0) {
            *is_longitude = 1;
        } else {
            return -1;
        }
        *is_count = 0;
    }
    *value = line + header_len;
    *value_len = line_len - header_len;
    return 0;
}

int fence_config_decode(const char *input,
                        size_t input_len,
                        FenceConfig_t *config)
{
    const char *cursor;
    const char *end;
    FenceConfig_t parsed;
    uint32_t seen_fence = 0U;
    uint32_t seen_safe = 0U;
    unsigned int counts_seen = 0U;
    uint8_t i;

    if (!config) {
        return -1;
    }
    memset(config, 0, sizeof(*config));
    memset(&parsed, 0, sizeof(parsed));
    if (!input || input_len == 0U || input_len >= FENCE_CONFIG_TEXT_MAX_SIZE ||
        memchr(input, '\0', input_len) != NULL) {
        return -2;
    }
    cursor = input;
    end = input + input_len;
    while (cursor < end) {
        const char *newline = memchr(cursor, '\n', (size_t)(end - cursor));
        const char *value;
        size_t line_len;
        size_t value_len;
        int is_safe;
        int is_count;
        int is_longitude = 0;
        uint8_t index = 0U;
        GeoPolygon_t *polygon;
        char *coordinate;
        uint32_t *seen;
        uint32_t bit;

        if (!newline || newline == cursor || newline[-1] != '\r') {
            return -3;
        }
        line_len = (size_t)((newline - 1) - cursor);
        if (parse_fence_header(cursor, line_len, &value, &value_len, &is_safe,
                               &is_count, &index, &is_longitude) < 0) {
            return -4;
        }
        polygon = is_safe ? &parsed.safe_zone : &parsed.fence;
        seen = is_safe ? &seen_safe : &seen_fence;
        if (is_count == 2) {
            if ((counts_seen & 4U) != 0U ||
                !((value_len == 1U && value[0] >= '1' && value[0] <= '9') ||
                  (value_len == 2U && value[0] == '1' && value[1] == '0'))) {
                return -5;
            }
            parsed.fence_id = (value_len == 1U) ? (uint8_t)(value[0] - '0') : 10U;
            counts_seen |= 4U;
        } else if (is_count) {
            unsigned int count_bit = is_safe ? 2U : 1U;

            if ((counts_seen & count_bit) != 0U ||
                parse_fence_count(value, value_len, &polygon->count) < 0) {
                return -5;
            }
            counts_seen |= count_bit;
        } else {
            bit = 1U << (2U * index + (is_longitude ? 1U : 0U));
            coordinate = is_longitude ? polygon->points[index].longitude :
                                        polygon->points[index].latitude;
            if ((*seen & bit) != 0U ||
                parse_fence_coordinate(value, value_len, coordinate,
                                       FENCE_COORD_STR_LEN + 1U) < 0) {
                return -5;
            }
            *seen |= bit;
        }
        cursor = newline + 1;
    }
    if (counts_seen != 7U) {
        return -6;
    }
    for (i = 0U; i < FENCE_MAX_POINTS; ++i) {
        uint32_t expected = 3U << (2U * i);
        if ((i < parsed.fence.count && (seen_fence & expected) != expected) ||
            (i >= parsed.fence.count && (seen_fence & expected) != 0U) ||
            (i < parsed.safe_zone.count && (seen_safe & expected) != expected) ||
            (i >= parsed.safe_zone.count && (seen_safe & expected) != 0U)) {
            return -6;
        }
    }
    if (!valid_polygon(&parsed.fence) || !valid_polygon(&parsed.safe_zone)) {
        return -6;
    }
    *config = parsed;
    return 0;
}

static int decimal_coordinate_exceeds_maximum(const char *text,
                                              size_t integer_length,
                                              size_t length,
                                              double maximum)
{
    char maximum_text[sizeof("180")];
    const char *integer = text;
    size_t significant_length = integer_length;
    size_t maximum_length;
    size_t i;
    int result;

    while (significant_length > 1U && integer[0] == '0') {
        ++integer;
        --significant_length;
    }
    result = snprintf(maximum_text, sizeof(maximum_text), "%u",
                      (unsigned int)maximum);
    if (result < 0 || (size_t)result >= sizeof(maximum_text)) {
        return 1;
    }
    maximum_length = (size_t)result;
    if (significant_length != maximum_length) {
        return significant_length > maximum_length;
    }
    result = memcmp(integer, maximum_text, maximum_length);
    if (result != 0) {
        return result > 0;
    }
    for (i = integer_length + 1U; i < length; ++i) {
        if (text[i] != '0') {
            return 1;
        }
    }
    return 0;
}

static int parse_unsigned_coordinate(const char *text,
                                     size_t length,
                                     double maximum,
                                     double *numeric_value)
{
    char number[WIFI_COORD_STR_LEN + 1U];
    char *parse_end;
    double value;
    size_t i = 0U;
    size_t integer_length;

    if (!text || !numeric_value || length == 0U ||
        length > WIFI_COORD_STR_LEN) {
        return -1;
    }
    while (i < length && text[i] >= '0' && text[i] <= '9') {
        ++i;
    }
    if (i == 0U) {
        return -1;
    }
    integer_length = i;
    if (i < length) {
        if (text[i] != '.' || ++i == length) {
            return -1;
        }
        for (; i < length; ++i) {
            if (text[i] < '0' || text[i] > '9') {
                return -1;
            }
        }
    }
    if (decimal_coordinate_exceeds_maximum(text, integer_length, length,
                                           maximum)) {
        return -1;
    }
    memcpy(number, text, length);
    number[length] = '\0';
    errno = 0;
    parse_end = NULL;
    value = strtod(number, &parse_end);
    if (errno == ERANGE || parse_end == number || *parse_end != '\0' ||
        !number_is_finite(value) || value > maximum) {
        return -1;
    }
    *numeric_value = value;
    return 0;
}

static int valid_direction(const char *type, char first, char second)
{
    return type[0] != '\0' && type[1] == '\0' &&
           (type[0] == first || type[0] == second);
}

static int validate_config(const WifiConfig_t *config)
{
    size_t ssid_len;
    size_t mac_len;
    size_t lat_len;
    size_t lon_len;

    if (!config) {
        return -1;
    }
    ssid_len = bounded_string_length(config->ssid, sizeof(config->ssid));
    mac_len = bounded_string_length(config->mac, sizeof(config->mac));
    lat_len = bounded_string_length(config->lat, sizeof(config->lat));
    lon_len = bounded_string_length(config->lon, sizeof(config->lon));
    if (ssid_len == 0U || ssid_len > WIFI_SSID_MAX_LEN ||
        contains_line_break(config->ssid, ssid_len)) {
        return -2;
    }
    if (!valid_mac(config->mac, mac_len) ||
        contains_line_break(config->mac, mac_len)) {
        return -3;
    }
    if (lat_len == 0U || lat_len >= sizeof(config->lat) ||
        lon_len == 0U || lon_len >= sizeof(config->lon) ||
        contains_line_break(config->lat, lat_len) ||
        contains_line_break(config->lon, lon_len)) {
        return -4;
    }
    if (bounded_string_length(config->latType, sizeof(config->latType)) !=
            WIFI_LATLON_TYPE_LEN ||
        bounded_string_length(config->lonType, sizeof(config->lonType)) !=
            WIFI_LATLON_TYPE_LEN ||
        (config->latType[0] != 'N' && config->latType[0] != 'S') ||
        (config->lonType[0] != 'E' && config->lonType[0] != 'W')) {
        return -5;
    }
    return 0;
}

int wifi_config_encode(const WifiConfig_t *config,
                       char *output,
                       size_t output_size)
{
    int result;

    if (!output || output_size == 0U) {
        return -1;
    }
    result = validate_config(config);
    if (result < 0) {
        output[0] = '\0';
        return result;
    }
    result = snprintf(output, output_size,
                      WIFI_SSID_HEADER "%s\r\n"
                      WIFI_MAC_HEADER "%s\r\n"
                      WIFI_LAT_HEADER "%s\r\n"
                      WIFI_LAT_TYPE_HEADER "%s\r\n"
                      WIFI_LON_HEADER "%s\r\n"
                      WIFI_LON_TYPE_HEADER "%s\r\n",
                      config->ssid, config->mac, config->lat, config->latType,
                      config->lon, config->lonType);
    if (result < 0 || (size_t)result >= output_size) {
        output[0] = '\0';
        return -6;
    }
    return result;
}

static int copy_text_field(char *destination,
                           size_t destination_size,
                           const char *value,
                           size_t value_len)
{
    if (value_len == 0U || value_len >= destination_size ||
        contains_line_break(value, value_len)) {
        return -1;
    }
    memcpy(destination, value, value_len);
    destination[value_len] = '\0';
    return 0;
}

static int normalize_legacy_coordinate(const char *raw,
                                       size_t raw_length,
                                       double maximum,
                                       char positive_type,
                                       char negative_type,
                                       char *coordinate,
                                       size_t coordinate_size,
                                       char type[WIFI_LATLON_TYPE_LEN + 1U])
{
    const char *magnitude = raw;
    size_t magnitude_length = raw_length;
    int negative = 0;
    double numeric_value;

    if (raw_length > 0U && raw[0] == '-') {
        negative = 1;
        magnitude = raw + 1;
        --magnitude_length;
    }
    if (magnitude_length == 0U || magnitude_length >= coordinate_size ||
        parse_unsigned_coordinate(magnitude, magnitude_length, maximum,
                                  &numeric_value) < 0) {
        return -1;
    }
    memcpy(coordinate, magnitude, magnitude_length);
    coordinate[magnitude_length] = '\0';
    type[0] = negative && numeric_value != 0.0 ? negative_type : positive_type;
    type[1] = '\0';
    return 0;
}

static int header_matches(const char *line,
                          size_t line_len,
                          const char *header,
                          size_t header_len)
{
    return line_len >= header_len &&
           memcmp(line, header, header_len) == 0;
}

int wifi_config_decode(const char *input,
                       size_t input_len,
                       WifiConfig_t *config)
{
    const char *cursor;
    const char *end;
    const unsigned int legacy_fields = WIFI_FIELD_SSID | WIFI_FIELD_MAC |
                                       WIFI_FIELD_LAT | WIFI_FIELD_LON;
    const unsigned int new_fields = legacy_fields | WIFI_FIELD_LAT_TYPE |
                                    WIFI_FIELD_LON_TYPE;
    unsigned int fields = 0;
    WifiConfig_t parsed;
    char raw_lat[WIFI_COORD_STR_LEN + 2U] = {0};
    char raw_lon[WIFI_COORD_STR_LEN + 2U] = {0};

    if (!config) {
        return -1;
    }
    memset(config, 0, sizeof(*config));
    memset(&parsed, 0, sizeof(parsed));
    if (!input || input_len == 0 || input_len >= WIFI_CONFIG_TEXT_MAX_SIZE ||
        memchr(input, '\0', input_len) != NULL) {
        return -2;
    }

    cursor = input;
    end = input + input_len;
    while (cursor < end) {
        const char *newline = memchr(cursor, '\n', (size_t)(end - cursor));
        const char *line_end = newline ? newline : end;
        size_t line_len;
        const char *value;
        size_t value_len;
        unsigned int field;
        int result;

        if (line_end > cursor && line_end[-1] == '\r') {
            --line_end;
        }
        line_len = (size_t)(line_end - cursor);
        if (line_len == 0) {
            return -3;
        }

        if (header_matches(cursor, line_len,
                           WIFI_SSID_HEADER, sizeof(WIFI_SSID_HEADER) - 1)) {
            field = WIFI_FIELD_SSID;
            value = cursor + sizeof(WIFI_SSID_HEADER) - 1;
            value_len = line_len - (sizeof(WIFI_SSID_HEADER) - 1);
            result = copy_text_field(parsed.ssid, sizeof(parsed.ssid),
                                     value, value_len);
        } else if (header_matches(cursor, line_len,
                                  WIFI_MAC_HEADER, sizeof(WIFI_MAC_HEADER) - 1)) {
            field = WIFI_FIELD_MAC;
            value = cursor + sizeof(WIFI_MAC_HEADER) - 1;
            value_len = line_len - (sizeof(WIFI_MAC_HEADER) - 1);
            result = copy_text_field(parsed.mac, sizeof(parsed.mac),
                                     value, value_len);
            if (result == 0 &&
                !valid_mac(parsed.mac, strlen(parsed.mac))) {
                result = -1;
            }
        } else if (header_matches(cursor, line_len,
                                  WIFI_LAT_HEADER, sizeof(WIFI_LAT_HEADER) - 1)) {
            field = WIFI_FIELD_LAT;
            value = cursor + sizeof(WIFI_LAT_HEADER) - 1;
            value_len = line_len - (sizeof(WIFI_LAT_HEADER) - 1);
            result = copy_text_field(raw_lat, sizeof(raw_lat), value, value_len);
        } else if (header_matches(cursor, line_len,
                                  WIFI_LAT_TYPE_HEADER,
                                  sizeof(WIFI_LAT_TYPE_HEADER) - 1U)) {
            field = WIFI_FIELD_LAT_TYPE;
            value = cursor + sizeof(WIFI_LAT_TYPE_HEADER) - 1U;
            value_len = line_len - (sizeof(WIFI_LAT_TYPE_HEADER) - 1U);
            result = copy_text_field(parsed.latType, sizeof(parsed.latType),
                                     value, value_len);
        } else if (header_matches(cursor, line_len,
                                  WIFI_LON_HEADER, sizeof(WIFI_LON_HEADER) - 1)) {
            field = WIFI_FIELD_LON;
            value = cursor + sizeof(WIFI_LON_HEADER) - 1;
            value_len = line_len - (sizeof(WIFI_LON_HEADER) - 1);
            result = copy_text_field(raw_lon, sizeof(raw_lon), value, value_len);
        } else if (header_matches(cursor, line_len,
                                  WIFI_LON_TYPE_HEADER,
                                  sizeof(WIFI_LON_TYPE_HEADER) - 1U)) {
            field = WIFI_FIELD_LON_TYPE;
            value = cursor + sizeof(WIFI_LON_TYPE_HEADER) - 1U;
            value_len = line_len - (sizeof(WIFI_LON_TYPE_HEADER) - 1U);
            result = copy_text_field(parsed.lonType, sizeof(parsed.lonType),
                                     value, value_len);
        } else {
            return -4;
        }

        if (result < 0 || (fields & field) != 0U) {
            return -5;
        }
        fields |= field;
        cursor = newline ? newline + 1 : end;
    }

    if (fields == new_fields) {
        if (!valid_direction(parsed.latType, 'N', 'S') ||
            !valid_direction(parsed.lonType, 'E', 'W') ||
            copy_text_field(parsed.lat, sizeof(parsed.lat), raw_lat,
                            strlen(raw_lat)) < 0 ||
            copy_text_field(parsed.lon, sizeof(parsed.lon), raw_lon,
                            strlen(raw_lon)) < 0) {
            return -6;
        }
    } else if (fields == legacy_fields) {
        if (normalize_legacy_coordinate(raw_lat, strlen(raw_lat), 90.0,
                                        'N', 'S', parsed.lat,
                                        sizeof(parsed.lat), parsed.latType) < 0 ||
            normalize_legacy_coordinate(raw_lon, strlen(raw_lon), 180.0,
                                        'E', 'W', parsed.lon,
                                        sizeof(parsed.lon), parsed.lonType) < 0) {
            return -6;
        }
    } else {
        return -6;
    }
    *config = parsed;
    return 0;
}

typedef enum {
    CONFIG_FIELD_U32,
    CONFIG_FIELD_BOOL
} ConfigFieldKind_t;

typedef struct {
    const char *header;
    size_t header_len;
    size_t offset;
    ConfigFieldKind_t kind;
    uint32_t default_value;
} ConfigFieldDescriptor_t;

#define CONFIG_TABLE_DESCRIPTOR(type, name, header, kind, default_value) \
    { header, sizeof(header) - 1U, offsetof(ConfigTable_t, name),         \
      kind, (uint32_t)(default_value) },

static const ConfigFieldDescriptor_t config_table_fields[] = {
    CONFIG_TABLE_FIELDS(CONFIG_TABLE_DESCRIPTOR)
};

#undef CONFIG_TABLE_DESCRIPTOR

#define CONFIG_TABLE_COUNT(type, name, header, kind, default_value) + 1
enum {
    CONFIG_TABLE_FIELD_COUNT = 0 CONFIG_TABLE_FIELDS(CONFIG_TABLE_COUNT)
};
#undef CONFIG_TABLE_COUNT

static int parse_config_table_u32(const char *value,
                                  size_t value_len,
                                  uint32_t *number)
{
    uint32_t parsed = 0;
    size_t i;

    if (value_len == 0) {
        return -1;
    }
    for (i = 0; i < value_len; ++i) {
        uint32_t digit;

        if (value[i] < '0' || value[i] > '9') {
            return -1;
        }
        digit = (uint32_t)(value[i] - '0');
        if (parsed > (UINT32_MAX - digit) / 10U) {
            return -1;
        }
        parsed = parsed * 10U + digit;
    }
    *number = parsed;
    return 0;
}

static int config_table_header_matches(const char *line,
                                       size_t line_len,
                                       const char *header,
                                       size_t header_len)
{
    return line_len >= header_len && memcmp(line, header, header_len) == 0;
}

static void config_table_set_defaults(ConfigTable_t *config)
{
#define CONFIG_TABLE_DEFAULT(type, name, header, kind, default_value) \
    config->name = (type)(default_value);
    CONFIG_TABLE_FIELDS(CONFIG_TABLE_DEFAULT)
#undef CONFIG_TABLE_DEFAULT
}

int config_table_encode(const ConfigTable_t *config,
                        char *output,
                        size_t output_size)
{
    size_t written = 0U;
    size_t i;

    if (!output || output_size == 0) {
        return -1;
    }
    output[0] = '\0';
    if (!config) {
        return -2;
    }

    for (i = 0U; i < CONFIG_TABLE_FIELD_COUNT; ++i) {
        const ConfigFieldDescriptor_t *field = &config_table_fields[i];
        const unsigned char *base = (const unsigned char *)config;
        uint32_t value;
        int result;

        if (field->kind == CONFIG_FIELD_U32) {
            memcpy(&value, base + field->offset, sizeof(value));
        } else {
            value = base[field->offset];
            if (value > 1U) {
                output[0] = '\0';
                return -2;
            }
        }

        result = snprintf(output + written,
                          output_size - written,
                          "%s%" PRIu32 "\r\n",
                          field->header,
                          value);
        if (result < 0 || (size_t)result >= output_size - written) {
            output[0] = '\0';
            return -3;
        }
        written += (size_t)result;
    }
    return (int)written;
}

int config_table_decode_ex(const char *input,
                           size_t input_len,
                           ConfigTable_t *config,
                           int *needs_upgrade)
{
    const char *cursor;
    const char *end;
    ConfigTable_t parsed;
    uint8_t seen[CONFIG_TABLE_FIELD_COUNT] = {0};
    size_t i;

    if (!config) {
        return -1;
    }
    memset(config, 0, sizeof(*config));
    if (!needs_upgrade) {
        return -1;
    }
    *needs_upgrade = 0;
    if (!input || input_len == 0 || input_len >= CONFIG_TABLE_TEXT_MAX_SIZE ||
        memchr(input, '\0', input_len) != NULL) {
        return -2;
    }

    config_table_set_defaults(&parsed);
    cursor = input;
    end = input + input_len;
    while (cursor < end) {
        const char *newline = memchr(cursor, '\n', (size_t)(end - cursor));
        const char *line_end;
        size_t line_len;
        const ConfigFieldDescriptor_t *field = NULL;
        uint32_t number;

        if (!newline || newline == cursor || newline[-1] != '\r') {
            return -3;
        }
        line_end = newline - 1;
        line_len = (size_t)(line_end - cursor);

        for (i = 0U; i < CONFIG_TABLE_FIELD_COUNT; ++i) {
            if (config_table_header_matches(cursor, line_len,
                                            config_table_fields[i].header,
                                            config_table_fields[i].header_len)) {
                field = &config_table_fields[i];
                break;
            }
        }
        if (!field || seen[i] != 0U) {
            return -4;
        }

        {
            const char *value = cursor + field->header_len;
            size_t value_len = line_len - field->header_len;
            unsigned char *base = (unsigned char *)&parsed;

            if (field->kind == CONFIG_FIELD_BOOL) {
                if (value_len != 1U ||
                    (value[0] != '0' && value[0] != '1')) {
                    return -5;
                }
                base[field->offset] = (uint8_t)(value[0] - '0');
            } else {
                if (parse_config_table_u32(value, value_len, &number) < 0) {
                    return -5;
                }
                memcpy(base + field->offset, &number, sizeof(number));
            }
        }
        seen[i] = 1U;
        cursor = newline + 1;
    }

    for (i = 0U; i < CONFIG_TABLE_FIELD_COUNT; ++i) {
        if (seen[i] == 0U) {
            *needs_upgrade = 1;
        }
    }
    *config = parsed;
    return 0;
}

int config_table_decode(const char *input,
                        size_t input_len,
                        ConfigTable_t *config)
{
    int needs_upgrade;

    return config_table_decode_ex(input, input_len, config, &needs_upgrade);
}
