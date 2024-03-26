#pragma once

#if NDEBUG
#define NSLog(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void init_apple();

void get_upgrade(const char *item);

const char *get_secret_key(const char *key);

float get_screen_scale();

#ifdef __cplusplus
}
#endif
