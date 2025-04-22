#include "../weebo_i.h"
#include <nfc/protocols/mf_ultralight/mf_ultralight_poller.h>

#define TAG "SceneWrite"

static uint8_t SLB[] = {0x00, 0x00, 0x0F, 0xE0};
static uint8_t CC[] = {0xf1, 0x10, 0xff, 0xee};
static uint8_t DLB[] = {0x01, 0x00, 0x0f, 0xbd};
static uint8_t CFG0[] = {0x00, 0x00, 0x00, 0x04};
static uint8_t CFG1[] = {0x5f, 0x00, 0x00, 0x00};
static uint8_t PACKRFUI[] = {0x80, 0x80, 0x00, 0x00};

enum NTAG215Pages {
    staticLockBits = 2,
    capabilityContainer = 3,
    userMemoryFirst = 4,
    userMemoryLast = 129,
    dynamicLockBits = 130,
    cfg0 = 131,
    cfg1 = 132,
    pwd = 133,
    pack = 134,
    total = 135
};

void weebo_scene_write_calculate_pwd(uint8_t* uid, uint8_t* pwd) {
    pwd[0] = uid[1] ^ uid[3] ^ 0xAA;
    pwd[1] = uid[2] ^ uid[4] ^ 0x55;
    pwd[2] = uid[3] ^ uid[5] ^ 0xAA;
    pwd[3] = uid[4] ^ uid[6] ^ 0x55;
}

NfcCommand weebo_scene_write_poller_callback(NfcGenericEvent event, void* context) {
    furi_assert(event.protocol == NfcProtocolMfUltralight);
    Weebo* weebo = context;
    NfcCommand ret = NfcCommandContinue;

    const MfUltralightPollerEvent* mf_ultralight_event = event.event_data;

    if(mf_ultralight_event->type == MfUltralightPollerEventTypeReadSuccess) {
        nfc_device_set_data(
            weebo->nfc_device, NfcProtocolMfUltralight, nfc_poller_get_data(weebo->poller));
        const MfUltralightData* data =
            nfc_device_get_data(weebo->nfc_device, NfcProtocolMfUltralight);

        if(!mf_ultralight_is_all_data_read(data)) {
            ret = NfcCommandStop;
        }
        if(data->type != MfUltralightTypeNTAG215) {
            ret = NfcCommandStop;
        }

        FURI_LOG_D(
            TAG,
            "UID: %02X%02X%02X%02X%02X%02X",
            data->iso14443_3a_data->uid[0],
            data->iso14443_3a_data->uid[1],
            data->iso14443_3a_data->uid[2],
            data->iso14443_3a_data->uid[3],
            data->iso14443_3a_data->uid[4],
            data->iso14443_3a_data->uid[5]);

        for(size_t p = 0; p < 2; p++) {
            for(size_t i = 0; i < MF_ULTRALIGHT_PAGE_SIZE; i++) {
                weebo->figure[NFC3D_UID_OFFSET + p * MF_ULTRALIGHT_PAGE_SIZE + i] =
                    data->page[p].data[i];
            }
        }

        uint8_t modified[NTAG215_SIZE];
        nfc3d_amiibo_pack(&weebo->amiiboKeys, weebo->figure, modified);

        /*
        MfUltralightAuth* mf_ul_auth;
        mf_ul_auth = mf_ultralight_auth_alloc();
        mf_ultralight_generate_amiibo_pass(
            mf_ul_auth, data->iso14443_3a_data->uid, data->iso14443_3a_data->uid_len);
        */
        uint8_t PWD[4];
        weebo_scene_write_calculate_pwd(data->iso14443_3a_data->uid, PWD);

        MfUltralightData* newdata = mf_ultralight_alloc();
        nfc_device_copy_data(weebo->nfc_device, NfcProtocolMfUltralight, newdata);

        do {
            // user data
            for(size_t i = userMemoryFirst; i <= userMemoryLast; i++) {
                newdata->page[i / MF_ULTRALIGHT_PAGE_SIZE].data[i % MF_ULTRALIGHT_PAGE_SIZE] =
                    modified[i];
            }
            // pwd
            memcpy(newdata->page[pwd].data, PWD, sizeof(PWD));
            // pack
            memcpy(newdata->page[pack].data, PACKRFUI, sizeof(PACKRFUI));
            // capability container
            memcpy(newdata->page[capabilityContainer].data, CC, sizeof(CC));
            // cfg0
            memcpy(newdata->page[cfg0].data, CFG0, sizeof(CFG0));
            // cfg1
            memcpy(newdata->page[cfg1].data, CFG1, sizeof(CFG1));
            // dynamic lock bits
            memcpy(newdata->page[dynamicLockBits].data, DLB, sizeof(DLB));
            // static lock bits
            memcpy(newdata->page[staticLockBits].data, SLB, sizeof(SLB));

            nfc_device_set_data(weebo->nfc_device, NfcProtocolMfUltralight, newdata);

        } while(false);

        mf_ultralight_free(newdata);
        //mf_ultralight_auth_free(instance->mf_ul_auth);

        ret = NfcCommandStop;
    }
    return ret;
}

void weebo_scene_write_on_enter(void* context) {
    Weebo* weebo = context;
    Popup* popup = weebo->popup;

    popup_set_header(popup, "Writing", 58, 28, AlignCenter, AlignCenter);
    //popup_set_icon(popup, 0, 3, &I_RFIDDolphinReceive_97x61);
    // popup_set_text(popup, "words", 64, 36, AlignCenter, AlignTop);

    weebo->poller = nfc_poller_alloc(weebo->nfc, NfcProtocolMfUltralight);
    nfc_poller_start(weebo->poller, weebo_scene_write_poller_callback, weebo);

    weebo_blink_start(weebo);

    view_dispatcher_switch_to_view(weebo->view_dispatcher, WeeboViewPopup);
}

bool weebo_scene_write_on_event(void* context, SceneManagerEvent event) {
    Weebo* weebo = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(weebo->scene_manager, WeeboSceneWrite, event.event);
    }

    return consumed;
}

void weebo_scene_write_on_exit(void* context) {
    Weebo* weebo = context;

    if(weebo->poller) {
        nfc_poller_stop(weebo->poller);
        nfc_poller_free(weebo->poller);
        weebo->poller = NULL;
    }

    popup_reset(weebo->popup);
    weebo_blink_stop(weebo);
}
