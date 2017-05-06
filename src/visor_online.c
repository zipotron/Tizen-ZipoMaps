#include "visor_online.h"


void draw_trk_line_incremental(double lon, double lat, appdata_s *ad){
	static double lon_or = 0;
	static double lat_or = 0;
	static int initialized = 0;

	if(initialized && (!((lon_or == lon) && (lat_or == lat))))
		ad->map.ovl = elm_map_overlay_line_add(ad->map.mapService, lon_or, lat_or, lon, lat);

	initialized = true;
	lon_or = lon;
	lat_or = lat;
}

void show_home_mark(double lon, double lat, appdata_s *ad){
	Evas_Object *icon;
	ad->map.ovl = elm_map_overlay_add(ad->map.mapService, lon, lat);
	icon = elm_icon_add(ad->map.mapService);
	elm_icon_standard_set(icon, "home");
	elm_map_overlay_icon_set(ad->map.ovl, icon);
}

void show_wpt_mark(double lon, double lat, appdata_s *ad){
	Evas_Object *icon;
	ad->map.ovl = elm_map_overlay_add(ad->map.mapService, lon, lat);
	icon = elm_icon_add(ad->map.mapService);
	elm_icon_standard_set(icon, "clock");
	elm_map_overlay_icon_set(ad->map.ovl, icon);
}
