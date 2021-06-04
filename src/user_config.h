#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

// clang-format off

/**
 *      WiFi Details
 */

// Maximum of 32 characters
#define USER_CONFIG_WIFI_SSID \
        ("")

// Between 8 - 64 characters
#define USER_CONFIG_WIFI_PASS \
        ("")

/**
 *      SNTP Server address
 */

#define USER_CONFIG_SNTP_SERVER_ADDR \
        ("pool.ntp.org")

/**
 *      DECADA Credentials
 */

// Root URL to access DECADA cloud API
#define USER_CONFIG_DECADA_API_URL \
        ("https://ag.decada.gov.sg")

// Organization Unit ID for DECADA cloud
#define USER_CONFIG_DECADA_OU_ID \
        ("")

// Access key for DECADA cloud application
#define USER_CONFIG_DECADA_ACCESS_KEY \
        ("")

// Access secret for DECADA cloud application
#define USER_CONFIG_DECADA_ACCESS_SECRET \
        ("")

// Product key for DECADA cloud product
#define USER_CONFIG_DECADA_PRODUCT_KEY \
        ("")

// Product secret for DECADA cloud product
#define USER_CONFIG_DECADA_PRODUCT_SECRET \
        ("")

// clang-format on

#endif // _USER_CONFIG_H_