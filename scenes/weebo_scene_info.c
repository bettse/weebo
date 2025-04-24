#include "../weebo_i.h"
#include <dolphin/dolphin.h>

#define TAG "WeeboSceneInfo"

void weebo_scene_info_on_enter(void* context) {
    Weebo* weebo = context;

    furi_string_reset(weebo->text_box_store);
    FuriString* str = weebo->text_box_store;

    FuriString* name = furi_string_alloc();
    furi_string_cat_printf(str, "Info:\n");
    if(weebo_get_figure_name(weebo, name)) {
        furi_string_cat_printf(str, "%s\n", furi_string_get_cstr(name));
    } else {
        furi_string_cat_printf(str, "Unknown\n");
    }
    furi_string_free(name);
    uint16_t id = 0;
    id |= weebo->figure[UNPACKED_FIGURE_ID + 0] << 8;
    id |= weebo->figure[UNPACKED_FIGURE_ID + 1] << 0;
    furi_string_cat_printf(str, "ID: %04x\n", id);

    text_box_set_font(weebo->text_box, TextBoxFontText);
    text_box_set_text(weebo->text_box, furi_string_get_cstr(weebo->text_box_store));
    view_dispatcher_switch_to_view(weebo->view_dispatcher, WeeboViewTextBox);
}

bool weebo_scene_info_on_event(void* context, SceneManagerEvent event) {
    Weebo* weebo = context;
    bool consumed = false;
    UNUSED(weebo);

    if(event.type == SceneManagerEventTypeCustom) {
    }
    return consumed;
}

void weebo_scene_info_on_exit(void* context) {
    Weebo* weebo = context;

    // Clear view
    text_box_reset(weebo->text_box);
}
