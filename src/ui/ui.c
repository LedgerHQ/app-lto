
/*******************************************************************************
*   LTO Network Wallet App for Ledger devices
*   2019-2021 Ignacio Iglesias (iicc) https://github.com/iicc1/ledger-app-lto
********************************************************************************
*   Burstcoin Wallet App for Nano Ledger S. Updated By Waves community.
*   Copyright (c) 2017-2018 Jake B.
* 
*   Based on Sample code provided and (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "ui.h"
#include <stdbool.h>
#include "glyphs.h"
#include "../main.h"
#include "../crypto/lto.h"
#include "ui_menus.h"

#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

// UI currently displayed
enum UI_STATE ui_state;

int ux_step, ux_step_count;

bool print_amount(uint64_t amount, int decimals, char *out, uint8_t len);

void menu_address_init() {
    ux_step = 0;
    ux_step_count = 2;
    ux_flow_init(0, ux_display_address_flow, NULL);
}

// Idle state, show the menu
void ui_idle() {
    ux_step = 0; ux_step_count = 0;
    ui_state = UI_IDLE;
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

// Show the transaction details for the user to approve
void menu_sign_init() {
    memset(&ui_context, 0, sizeof(uiContext_t));
    unsigned char tx_type = tmp_ctx.signing_context.data_type;
    unsigned char tx_version = tmp_ctx.signing_context.data_version;

    // Transfer https://docs.ltonetwork.com/protocol/public/transactions/transfer-transaction
    if (tx_type == 4) {
        unsigned int processed = 1;
        
        // Transaction type - 1/2 bytes
        if (tx_version == 2) {
            processed += 1;
        }

        // Sender public key - 32 bytes
        lto_public_key_to_address(&tmp_ctx.signing_context.buffer[processed], tmp_ctx.signing_context.network_byte, ui_context.line5);
        processed += 32;

        // Timestamp - 8 bytes
        processed += 8;

        // Amount - 8 bytes
        uint64_t amount = 0;
        copy_in_reverse_order(&amount, &tmp_ctx.signing_context.buffer[processed], 8);
        print_amount(amount, tmp_ctx.signing_context.amount_decimals, ui_context.line1, 45);
        processed += 8;

        // Fees - 8 bytes
        uint64_t fee = 0;
        copy_in_reverse_order(&fee, &tmp_ctx.signing_context.buffer[processed], 8);
        print_amount(fee, tmp_ctx.signing_context.fee_decimals, ui_context.line3, 45);
        processed += 8;

        // To address or alias flag is a part of address - 26 bytes
        if (tmp_ctx.signing_context.buffer[processed] == 1) {
          size_t length = 45;
          if (!b58enc(ui_context.line2, &length, (const void *) &tmp_ctx.signing_context.buffer[processed], 26)) {
              THROW(SW_CONDITIONS_NOT_SATISFIED);
          }
          processed += 26;
        } else {
          // also skip address scheme byte
          processed += 2;
          uint16_t alias_size = 0;
          copy_in_reverse_order(&alias_size, &tmp_ctx.signing_context.buffer[processed], 2);
          processed += 2;

          memcpy(ui_context.line2, &tmp_ctx.signing_context.buffer[processed], alias_size);
          processed += alias_size;
        }

        // Attachment size - 2 bytes
        uint16_t attachment_size = 0;
        copy_in_reverse_order(&attachment_size, &tmp_ctx.signing_context.buffer[processed], 2);
        processed += 2;

        if (attachment_size > 41) {
          memcpy(&ui_context.line4[41], &"...\0", 4);
          attachment_size = 41;
        }

        // Attachment - depends on the value of attachment size
        memcpy(ui_context.line4, &tmp_ctx.signing_context.buffer[processed], attachment_size);
        // processed += attachment_size;

        // Transaction id
        unsigned char id[32];
        blake2b_256(tmp_ctx.signing_context.buffer, tmp_ctx.signing_context.buffer_used, &id);
        size_t length = 45;
        if (!b58enc(ui_context.line6, &length, (const void *) &id, 32)) {
          THROW(SW_CONDITIONS_NOT_SATISFIED);
        }

        // Set the step/step count, and ui_state before requesting the UI
        ux_step = 0; ux_step_count = 7;
        ui_state = UI_VERIFY;

        ux_flow_init(0, ux_transfer_flow, NULL);
        return;
    
    // Start lease: https://docs.ltonetwork.com/protocol/public/transactions/lease-transaction
    } else if (tx_type == 8) {
        unsigned int processed = 1;

        // Transaction type - 1/2 bytes
        if (tx_version == 2) {
            processed += 1;
        }

        // Transaction name
        strcpy(ui_context.line1, "Start Lease");

        // Sender public key - 32 bytes
        processed += 32;

        // To address or alias flag is a part of address - 26 bytes
        if (tmp_ctx.signing_context.buffer[processed] == 1) {
          size_t length = 45;
          if (!b58enc(ui_context.line2, &length, (const void *) &tmp_ctx.signing_context.buffer[processed], 26)) {
              THROW(SW_CONDITIONS_NOT_SATISFIED);
          }
          processed += 26;
        } else {
          // also skip address scheme byte
          processed += 2;
          uint16_t alias_size = 0;
          copy_in_reverse_order(&alias_size, &tmp_ctx.signing_context.buffer[processed], 2);
          processed += 2;

          memcpy(ui_context.line2, &tmp_ctx.signing_context.buffer[processed], alias_size);
          processed += alias_size;
        }

        // Lease amount - 8 bytes
        uint64_t amount = 0;
        copy_in_reverse_order(&amount, &tmp_ctx.signing_context.buffer[processed], 8);
        print_amount(amount, tmp_ctx.signing_context.amount_decimals, ui_context.line3, 45);
        processed += 8;

        // Fee amount - 8 bytes
        uint64_t fee = 0;
        copy_in_reverse_order(&fee, &tmp_ctx.signing_context.buffer[processed], 8);
        print_amount(fee, tmp_ctx.signing_context.fee_decimals, ui_context.line4, 45);
        // processed += 8;

        // Timestamp - 8 bytes
        // processed += 8;
        
        // Transaction id
        memcpy(&ui_context.line5, &"Transaction Id\0", 15);
        unsigned char id[32];
        blake2b_256(tmp_ctx.signing_context.buffer, tmp_ctx.signing_context.buffer_used, &id);
        size_t length = 45;
        if (!b58enc(ui_context.line6, &length, (const void *) &id, 32)) {
            THROW(SW_CONDITIONS_NOT_SATISFIED);
        }

        // Get the public key and return it.
        cx_ecfp_public_key_t public_key;

        get_ed25519_public_key_for_path((uint32_t *) tmp_ctx.signing_context.bip32, &public_key);

        lto_public_key_to_address(public_key.W, tmp_ctx.signing_context.network_byte, ui_context.line7);

        ux_step = 0; ux_step_count = 6;
        ui_state = UI_VERIFY;
        ux_flow_init(0, ux_start_lease_flow, NULL);
        return;

    // Cancel lease: https://docs.ltonetwork.com/protocol/public/transactions/cancel-lease-transaction
    } else if (tx_type == 9) {
        unsigned int processed = 1;

        // Transaction type - 1/2 bytes
        if (tx_version == 2) {
            processed += 1;
        }

        // Transaction name
        strcpy(ui_context.line1, "Cancel Lease");

        // Sender public key - 32 bytes
        processed += 32;

        // Fee amount - 8 bytes
        uint64_t fee = 0;
        copy_in_reverse_order(&fee, &tmp_ctx.signing_context.buffer[processed], 8);
        print_amount(fee, tmp_ctx.signing_context.fee_decimals, ui_context.line2, 45);
        // processed += 8;
        
        // Transaction id
        strcpy(ui_context.line3, "Transaction Id");
        unsigned char id[32];
        blake2b_256(tmp_ctx.signing_context.buffer, tmp_ctx.signing_context.buffer_used, &id);
        size_t length = 45;
        if (!b58enc(ui_context.line4, &length, (const void *) &id, 32)) {
            THROW(SW_CONDITIONS_NOT_SATISFIED);
        }

        // Get the public key and return it.
        cx_ecfp_public_key_t public_key;

        get_ed25519_public_key_for_path((uint32_t *) tmp_ctx.signing_context.bip32, &public_key);

        lto_public_key_to_address(public_key.W, tmp_ctx.signing_context.network_byte, ui_context.line5);

        ux_step = 0; ux_step_count = 4;
        ui_state = UI_VERIFY;
        ux_flow_init(0, ux_cancel_lease_flow, NULL);
        return;

    // Anchor: https://docs.ltonetwork.com/protocol/public/transactions/anchor
    } else if (tx_type == 15) {
        unsigned int processed = 1;

        // Transaction type - 2 bytes
        processed += 1;

        // Transaction name
        strcpy(ui_context.line1, "Anchor");
        
        // Sender public key - 32 bytes
        processed += 32;

        // Anchor number - 2 bytes
        processed += 2;

        // Anchor length - 2 bytes
        uint16_t anchor_size = 0;
        copy_in_reverse_order(&anchor_size, &tmp_ctx.signing_context.buffer[processed], 2);
        processed += 2;

        // Anchor - depends on the value of anchor size
        processed += anchor_size;

        // Timestamp - 8 bytes
        processed += 8;

        // Fee amount - 8 bytes
        uint64_t fee = 0;
        copy_in_reverse_order(&fee, &tmp_ctx.signing_context.buffer[processed], 8);
        print_amount(fee, tmp_ctx.signing_context.fee_decimals, ui_context.line2, 45);
        // processed += 8;
        
        // Transaction id
        strcpy(ui_context.line3, "Transaction Id");
        unsigned char id[32];
        blake2b_256(tmp_ctx.signing_context.buffer, tmp_ctx.signing_context.buffer_used, &id);
        size_t length = 45;
        if (!b58enc(ui_context.line4, &length, (const void *) &id, 32)) {
            THROW(SW_CONDITIONS_NOT_SATISFIED);
        }

        // Get the public key and return it.
        cx_ecfp_public_key_t public_key;

        get_ed25519_public_key_for_path((uint32_t *) tmp_ctx.signing_context.bip32, &public_key);

        lto_public_key_to_address(public_key.W, tmp_ctx.signing_context.network_byte, ui_context.line5);

        ux_step = 0; ux_step_count = 4;
        ui_state = UI_VERIFY;
        ux_flow_init(0, ux_anchor_flow, NULL);
        return;

    // Other transaction types: https://docs.ltonetwork.com/protocol/public/transactions
    } else {
        strcpy(ui_context.line2, "Transaction Id");
        if (tx_type == 11) {
            strcpy(ui_context.line1, "mass transfer");
        } else if (tx_type == 13) {
            strcpy(ui_context.line1, "set script");
        } else if (tx_type == 15) {
            strcpy(ui_context.line1, "anchor");
        } else {
            // type byte >200 are 'reserved', it will not be signed
            strcpy(ui_context.line2, "Hash");
            if (tx_type == 252) {
                strcpy(ui_context.line1, "order");
            } else if (tx_type == 253) {
                strcpy(ui_context.line1, "data");
            } else if (tx_type == 254) {
                strcpy(ui_context.line1, "request");
            } else if (tx_type == 255) {
                strcpy(ui_context.line1, "message");
            }
        }
    }

    if (strlen((const char *) ui_context.line1) == 0) {
        strcpy(ui_context.line1, "transaction");
    }
    // id
    unsigned char id[32];
    blake2b_256(tmp_ctx.signing_context.buffer, tmp_ctx.signing_context.buffer_used, &id);
    size_t length = 45;
    if (!b58enc(ui_context.line3, &length, (const void *) &id, 32)) {
        THROW(SW_CONDITIONS_NOT_SATISFIED);
    }

    // Get the public key and return it.
    cx_ecfp_public_key_t public_key;

    get_ed25519_public_key_for_path((uint32_t *) tmp_ctx.signing_context.bip32, &public_key);

    lto_public_key_to_address(public_key.W, tmp_ctx.signing_context.network_byte, ui_context.line4);

    ux_step = 0; ux_step_count = 3;
    ui_state = UI_VERIFY;
    ux_flow_init(0, ux_verify_transaction_flow, NULL);
}


// borrowed from the Stellar wallet code and modified
bool print_amount(uint64_t amount, int decimals, char *out, uint8_t len) {
    char buffer[len];
    uint64_t dVal = amount;
    int i, j;

    if (decimals == 0) decimals--;

    memset(buffer, 0, len);
    for (i = 0; dVal > 0 || i < decimals + 2; i++) {
        if (dVal > 0) {
            buffer[i] = (char) ((dVal % 10) + '0');
            dVal /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == decimals - 1) {
            i += 1;
            buffer[i] = '.';
        }
        if (i >= len) {
            return false;
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < len-1; i--, j++) {
        out[j] = buffer[i];
    }
    if (decimals > 0) {
        // strip trailing 0s
        for (j -= 1; j > 0; j--) {
            if (out[j] != '0') break;
        }
        j += 1;
        if (out[j - 1] == '.') j -= 1;
    }

    out[j] = '\0';
    return  true;
}