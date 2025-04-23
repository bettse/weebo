#include "../weebo_i.h"
#include <nfc/protocols/mf_ultralight/mf_ultralight_listener.h>

#define TAG "SceneEmulate"

void weebo_scene_emulate_calculate_pwd(uint8_t* uid, uint8_t* pwd) {
    pwd[0] = uid[1] ^ uid[3] ^ 0xAA;
    pwd[1] = uid[2] ^ uid[4] ^ 0x55;
    pwd[2] = uid[3] ^ uid[5] ^ 0xAA;
    pwd[3] = uid[4] ^ uid[6] ^ 0x55;
}

void weebo_scene_emulate_remix(Weebo* weebo) {
    uint8_t PWD[4];
    uint8_t UID[8];
    uint8_t modified[NTAG215_SIZE];
    MfUltralightData* data = mf_ultralight_alloc();
    nfc_device_copy_data(weebo->nfc_device, NfcProtocolMfUltralight, data);

    //random uid
    FURI_LOG_D(TAG, "Generating random UID");
    UID[0] = 0x04;
    furi_hal_random_fill_buf(UID + 1, 6);
    UID[7] = UID[3] ^ UID[4] ^ UID[5] ^ UID[6];
    memcpy(weebo->figure + NFC3D_UID_OFFSET, UID, 8);
    memcpy(data->iso14443_3a_data->uid, UID, 7);

    //pack
    nfc3d_amiibo_pack(&weebo->amiiboKeys, weebo->figure, modified);

    //copy data in
    for(size_t i = 0; i < 130; i++) {
        memcpy(
            data->page[i].data, modified + i * MF_ULTRALIGHT_PAGE_SIZE, MF_ULTRALIGHT_PAGE_SIZE);
    }

    //new pwd
    weebo_scene_emulate_calculate_pwd(data->iso14443_3a_data->uid, PWD);
    memcpy(data->page[133].data, PWD, sizeof(PWD));

    //set data
    nfc_device_set_data(weebo->nfc_device, NfcProtocolMfUltralight, data);

    mf_ultralight_free(data);
}

void weebo_scene_emulate_widget_callback(GuiButtonType result, InputType type, void* context) {
    furi_assert(context);
    Weebo* weebo = context;

    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(weebo->view_dispatcher, result);
    }
}

void weebo_scene_emulate_on_enter(void* context) {
    Weebo* weebo = context;
    Widget* widget = weebo->widget;

    nfc_device_load(weebo->nfc_device, furi_string_get_cstr(weebo->load_path));
    const MfUltralightData* data = nfc_device_get_data(weebo->nfc_device, NfcProtocolMfUltralight);
    weebo->listener = nfc_listener_alloc(weebo->nfc, NfcProtocolMfUltralight, data);
    nfc_listener_start(weebo->listener, NULL, NULL);

    FuriString* info_str = furi_string_alloc();
    furi_string_cat_printf(info_str, "Emulating");
    widget_add_string_element(
        widget, 64, 5, AlignCenter, AlignCenter, FontSecondary, furi_string_get_cstr(info_str));

    widget_add_button_element(
        widget, GuiButtonTypeCenter, "Remix", weebo_scene_emulate_widget_callback, weebo);

    furi_string_free(info_str);
    weebo_blink_start(weebo);
    view_dispatcher_switch_to_view(weebo->view_dispatcher, WeeboViewWidget);
}

bool weebo_scene_emulate_on_event(void* context, SceneManagerEvent event) {
    Weebo* weebo = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(weebo->scene_manager, WeeboSceneEmulate, event.event);
        if(event.event == GuiButtonTypeCenter) {
            //stop listener
            FURI_LOG_D(TAG, "Stopping listener");
            nfc_listener_stop(weebo->listener);
            nfc_listener_free(weebo->listener);
            weebo->listener = NULL;

            weebo_scene_emulate_remix(weebo);
            //start listener
            FURI_LOG_D(TAG, "Starting listener");
            const MfUltralightData* data =
                nfc_device_get_data(weebo->nfc_device, NfcProtocolMfUltralight);
            weebo->listener = nfc_listener_alloc(weebo->nfc, NfcProtocolMfUltralight, data);
            nfc_listener_start(weebo->listener, NULL, NULL);

            consumed = true;
        }
    }

    return consumed;
}

void weebo_scene_emulate_on_exit(void* context) {
    Weebo* weebo = context;

    if(weebo->listener) {
        nfc_listener_stop(weebo->listener);
        nfc_listener_free(weebo->listener);
        weebo->listener = NULL;
    }

    widget_reset(weebo->widget);
    weebo_blink_stop(weebo);
}
