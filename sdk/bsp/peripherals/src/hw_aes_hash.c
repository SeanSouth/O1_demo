/**
 * \addtogroup BSP
 * \{
 * \addtogroup DEVICES
 * \{
 * \addtogroup AES_HASH
 * \{
 */

/**
 ****************************************************************************************
 *
 * @file hw_aes_hash.c
 *
 * @brief Implementation of the AES/Hash Engine Low Level Driver.
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

#if dg_configUSE_HW_AES_HASH

#include <hw_aes_hash.h>

#if (dg_configSYSTEMVIEW)
#  include "SEGGER_SYSVIEW_FreeRTOS.h"
#else
#  define SEGGER_SYSTEMVIEW_ISR_ENTER()
#  define SEGGER_SYSTEMVIEW_ISR_EXIT()
#endif

#define MODE_IS_AES(m)  (m <= HW_AES_CTR)

static hw_aes_hash_cb callback;

static void hw_aes_hash_wait_on_inactive(void)
{
        while (!REG_GETF(AES_HASH, CRYPTO_STATUS_REG, CRYPTO_INACTIVE)) {
                ;
        };
}

static void hw_aes_hash_set_mode(const hw_aes_hash_setup* setup)
{
        uint32_t crypt0_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;

        switch (setup->mode) {
        case HW_AES_ECB:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 0);
                break;
        case HW_AES_CBC:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 3);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 0);
                AES_HASH->CRYPTO_MREG0_REG = setup->aesIvCtrblk_0_31;
                AES_HASH->CRYPTO_MREG1_REG = setup->aesIvCtrblk_32_63;
                AES_HASH->CRYPTO_MREG2_REG = setup->aesIvCtrblk_64_95;
                AES_HASH->CRYPTO_MREG3_REG = setup->aesIvCtrblk_96_127;
                break;
        case HW_AES_CTR:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 2);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 0);
                AES_HASH->CRYPTO_MREG0_REG = setup->aesIvCtrblk_0_31;
                AES_HASH->CRYPTO_MREG1_REG = setup->aesIvCtrblk_32_63;
                AES_HASH->CRYPTO_MREG2_REG = setup->aesIvCtrblk_64_95;
                AES_HASH->CRYPTO_MREG3_REG = setup->aesIvCtrblk_96_127;
                break;
        case HW_HASH_MD5:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 0);
                break;
        case HW_HASH_SHA_1:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 1);
                break;
        case HW_HASH_SHA_256_224:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 2);
                break;
        case HW_HASH_SHA_256:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 0);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 3);
                break;
        case HW_HASH_SHA_384:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 0);
                break;
        case HW_HASH_SHA_512:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 1);
                break;
        case HW_HASH_SHA_512_224:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 2);
                break;
        case HW_HASH_SHA_512_256:
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_SEL, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG_MD, crypt0_ctrl_reg, 1);
                REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ALG, crypt0_ctrl_reg, 3);
                break;
        }

        AES_HASH->CRYPTO_CTRL_REG = crypt0_ctrl_reg;
}

static void hw_aes_hash_check_data_size(const hw_aes_hash_setup* setup)
{
        switch (setup->mode) {
        case HW_AES_ECB:
                // In ECB mode the dataSize needs to be a multiple of 16.
                ASSERT_ERROR(setup->dataSize % 0x10 == 0);
                break;
        case HW_AES_CBC:
        case HW_AES_CTR:
                // If more data is to come in CBC or CTR mode the dataSize needs to be a multiple of 16.
                if (setup->moreDataToCome) {
                        ASSERT_ERROR(setup->dataSize % 0x10 == 0);
                }
                break;
        case HW_HASH_MD5:
        case HW_HASH_SHA_1:
        case HW_HASH_SHA_256_224:
        case HW_HASH_SHA_256:
        case HW_HASH_SHA_384:
        case HW_HASH_SHA_512:
        case HW_HASH_SHA_512_224:
        case HW_HASH_SHA_512_256:
                // If more data is to come in hash mode the dataSize needs to be a multiple of 8.
                if (setup->moreDataToCome) {
                        ASSERT_ERROR(setup->dataSize % 0x8 == 0);
                }
                break;
        }
}

void hw_aes_hash_enable_interrupt(hw_aes_hash_cb cb)
{
        callback = cb;
        NVIC_EnableIRQ(CRYPTO_IRQn);
        REG_SET_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_IRQ_EN);
}

void hw_aes_hash_disable_interrupt(void)
{
        REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_IRQ_EN);
        callback = NULL;
}

static inline uint32 hw_aes_hash_construct_word(const uint8 *data)
{
        if ((uint32)data & 0x3) {
                uint32 internal_buf;
                uint8 *p = (uint8 *)&internal_buf + 3;
                unsigned int i;

                for (i = 0; i < 4; i++) {
                        *(p--) = *(data++);
                }

                return internal_buf;
        }
        else {
                return SWAP32(*(uint32 *)data);
        }
}

void hw_aes_hash_store_keys(hw_aes_key_size key_size, const uint8 *aes_keys, hw_aes_hash_key_exp_t key_exp)
{
        volatile uint32 *kmem_ptr = &AES_HASH->CRYPTO_KEYS_START;
        unsigned int key_wrds;

        if (key_exp == HW_AES_DO_NOT_PERFORM_KEY_EXPANSION) {
                /* Key expansion is provided by the software */
                REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEXP);
                key_wrds = (key_size == HW_AES_256) ? 60 : (key_size == HW_AES_192) ? 52 : 44;
        }
        else {
                /* Key expansion needs to be performed by the engine */
                REG_SET_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEXP);
                key_wrds = (key_size == HW_AES_256) ? 8 : (key_size == HW_AES_192) ? 6 : 4;
        }

        do {
                *(kmem_ptr++) = hw_aes_hash_construct_word(aes_keys);
                aes_keys += 4;
                key_wrds--;
        } while (key_wrds > 0);
}

void hw_aes_hash_enable(const hw_aes_hash_setup setup)
{
        hw_aes_hash_check_data_size(&setup);

        hw_aes_hash_enable_clock();

        hw_aes_hash_set_mode(&setup);

        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_MORE_IN, crypto_ctrl_reg,
                setup.moreDataToCome);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (setup.hashOutLength - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ENCDEC, crypto_ctrl_reg,
                setup.aesDirection);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEXP, crypto_ctrl_reg,
                setup.aesKeyExpand);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEY_SZ, crypto_ctrl_reg,
                setup.aesKeySize);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_OUT_MD, crypto_ctrl_reg,
                !setup.aesWriteBackAll);
        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;

        if (MODE_IS_AES(setup.mode)) {
                hw_aes_hash_store_keys(setup.aesKeySize, (uint8 *)setup.aesKeys, !setup.aesKeyExpand);
        }

        hw_aes_hash_cfg_dma((const uint8 *)setup.sourceAddress, (uint8 *)setup.destinationAddress, 
                            (unsigned int)setup.dataSize);

        if (setup.enableInterrupt) {
                hw_aes_hash_enable_interrupt(setup.callback);
        }
        else {
                hw_aes_hash_disable_interrupt();
        }

        hw_aes_hash_start();
}

void hw_aes_hash_init(hw_aes_hash_setup *setup)
{
        hw_aes_hash_check_data_size(setup);

        hw_aes_hash_enable_clock();

        hw_aes_hash_set_mode(setup);

        uint32_t crypto_ctrl_reg = AES_HASH->CRYPTO_CTRL_REG;
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_MORE_IN, crypto_ctrl_reg,
                setup->moreDataToCome);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_HASH_OUT_LEN, crypto_ctrl_reg,
                (setup->hashOutLength - 1));
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_ENCDEC, crypto_ctrl_reg,
                setup->aesDirection);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEXP, crypto_ctrl_reg,
                setup->aesKeyExpand);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_AES_KEY_SZ, crypto_ctrl_reg,
                setup->aesKeySize);
        REG_SET_FIELD(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_OUT_MD, crypto_ctrl_reg,
                !setup->aesWriteBackAll);
        AES_HASH->CRYPTO_CTRL_REG = crypto_ctrl_reg;

        if (MODE_IS_AES(setup->mode)) {
                hw_aes_hash_store_keys(setup->aesKeySize, (uint8 *)setup->aesKeys, !setup->aesKeyExpand);
        }

        hw_aes_hash_cfg_dma((const uint8 *)setup->sourceAddress, (uint8 *)setup->destinationAddress, 
                            (unsigned int)setup->dataSize);

        if (setup->enableInterrupt) {
                hw_aes_hash_enable_interrupt(setup->callback);
        }
        else {
                hw_aes_hash_disable_interrupt();
        }
}

void hw_aes_hash_restart(const uint32 sourceAddress, const uint32 dataSize,
        const bool moreDataToCome)
{
        hw_aes_hash_cfg_dma((const uint8 *)sourceAddress, NULL, (unsigned int)dataSize);
        REG_SETF(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_MORE_IN, moreDataToCome);
        hw_aes_hash_start();
}

bool hw_aes_hash_is_active()
{
        return REG_GETF(AES_HASH, CRYPTO_STATUS_REG, CRYPTO_INACTIVE) == 0;
}

bool hw_aes_hash_wait_for_in()
{
        return REG_GETF(AES_HASH, CRYPTO_STATUS_REG, CRYPTO_WAIT_FOR_IN) == 1;
}

void hw_aes_hash_disable(const bool waitOnFinish)
{
        if (waitOnFinish)
                hw_aes_hash_wait_on_inactive();

        REG_CLR_BIT(AES_HASH, CRYPTO_CTRL_REG, CRYPTO_MORE_IN);
        GLOBAL_INT_DISABLE();
        REG_CLR_BIT(CRG_TOP, CLK_AMBA_REG, AES_CLK_ENABLE);
        GLOBAL_INT_RESTORE();
        NVIC_DisableIRQ(CRYPTO_IRQn);
}

void hw_aes_hash_cfg_dma(const uint8 *src, uint8 *dst, unsigned int len)
{
        /* Source address setting */
        AES_HASH->CRYPTO_FETCH_ADDR_REG = black_orca_phy_addr((uint32)src);

        /* Destination address setting */
        if (dst) {
                unsigned int remap_type = REG_GETF(CRG_TOP, SYS_CTRL_REG, REMAP_ADR0);

                if (IS_SYSRAM_ADDRESS(dst) ||
                    (IS_REMAPPED_ADDRESS(dst) && (remap_type == 0x3))) {
                        AES_HASH->CRYPTO_DEST_ADDR_REG = black_orca_phy_addr((uint32)dst);
                }
                else {
                        /* Destination address can only reside in RAM */
                        ASSERT_ERROR(0);
                }
        }

        /* Data length setting */
        AES_HASH->CRYPTO_LEN_REG = (uint32)len;
}

static void hw_aes_hash_store_in_mode_dependent_regs(const uint8 *buf)
{
        AES_HASH->CRYPTO_MREG0_REG = hw_aes_hash_construct_word(buf + 12);
        AES_HASH->CRYPTO_MREG1_REG = hw_aes_hash_construct_word(buf + 8);
        AES_HASH->CRYPTO_MREG2_REG = hw_aes_hash_construct_word(buf + 4);
        AES_HASH->CRYPTO_MREG3_REG = hw_aes_hash_construct_word(buf + 0);
}

void hw_aes_hash_store_iv(const uint8 *iv)
{
        hw_aes_hash_store_in_mode_dependent_regs(iv);
}

void hw_aes_hash_store_ic(const uint8 *ic)
{
        hw_aes_hash_store_in_mode_dependent_regs(ic);
}

int hw_aes_hash_check_restrictions(void)
{
        unsigned int is_hash = AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_HASH_SEL_Msk;
        unsigned int more_in = AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_MORE_IN_Msk;

        if (is_hash) {
                if (more_in && (AES_HASH->CRYPTO_LEN_REG & 0x7)) {
                        return -1;
                }
        }
        else {
                unsigned int is_ecb = (((AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk) == 0) |
                        ((AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk) == 0x0100));

                if ((is_ecb || more_in) && (AES_HASH->CRYPTO_LEN_REG & 0x15)) {
                        return -1;
                }
        }

        return 0;
}

void CRYPTO_Handler(void)
{
        SEGGER_SYSTEMVIEW_ISR_ENTER();

        AES_HASH->CRYPTO_CLRIRQ_REG = 0x1;
        NVIC_ClearPendingIRQ(CRYPTO_IRQn);
        if (callback != NULL) {
                callback();
        }

        SEGGER_SYSTEMVIEW_ISR_EXIT();
}

#endif /* dg_configUSE_HW_AES_HASH */
/**
 * \}
 * \}
 * \}
 */
