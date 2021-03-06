/**
 ****************************************************************************************
 *
 * @file ble_mgr_irb_gap.c
 *
 * @brief BLE IRB handlers (for GAP)
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

#include <string.h>
#include <stdlib.h>
#include "co_bt.h"
#include "co_version.h"
#include "rwble_hl_error.h"
#include "ble_mgr.h"
#include "ble_mgr_gtl.h"
#include "ble_mgr_irb.h"
#include "ble_mgr_irb_gap.h"
#include "ble_mgr_irb_l2cap.h"
#include "ble_irb_helper.h"
#include "ble_common.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_storage.h"
#include "storage.h"

#include "gapc_task.h"
#include "gapm_task.h"

/** map API GAP device name write permissions to RW write permissions */
static uint8_t devname_perm_to_perm(uint16_t perm_in)
{
        uint16_t perm_out = 0;

        /* translate write permissions */
        if (perm_in & ATT_PERM_WRITE_AUTH) {
                perm_out = (PERM_RIGHT_AUTH << GAPM_POS_ATT_NAME_PERM);
        } else if (perm_in & ATT_PERM_WRITE_ENCRYPT) {
                perm_out = (PERM_RIGHT_UNAUTH << GAPM_POS_ATT_NAME_PERM);
        } else if (perm_in & ATT_PERM_WRITE) {
                perm_out = (PERM_RIGHT_ENABLE << GAPM_POS_ATT_NAME_PERM);
        }

        return perm_out;
}

/** map API GAP appearance write permissions to RW write permissions */
static uint8_t appearance_perm_to_perm(uint16_t perm_in)
{
        uint16_t perm_out = 0;

        /* translate write permissions */
        if (perm_in & ATT_PERM_WRITE_AUTH) {
                perm_out = (PERM_RIGHT_AUTH << GAPM_POS_ATT_APPEARENCE_PERM);
        } else if (perm_in & ATT_PERM_WRITE_ENCRYPT) {
                perm_out = (PERM_RIGHT_UNAUTH << GAPM_POS_ATT_APPEARENCE_PERM);
        } else if (perm_in & ATT_PERM_WRITE) {
                perm_out = (PERM_RIGHT_ENABLE << GAPM_POS_ATT_APPEARENCE_PERM);
        }

        return perm_out;
}

static irb_ble_stack_msg_t *ble_gap_dev_params_to_gtl(ble_dev_params_t *ble_dev_params)
{
        // Set device configuration
        irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;

        gmsg = ble_gtl_alloc(GAPM_SET_DEV_CONFIG_CMD,TASK_ID_GAPM,
                       sizeof(struct gapm_set_dev_config_cmd));
        gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;

        gcmd->operation = GAPM_SET_DEV_CONFIG;
        gcmd->role      = ble_dev_params->role;
        gcmd->renew_dur = ble_dev_params->addr_renew_duration;
        gcmd->att_cfg   = ble_dev_params->att_db_cfg;
        gcmd->max_mtu   = ble_dev_params->mtu_size;

        memcpy(&gcmd->addr, &ble_dev_params->own_addr.addr, BD_ADDR_LEN);
        switch (ble_dev_params->own_addr.addr_type) {
        case PUBLIC_STATIC_ADDRESS:
                gcmd->addr_type = GAPM_CFG_ADDR_PUBLIC;
                break;
        case PRIVATE_STATIC_ADDRESS:
                gcmd->addr_type = GAPM_CFG_ADDR_PRIVATE;
                break;
        case PRIVATE_RANDOM_RESOLVABLE_ADDRESS:
        case PRIVATE_RANDOM_NONRESOLVABLE_ADDRESS:
                gcmd->addr_type = GAPM_CFG_ADDR_PRIVACY;
                break;
        default:
                gcmd->addr_type = GAPM_CFG_ADDR_PUBLIC;
                break;
        }

        memcpy(&gcmd->irk, &ble_dev_params->irk, sizeof(gap_sec_key_t));

#if (RWBLE_SW_VERSION_MAJOR >= 8)
        /* Set max TX octets and time according to the defined maximum TX data length. */
        gcmd->max_txoctets = dg_configBLE_DATA_LENGTH_TX_MAX;
        gcmd->max_txtime   = (dg_configBLE_DATA_LENGTH_TX_MAX + 11 + 3) * 8;  // Conversion from llm.h
#endif

        return gmsg;
}

void irb_ble_handler_gap_dev_bdaddr_ind_evt(ble_gtl_msg_t *gtl)
{
        struct gapm_dev_bdaddr_ind *gevt = (void *) gtl->param;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        // Update device BD address
        memcpy(ble_dev_params->own_addr.addr, gevt->addr.addr.addr, BD_ADDR_LEN);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_adv_report_evt(ble_gtl_msg_t *gtl)
{
        struct gapm_adv_report_ind *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_adv_report_t *evt;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_ADV_REPORT, sizeof(*evt) + gevt->report.data_len);
        evt->type = gevt->report.evt_type;
        evt->rssi = gevt->report.rssi;
        evt->address.addr_type = gevt->report.adv_addr_type;
        memcpy(evt->address.addr, gevt->report.adv_addr.addr, sizeof(evt->address.addr));
        evt->length = gevt->report.data_len;
        memcpy(evt->data, gevt->report.data, gevt->report.data_len);

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */
}

static void gapm_address_resolve_complete(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = (OS_IRB *) param;
        ble_evt_gap_connected_t *evt = (ble_evt_gap_connected_t *) irb->ptr_buf;
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_connection_cfm *gcmd;
        device_t *dev;
        uint16_t svc_chg_ccc = 0x0000;

        gmsg = ble_gtl_alloc_with_conn(GAPC_CONNECTION_CFM, TASK_ID_GAPC, evt->conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_connection_cfm *) gmsg->msg.gtl.param;

        storage_acquire();

        dev = find_device_by_conn_idx(evt->conn_idx);
        if (!dev) {
                OS_FREE(irb);
                storage_release();
                return;
        }

        gcmd->auth = dev->bonded ? GAP_AUTH_BOND : 0;
        gcmd->auth |= dev->mitm ? GAP_AUTH_MITM : 0;

        /* Check if device was resolved and change address */
        if (gevt->status == GAP_ERR_NO_ERROR && dev->addr.addr_type !=
                                                                evt->peer_address.addr_type) {
                evt->peer_address.addr_type = dev->addr.addr_type;
                memcpy(evt->peer_address.addr, dev->addr.addr, sizeof(evt->peer_address.addr));
        }

        if (dev->csrk) {
                gcmd->lsign_counter = dev->csrk->sign_cnt;
                memcpy(&gcmd->lcsrk.key, dev->csrk->key, sizeof(gcmd->lcsrk.key));
        }

        if (dev->remote_csrk) {
                gcmd->rsign_counter = dev->remote_csrk->sign_cnt;
                memcpy(&gcmd->rcsrk.key, dev->remote_csrk->key, sizeof(gcmd->rcsrk.key));
        }

        dev->resolving = false;

        storage_release();

        /*
         * Retrieve value for Service Changed Characteristic CCC value
         */
        ble_storage_get_u16(evt->conn_idx, STORAGE_KEY_SVC_CHANGED_CCC, &svc_chg_ccc);
        gcmd->svc_changed_ind_enable = !!(svc_chg_ccc & GATT_CCC_INDICATIONS);

        ble_mgr_event_queue_send(irb, OS_QUEUE_FOREVER);
        OS_FREE(irb);

        ble_gtl_send(gmsg);
}

static bool device_match_irk(const device_t *dev, void *ud)
{
        struct gap_sec_key *irk = (struct gap_sec_key *) ud;

        if (!dev->irk) {
                return false;
        }

        return !memcmp(irk->key, dev->irk->key, sizeof(dev->irk->key));
}

void irb_ble_handler_gap_addr_solved_evt(ble_gtl_msg_t *gtl)
{
        struct gapm_addr_solved_ind *gevt = (void *) gtl->param;
        bd_address_t resolved_address;
        device_t *resolved_dev, *dev;

        memcpy(&resolved_address.addr, gevt->addr.addr, sizeof(resolved_address.addr));
        resolved_address.addr_type = PRIVATE_ADDRESS;

        storage_acquire();

        resolved_dev = find_device_by_addr(&resolved_address, false);
        if (!resolved_dev || !resolved_dev->connected) {
                goto done;
        }

        dev = find_device(device_match_irk, &gevt->irk);
        if (!dev) {
                goto done;
        }

        dev->conn_idx = resolved_dev->conn_idx;
        dev->master = resolved_dev->master;
        dev->connected = true;

        device_remove(resolved_dev);
done:
        storage_release();
}

static void irk_count_cb(const device_t *dev, void *ud)
{
        uint8_t *irk_count = (uint8_t *) ud;

        if (dev->irk) {
                (*irk_count)++;
        }
}

struct irk_copy_data {
        uint8_t index;
        struct gap_sec_key *array;
};

static void irk_copy_cb(const device_t *dev, void *ud)
{
        struct irk_copy_data *copy_data = (struct irk_copy_data *) ud;

        if (dev->irk) {
                memcpy(&copy_data->array[copy_data->index++], dev->irk->key, sizeof(dev->irk->key));
        }
}

static bool resolve_address_from_connected_evt(const struct gapc_connection_req_ind *evt, OS_IRB *irb)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_resolv_addr_cmd *gcmd;
        struct irk_copy_data copy_data;
        uint8_t irk_count = 0;

        /* Check if address is random */
        if (evt->peer_addr_type != PRIVATE_ADDRESS) {
                return false;
        }

        /* Check if address is resolvable */
        if ((evt->peer_addr.addr[5] & 0xc0) != 0x40) {
                return false;
        }

        device_foreach(irk_count_cb, &irk_count);
        if (irk_count == 0) {
                return false;
        }

        gmsg = ble_gtl_alloc(GAPM_RESOLV_ADDR_CMD, TASK_ID_GAPM, sizeof(*gcmd) +
                                                        (sizeof(struct gap_sec_key) * irk_count));
        gcmd = (struct gapm_resolv_addr_cmd *) gmsg->msg.gtl.param;
        memcpy(&gcmd->addr, &evt->peer_addr, sizeof(gcmd->addr));
        gcmd->operation = GAPM_RESOLV_ADDR;
        gcmd->nb_key = irk_count;

        /* Copy IRKs */
        copy_data.array = gcmd->irk;
        copy_data.index = 0;
        device_foreach(irk_copy_cb, &copy_data);

        ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_RESOLV_ADDR, gapm_address_resolve_complete, irb);
        ble_gtl_send(gmsg);

        return true;
}

static void get_peer_features(uint16_t conn_idx)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_get_info_cmd *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_GET_INFO_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_get_info_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_GET_PEER_FEATURES;

        ble_gtl_send(gmsg);
}

#if (RWBLE_SW_VERSION_MAJOR >= 8)
static void change_conn_data_length(uint16_t conn_idx, uint16_t tx_length, uint16_t tx_time)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_set_le_pkt_size_cmd *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_SET_LE_PKT_SIZE_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_set_le_pkt_size_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_SET_LE_PKT_SIZE;
        gcmd->tx_octets = tx_length;
        gcmd->tx_time = tx_time;

        ble_gtl_send(gmsg);
}
#endif

void irb_ble_handler_gap_peer_features_ind_evt(ble_gtl_msg_t *gtl)
{
#if ((RWBLE_SW_VERSION_MAJOR >= 8) \
        && ((dg_configBLE_DATA_LENGTH_RX_MAX > GAPM_LE_LENGTH_EXT_OCTETS_MIN) \
                     || (dg_configBLE_DATA_LENGTH_TX_MAX > GAPM_LE_LENGTH_EXT_OCTETS_MIN)))
        struct gapc_peer_features_ind *gevt = (void *) gtl->param;
        device_t *dev;

        /* Check if peer supports LE Data Packet Length Extension feature. */
        if (gevt->features[0] & BLE_LE_LENGTH_FEATURE) {
                storage_acquire();

                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));

                /* If we are the master of the connection initiate a Data Length Update procedure. */
                if (dev && dev->master) {
                        /* Set TX data length for connection to the maximum supported. */
                        change_conn_data_length(TASK_2_CONNIDX(gtl->src_id),
                                dg_configBLE_DATA_LENGTH_TX_MAX,
                                BLE_DATA_LENGTH_TO_TIME(dg_configBLE_DATA_LENGTH_TX_MAX));
                }

                storage_release();
        }
#endif
}

static void get_peer_version(uint16_t conn_idx)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_get_info_cmd *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_GET_INFO_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_get_info_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_GET_PEER_VERSION;

        ble_gtl_send(gmsg);
}

void irb_ble_handler_gap_peer_version_ind_evt(ble_gtl_msg_t *gtl)
{
        device_t *dev;

        storage_acquire();

        dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));

        if (dev && dev->master) {
                /* Initiate a Feature Exchange procedure. */
                get_peer_features(TASK_2_CONNIDX(gtl->src_id));
        }

        storage_release();
}

void irb_ble_handler_gap_connected_evt(ble_gtl_msg_t *gtl)
{
        // for IRB
        OS_IRB *irb;
        ble_evt_gap_connected_t *evt;
        // for GTL
        struct gapc_connection_req_ind *gevt = (void *) gtl->param;
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_connection_cfm *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        device_t *dev;
        uint16_t svc_chg_ccc = 0x0000;

        /*
         * In most cases OS_IRB structs are not allocated. We have to do it here, because
         * we must store connection details. If address is resolvable, irb will be passed
         * to gapm_address_resolve_complete.
         */
        irb = OS_MALLOC(sizeof(*irb));

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(irb, BLE_EVT_GAP_CONNECTED, sizeof(*evt));
        evt->conn_idx                  = TASK_2_CONNIDX(gtl->src_id);
        evt->own_addr.addr_type        = ble_dev_params->own_addr.addr_type;
        memcpy(evt->own_addr.addr, ble_dev_params->own_addr.addr, sizeof(evt->own_addr.addr));
        evt->peer_address.addr_type    = gevt->peer_addr_type;
        memcpy(evt->peer_address.addr, gevt->peer_addr.addr, sizeof(evt->peer_address.addr));
        evt->conn_params.interval_min  = gevt->con_interval;
        evt->conn_params.interval_max  = gevt->con_interval;
        evt->conn_params.slave_latency = gevt->con_latency;
        evt->conn_params.sup_timeout   = gevt->sup_to;

        //evt->clk_accuracy             = gevt->clk_accuracy;
#if (dg_configBLE_SKIP_LATENCY_API == 1) && (RWBLE_SW_VERSION_MAJOR >= 8)
        ble_mgr_skip_latency_set(evt->conn_idx, false);
#endif /* (dg_configBLE_SKIP_LATENCY_API == 1) && (RWBLE_SW_VERSION_MAJOR >= 8) */

        storage_acquire();

        dev = find_device_by_addr(&evt->peer_address, true);
        dev->conn_idx = evt->conn_idx;
        dev->connected = true;
        dev->mtu = ATT_DEFAULT_MTU;

        if (dev->connecting) {
                dev->master = true;
                dev->connecting = false;
        } else {
                dev->master = false;
        }

        if (dev->master) {
                /* Initiate a Version Exchange. */
                get_peer_version(evt->conn_idx);
        }

        if (resolve_address_from_connected_evt(gevt, irb)) {
                dev->resolving = true;
                goto done;
        }

        ble_mgr_event_queue_send(irb, OS_QUEUE_FOREVER);
        OS_FREE(irb);

        gmsg = ble_gtl_alloc_with_conn(GAPC_CONNECTION_CFM, TASK_ID_GAPC, evt->conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_connection_cfm *) gmsg->msg.gtl.param;
        gcmd->auth = dev->bonded ? GAP_AUTH_BOND : 0;
        gcmd->auth |= dev->mitm ? GAP_AUTH_MITM : 0;

        if (dev->csrk) {
                gcmd->lsign_counter = dev->csrk->sign_cnt;
                memcpy(&gcmd->lcsrk.key, dev->csrk->key, sizeof(gcmd->lcsrk.key));
        }

        if (dev->remote_csrk) {
                gcmd->rsign_counter = dev->remote_csrk->sign_cnt;
                memcpy(&gcmd->rcsrk.key, dev->remote_csrk->key, sizeof(gcmd->rcsrk.key));
        }

        /*
         * Retrieve value for Service Changed Characteristic CCC value
         */
        ble_storage_get_u16(evt->conn_idx, STORAGE_KEY_SVC_CHANGED_CCC, &svc_chg_ccc);
        gcmd->svc_changed_ind_enable = !!(svc_chg_ccc & GATT_CCC_INDICATIONS);

        ble_gtl_send(gmsg);

        // GAPC_CONNECTION_CFM does not have a response message, so just complete IRB and send back
done:
        storage_release();
        ble_mgr_dev_params_release();
}

static void gapm_address_set_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_address_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_address_set_rsp_t *rsp;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

                /* Update ble_dev_params with new values for address and address_type */
                ble_dev_params->own_addr.addr_type = cmd->address->addr_type;

                switch (cmd->address->addr_type) {
                case PUBLIC_STATIC_ADDRESS:
                        memcpy(ble_dev_params->own_addr.addr, ad_ble_get_public_address(),
                                                                                BD_ADDR_LEN);
                        break;
                case PRIVATE_STATIC_ADDRESS:
                        memcpy(ble_dev_params->own_addr.addr, cmd->address->addr, BD_ADDR_LEN);
                        break;
                default:
                        /*
                         * Assume it's either private random non-resolvable or resolvable address.
                         * We clear addr field to avoid confusion in application - only address type
                         * matters here. Proper address will be written when GAPM_DEV_BDADDR_IND
                         * is received.
                         */
                        memset(ble_dev_params->own_addr.addr, 0, BD_ADDR_LEN);

                        ble_dev_params->addr_renew_duration = cmd->renew_dur;
                        break;
                }

                ble_mgr_dev_params_release();
        }

        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ADDRESS_SET_CMD, sizeof(*rsp));
        rsp->status = (gevt->status == 0 ? BLE_STATUS_OK : BLE_ERROR_FAILED);

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_address_set_cmd(OS_IRB *irb)
{
        const irb_ble_gap_address_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_address_set_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        // Check if an air operation is in progress (GAPM_SET_DEV_CONFIG_CMD cannot be sent now)
        if ((ble_dev_params->advertising == true) || (ble_dev_params->scanning == true)) {
                goto done;
        }

        // Setup GTL message to update device configuration (for addr_type and addr)
        gmsg = ble_gap_dev_params_to_gtl(ble_dev_params);
        gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;
        switch (cmd->address->addr_type) {
        case PUBLIC_STATIC_ADDRESS:
                gcmd->addr_type = GAPM_CFG_ADDR_PUBLIC;
                break;
        case PRIVATE_STATIC_ADDRESS:
                gcmd->addr_type = GAPM_CFG_ADDR_PRIVATE;
                memcpy(gcmd->addr.addr, cmd->address->addr, BD_ADDR_LEN);
                break;
        case PRIVATE_RANDOM_RESOLVABLE_ADDRESS:
        case PRIVATE_RANDOM_NONRESOLVABLE_ADDRESS:
                // copy irk from ble_dev_params
                memcpy(gcmd->irk.key, ble_dev_params->irk.key, RAND_NB_LEN);
                gcmd->renew_dur = cmd->renew_dur;
                gcmd->addr_type = GAPM_CFG_ADDR_PRIVACY;
                break;
        default:
                gcmd->addr_type = GAPM_CFG_ADDR_PUBLIC;
                break;
        }

        // Keep cmd buffer, we'll need it when creating response
        irb_ble_mark_completed(irb, false);
        ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_DEV_CONFIG, gapm_address_set_rsp, irb);

        ble_gtl_send(gmsg);

        ble_mgr_dev_params_release();
        return;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ADDRESS_SET_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

static void gapm_att_db_cfg_devname_perm_set_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_device_name_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_device_name_set_rsp_t *rsp;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

                /* Update ble_dev_params with new values for device name and att_db_cfg */
                strcpy(ble_dev_params->dev_name, cmd->name);
                ble_dev_params->att_db_cfg = (ble_dev_params->att_db_cfg & ~GAPM_MASK_ATT_NAME_PERM)
                                             | devname_perm_to_perm(cmd->perm);

                ble_mgr_dev_params_release();
        }

        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_DEVICE_NAME_SET_CMD, sizeof(*rsp));
        rsp->status = (gevt->status == 0 ? BLE_STATUS_OK : BLE_ERROR_FAILED);

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_device_name_set_cmd(OS_IRB *irb)
{
        const irb_ble_gap_device_name_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_device_name_set_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        // Check if provided name is longer than the defined max size for the device name
        if ( (strlen(cmd->name) > BLE_GAP_DEVNAME_LEN_MAX) ) {
                goto done;
        }

        // Check if the attribute database configuration bit flag needs updating
        if ((ble_dev_params->att_db_cfg & GAPM_MASK_ATT_NAME_PERM) != devname_perm_to_perm(cmd->perm)) {
                /* att_db_cfg has to be updated */

                // Check if an air operation is in progress (att_db_cfg cannot be updated now)
                if ((ble_dev_params->advertising == true) || (ble_dev_params->scanning == true)) {
                        goto done;
                }

                // Setup GTL message to update device configuration (for att_db_cfg)
                gmsg = ble_gap_dev_params_to_gtl(ble_dev_params);
                gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;
                gcmd->att_cfg = (ble_dev_params->att_db_cfg & ~GAPM_MASK_ATT_NAME_PERM) |
                        devname_perm_to_perm(cmd->perm);

                // Keep cmd buffer, we'll need it when creating response
                irb_ble_mark_completed(irb, false);
                ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_DEV_CONFIG,
                                      gapm_att_db_cfg_devname_perm_set_rsp, irb);

                ble_gtl_send(gmsg);

                ble_mgr_dev_params_release();
                return;
        }

        /* No att_db_cfg update needed */

        // Update ble_dev_params with the new value for device name
        strcpy(ble_dev_params->dev_name, cmd->name);
        ret = BLE_STATUS_OK;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_DEVICE_NAME_SET_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

static void gapm_att_db_cfg_appearance_perm_set_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_appearance_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_appearance_set_rsp_t *rsp;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

                // Update appearance member and attribute DB configuration in ble_dev_params
                ble_dev_params->appearance = cmd->appearance;
                ble_dev_params->att_db_cfg = (ble_dev_params->att_db_cfg &
                        ~GAPM_MASK_ATT_APPEARENCE_PERM) | appearance_perm_to_perm(cmd->perm);

                ble_mgr_dev_params_release();
        }

        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_APPEARANCE_SET_CMD, sizeof(*rsp));
        rsp->status = (gevt->status == 0 ? BLE_STATUS_OK : BLE_ERROR_FAILED);

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_appearance_set_cmd(OS_IRB *irb)
{
        const irb_ble_gap_appearance_set_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_appearance_set_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        // Check if the attribute database configuration bit flag needs updating
        if ((ble_dev_params->att_db_cfg & GAPM_MASK_ATT_APPEARENCE_PERM) !=
                appearance_perm_to_perm(cmd->perm)) {
                /* att_db_cfg has to be updated */

                // Check if an air operation is in progress (att_db_cfg cannot be updated now)
                if ((ble_dev_params->advertising == true) || (ble_dev_params->scanning == true)) {
                        goto done;
                }

                // Setup GTL message to update att_db_cfg in device configuration
                gmsg = ble_gap_dev_params_to_gtl(ble_dev_params);
                gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;
                gcmd->att_cfg = (ble_dev_params->att_db_cfg & ~GAPM_MASK_ATT_APPEARENCE_PERM) |
                        appearance_perm_to_perm(cmd->perm);

                // Keep cmd buffer, we'll need it when creating response
                irb_ble_mark_completed(irb, false);
                ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_DEV_CONFIG,
                                      gapm_att_db_cfg_appearance_perm_set_rsp, irb);
                ble_gtl_send(gmsg);

                ble_mgr_dev_params_release();
                return;
        }

        /* No att_db_cfg update needed */

        // Just update appearance member in ble_dev_params
        ble_dev_params->appearance = cmd->appearance;

        ret = BLE_STATUS_OK;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_APPEARANCE_SET_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

static void gapm_att_db_cfg_ppcp_en_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        const irb_ble_gap_ppcp_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_ppcp_set_rsp_t *rsp;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

                // Update PPCP member in ble_dev_params
                memcpy(&ble_dev_params->gap_ppcp, cmd->gap_ppcp, sizeof(ble_dev_params->gap_ppcp));
                // Update the PPCP present bit in att_db_cfg
                ble_dev_params->att_db_cfg |= GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;

                ble_mgr_dev_params_release();
        }

        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_PPCP_SET_CMD, sizeof(*rsp));
        rsp->status = (gevt->status == 0 ? BLE_STATUS_OK : BLE_ERROR_FAILED);

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_ppcp_set_cmd(OS_IRB *irb)
{
        const irb_ble_gap_ppcp_set_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_ppcp_set_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        // Check if the attribute database configuration bit flag needs updating
        if ((ble_dev_params->att_db_cfg & GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN) == 0x00) {
                /* att_db_cfg has to be updated */

                // Check if an air operation is in progress (att_db_cfg cannot be updated now)
                if ((ble_dev_params->advertising == true) || (ble_dev_params->scanning == true)) {
                        goto done;
                }

                // Setup GTL message to update att_db_cfg in device configuration
                gmsg = ble_gap_dev_params_to_gtl(ble_dev_params);
                gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;

                // Enable PPCP present bit in att_db_cfg
                gcmd->att_cfg = ble_dev_params->att_db_cfg | GAPM_MASK_ATT_SLV_PREF_CON_PAR_EN;

                irb_ble_mark_completed(irb, false);
                ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_DEV_CONFIG,
                                      gapm_att_db_cfg_ppcp_en_rsp, irb);
                ble_gtl_send(gmsg);

                ble_mgr_dev_params_release();

                return;
        }

        /* No att_db_cfg update needed */

        // Just update PPCP member in ble_dev_params
        memcpy(&ble_dev_params->gap_ppcp, cmd->gap_ppcp, sizeof(ble_dev_params->gap_ppcp));

        ret = BLE_STATUS_OK;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_PPCP_SET_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_adv_start_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_adv_start_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_adv_start_rsp_t *rsp;
        // for GTL
        irb_ble_stack_msg_t *gmsg;
        struct gapm_start_advertise_cmd *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        // Check if an advertising operation is already in progress
        if (ble_dev_params->advertising == true) {
                ret = BLE_ERROR_IN_PROGRESS;
                goto done;
        }

        // Update BLE device advertising parameters
        ble_dev_params->adv_type = cmd->adv_type;

        gmsg = ble_gtl_alloc(GAPM_START_ADVERTISE_CMD, TASK_ID_GAPM, sizeof(struct gapm_start_advertise_cmd));
        gcmd = (struct gapm_start_advertise_cmd *) gmsg->msg.gtl.param;

        // Translate advertising type to stack operation code
        switch(cmd->adv_type) {
        case GAP_CONN_MODE_NON_CONN:
                gcmd->op.code = GAPM_ADV_NON_CONN;
                break;
        case GAP_CONN_MODE_UNDIRECTED:
                gcmd->op.code = GAPM_ADV_UNDIRECT;
                break;
        case GAP_CONN_MODE_DIRECTED:
                gcmd->op.code = GAPM_ADV_DIRECT;
                break;
        case GAP_CONN_MODE_DIRECTED_LDC:
                gcmd->op.code = GAPM_ADV_DIRECT_LDC;
                break;
        }

        switch(ble_dev_params->own_addr.addr_type) {
        case PUBLIC_STATIC_ADDRESS:
        case PRIVATE_STATIC_ADDRESS:
                gcmd->op.addr_src = GAPM_STATIC_ADDR;
                break;
        case PRIVATE_RANDOM_RESOLVABLE_ADDRESS:
                gcmd->op.addr_src = GAPM_GEN_RSLV_ADDR;
                break;
        case PRIVATE_RANDOM_NONRESOLVABLE_ADDRESS:
                gcmd->op.addr_src = GAPM_GEN_NON_RSLV_ADDR;
                break;
        default:
                gcmd->op.addr_src = GAPM_STATIC_ADDR;
                break;
        }

        gcmd->intv_min                    = ble_dev_params->adv_intv_min;
        gcmd->intv_max                    = ble_dev_params->adv_intv_max;
        gcmd->channel_map                 = ble_dev_params->adv_channel_map;
        if (cmd->adv_type < GAP_CONN_MODE_DIRECTED) {
                /* Fill info for undirected or broadcaster mode advertising */
                gcmd->info.host.mode              = ble_dev_params->adv_mode;
                gcmd->info.host.adv_filt_policy   = ble_dev_params->adv_filter_policy;
                gcmd->info.host.adv_data_len      = ble_dev_params->adv_data_length;
                memcpy(gcmd->info.host.adv_data, ble_dev_params->adv_data,
                        ble_dev_params->adv_data_length);
                gcmd->info.host.scan_rsp_data_len = ble_dev_params->scan_rsp_data_length;
                memcpy(gcmd->info.host.scan_rsp_data, ble_dev_params->scan_rsp_data,
                        ble_dev_params->scan_rsp_data_length);
        }
        else {
                /* Fill info for directed advertising */
                gcmd->info.direct.addr_type = ble_dev_params->adv_direct_address.addr_type;
                memcpy(gcmd->info.direct.addr.addr, ble_dev_params->adv_direct_address.addr,
                        BD_ADDR_LEN);
        }

        // set advertising boolean to true
        ble_dev_params->advertising = true;

        ble_gtl_send(gmsg);

        ret = BLE_STATUS_OK;
done:
        irb_ble_mark_completed(irb, true);

        // Replace IRB message with response
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ADV_START_CMD, sizeof(*rsp));

        rsp = irb->ptr_buf;

        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gapm_adv_cmp_evt(ble_gtl_msg_t *gtl)
{
        // for IRB
        OS_IRB irb;
        ble_evt_gap_adv_completed_t *evt;
        // for GTL
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        // set advertising boolean to false
        ble_dev_params->advertising = false;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_ADV_COMPLETED, sizeof(*evt));

        // Translate stack advertising operation type
        switch(gevt->operation) {
        case GAPM_ADV_NON_CONN:
                evt->adv_type = GAP_CONN_MODE_NON_CONN;
                break;
        case GAPM_ADV_UNDIRECT:
                evt->adv_type = GAP_CONN_MODE_UNDIRECTED;
                break;
        case GAPM_ADV_DIRECT:
                evt->adv_type = GAP_CONN_MODE_DIRECTED;
                break;
        case GAPM_ADV_DIRECT_LDC:
                evt->adv_type = GAP_CONN_MODE_DIRECTED_LDC;
                break;
        }

        // Translate stack status
        switch(gevt->status) {
        case GAP_ERR_NO_ERROR:
                evt->status = BLE_STATUS_OK;
                break;
        case GAP_ERR_CANCELED:
                evt->status = BLE_ERROR_CANCELED;
                break;
        case GAP_ERR_COMMAND_DISALLOWED:
                evt->status = BLE_ERROR_NOT_ALLOWED;
                break;
        case GAP_ERR_INVALID_PARAM:
        case GAP_ERR_ADV_DATA_INVALID:
        case LL_ERR_PARAM_OUT_OF_MAND_RANGE:
                evt->status = BLE_ERROR_INVALID_PARAM;
                break;
        case GAP_ERR_NOT_SUPPORTED:
        case GAP_ERR_PRIVACY_CFG_PB:
                evt->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case GAP_ERR_TIMEOUT:
                evt->status = BLE_ERROR_TIMEOUT;
                break;
        default:
                evt->status   = gevt->status;
                break;
        }

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */

        ble_mgr_dev_params_release();
}

static void send_gapm_cancel_cmd(void)
{
        irb_ble_stack_msg_t *gmsg;
        struct gapm_cancel_cmd *gcmd;

        // NOTE: this actually cancels *any* ongoing air operation
        gmsg = ble_gtl_alloc(GAPM_CANCEL_CMD, TASK_ID_GAPM, sizeof(*gcmd));
        gcmd = (struct gapm_cancel_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPM_CANCEL;

        ble_gtl_send(gmsg);
}

void irb_ble_handler_gap_adv_stop_cmd(OS_IRB *irb)
{
        // for IRB
        irb_ble_gap_adv_stop_rsp_t *rsp;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        if (!ble_dev_params->advertising) {
                ret = BLE_ERROR_NOT_ALLOWED;
                goto done;
        }

        /*
         * This assumes we will cancel this properly (as we check whether we're actually advertising)
         * and event will be generated to application when GAPM_CMP_EVT is received with proper
         * operation for advertising.
         * In case something failed, GAPM_CMD_EVT will have operation set to GAPM_CANCEL and we will
         * just discard it silently - not much we can do there anyway.
         */
        send_gapm_cancel_cmd();
        ret = BLE_STATUS_OK;

done:
        ble_mgr_dev_params_release();
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ADV_STOP_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

static void gapm_adv_data_update_cmd_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_adv_data_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_adv_data_set_rsp_t *rsp;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

                ble_dev_params->adv_data_length = cmd->adv_data_len;
                memcpy(ble_dev_params->adv_data, cmd->adv_data, cmd->adv_data_len);
                ble_dev_params->scan_rsp_data_length = cmd->scan_rsp_data_len;
                memcpy(ble_dev_params->scan_rsp_data, cmd->scan_rsp_data, cmd->scan_rsp_data_len);

                ble_mgr_dev_params_release();
        }

        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ADV_DATA_SET_CMD, sizeof(*rsp));

        rsp->status = (gevt->status == 0 ? BLE_STATUS_OK : BLE_ERROR_FAILED);

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_adv_data_set_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_adv_data_set_cmd_t *cmd = (void *) irb->ptr_buf;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        if (ble_dev_params->advertising == true) {
                // for GTL
                static irb_ble_stack_msg_t *gmsg;
                struct gapm_update_advertise_data_cmd *gcmd;

                // setup GTL message
                gmsg = ble_gtl_alloc(GAPM_UPDATE_ADVERTISE_DATA_CMD, TASK_ID_GAPM, sizeof(*gcmd));
                gcmd = (struct gapm_update_advertise_data_cmd *) gmsg->msg.gtl.param;
                gcmd->operation = GAPM_UPDATE_ADVERTISE_DATA;
                gcmd->adv_data_len = cmd->adv_data_len;
                memcpy(gcmd->adv_data, cmd->adv_data, cmd->adv_data_len);
                gcmd->scan_rsp_data_len = cmd->scan_rsp_data_len;
                memcpy(gcmd->scan_rsp_data, cmd->scan_rsp_data, cmd->scan_rsp_data_len);


                irb_ble_mark_completed(irb, false);

                ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_UPDATE_ADVERTISE_DATA,
                                      gapm_adv_data_update_cmd_rsp, irb);
                ble_gtl_send(gmsg);
        }
        else {
                irb_ble_gap_adv_data_set_rsp_t *rsp;

                ble_dev_params->adv_data_length = cmd->adv_data_len;
                memcpy(ble_dev_params->adv_data, cmd->adv_data, cmd->adv_data_len);
                ble_dev_params->scan_rsp_data_length = cmd->scan_rsp_data_len;
                memcpy(ble_dev_params->scan_rsp_data, cmd->scan_rsp_data, cmd->scan_rsp_data_len);

                irb_ble_mark_completed(irb, true);
                rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ADV_DATA_SET_CMD, sizeof(*rsp));

                rsp->status = BLE_STATUS_OK;

                ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
        }

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_scan_start_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_scan_start_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_scan_start_rsp_t *rsp;
        // for GTL
        irb_ble_stack_msg_t *gmsg;
        struct gapm_start_scan_cmd *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        gmsg = ble_gtl_alloc(GAPM_START_SCAN_CMD, TASK_ID_GAPM, sizeof(struct gapm_start_scan_cmd));
        gcmd = (struct gapm_start_scan_cmd *) gmsg->msg.gtl.param;

        // Translate scan type to stack operation code
        switch(cmd->type) {
        case GAP_SCAN_ACTIVE:
                gcmd->op.code = GAPM_SCAN_ACTIVE;
                break;
        case GAP_SCAN_PASSIVE:
                gcmd->op.code = GAPM_SCAN_PASSIVE;
                break;
        }

        switch(ble_dev_params->own_addr.addr_type) {
        case PUBLIC_STATIC_ADDRESS:
        case PRIVATE_STATIC_ADDRESS:
                gcmd->op.addr_src = GAPM_STATIC_ADDR;
                break;
        case PRIVATE_RANDOM_RESOLVABLE_ADDRESS:
                gcmd->op.addr_src = GAPM_GEN_RSLV_ADDR;
                break;
        case PRIVATE_RANDOM_NONRESOLVABLE_ADDRESS:
                gcmd->op.addr_src = GAPM_GEN_NON_RSLV_ADDR;
                break;
        default:
                gcmd->op.addr_src = GAPM_STATIC_ADDR;
                break;
        }

        gcmd->interval      = cmd->interval;
        gcmd->window        = cmd->window;
        gcmd->mode          = cmd->mode;
        gcmd->filt_policy   = cmd->filt_wlist ? SCAN_ALLOW_ADV_WLST : SCAN_ALLOW_ADV_ALL;
        gcmd->filter_duplic = cmd->filt_dupl ? SCAN_FILT_DUPLIC_EN : SCAN_FILT_DUPLIC_DIS;

        // set advertising boolean to true
        ble_dev_params->scanning = true;

        ble_gtl_send(gmsg);

        irb_ble_mark_completed(irb, true);

        // Replace IRB message with response
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_SCAN_START_CMD, sizeof(*rsp));

        rsp = irb->ptr_buf;

        rsp->status = BLE_STATUS_OK;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gapm_scan_cmp_evt(ble_gtl_msg_t *gtl)
{
        // for IRB
        OS_IRB irb;
        ble_evt_gap_scan_completed_t *evt;
        // for GTL
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        // set advertising boolean to false
        ble_dev_params->scanning = false;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_SCAN_COMPLETED, sizeof(*evt));

        switch(gevt->operation) {
        case GAPM_SCAN_ACTIVE:
                evt->scan_type = GAP_SCAN_ACTIVE;
                break;
        case GAPM_SCAN_PASSIVE:
                evt->scan_type = GAP_SCAN_PASSIVE;
                break;
        }

        // Translate stack status
        switch(gevt->status) {
        case GAP_ERR_NO_ERROR:
        case GAP_ERR_CANCELED:
                evt->status = BLE_STATUS_OK;
                break;
        case GAP_ERR_INVALID_PARAM:
                evt->status = BLE_ERROR_INVALID_PARAM;
                break;
        case GAP_ERR_NOT_SUPPORTED:
        case GAP_ERR_PRIVACY_CFG_PB:
                evt->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case GAP_ERR_TIMEOUT:
                evt->status = BLE_ERROR_TIMEOUT;
                break;
        case GAP_ERR_COMMAND_DISALLOWED:
                evt->status = BLE_ERROR_NOT_ALLOWED;
                break;
        default:
                evt->status   = gevt->status;
                break;
        }

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_scan_stop_cmd(OS_IRB *irb)
{
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;
        // for IRB
        irb_ble_gap_scan_stop_rsp_t *rsp;

        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_SCAN_STOP_CMD, sizeof(*rsp));

        // Check if a scanning is in progress
        if (!ble_dev_params->scanning) {
                ret = BLE_ERROR_NOT_ALLOWED;
                goto done;
        }

        send_gapm_cancel_cmd();
        ret = BLE_STATUS_OK;
done:
        ble_mgr_dev_params_release();
        rsp->status = ret;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

static bool match_connecting_dev(const device_t *dev, void *ud)
{
        return dev->connecting;
}

void irb_ble_handler_gapm_connect_cmp_evt(ble_gtl_msg_t *gtl)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        device_t *dev;

        // Reset connecting flag in ble_dev_params
        ble_dev_params->connecting = false;

        ble_mgr_dev_params_release();

        // remove temporary device which was created in connect command
        if (gevt->status != GAP_ERR_NO_ERROR) {
                storage_acquire();

                dev = find_device(match_connecting_dev, NULL);
                if (dev) {
                        device_remove(dev);
                }

                storage_release();
        }
}

void irb_ble_handler_gap_connect_cmd(OS_IRB *irb)
{
        const irb_ble_gap_connect_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_connect_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_start_connection_cmd *gcmd;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        device_t *dev;
        ble_error_t ret = BLE_ERROR_FAILED;

        storage_acquire();

        dev = find_device(match_connecting_dev, NULL);
        if (dev) {
                ret = BLE_ERROR_BUSY;
                goto done;
        }

        /* Check if we are already connected */
        dev = find_device_by_addr(cmd->peer_addr, true);
        if (!dev) {
                goto done;
        }

        if (dev->connected) {
                ret = BLE_ERROR_ALREADY_DONE;
                goto done;
        }

        // setup GTL message
        gmsg = ble_gtl_alloc(GAPM_START_CONNECTION_CMD, TASK_ID_GAPM, sizeof(*gcmd) + sizeof(struct gap_bdaddr));
        gcmd = (struct gapm_start_connection_cmd *) gmsg->msg.gtl.param;
        gcmd->op.code = GAPM_CONNECTION_DIRECT;
        switch (ble_dev_params->own_addr.addr_type) {
        case PUBLIC_STATIC_ADDRESS:
        case PRIVATE_STATIC_ADDRESS:
                gcmd->op.addr_src = GAPM_STATIC_ADDR;
                break;
        case PRIVATE_RANDOM_RESOLVABLE_ADDRESS:
                gcmd->op.addr_src = GAPM_GEN_RSLV_ADDR;
                break;
        case PRIVATE_RANDOM_NONRESOLVABLE_ADDRESS:
                gcmd->op.addr_src = GAPM_GEN_NON_RSLV_ADDR;
                break;
        }
        gcmd->scan_interval = ble_dev_params->scan_params.interval;
        gcmd->scan_window = ble_dev_params->scan_params.window;
        gcmd->con_intv_min = cmd->conn_params->interval_min;
        gcmd->con_intv_max = cmd->conn_params->interval_max;
        gcmd->con_latency = cmd->conn_params->slave_latency;
        gcmd->superv_to = cmd->conn_params->sup_timeout;
        gcmd->ce_len_min = 0;
        gcmd->ce_len_max = 200;
        gcmd->nb_peers = 1;
        gcmd->peers[0].addr_type = cmd->peer_addr->addr_type;
        memcpy(&gcmd->peers[0].addr.addr, cmd->peer_addr->addr, sizeof(gcmd->peers[0].addr.addr));

        // Set connecting flag in ble_dev_params
        ble_dev_params->connecting = true;
        dev->connecting = true;

        ble_gtl_send(gmsg);

        ret = BLE_STATUS_OK;
done:
        storage_release();
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CONNECT_CMD, sizeof(*rsp));

        rsp->status = ret;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_connect_cancel_cmd(OS_IRB *irb)
{
        irb_ble_gap_connect_cancel_rsp_t *rsp;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CONNECT_CANCEL_CMD, sizeof(*rsp));

        // Check if a connection is in progress
        if (!ble_dev_params->connecting) {
                ret = BLE_ERROR_NOT_ALLOWED;
                goto done;
        }

        send_gapm_cancel_cmd();
        ret = BLE_STATUS_OK;
done:
        ble_mgr_dev_params_release();
        rsp->status = ret;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gapc_cmp__disconnect_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_disconnect_failed_t *evt;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                /* nothing to do, event should be sent only when error occurred */
                return;
        }

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_DISCONNECT_FAILED, sizeof(*evt));
        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);

        /* translate error code */
        switch (gevt->status) {
        case GAP_ERR_INVALID_PARAM:
        case LL_ERR_INVALID_HCI_PARAM:
                evt->status = BLE_ERROR_INVALID_PARAM;
                break;
        case LL_ERR_COMMAND_DISALLOWED:
                evt->status = BLE_ERROR_NOT_ALLOWED;
                break;
        default:
                evt->status = BLE_ERROR_FAILED;
                break;
        }

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
}

static void send_gapc_disconnect_cmd(uint16_t conn_idx, uint8_t reason)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_disconnect_cmd *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_DISCONNECT_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_disconnect_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_DISCONNECT;
        gcmd->reason = reason;

        ble_gtl_send(gmsg);
}

void irb_ble_handler_gap_disconnect_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_disconnect_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_disconnect_rsp_t *rsp;
        device_t *dev;
        ble_error_t ret = BLE_ERROR_FAILED;

        storage_acquire();

        /* Check if the connection exists */
        dev = find_device_by_conn_idx(cmd->conn_idx);

        storage_release();

        if (!dev) {
                ret = BLE_ERROR_NOT_CONNECTED;
        } else {
                send_gapc_disconnect_cmd(cmd->conn_idx, cmd->reason);
                ret = BLE_STATUS_OK;
        }

        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_DISCONNECT_CMD, sizeof(*rsp));

        rsp->status = ret;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_disconnected_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_disconnect_ind *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_disconnected_t *evt;
        device_t *dev;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();


        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_DISCONNECTED, sizeof(*evt));
        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);
        evt->reason   = gevt->reason;

        /* need to notify L2CAP IRB handler so it can 'deallocate' all channels for given conn_idx */
        l2cap_disconnect_ind(evt->conn_idx);

        storage_acquire();

        dev = find_device_by_conn_idx(evt->conn_idx);
        if (!dev) {
                /* device not found in storage - ignore event */
                storage_release();
                ble_mgr_dev_params_release();
                OS_FREE(evt);
                return;
        }

        /* copy peer address to event */
        memcpy(&evt->address, &dev->addr, sizeof(evt->address));

        /*
         * for bonded device we only need to remove non-persistent appvals and mark device as not
         * connected; otherwise remove device from storage
         */
        if (dev->bonded) {
                dev->connected = false;
                dev->encrypted = false;
                app_value_remove_np(dev);
        } else {
                device_remove(dev);
        }

        storage_release();

        /* flush any waitqueue elements corresponding to this connection */
        ble_gtl_waitqueue_flush(evt->conn_idx);

        /* Clear updating flag */
        ble_dev_params->updating = false;

#if (dg_configBLE_SKIP_LATENCY_API == 1) && (RWBLE_SW_VERSION_MAJOR >= 8)
        ble_mgr_skip_latency_set(evt->conn_idx, false);
#endif /* (dg_configBLE_SKIP_LATENCY_API == 1) && (RWBLE_SW_VERSION_MAJOR >= 8) */

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */

        ble_mgr_dev_params_release();
}

static void gap_get_con_rssi_rsp(ble_gtl_msg_t *gtl, void *param)
{
        OS_IRB *irb = param;
        irb_ble_gap_conn_rssi_get_rsp_t *rsp = irb->ptr_buf;

        if (gtl != NULL) {
                struct gapc_con_rssi_ind *grsp = (void *) gtl->param;

                rsp->conn_rssi = (int8_t) grsp->rssi;

                rsp->status = BLE_STATUS_OK;
        }
        else {
                rsp->status = BLE_ERROR_NOT_CONNECTED;
        }

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_conn_rssi_get_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_conn_rssi_get_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_conn_rssi_get_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_get_info_cmd *gcmd;
        // local
        ble_error_t ret = BLE_ERROR_FAILED;
        device_t *dev;

        storage_acquire();

        dev = find_device_by_conn_idx(cmd->conn_idx);
        if (!dev) {
                /* no active connection corresponds to provided index */
                ret = BLE_ERROR_NOT_CONNECTED;
                storage_release();
                goto done;
        }

        storage_release();

        // setup GTL message
        gmsg = ble_gtl_alloc_with_conn(GAPC_GET_INFO_CMD, TASK_ID_GAPC, cmd->conn_idx, sizeof(*gcmd));
        gcmd = (typeof(gcmd)) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_GET_CON_RSSI;

        ble_gtl_waitqueue_add(cmd->conn_idx, GAPC_CON_RSSI_IND, 0, gap_get_con_rssi_rsp, irb);

        // replace command IRB message with response
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CONN_RSSI_GET_CMD, sizeof(*rsp));

        ble_gtl_send(gmsg);

        return;
done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CONN_RSSI_GET_CMD, sizeof(*rsp));
        rsp->status = ret;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_get_device_info_req_evt(ble_gtl_msg_t *gtl)
{
        // for GTL
        struct gapc_get_dev_info_req_ind *gevt = (void *) gtl->param;
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_get_dev_info_cfm *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        /* send GAPC_GET_DEV_INFO_CFM message to stack */
        gmsg = ble_gtl_alloc(GAPC_GET_DEV_INFO_CFM, gtl->src_id,
                sizeof(*gcmd) + sizeof(ble_dev_params->dev_name));
        gcmd = (struct gapc_get_dev_info_cfm *) gmsg->msg.gtl.param;

        gcmd->req = gevt->req;

        switch (gevt->req) {
        case GAPC_DEV_NAME:
                gcmd->info.name.length = strlen(ble_dev_params->dev_name);
                memcpy(&gcmd->info.name.value, &ble_dev_params->dev_name,
                        strlen(ble_dev_params->dev_name));
                break;
        case GAPC_DEV_APPEARANCE:
                gcmd->info.appearance = ble_dev_params->appearance;
                break;
        case GAPC_DEV_SLV_PREF_PARAMS:
                gcmd->info.slv_params.con_intv_min  = ble_dev_params->gap_ppcp.interval_min;
                gcmd->info.slv_params.con_intv_max  = ble_dev_params->gap_ppcp.interval_max;
                gcmd->info.slv_params.slave_latency = ble_dev_params->gap_ppcp.slave_latency;
                gcmd->info.slv_params.conn_timeout  = ble_dev_params->gap_ppcp.sup_timeout;
                break;
        default:
                /* do nothing */
                break;
        }

        ble_gtl_send(gmsg);

        // GAPC_GET_DEV_INFO_CFM does not have a response message, so just send the message

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_set_device_info_req_evt(ble_gtl_msg_t *gtl)
{
        // for GTL
        struct gapc_set_dev_info_req_ind *gevt = (void *) gtl->param;
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_set_dev_info_cfm *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        /* send GAPC_SET_DEV_INFO_CFM message to stack */
        gmsg = ble_gtl_alloc(GAPC_SET_DEV_INFO_CFM, gtl->src_id, sizeof(*gcmd));
        gcmd = (struct gapc_set_dev_info_cfm *) gmsg->msg.gtl.param;

        gcmd->req = gevt->req;

        switch (gevt->req) {
        case GAPC_DEV_NAME:
                if (gevt->info.name.length > BLE_GAP_DEVNAME_LEN_MAX) {
                        gcmd->status = GAP_ERR_INSUFF_RESOURCES;
                        goto done;
                }
                // copy name written by peer device to ble_dev_params
                memcpy(ble_dev_params->dev_name, &gevt->info.name.value, gevt->info.name.length);
                // insert NULL character
                ble_dev_params->dev_name[gevt->info.name.length] = '\0';
                gcmd->status = GAP_ERR_NO_ERROR;
done:
                ble_gtl_send(gmsg);
                // GAPC_SET_DEV_INFO_CFM does not have a response message, so just send the message
                break;
        case GAPC_DEV_APPEARANCE:
                /* Update appearance value in ble_dev_params */
                ble_dev_params->appearance = gevt->info.appearance;
                break;
        default:
                /* do nothing */
                break;
        }

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_conn_param_update_req_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_param_update_req_ind *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_conn_param_update_req_t *evt;
        ble_dev_params_t *params = ble_mgr_dev_params_acquire();

        /* Set updating flag */
        params->updating = true;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_CONN_PARAM_UPDATE_REQ, sizeof(*evt));
        evt->conn_idx                  = TASK_2_CONNIDX(gtl->src_id);
        evt->conn_params.interval_min  = gevt->intv_min;
        evt->conn_params.interval_max  = gevt->intv_max;
        evt->conn_params.slave_latency = gevt->latency;
        evt->conn_params.sup_timeout   = gevt->time_out;

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_conn_param_updated_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_param_updated_ind *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_conn_param_updated_t *evt;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_CONN_PARAM_UPDATED, sizeof(*evt));
        evt->conn_idx                  = TASK_2_CONNIDX(gtl->src_id);
        evt->conn_params.interval_min  = gevt->con_interval;
        evt->conn_params.interval_max  = gevt->con_interval;
        evt->conn_params.slave_latency = gevt->con_latency;
        evt->conn_params.sup_timeout   = gevt->sup_to;

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */
}

static void gapm_set_role_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_role_set_rsp_t *rsp = irb->ptr_buf;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params->role = rsp->new_role;
        }

        switch (gevt->status) {
        case GAP_ERR_NO_ERROR:
                rsp->status = BLE_STATUS_OK;
                break;
        case GAP_ERR_INVALID_PARAM:
                rsp->status = BLE_ERROR_INVALID_PARAM;
                break;
        case GAP_ERR_NOT_SUPPORTED:
                rsp->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case GAP_ERR_COMMAND_DISALLOWED:
                rsp->status = BLE_ERROR_NOT_ALLOWED;
                break;
        default:
                rsp->status = gevt->status;
                break;
        }

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_role_set_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_role_set_cmd_t *cmd = (irb_ble_gap_role_set_cmd_t *) irb->ptr_buf;
        irb_ble_gap_role_set_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        // setup GTL message
        gmsg = ble_gap_dev_params_to_gtl(ble_dev_params);
        gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;

        /* translate Dialog API roles to stack API roles */
        if (cmd->role & GAP_CENTRAL_ROLE) {
                gcmd->role |= GAP_ROLE_CENTRAL;
        }
        if (cmd->role & GAP_PERIPHERAL_ROLE) {
                gcmd->role |= GAP_ROLE_PERIPHERAL;
        }
        if (cmd->role & GAP_BROADCASTER_ROLE) {
                gcmd->role |= GAP_ROLE_BROADCASTER;
        }
        if (cmd->role & GAP_OBSERVER_ROLE) {
                gcmd->role |= GAP_ROLE_OBSERVER;
        }

        // replace command IRB message with response
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_ROLE_SET_CMD, sizeof(*rsp));

        rsp->previous_role = ble_dev_params->role;
        rsp->new_role      = gcmd->role;

        ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_DEV_CONFIG, gapm_set_role_rsp, irb);
        ble_gtl_send(gmsg);

        ble_mgr_dev_params_release();
}

static void gapm_set_mtu_size_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_mtu_size_set_rsp_t *rsp = irb->ptr_buf;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params->mtu_size = rsp->new_mtu_size;
        }

        switch (gevt->status) {
        case GAP_ERR_NO_ERROR:
                rsp->status = BLE_STATUS_OK;
                break;
        case GAP_ERR_INVALID_PARAM:
                rsp->status = BLE_ERROR_INVALID_PARAM;
                break;
        case GAP_ERR_NOT_SUPPORTED:
                rsp->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case GAP_ERR_COMMAND_DISALLOWED:
                rsp->status = BLE_ERROR_NOT_ALLOWED;
                break;
        default:
                rsp->status = gevt->status;
                break;
        }

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_mtu_size_set_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_mtu_size_set_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_mtu_size_set_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_dev_config_cmd *gcmd;
        // for BLE device parameters
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        // setup GTL message
        gmsg = ble_gap_dev_params_to_gtl(ble_dev_params);
        gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;
        gcmd->max_mtu = cmd->mtu_size;

        // replace command IRB message with response
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_MTU_SIZE_SET_CMD, sizeof(*rsp));

        rsp->previous_mtu_size = ble_dev_params->mtu_size;
        rsp->new_mtu_size      = gcmd->max_mtu;

        ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_DEV_CONFIG, gapm_set_mtu_size_rsp, irb);
        ble_gtl_send(gmsg);

        ble_mgr_dev_params_release();
}

static void gapm_set_channel_map_rsp(ble_gtl_msg_t *gtl, void *param)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB *irb = param;
        irb_ble_gap_channel_map_set_cmd_t *cmd = irb->ptr_buf;
        irb_ble_gap_channel_map_set_rsp_t *rsp;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        if (gevt->status == GAP_ERR_NO_ERROR) {
                ble_dev_params->channel_map.map[0] = (*cmd->chnl_map);
                ble_dev_params->channel_map.map[1] = (*cmd->chnl_map) >> 8;
                ble_dev_params->channel_map.map[2] = (*cmd->chnl_map) >> 16;
                ble_dev_params->channel_map.map[3] = (*cmd->chnl_map) >> 24;
                ble_dev_params->channel_map.map[4] = (*cmd->chnl_map) >> 32;
        }

        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CHANNEL_MAP_SET_CMD, sizeof(*rsp));

        switch (gevt->status) {
        case GAP_ERR_NO_ERROR:
                rsp->status = BLE_STATUS_OK;
                break;
        case GAP_ERR_INVALID_PARAM:
                rsp->status = BLE_ERROR_INVALID_PARAM;
                break;
        case GAP_ERR_NOT_SUPPORTED:
                rsp->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case GAP_ERR_COMMAND_DISALLOWED:
                rsp->status = BLE_ERROR_NOT_ALLOWED;
                break;
        default:
                rsp->status = gevt->status;
                break;
        }

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_channel_map_set_cmd(OS_IRB *irb)
{
        const irb_ble_gap_channel_map_set_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_channel_map_set_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        struct gapm_set_channel_map_cmd *gcmd;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();
        ble_error_t ret = BLE_ERROR_FAILED;

        // Check if device is central
        if (!(ble_dev_params->role & GAP_CENTRAL_ROLE)) {
                goto done;
        }

        // setup GTL message
        gmsg = ble_gtl_alloc(GAPM_SET_CHANNEL_MAP_CMD, TASK_ID_GAPM, sizeof(*gcmd));
        gcmd = (struct gapm_set_channel_map_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPM_SET_CHANNEL_MAP;
        gcmd->chmap.map[0] = (*cmd->chnl_map);
        gcmd->chmap.map[1] = (*cmd->chnl_map) >> 8;
        gcmd->chmap.map[2] = (*cmd->chnl_map) >> 16;
        gcmd->chmap.map[3] = (*cmd->chnl_map) >> 24;
        gcmd->chmap.map[4] = (*cmd->chnl_map) >> 32;

        // keep cmd buffer, we'll need it when creating response
        irb_ble_mark_completed(irb, false);
        // response message will be allocated in gap_set_channel_map_rsp

        ble_gtl_waitqueue_add(0, GAPM_CMP_EVT, GAPM_SET_CHANNEL_MAP, gapm_set_channel_map_rsp, irb);
        ble_gtl_send(gmsg);

        ble_mgr_dev_params_release();
        return;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CHANNEL_MAP_SET_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_conn_param_update_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_conn_param_update_cmd_t *cmd =
                (irb_ble_gap_conn_param_update_cmd_t *) irb->ptr_buf;
        irb_ble_gap_conn_param_update_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_param_update_cmd *gcmd;
        // local
        ble_error_t ret = BLE_ERROR_FAILED;
        device_t *dev;
        ble_dev_params_t *params = ble_mgr_dev_params_acquire();

        if (params->updating) {
                ret = BLE_ERROR_IN_PROGRESS;
                goto done;
        }
        storage_acquire();

        dev = find_device_by_conn_idx(cmd->conn_idx);
        if (!dev) {
                /* no active connection corresponds to provided index */
                ret = BLE_ERROR_NOT_CONNECTED;
                storage_release();
                goto done;
        }

        storage_release();

        // setup GTL message
        gmsg = ble_gtl_alloc_with_conn(GAPC_PARAM_UPDATE_CMD, TASK_ID_GAPC, cmd->conn_idx,
                                                                                     sizeof(*gcmd));
        gcmd = (struct gapc_param_update_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_UPDATE_PARAMS;
        gcmd->intv_min = cmd->conn_params->interval_min;
        gcmd->intv_max = cmd->conn_params->interval_max;
        gcmd->latency = cmd->conn_params->slave_latency;
        gcmd->time_out = cmd->conn_params->sup_timeout;
        gcmd->ce_len_min = cmd->conn_params->interval_min;
        gcmd->ce_len_max = cmd->conn_params->interval_max;

        /* Set updating flag to prevent starting another update when one is ongoing */
        params->updating = true;

        ble_gtl_send(gmsg);

        ret = BLE_STATUS_OK;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CONN_PARAM_UPDATE_CMD, sizeof(*rsp));
        rsp->status = ret;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
        ble_mgr_dev_params_release();
}

void irb_ble_handler_gap_conn_param_update_reply_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_conn_param_update_reply_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_conn_param_update_reply_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_param_update_cfm *gcmd;
        // local
        ble_error_t ret = BLE_ERROR_FAILED;
        device_t *dev;
        ble_dev_params_t *ble_dev_params = ble_mgr_dev_params_acquire();

        storage_acquire();

        dev = find_device_by_conn_idx(cmd->conn_idx);
        if (!dev) {
                /* no active connection corresponds to provided index */
                ret = BLE_ERROR_NOT_CONNECTED;
                storage_release();
                goto done;
        }

        storage_release();

        gmsg = ble_gtl_alloc_with_conn(GAPC_PARAM_UPDATE_CFM, TASK_ID_GAPC, cmd->conn_idx,
                                                                                     sizeof(*gcmd));
        gcmd = (struct gapc_param_update_cfm *) gmsg->msg.gtl.param;

        gcmd->accept = cmd->accept;

        ble_gtl_send(gmsg);

        ble_dev_params->updating = false;

        ret = BLE_STATUS_OK;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_CONN_PARAM_UPDATE_REPLY_CMD, sizeof(*rsp));
        rsp->status = ret;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
        ble_mgr_dev_params_release();
}

static uint8_t translate_io_cap(gap_io_cap_t io_cap)
{
        switch (io_cap) {
        case GAP_IO_CAP_DISP_ONLY:
                return GAP_IO_CAP_DISPLAY_ONLY;
        case GAP_IO_CAP_DISP_YES_NO:
                return GAP_IO_CAP_DISPLAY_YES_NO;
        case GAP_IO_CAP_KEYBOARD_ONLY:
                return GAP_IO_CAP_KB_ONLY;
        case GAP_IO_CAP_NO_INPUT_OUTPUT:
                return GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
        case GAP_IO_CAP_KEYBOARD_DISP:
                return GAP_IO_CAP_KB_DISPLAY;
        default:
                return GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
        }
}

static void send_bond_cmd(uint16_t conn_idx, gap_io_cap_t io_cap, bool bond, bool mitm)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_bond_cmd *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_BOND_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_bond_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_BOND;
        gcmd->pairing.iocap = translate_io_cap(io_cap);
        gcmd->pairing.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;
        gcmd->pairing.auth = bond ? GAP_AUTH_BOND : 0;
        gcmd->pairing.auth |= mitm ? GAP_AUTH_MITM : 0;
        gcmd->pairing.key_size = 16;
        gcmd->pairing.ikey_dist = GAP_KDIST_ENCKEY;
        gcmd->pairing.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY;
        gcmd->pairing.sec_req = GAP_NO_SEC;

        ble_gtl_send(gmsg);
}

static void send_security_req(uint16_t conn_idx, bool bond, bool mitm)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_security_cmd *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_SECURITY_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_security_cmd *) gmsg->msg.gtl.param;
        gcmd->operation = GAPC_SECURITY_REQ;

        gcmd->auth = bond ? GAP_AUTH_BOND : 0;
        gcmd->auth |= mitm ? GAP_AUTH_MITM : 0;

        ble_gtl_send(gmsg);
}

static gap_io_cap_t get_local_io_cap(void)
{
        ble_dev_params_t *ble_dev_params;
        gap_io_cap_t io_cap;

        ble_dev_params = ble_mgr_dev_params_acquire();
        io_cap = ble_dev_params->io_capabilities;
        ble_mgr_dev_params_release();

        return io_cap;
}

static void count_bonded_cb(const device_t *dev, void *ud)
{
        int *bonded_count = ud;

        if (dev->bonded) {
                (*bonded_count)++;
        }
}

static int count_bonded(void)
{
        int bonded_count = 0;

        device_foreach(count_bonded_cb, &bonded_count);

        return bonded_count;
}

void irb_ble_handler_gap_pair_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_pair_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_pair_rsp_t *rsp;
        ble_error_t status = BLE_ERROR_FAILED;
        gap_io_cap_t io_cap;
        bool master, bonded, paired;
        bool bond = cmd->bond;
        uint16_t conn_idx = cmd->conn_idx;
        device_t *dev;

        // create response
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_PAIR_CMD, sizeof(*rsp));

        storage_acquire();
        dev = find_device_by_conn_idx(conn_idx);
        if (!dev) {
                storage_release();
                goto done;
        }
        master = dev->master;
        bonded = dev->bonded;
        paired = dev->paired;
        storage_release();

        /* Get local IO capabilities */
        io_cap = get_local_io_cap();

        /* We allow to overwrite old keys with new bonding */
        if ((!bond && (paired || bonded))) {
                status = BLE_ERROR_ALREADY_DONE;
                goto done;
        }

        /* don't exceed MAX BONDED DEVICES count */
        if (bond && !bonded && (count_bonded() >= BLE_GAP_MAX_BONDED)) {
                status = BLE_ERROR_INS_RESOURCES;
                goto done;
        }

        if (master) {
                send_bond_cmd(conn_idx, io_cap, bond,
                                        io_cap == GAP_IO_CAP_NO_INPUT_OUTPUT ? false : true);
        } else {

                send_security_req(conn_idx, bond,
                                        io_cap == GAP_IO_CAP_NO_INPUT_OUTPUT ? false : true);
        }

        status = BLE_STATUS_OK;

done:
        rsp->status = status;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_bond_req_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_bond_req_ind *ind = (void *) &gtl->param;
        device_t *dev;

        /* will be freed once completed */
        switch (ind->request) {
        case GAPC_PAIRING_REQ:
        {
                ble_evt_gap_pair_req_t *evt;
                OS_IRB irb;

                evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_PAIR_REQ, sizeof(*evt));
                evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);
                evt->bond = (ind->data.auth_req & GAP_AUTH_BOND);

                /* send to event queue */
                ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

                break;
        }
        case GAPC_LTK_EXCH:
        {
                static irb_ble_stack_msg_t *gmsg;
                struct gapc_bond_cfm *gcmd;
                int i;

                gmsg = ble_gtl_alloc(GAPC_BOND_CFM, gtl->src_id, sizeof(*gcmd));
                gcmd = (struct gapc_bond_cfm *) gmsg->msg.gtl.param;

                gcmd->accept = 0x01;
                gcmd->request = GAPC_LTK_EXCH;

                gcmd->data.ltk.ediv = rand();
                gcmd->data.ltk.key_size = ind->data.key_size;

                for (i = 0; i < RAND_NB_LEN; i++) {
                    gcmd->data.ltk.randnb.nb[i] = rand();
                }

                for (i = 0; i < ind->data.key_size; i++) {
                    gcmd->data.ltk.ltk.key[i] = rand();
                }

                storage_acquire();
                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
                if (dev) {
                        struct gapc_ltk *gltk = &gcmd->data.ltk;
                        key_ltk_t *ltk = dev->ltk;

                        if (!ltk) {
                                ltk = OS_MALLOC(sizeof(*ltk));
                                dev->ltk = ltk;
                        }

                        ltk->key_size = gltk->key_size;
                        memcpy(&ltk->rand, gltk->randnb.nb, sizeof(ltk->rand));
                        ltk->ediv = gltk->ediv;
                        memcpy(ltk->key, gltk->ltk.key, sizeof(ltk->key));


                        /* Storage will be written back to flash on pairing complete */
                        storage_mark_dirty(false);
                }
                storage_release();

                ble_gtl_send(gmsg);

                break;
        }
        case GAPC_CSRK_EXCH:
        {
                static irb_ble_stack_msg_t *gmsg;
                struct gapc_bond_cfm *gcmd;
                int i;

                gmsg = ble_gtl_alloc(GAPC_BOND_CFM, gtl->src_id, sizeof(*gcmd));
                gcmd = (struct gapc_bond_cfm *) gmsg->msg.gtl.param;

                gcmd->accept = 0x01;
                gcmd->request = GAPC_CSRK_EXCH;

                for (i = 0; i < KEY_LEN; i++) {
                        gcmd->data.csrk.key[i] = rand();
                }

                storage_acquire();
                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
                if (dev) {
                        key_csrk_t *csrk = dev->csrk;

                        if (!csrk) {
                                csrk = OS_MALLOC(sizeof(*csrk));
                                dev->csrk = csrk;
                        }

                        memcpy(csrk->key, gcmd->data.csrk.key, sizeof(csrk->key));
                        csrk->sign_cnt = 0;


                        /* Storage will be written back to flash on pairing complete */
                        storage_mark_dirty(false);
                }
                storage_release();

                ble_gtl_send(gmsg);

                break;
        }
        case GAPC_TK_EXCH:
        {
                if (ind->data.tk_type == GAP_TK_DISPLAY) {
                        static irb_ble_stack_msg_t *gmsg;
                        struct gapc_bond_cfm *gcmd;
                        ble_evt_gap_passkey_notify_t *evt;
                        uint32_t passkey;
                        OS_IRB irb;

                        gmsg = ble_gtl_alloc(GAPC_BOND_CFM, gtl->src_id, sizeof(*gcmd));
                        gcmd = (struct gapc_bond_cfm *) gmsg->msg.gtl.param;

                        gcmd->accept = 0x01;
                        gcmd->request = GAPC_TK_EXCH;

                        /* Generate passkey */
                        passkey = rand() % 1000000;

                        gcmd->data.tk.key[0] = passkey;
                        gcmd->data.tk.key[1] = passkey >> 8;
                        gcmd->data.tk.key[2] = passkey >> 16;
                        gcmd->data.tk.key[3] = passkey >> 24;

                        /* Send confirmation to stack */
                        ble_gtl_send(gmsg);

                        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_PASSKEY_NOTIFY, sizeof(*evt));
                        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);
                        evt->passkey = passkey;

                        /* Send notification to app */
                        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
                } else if (ind->data.tk_type == GAP_TK_KEY_ENTRY) {
                        ble_evt_gap_passkey_request_t *evt;
                        OS_IRB irb;

                        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_PASSKEY_REQUEST, sizeof(*evt));
                        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);

                        /* Send request to app */
                        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
                }

                break;
        }
        default:
                return;
        }
}

void irb_ble_handler_gap_pair_reply_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_pair_reply_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_pair_reply_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_bond_cfm *gcmd;
        gap_io_cap_t io_cap;
        device_t *dev;
        bool bonded;
        ble_error_t status = BLE_STATUS_OK;

        storage_acquire();
        dev = find_device_by_conn_idx(cmd->conn_idx);
        if (!dev) {
                storage_release();
                status = BLE_ERROR_NOT_CONNECTED;
                goto reply;
        }

        bonded = dev->bonded;
        storage_release();

        /* don't exceed MAX BONDED DEVICES count */
        if (cmd->bond && cmd->accept && !bonded && (count_bonded() >= BLE_GAP_MAX_BONDED)) {
                status = BLE_ERROR_INS_RESOURCES;
                goto reply;
        }

        /* Get local IO capabilities */
        io_cap = get_local_io_cap();

        gmsg = ble_gtl_alloc_with_conn(GAPC_BOND_CFM, TASK_ID_GAPC, cmd->conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_bond_cfm *) gmsg->msg.gtl.param;

        gcmd->request = GAPC_PAIRING_RSP;
        gcmd->accept = cmd->accept;

        if (!cmd->accept) {
                goto done;
        }

        gcmd->data.pairing_feat.auth = cmd->bond ? GAP_AUTH_BOND : 0;
        gcmd->data.pairing_feat.auth |= (io_cap != GAP_IO_CAP_NO_INPUT_OUTPUT) ? GAP_AUTH_MITM : 0;

        gcmd->data.pairing_feat.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;
        gcmd->data.pairing_feat.key_size = KEY_LEN;
        gcmd->data.pairing_feat.iocap = translate_io_cap(io_cap);
        gcmd->data.pairing_feat.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_SIGNKEY | GAP_KDIST_IDKEY;
        gcmd->data.pairing_feat.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_SIGNKEY;
        gcmd->data.pairing_feat.sec_req = GAP_NO_SEC;

done:
        ble_gtl_send(gmsg);

reply:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_PAIR_REPLY_CMD, sizeof(*rsp));
        rsp->status = status;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_passkey_reply_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_passkey_reply_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_passkey_reply_rsp_t *rsp;
        // for GTL
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_bond_cfm *gcmd;

        gmsg = ble_gtl_alloc_with_conn(GAPC_BOND_CFM, TASK_ID_GAPC, cmd->conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_bond_cfm *) gmsg->msg.gtl.param;

        gcmd->request = GAPC_TK_EXCH;
        gcmd->accept = cmd->accept;

        if (!cmd->accept) {
                goto done;
        }

        gcmd->data.tk.key[0] = cmd->passkey;
        gcmd->data.tk.key[1] = cmd->passkey >> 8;
        gcmd->data.tk.key[2] = cmd->passkey >> 16;
        gcmd->data.tk.key[3] = cmd->passkey >> 24;

done:
        ble_gtl_send(gmsg);

        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_PASSKEY_REPLY_CMD, sizeof(*rsp));
        rsp->status = BLE_STATUS_OK;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

static void send_sec_level_changed_evt(uint16_t conn_idx, gap_sec_level_t level)
{
        OS_IRB irb;
        ble_evt_gap_sec_level_changed_t *evt;

        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_SEC_LEVEL_CHANGED, sizeof(*evt));
        evt->conn_idx = conn_idx;
        evt->level = level;

        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_bond_ind_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_bond_ind *ind = (void *) &gtl->param;
        OS_IRB irb;
        device_t *dev;

        switch (ind->info) {
        case GAPC_PAIRING_SUCCEED:
        {
                ble_evt_gap_pair_completed_t *evt;

                evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_PAIR_COMPLETED, sizeof(*evt));
                evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);
                evt->status = 0x00;
                evt->bond = ind->data.auth & GAP_AUTH_BOND;
                evt->mitm = ind->data.auth & GAP_AUTH_MITM;

                /* Store auth req in pairing data */
                storage_acquire();
                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
                if (dev) {
                        dev->paired = true;
                        dev->bonded = evt->bond;
                        dev->encrypted = true;
                        dev->mitm = evt->mitm;
                        if (dev->bonded) {
                                /* Move device to the front of the connected devices list */
                                device_move_front(dev);
                        }

                }

                /* Write storage back to flash immediately */
                storage_mark_dirty(true);

                storage_release();

                send_sec_level_changed_evt(evt->conn_idx, evt->mitm ?
                                                                GAP_SEC_LEVEL_3 : GAP_SEC_LEVEL_2);

                /* send to event queue */
                ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

                break;
        }
        case GAPC_PAIRING_FAILED:
        {
                ble_evt_gap_pair_completed_t *evt;


                /* Write storage back to flash immediately */
                storage_mark_dirty(true);

                evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_PAIR_COMPLETED, sizeof(*evt));
                evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);
                evt->status = ind->data.reason;
                evt->bond = false;
                evt->mitm = false;

                /* send to event queue */
                ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

                break;
        }
        case GAPC_LTK_EXCH:
                storage_acquire();
                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
                if (dev) {
                        struct gapc_ltk *gltk = &ind->data.ltk;
                        key_ltk_t *ltk = dev->remote_ltk;

                        if (!ltk) {
                                ltk = OS_MALLOC(sizeof(*ltk));
                                dev->remote_ltk = ltk;
                        }

                        ltk->key_size = gltk->key_size;
                        memcpy(&ltk->rand, gltk->randnb.nb, sizeof(ltk->rand));
                        ltk->ediv = gltk->ediv;
                        memcpy(ltk->key, gltk->ltk.key, sizeof(ltk->key));


                        /* Storage will be written back to flash on pairing complete */
                        storage_mark_dirty(false);
                }
                storage_release();
                break;
        case GAPC_CSRK_EXCH:
                storage_acquire();
                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
                if (dev) {
                        key_csrk_t *csrk = dev->remote_csrk;

                        if (!csrk) {
                                csrk = OS_MALLOC(sizeof(*csrk));
                                dev->remote_csrk = csrk;
                        }

                        memcpy(csrk->key, ind->data.csrk.key, sizeof(csrk->key));
                        csrk->sign_cnt = 0;


                        /* Storage will be written back to flash on pairing complete */
                        storage_mark_dirty(false);
                }
                storage_release();
                break;
        case GAPC_IRK_EXCH:
        {
                ble_evt_gap_address_resolved_t *evt;

                storage_acquire();
                dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
                if (dev) {
                        key_irk_t *irk = dev->irk;

                        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_ADDRESS_RESOLVED, sizeof(*evt));

                        if (!irk) {
                                irk = OS_MALLOC(sizeof(*irk));
                                dev->irk = irk;
                        }

                        memcpy(irk->key, ind->data.irk.irk.key, sizeof(irk->key));

                        memcpy(&evt->address, &dev->addr, sizeof(evt->address));
                        dev->addr.addr_type = ind->data.irk.addr.addr_type;
                        memcpy(dev->addr.addr, ind->data.irk.addr.addr.addr,
                                                                        sizeof(dev->addr.addr));
                        memcpy(&evt->resolved_address, &dev->addr, sizeof(evt->resolved_address));
                        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);

                        /* send to event queue */
                        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

                        /* Storage will be written back to flash on pairing complete */
                        storage_mark_dirty(false);
                }
                storage_release();
                break;
        }
        default:
                break;
        }
}

static bool encrypt_conn_using_ltk(uint16_t conn_idx, uint8_t auth)
{
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_encrypt_cmd *gcmd;
        device_t *dev;

        storage_acquire();
        dev = find_device_by_conn_idx(conn_idx);
        if (!dev || !dev->remote_ltk) {
                storage_release();
                return false;
        }

        if ((auth & GAP_AUTH_MITM) && !dev->mitm) {
                storage_release();
                return false;
        }

        gmsg = ble_gtl_alloc_with_conn(GAPC_ENCRYPT_CMD, TASK_ID_GAPC, conn_idx, sizeof(*gcmd));
        gcmd = (struct gapc_encrypt_cmd *) gmsg->msg.gtl.param;

        gcmd->operation = GAPC_ENCRYPT;
        gcmd->ltk.ediv = dev->remote_ltk->ediv;
        gcmd->ltk.key_size = dev->remote_ltk->key_size;
        memcpy(&gcmd->ltk.ltk.key, dev->remote_ltk->key, sizeof(gcmd->ltk.ltk.key));
        memcpy(&gcmd->ltk.randnb.nb, &dev->remote_ltk->rand, sizeof(gcmd->ltk.randnb.nb));

        storage_release();
        ble_gtl_send(gmsg);

        return true;
}

void irb_ble_handler_gap_security_ind_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_security_ind *ind = (void *) &gtl->param;
        uint16_t conn_idx = TASK_2_CONNIDX(gtl->src_id);
        ble_evt_gap_security_request_t *evt;
        OS_IRB irb;

        if (encrypt_conn_using_ltk(conn_idx, ind->auth)) {
                return;
        }

        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_SECURITY_REQUEST, sizeof(*evt));
        evt->conn_idx = conn_idx;
        evt->bond = ind->auth & GAP_AUTH_BOND;
        evt->mitm = ind->auth & GAP_AUTH_MITM;

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_sign_counter_ind_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_sign_counter_ind *ind = (void *) &gtl->param;
        device_t *dev;

        storage_acquire();
        dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
        if (dev) {
                /* Local and remote CSRKs should exist for this device. */
                OS_ASSERT(dev->csrk);
                OS_ASSERT(dev->remote_csrk);

                dev->csrk->sign_cnt = ind->local_sign_counter;
                dev->remote_csrk->sign_cnt = ind->peer_sign_counter;
        }
        storage_release();
}

void irb_ble_handler_gap_encrypt_req_ind_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_encrypt_req_ind *ind = (void *) &gtl->param;
        static irb_ble_stack_msg_t *gmsg;
        struct gapc_encrypt_cfm *gcmd;
        device_t *dev;

        gmsg = ble_gtl_alloc(GAPC_ENCRYPT_CFM, gtl->src_id, sizeof(*gcmd));
        gcmd = (struct gapc_encrypt_cfm *) gmsg->msg.gtl.param;
        gcmd->found = 0x00;

        storage_acquire();

        dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));

        if (!dev || !dev->ltk) {
                goto done;
        }

        if (dev->ltk->ediv != ind->ediv) {
                goto done;
        }

        // our Rand is stored in the same endianess as RW
        if (memcmp(&dev->ltk->rand, &ind->rand_nb.nb, sizeof(dev->ltk->rand))) {
                goto done;
        }

        gcmd->found = 0x01;
        gcmd->key_size = dev->ltk->key_size;
        memcpy(&gcmd->ltk.key, dev->ltk->key, sizeof(gcmd->ltk.key));

done:
        storage_release();
        ble_gtl_send(gmsg);
}

void irb_ble_handler_gap_encrypt_ind_evt(ble_gtl_msg_t *gtl)
{
        device_t *dev;

        storage_acquire();

        dev = find_device_by_conn_idx(TASK_2_CONNIDX(gtl->src_id));
        if (dev) {
                dev->encrypted = true;
                send_sec_level_changed_evt(TASK_2_CONNIDX(gtl->src_id), dev->mitm ?
                                                GAP_SEC_LEVEL_3 : GAP_SEC_LEVEL_2);
        }

        storage_release();
}

void irb_ble_handler_gapc_cmp__update_params_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_conn_param_update_completed_t *evt;
        ble_dev_params_t *params = ble_mgr_dev_params_acquire();

        /* Reset updating flag, update is completed */
        params->updating = false;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_CONN_PARAM_UPDATE_COMPLETED, sizeof(*evt));
        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);

        switch (gevt->status) {
        case GAP_ERR_NO_ERROR:
                evt->status = BLE_STATUS_OK;
                break;
        case GAP_ERR_INVALID_PARAM:
        case LL_ERR_INVALID_HCI_PARAM:
                evt->status = BLE_ERROR_INVALID_PARAM;
                break;
        case GAP_ERR_TIMEOUT:
                evt->status = BLE_ERROR_TIMEOUT;
                break;
        case GAP_ERR_REJECTED:
                evt->status = BLE_ERROR_NOT_ACCEPTED;
                break;
        case LL_ERR_COMMAND_DISALLOWED:
                evt->status = BLE_ERROR_NOT_ALLOWED;
                break;
        case LL_ERR_UNKNOWN_HCI_COMMAND:
        case LL_ERR_UNSUPPORTED:
        case LL_ERR_UNKNOWN_LMP_PDU:
        case LL_ERR_UNSUPPORTED_LMP_PARAM_VALUE:
                evt->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case LL_ERR_UNSUPPORTED_REMOTE_FEATURE:
                evt->status = BLE_ERROR_NOT_SUPPORTED_BY_PEER;
                break;
        default:
                evt->status = BLE_ERROR_FAILED;
                break;
        }

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */

        ble_mgr_dev_params_release();
}

void irb_ble_handler_gapc_cmp__bond_evt(ble_gtl_msg_t *gtl)
{
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_pair_completed_t *evt;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                // nothing to do, we replied in GAPC_BOND_IND
                return;
        }

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_PAIR_COMPLETED, sizeof(*evt));
        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);
        evt->status = gevt->status;
        evt->bond = false;
        evt->mitm = false;

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */
}

void irb_ble_handler_gapc_cmp__encrypt_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_set_sec_level_failed_t *evt;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                /* nothing to do, event should be sent only when error occurred */
                return;
        }

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_SET_SEC_LEVEL_FAILED, sizeof(*evt));
        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);

        /* translate error code */
        if (gevt->status == SMP_ERROR_REM_ENC_KEY_MISSING) {
                evt->status = BLE_ERROR_ENC_KEY_MISSING;
        } else {
                evt->status = BLE_ERROR_FAILED;
        }

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);

        /* will be freed once completed */
}

void irb_ble_handler_gap_unpair_cmd(OS_IRB *irb)
{
        // for IRB
        const irb_ble_gap_unpair_cmd_t *cmd = (void *) irb->ptr_buf;
        uint8_t status = BLE_ERROR_FAILED;
        irb_ble_gap_unpair_rsp_t *rsp;
        device_t *dev;

        /* Remove pairing info if we are not bonded */
        storage_acquire();

        dev = find_device_by_addr(&cmd->addr, false);
        if (!dev) {
                goto done;
        }

        device_remove_pairing(dev);
        status = BLE_STATUS_OK;

        if (!dev->connected) {
                device_remove(dev);
                goto done;
        }

        send_gapc_disconnect_cmd(dev->conn_idx, BLE_HCI_ERROR_REMOTE_USER_TERM_CON);

done:
        storage_release();
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_UNPAIR_CMD, sizeof(*rsp));
        rsp->status = status;

        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_set_sec_level_cmd(OS_IRB *irb)
{
        const irb_ble_gap_set_sec_level_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_set_sec_level_rsp_t *rsp;
        gap_sec_level_t level = cmd->level;
        bool mitm = level > GAP_SEC_LEVEL_2 ? true : false;
        uint16_t conn_idx = cmd->conn_idx;
        ble_dev_params_t *ble_dev_params;
        gap_io_cap_t io_cap;
        ble_error_t status;
        device_t *dev;
        bool bonded;

        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_SET_SEC_LEVEL_CMD, sizeof(*rsp));

        if (level == GAP_SEC_LEVEL_4) {
                status = BLE_ERROR_NOT_SUPPORTED;
                goto done;
        }

        ble_dev_params = ble_mgr_dev_params_acquire();
        io_cap = ble_dev_params->io_capabilities;
        ble_mgr_dev_params_release();

        storage_acquire();
        dev = find_device_by_conn_idx(conn_idx);
        if (!dev) {
                storage_release();
                status = BLE_ERROR_NOT_CONNECTED;
                goto done;
        }

        bonded = dev->bonded;
        storage_release();

        if (!dev->master) {
                send_security_req(conn_idx, bonded, mitm);
                status = BLE_STATUS_OK;
                goto done;
        }

        if (encrypt_conn_using_ltk(conn_idx, mitm ? GAP_AUTH_MITM : 0)) {
                status = BLE_STATUS_OK;
                goto done;
        }

        send_bond_cmd(conn_idx, io_cap, bonded, mitm);
        status = BLE_STATUS_OK;

done:
        rsp->status = status;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

#if (RWBLE_SW_VERSION_MAJOR >= 8)
#if (dg_configBLE_SKIP_LATENCY_API == 1)
void irb_ble_handler_gap_skip_latency_cmd(OS_IRB *irb)
{
        const irb_ble_gap_skip_latency_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_skip_latency_rsp_t *rsp;
        uint16_t conn_idx = cmd->conn_idx;
        bool enable = cmd->enable;
        ble_error_t status;
        device_t *dev;

        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_SKIP_LATENCY_CMD, sizeof(*rsp));

        storage_acquire();
        dev = find_device_by_conn_idx(conn_idx);
        if (!dev) {
                status = BLE_ERROR_NOT_CONNECTED;
        } else if (dev->master) {
                status = BLE_ERROR_NOT_ALLOWED;
        } else {
                ble_mgr_skip_latency_set(conn_idx, enable);
                status = BLE_STATUS_OK;
        }
        storage_release();

        rsp->status = status;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}
#endif /* (dg_configBLE_SKIP_LATENCY_API == 1) */

void irb_ble_handler_gap_le_pkt_size_ind_evt(ble_gtl_msg_t *gtl)
{
        struct gapc_le_pkt_size_ind *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_data_length_changed_t *evt;

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_DATA_LENGTH_CHANGED, sizeof(*evt));
        evt->conn_idx      = TASK_2_CONNIDX(gtl->src_id);
        evt->max_rx_length = gevt->max_rx_octets;
        evt->max_rx_time   = gevt->max_rx_time;
        evt->max_tx_length = gevt->max_tx_octets;
        evt->max_tx_time   = gevt->max_tx_time;

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_data_length_set_cmd(OS_IRB *irb)
{
        const irb_ble_gap_data_length_set_cmd_t *cmd = (void *) irb->ptr_buf;
        irb_ble_gap_data_length_set_rsp_t *rsp;
        static irb_ble_stack_msg_t *gmsg;
        ble_error_t status = BLE_ERROR_FAILED;
        device_t *dev;

        /* Check if data length and time values provided are within the supported range. */
        if ((cmd->tx_length < GAPM_LE_LENGTH_EXT_OCTETS_MIN) ||
            (cmd->tx_length > GAPM_LE_LENGTH_EXT_OCTETS_MAX) ||
            (cmd->tx_time && cmd->tx_time != BLE_DATA_LENGTH_TO_TIME(cmd->tx_length))) {
                status = BLE_ERROR_INVALID_PARAM;
                goto done;
        }

        if (cmd->conn_idx == BLE_CONN_IDX_INVALID) {
                /* Set preferred data length for future connections. */
                struct gapm_set_dev_config_cmd *gcmd;

                gmsg = ble_gtl_alloc(GAPM_SET_DEV_CONFIG_CMD,TASK_ID_GAPM,
                                                            sizeof(struct gapm_set_dev_config_cmd));
                gcmd = (struct gapm_set_dev_config_cmd *) gmsg->msg.gtl.param;

                gcmd->operation = GAPM_SET_SUGGESTED_DFLT_LE_DATA_LEN;
                gcmd->max_txoctets = cmd->tx_length;
                gcmd->max_txtime = cmd->tx_time;


                ble_gtl_send(gmsg);
        } else {
                /* Set data length for specified connection. */
                storage_acquire();

                dev = find_device_by_conn_idx(cmd->conn_idx);
                if (!dev) {
                        storage_release();
                        status = BLE_ERROR_NOT_CONNECTED;
                        goto done;
                }

                storage_release();

                change_conn_data_length(cmd->conn_idx, cmd->tx_length, cmd->tx_time);
        }

        status = BLE_STATUS_OK;

done:
        irb_ble_mark_completed(irb, true);
        rsp = irb_ble_replace_msg(irb, IRB_BLE_GAP_DATA_LENGTH_SET_CMD, sizeof(*rsp));
        rsp->status = status;
        ble_mgr_response_queue_send(irb, OS_QUEUE_FOREVER);
}

void irb_ble_handler_gap_cmp__data_length_set_evt(ble_gtl_msg_t *gtl)
{
        /*
         * This handler will handle both gapm_cmp_evt and gapc_cmp_evt events. These event
         * structures are identical so there is no need to differentiate.
         */
        struct gapm_cmp_evt *gevt = (void *) gtl->param;
        OS_IRB irb;
        ble_evt_gap_data_length_set_failed_t *evt;

        if (gevt->status == GAP_ERR_NO_ERROR) {
                /* nothing to do, event should be sent only when error occurred */
                return;
        }

        /* setup IRB with new event and fill it */
        evt = irb_ble_init_evt(&irb, BLE_EVT_GAP_DATA_LENGTH_SET_FAILED, sizeof(*evt));
        evt->conn_idx = TASK_2_CONNIDX(gtl->src_id);

        /* translate error code */
        switch (gevt->status) {
        case CO_ERROR_COMMAND_DISALLOWED:
                evt->status = BLE_ERROR_NOT_ALLOWED;
                break;
        case CO_ERROR_UNSUPPORTED_REMOTE_FEATURE:
                evt->status = BLE_ERROR_NOT_SUPPORTED;
                break;
        case CO_ERROR_INVALID_HCI_PARAM:
                evt->status = BLE_ERROR_INVALID_PARAM;
                break;
        default:
                evt->status = BLE_ERROR_FAILED;
                break;
        }

        /* send to event queue */
        ble_mgr_event_queue_send(&irb, OS_QUEUE_FOREVER);
}
#endif /* (RWBLE_SW_VERSION_MAJOR >= 8) */
