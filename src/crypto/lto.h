#ifndef __LTO_H__
#define __LTO_H__

#include <stdint.h>
#include <stdbool.h>

#include "base58.h"
#include "os.h"
#include "cx.h"

typedef unsigned char ed25519_signature[64];
typedef unsigned char ed25519_public_key[32];
typedef unsigned char ed25519_secret_key[32];

void lto_public_key_to_address(const ed25519_public_key public_key, const unsigned char network_byte, char *output);
void lto_message_sign(const cx_ecfp_private_key_t *private_key, const unsigned char *message, const size_t message_size, ed25519_signature signature);
void copy_in_reverse_order(void *to, const void *from, const size_t len);

#endif
