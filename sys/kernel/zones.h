#ifndef ZONES_H
#define ZONES_H

/* All time zones worldwide - UTC offsets in hours */
static const char* time_zone_names[] = {
    "UTC-12:00 (Baker Island)",
    "UTC-11:00 (Pago Pago)",
    "UTC-10:00 (Honolulu)",
    "UTC-09:30 (Marquesas)",
    "UTC-09:00 (Anchorage)",
    "UTC-08:00 (Los Angeles)",
    "UTC-07:00 (Denver)",
    "UTC-06:00 (Mexico City/Chicago)",
    "UTC-05:00 (New York/Bogota)",
    "UTC-04:00 (Caracas/Halifax)",
    "UTC-03:30 (St. John's)",
    "UTC-03:00 (Sao Paulo/Buenos Aires)",
    "UTC-02:00 (South Georgia)",
    "UTC-01:00 (Azores/Cape Verde)",
    "UTC+00:00 (London/Lisbon)",
    "UTC+01:00 (Berlin/Paris/Rome)",
    "UTC+02:00 (Cairo/Helsinki)",
    "UTC+03:00 (Moscow/Istanbul)",
    "UTC+03:30 (Tehran)",
    "UTC+04:00 (Dubai/Baku)",
    "UTC+04:30 (Kabul)",
    "UTC+05:00 (Karachi/Tashkent)",
    "UTC+05:30 (New Delhi/Colombo)",
    "UTC+05:45 (Kathmandu)",
    "UTC+06:00 (Dhaka/Almaty)",
    "UTC+06:30 (Yangon)",
    "UTC+07:00 (Bangkok/Jakarta)",
    "UTC+08:00 (Beijing/Singapore)",
    "UTC+08:45 (Eucla)",
    "UTC+09:00 (Tokyo/Seoul)",
    "UTC+09:30 (Adelaide/Darwin)",
    "UTC+10:00 (Sydney/Brisbane)",
    "UTC+10:30 (Lord Howe)",
    "UTC+11:00 (Solomon Is/Vladivostok)",
    "UTC+12:00 (Auckland/Fiji)",
    "UTC+12:45 (Chatham)",
    "UTC+13:00 (Samoa/Tonga)",
    "UTC+14:00 (Kiritimati)"
};

static const int time_zone_offsets[] = {
    -12, -11, -10, -9,  -9, -8, -7, -6, -5, -4,
    -3,  -3, -2, -1,  0,  1,  2,  3,  3,  4,
    4,   5,  5,  5,   6,  6,  7,  8,  8,  9,
    9,   10, 10, 11,  12, 12, 13, 14
};

#define TIME_ZONE_COUNT 38

#endif
