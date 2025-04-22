#include "../weebo_i.h"
enum SubmenuIndex {
    SubmenuIndexSamPresent,
    SubmenuIndexSamMissing,
};

static void weebo_scene_start_detect_callback(void* context) {
    Weebo* weebo = context;
    UNUSED(weebo);
    // view_dispatcher_send_custom_event(weebo->view_dispatcher, WeeboWorkerEventSamMissing);
}

void weebo_scene_start_submenu_callback(void* context, uint32_t index) {
    Weebo* weebo = context;
    view_dispatcher_send_custom_event(weebo->view_dispatcher, index);
}

void weebo_scene_start_on_enter(void* context) {
    Weebo* weebo = context;

    Popup* popup = weebo->popup;

    popup_set_context(weebo->popup, weebo);
    popup_set_callback(weebo->popup, weebo_scene_start_detect_callback);
    popup_set_header(popup, "Detecting SAM", 58, 48, AlignCenter, AlignCenter);
    popup_set_timeout(weebo->popup, 2500);
    popup_enable_timeout(weebo->popup);

    view_dispatcher_switch_to_view(weebo->view_dispatcher, WeeboViewPopup);
}

bool weebo_scene_start_on_event(void* context, SceneManagerEvent event) {
    Weebo* weebo = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(weebo->scene_manager, WeeboSceneStart, event.event);
        /*
        if(event.event == WeeboWorkerEventSamPresent) {
            scene_manager_next_scene(weebo->scene_manager, WeeboSceneSamPresent);
            consumed = true;
        } else if(event.event == WeeboWorkerEventSamMissing) {
            scene_manager_next_scene(weebo->scene_manager, WeeboSceneSamMissing);
            consumed = true;
        } else if(event.event == WeeboWorkerEventSamWrong) {
            scene_manager_next_scene(weebo->scene_manager, WeeboSceneSamWrong);
            consumed = true;
        }
        */
    }

    return consumed;
}

void weebo_scene_start_on_exit(void* context) {
    Weebo* weebo = context;
    popup_reset(weebo->popup);
}
