#include "weebo_i.h"

#define TAG "weebo"

#define WEEBO_KEY_RETAIL_FILENAME "key_retail"

bool weebo_load_key_retail(Weebo* weebo) {
    FuriString* path = furi_string_alloc();
    bool parsed = false;
    uint8_t buffer[160];
    memset(buffer, 0, sizeof(buffer));
    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    do {
        furi_string_printf(
            path, "%s/%s%s", STORAGE_APP_DATA_PATH_PREFIX, WEEBO_KEY_RETAIL_FILENAME, ".bin");

        bool opened =
            file_stream_open(stream, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING);
        if(!opened) {
            FURI_LOG_E(TAG, "Failed to open file");
            break;
        }

        size_t bytes_read = stream_read(stream, buffer, sizeof(buffer));
        if(bytes_read != sizeof(buffer)) {
            FURI_LOG_E(TAG, "Insufficient data");
            break;
        }

        memcpy(&weebo->amiiboKeys, buffer, bytes_read);

        // TODO: compare SHA1
        parsed = true;
    } while(false);

    file_stream_close(stream);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(path);

    return parsed;
}

bool weebo_load_figure(Weebo* weebo, FuriString* path, bool show_dialog) {
    bool parsed = false;
    uint8_t buffer[NTAG215_SIZE];
    memset(buffer, 0, sizeof(buffer));

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);

    if(weebo->loading_cb) {
        weebo->loading_cb(weebo->loading_cb_ctx, true);
    }

    // TODO: reject if the selected file is key_retail
    do {
        bool opened =
            file_stream_open(stream, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING);
        if(!opened) {
            FURI_LOG_E(TAG, "Failed to open file");
            break;
        }

        size_t bytes_read = stream_read(stream, buffer, sizeof(buffer));
        if(bytes_read != sizeof(buffer)) {
            FURI_LOG_E(TAG, "Insufficient data");
            break;
        }

        if(!nfc3d_amiibo_unpack(&weebo->amiiboKeys, buffer, weebo->figure)) {
            FURI_LOG_E(TAG, "Failed to unpack");
            break;
        }

        parsed = true;
    } while(false);

    file_stream_close(stream);
    furi_record_close(RECORD_STORAGE);

    if(weebo->loading_cb) {
        weebo->loading_cb(weebo->loading_cb_ctx, false);
    }

    if((!parsed) && (show_dialog)) {
        dialog_message_show_storage_error(weebo->dialogs, "Can not parse\nfile");
    }

    return parsed;
}

void weebo_set_loading_callback(Weebo* weebo, WeeboLoadingCallback callback, void* context) {
    furi_assert(weebo);

    weebo->loading_cb = callback;
    weebo->loading_cb_ctx = context;
}

bool weebo_file_select(Weebo* weebo) {
    furi_assert(weebo);
    bool res = false;

    FuriString* weebo_app_folder = furi_string_alloc_set(STORAGE_APP_DATA_PATH_PREFIX);

    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, ".bin", &I_Nfc_10px);
    browser_options.base_path = STORAGE_APP_DATA_PATH_PREFIX;

    res = dialog_file_browser_show(
        weebo->dialogs, weebo->load_path, weebo_app_folder, &browser_options);

    furi_string_free(weebo_app_folder);
    if(res) {
        FuriString* filename;
        filename = furi_string_alloc();
        path_extract_filename(weebo->load_path, filename, true);
        strncpy(weebo->file_name, furi_string_get_cstr(filename), WEEBO_FILE_NAME_MAX_LENGTH);
        res = weebo_load_figure(weebo, weebo->load_path, true);
        furi_string_free(filename);
    }

    return res;
}

bool weebo_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Weebo* weebo = context;
    return scene_manager_handle_custom_event(weebo->scene_manager, event);
}

bool weebo_back_event_callback(void* context) {
    furi_assert(context);
    Weebo* weebo = context;
    return scene_manager_handle_back_event(weebo->scene_manager);
}

void weebo_tick_event_callback(void* context) {
    furi_assert(context);
    Weebo* weebo = context;
    scene_manager_handle_tick_event(weebo->scene_manager);
}

Weebo* weebo_alloc() {
    Weebo* weebo = malloc(sizeof(Weebo));

    weebo->view_dispatcher = view_dispatcher_alloc();
    weebo->scene_manager = scene_manager_alloc(&weebo_scene_handlers, weebo);
    view_dispatcher_set_event_callback_context(weebo->view_dispatcher, weebo);
    view_dispatcher_set_custom_event_callback(weebo->view_dispatcher, weebo_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        weebo->view_dispatcher, weebo_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        weebo->view_dispatcher, weebo_tick_event_callback, 100);

    weebo->nfc = nfc_alloc();

    // Nfc device
    weebo->nfc_device = nfc_device_alloc();
    nfc_device_set_loading_callback(weebo->nfc_device, weebo_show_loading_popup, weebo);

    // Open GUI record
    weebo->gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(
        weebo->view_dispatcher, weebo->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    weebo->notifications = furi_record_open(RECORD_NOTIFICATION);

    // Submenu
    weebo->submenu = submenu_alloc();
    view_dispatcher_add_view(
        weebo->view_dispatcher, WeeboViewMenu, submenu_get_view(weebo->submenu));

    // Popup
    weebo->popup = popup_alloc();
    view_dispatcher_add_view(weebo->view_dispatcher, WeeboViewPopup, popup_get_view(weebo->popup));

    // Loading
    weebo->loading = loading_alloc();
    view_dispatcher_add_view(
        weebo->view_dispatcher, WeeboViewLoading, loading_get_view(weebo->loading));

    // Text Input
    weebo->text_input = text_input_alloc();
    view_dispatcher_add_view(
        weebo->view_dispatcher, WeeboViewTextInput, text_input_get_view(weebo->text_input));

    // Number Input
    weebo->number_input = number_input_alloc();
    view_dispatcher_add_view(
        weebo->view_dispatcher, WeeboViewNumberInput, number_input_get_view(weebo->number_input));

    // TextBox
    weebo->text_box = text_box_alloc();
    view_dispatcher_add_view(
        weebo->view_dispatcher, WeeboViewTextBox, text_box_get_view(weebo->text_box));
    weebo->text_box_store = furi_string_alloc();

    // Custom Widget
    weebo->widget = widget_alloc();
    view_dispatcher_add_view(
        weebo->view_dispatcher, WeeboViewWidget, widget_get_view(weebo->widget));

    weebo->storage = furi_record_open(RECORD_STORAGE);
    weebo->dialogs = furi_record_open(RECORD_DIALOGS);
    weebo->load_path = furi_string_alloc();

    weebo->keys_loaded = false;

    return weebo;
}

void weebo_free(Weebo* weebo) {
    furi_assert(weebo);

    nfc_free(weebo->nfc);

    // Nfc device
    nfc_device_free(weebo->nfc_device);

    // Submenu
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewMenu);
    submenu_free(weebo->submenu);

    // Popup
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewPopup);
    popup_free(weebo->popup);

    // Loading
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewLoading);
    loading_free(weebo->loading);

    // TextInput
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewTextInput);
    text_input_free(weebo->text_input);

    // NumberInput
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewNumberInput);
    number_input_free(weebo->number_input);

    // TextBox
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewTextBox);
    text_box_free(weebo->text_box);
    furi_string_free(weebo->text_box_store);

    // Custom Widget
    view_dispatcher_remove_view(weebo->view_dispatcher, WeeboViewWidget);
    widget_free(weebo->widget);

    // View Dispatcher
    view_dispatcher_free(weebo->view_dispatcher);

    // Scene Manager
    scene_manager_free(weebo->scene_manager);

    // GUI
    furi_record_close(RECORD_GUI);
    weebo->gui = NULL;

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    weebo->notifications = NULL;

    furi_string_free(weebo->load_path);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_DIALOGS);

    free(weebo);
}

void weebo_text_store_set(Weebo* weebo, const char* text, ...) {
    va_list args;
    va_start(args, text);

    vsnprintf(weebo->text_store, sizeof(weebo->text_store), text, args);

    va_end(args);
}

void weebo_text_store_clear(Weebo* weebo) {
    memset(weebo->text_store, 0, sizeof(weebo->text_store));
}

static const NotificationSequence weebo_sequence_blink_start_blue = {
    &message_blink_start_10,
    &message_blink_set_color_blue,
    &message_do_not_reset,
    NULL,
};

static const NotificationSequence weebo_sequence_blink_stop = {
    &message_blink_stop,
    NULL,
};

void weebo_blink_start(Weebo* weebo) {
    notification_message(weebo->notifications, &weebo_sequence_blink_start_blue);
}

void weebo_blink_stop(Weebo* weebo) {
    notification_message(weebo->notifications, &weebo_sequence_blink_stop);
}

void weebo_show_loading_popup(void* context, bool show) {
    Weebo* weebo = context;

    if(show) {
        // Raise timer priority so that animations can play
        furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);
        view_dispatcher_switch_to_view(weebo->view_dispatcher, WeeboViewLoading);
    } else {
        // Restore default timer priority
        furi_timer_set_thread_priority(FuriTimerThreadPriorityNormal);
    }
}

int32_t weebo_app(void* p) {
    UNUSED(p);
    Weebo* weebo = weebo_alloc();

    weebo->keys_loaded = weebo_load_key_retail(weebo);
    if(weebo->keys_loaded) {
        scene_manager_next_scene(weebo->scene_manager, WeeboSceneMainMenu);
    } else {
        scene_manager_next_scene(weebo->scene_manager, WeeboSceneKeysMissing);
    }

    view_dispatcher_run(weebo->view_dispatcher);

    weebo_free(weebo);

    return 0;
}
