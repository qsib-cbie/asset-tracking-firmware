#include <stdint.h>

extern "C" {
void rust_init();
char* rust_greeting(const char* to);
void rust_greeting_free(char *);
}

