#include "eek/layersurface.h"

void squeek_manager_set_surface_height(PhoshLayerSurface *surface, uint32_t desired_height) {
    phosh_layer_surface_set_size(surface, 0,
                             (gint)desired_height);
    phosh_layer_surface_set_exclusive_zone(surface, (gint)desired_height);
    phosh_layer_surface_wl_surface_commit (surface);
}
