#include "../weebo_i.h"
#include <nfc/protocols/mf_ultralight/mf_ultralight_poller.h>

#define TAG "SceneWrite"

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
        ret = NfcCommandStop;
    }
    /*
        } else if(instance->mf_ul_auth->type == MfUltralightAuthTypeAmiibo) {
            if(mf_ultralight_generate_amiibo_pass(
                   weebo->mf_ul_auth,
                   data->iso14443_3a_data->uid,
                   data->iso14443_3a_data->uid_len)) {
                mf_ultralight_event->data->auth_context.skip_auth = false;
            }
*/

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
