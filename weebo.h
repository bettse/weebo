#pragma once

typedef struct Weebo Weebo;

uint16_t weebo_get_figure_id(Weebo* weebo);

bool weebo_get_figure_name(Weebo* weebo, FuriString* name);
