#ifndef WIFI_CONFIG_TYPES_H
#define WIFI_CONFIG_TYPES_H

#include <stdint.h>

#define WIFI_SSID_MAX_LEN      32U
#define WIFI_MAC_STR_LEN       12U
#define WIFI_COORD_STR_LEN     20U
#define WIFI_LATLON_TYPE_LEN   1U
#define FENCE_MAX_POINTS       10U
#define FENCE_COORD_STR_LEN    20U

typedef struct {
    char latitude[FENCE_COORD_STR_LEN + 1U];
    char longitude[FENCE_COORD_STR_LEN + 1U];
} GeoPoint_t;

typedef struct {
    uint8_t count;
    GeoPoint_t points[FENCE_MAX_POINTS];
} GeoPolygon_t;

typedef struct {
    uint8_t fence_id;
    GeoPolygon_t fence;
    GeoPolygon_t safe_zone;
} FenceConfig_t;

typedef struct {
    char ssid[WIFI_SSID_MAX_LEN + 1U];
    char mac[WIFI_MAC_STR_LEN + 1U];
    char lat[WIFI_COORD_STR_LEN + 1U];
    char latType[WIFI_LATLON_TYPE_LEN + 1U];
    char lon[WIFI_COORD_STR_LEN + 1U];
    char lonType[WIFI_LATLON_TYPE_LEN + 1U];
} WifiConfig_t;

#define CONFIG_TABLE_FIELDS(X)                                      \
    X(uint32_t, find,       "[FIND]",        CONFIG_FIELD_U32,  60U)  \
    X(uint32_t, normal,     "[NORMAL]",      CONFIG_FIELD_U32,  300U) \
    X(uint32_t, normalGps,  "[NORMAL_GPS]",  CONFIG_FIELD_U32,  120U) \
    X(uint32_t, fenceAlert, "[FENCE_ALERT]", CONFIG_FIELD_U32,  30U)  \
    X(uint32_t, vibrate,    "[VIBRATE]",     CONFIG_FIELD_U32,  5U)   \
    X(uint32_t, play_num,   "[PLAY_NUM]",    CONFIG_FIELD_U32,  5U)   \
    X(uint8_t,  wifi,       "[WIFI]",        CONFIG_FIELD_BOOL, 1U)   \
    X(uint8_t,  valid,      "[VALID]",       CONFIG_FIELD_BOOL, 1U)

#define CONFIG_TABLE_MEMBER(type, name, header, kind, default_value) \
    type name;

typedef struct {
    CONFIG_TABLE_FIELDS(CONFIG_TABLE_MEMBER)
} ConfigTable_t;

#undef CONFIG_TABLE_MEMBER

#endif
