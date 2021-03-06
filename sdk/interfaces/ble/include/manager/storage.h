/**
 ****************************************************************************************
 * @file storage.h
 *
 * @brief BLE Manager storage interface
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <black.orca.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include <stdbool.h>
#include "ble_common.h"
#include "ble_storage.h"
#include "util/queue.h"

/*
 * Following are internal storage keys (for app values) to be used internally - the should be
 * outside ranges allowed to be set by application, so start them with 0xF0000000.
 */
enum {
        STORAGE_KEY_SVC_CHANGED_CCC = 0xF0000000,
};

typedef struct {
        void     *next;

        ble_storage_key_t key;

        bool     persistent;

        uint16_t length;
        void     *ptr;

        ble_storage_free_cb_t free_cb;
} app_value_t;

typedef struct {
        uint64_t        rand;
        uint16_t        ediv;
        uint8_t         key[16];
        uint8_t         key_size;
} key_ltk_t;

typedef struct {
        uint8_t         key[16];
} key_irk_t;

typedef struct {
        uint8_t         key[16];
        uint32_t        sign_cnt;
} key_csrk_t;

typedef struct {
        void            *next;

        bd_address_t    addr;
        uint16_t        conn_idx;

        // state flags
        bool            connecting:1;
        bool            connected:1;
        bool            master:1;
        bool            paired:1;
        bool            bonded:1;
        bool            encrypted:1;
        bool            mitm:1;
        bool            resolving:1;

        // parameters
        uint16_t        mtu;

        // pairing information
        key_ltk_t       *ltk;
        key_ltk_t       *remote_ltk;
        key_irk_t       *irk;
        key_csrk_t      *csrk;
        key_csrk_t      *remote_csrk;

        // custom values set from application
        queue_t         app_value;
} device_t;

typedef void (* device_cb_t) (const device_t *dev, void *ud);

typedef bool (* device_match_cb_t) (const device_t *dev, void *ud);

device_t *find_device_by_addr(const bd_address_t *addr, bool create);

device_t *find_device_by_conn_idx(uint16_t conn_idx);

device_t *find_device(device_match_cb_t cb, void *ud);

void storage_init(void);

void storage_cleanup(void);

void storage_acquire(void);

void storage_release(void);

void storage_mark_dirty(bool flush_now);

void device_foreach(device_cb_t cb, void *ud);

void device_move_front(device_t *dev);

void device_remove(device_t *dev);

void device_remove_pairing(device_t *dev);

void app_value_put(device_t *dev, ble_storage_key_t key, uint16_t length, void *ptr,
                                                ble_storage_free_cb_t free_cb, bool persistent);

bool app_value_get(device_t *dev, ble_storage_key_t key, uint16_t *length, void **ptr);

void app_value_remove(device_t *dev, ble_storage_key_t key);

void app_value_remove_np(device_t *dev);

#endif /* STORAGE_H_ */
