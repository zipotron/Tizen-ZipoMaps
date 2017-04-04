/* Author: Carlos Dominguez */

#include "zipomaps.h"
#include "calcfunctions.h"
#include "xmlfunctions.h"
#include "button_bar.h"
#include "config.h"
#include <tizen.h>
#include <stdio.h>
#include <string.h>
#include <system_info.h>
#include <system_settings.h>
#include <dlog.h>
#include <libxml/encoding.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * https://{s}.tile.thunderforest.com/cycle/{z}/{x}/{y}.png?apikey=f23adf67ad974aa38a80c8a94b114e44
 */

static void
state_changed_cb(location_service_state_e state, void *user_data)
{
    appdata_s *ad = user_data;
    char buf[100];
    char *enable = (state == LOCATIONS_SERVICE_ENABLED) ? "Enable" : "Disable";
    dlog_print(DLOG_INFO, "tag", "%s - %s", __func__, enable);
    sprintf(buf, "State is %s", enable);
    elm_object_text_set(ad->labelGps, buf);
}

void
start_gps(appdata_s *ad)
{
	dlog_print(DLOG_INFO, "tag", "%s", __func__);
    location_manager_create(LOCATIONS_METHOD_GPS, &ad->manager);
    location_manager_set_service_state_changed_cb(ad->manager, state_changed_cb, ad);
    location_manager_start(ad->manager);
}

void
stop_gps(appdata_s *ad)
{
    location_manager_stop(ad->manager);
}

static void
show_map_point(void *user_data){
	appdata_s *ad = user_data;
	//elm_map_zoom_set(ad->map.mapService, ad->visor.zoom);
	elm_map_region_show(ad->map.mapService, ad->visor.longitude, ad->visor.latitude);
	if(!ad->map.ovl){
		Evas_Object *icon;
		ad->map.ovl = elm_map_overlay_add(ad->map.mapService, ad->visor.longitude, ad->visor.latitude);
		icon = elm_icon_add(ad->map.mapService);
		elm_icon_standard_set(icon, "home");
		elm_map_overlay_icon_set(ad->map.ovl, icon);
	}
}

static void
set_position(double latitude, double longitude, double altitude, time_t timestamp, void *user_data){
	appdata_s *ad = user_data;
	if((ad->map.recording) && (ad->xml.trkData)){
		ad->map.ovl = elm_map_overlay_line_add(ad->map.mapService, ad->visor.longitude, ad->visor.latitude, longitude, latitude);
	}
	ad->visor.latitude = latitude;
	ad->visor.longitude = longitude;
	ad->visor.altitude = altitude;
	ad->visor.timestamp = timestamp;
}

void
position_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	if(timestamp){
		appdata_s *ad = user_data;
		set_position(latitude, longitude, altitude, timestamp, ad);
		//show_map_point(ad);

		char buf[100];
		sprintf(buf, "%sPos:%0.5f/%0.5f Alt:%0.1f%s", LABELFORMATSTART, latitude, longitude, altitude, LABELFORMATEND);
		elm_object_text_set(ad->labelGps, buf);

		sprintf(buf, "%sSpeed:%0.1f m/s - Max:%0.1f m/s%s", LABELFORMATSTART, speedAndDistance(latitude, longitude, altitude, timestamp, &(ad->tracker.maxSpeed), ad->tracker.maxAcceleration,NULL), ad->tracker.maxSpeed, LABELFORMATEND);
		elm_object_text_set(ad->labelCalc, buf);

    //Temporal trick
    	/*char bufLink[512];
    	sprintf(bufLink, "https://tile.thunderforest.com/cycle/%d/%d/%d.png?apikey=f23adf67ad974aa38a80c8a94b114e44", ad->visor.zoom, long2tilex(longitude, ad->visor.zoom), lat2tiley(latitude, ad->visor.zoom));
    	ad->downloader.state = 0;
    	ad->downloader.download = download_create(&(ad->downloader.download_id));
    	//ad->downloader.download = download_set_url(ad->download_id, "https://tile.thunderforest.com/cycle/6/20/20.png?apikey=f23adf67ad974aa38a80c8a94b114e44");
    	ad->downloader.download = download_set_url(ad->downloader.download_id, bufLink);
    	char bufd[128];
    	sprintf(bufd, DIR_MAPS"/%d",ad->visor.zoom);
    	ad->downloader.download = download_set_destination(ad->downloader.download_id, bufd);
    	ad->downloader.download = download_set_file_name(ad->downloader.download_id, "map.png");
    	//ad->downloader.download = download_set_auto_download(download_id, true);
    	ad->downloader.download = download_start(ad->downloader.download_id);*/
    //Temporal trick end
	}
}

void
position_updated_record_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	if(timestamp){
		appdata_s *ad = user_data;
		set_position(latitude, longitude, altitude, timestamp, ad);
		show_map_point(ad);

		char *result;
		char buf[100];

		result = xmlwriterAddNode(latitude, longitude, altitude, timestamp, ad);
		sprintf(buf, "%sPos:%0.5f/%0.5f Alt:%0.1f - %s%s", LABELFORMATSTART, latitude, longitude, altitude, result, LABELFORMATEND);

		if(ad->xml.writeNextWpt){
			free(result);
			result = xmlwriterAddWpt(latitude, longitude, altitude, ad);
			ad->xml.writeNextWpt = 0;

			Evas_Object *icon;
			ad->map.ovl = elm_map_overlay_add(ad->map.mapService, ad->visor.longitude, ad->visor.latitude);
			icon = elm_icon_add(ad->map.mapService);
			elm_icon_standard_set(icon, "clock");
			elm_map_overlay_icon_set(ad->map.ovl, icon);
		}

		elm_object_text_set(ad->labelGps, buf);
		sprintf(buf, "%sSpeed:%0.1f m/s - Max:%0.1f m/s%s", LABELFORMATSTART, speedAndDistance(latitude, longitude, altitude, timestamp, &(ad->tracker.maxSpeed), ad->tracker.maxAcceleration, ad), ad->tracker.maxSpeed, LABELFORMATEND);
		elm_object_text_set(ad->labelCalc, buf);
		sprintf(buf, "%sTotal Distance:%0.1f%s", LABELFORMATSTART, ad->tracker.distance, LABELFORMATEND);
		elm_object_text_set(ad->labelDist, buf);
		free(result);

		/*ad->downloader.download = download_get_state(ad->downloader.download_id, &(ad->downloader.state));
    	if (ad->downloader.state == DOWNLOAD_STATE_COMPLETED){
    		ad->downloader.state = 0;

    		char bufd[12];
    		sprintf(bufd, DIR_MAPS"/%d/map.png",ad->visor.zoom);
    		evas_object_image_file_set(ad->img, bufd, NULL);
    	}*/
	}
}

static void
slider_Interval_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    appdata_s *ad = data;

    ad->interval = elm_slider_value_get(obj);
    location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
}

/*static void
slider_Zoom_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    appdata_s *ad = data;

    ad->visor.zoom = elm_slider_value_get(obj);
}*/

static void app_get_resource(const char *res_file_in, char *res_path_out, int res_path_max)
{
    char *res_path = app_get_resource_path();
    if (res_path) {
        snprintf(res_path_out, res_path_max, "%s%s", res_path, res_file_in);
        free(res_path);
    }
}

static void my_table_pack(Evas_Object *table, Evas_Object *child, int x, int y, int w, int h)
{
   evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_table_pack(table, child, x, y, w, h);
   evas_object_show(child);
}

static int
widget_instance_create(widget_context_h context, bundle *content, int w, int h, void *user_data)
{
	appdata_s *ad = (appdata_s*) malloc(sizeof(appdata_s));
	int ret;
	ad->visor.latitude = 0;
	ad->visor.longitude = 0;
	ad->interval = 5;
	ad->xml.writeNextWpt = 0;
	ad->xml.docWpt = NULL;
	ad->xml.writerWpt = NULL;
	ad->xml.wptData = false;
	ad->xml.docTrk = NULL;
	ad->xml.writerTrk = NULL;
	ad->xml.trkData = false;
	ad->tracker.maxSpeed = 0;
	ad->tracker.distance = 0;
	ad->tracker.maxAcceleration = 15;
	ad->map.ovl = NULL;
	ad->map.recording = false;
	//ad->visor.zoom = 7;

	if (content != NULL) {
		/* Recover the previous status with the bundle object. */

	}

	/* Window */
	ret = widget_app_get_elm_win(context, &ad->win);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to get window. err = %d", ret);
		return WIDGET_ERROR_FAULT;
	}

	evas_object_resize(ad->win, w, h);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Mapservice BG*/
	ad->map.mapService = elm_map_add(ad->conform);
	elm_map_zoom_mode_set(ad->map.mapService, ELM_MAP_ZOOM_MODE_MANUAL);
	elm_map_zoom_set(ad->map.mapService, 12);

	evas_object_size_hint_max_set(ad->map.mapService, w, h);
	evas_object_size_hint_min_set(ad->map.mapService, w, h);
	evas_object_size_hint_weight_set(ad->map.mapService, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->map.mapService, EVAS_HINT_FILL, 0.5);
	evas_object_resize(ad->map.mapService, w, h);
	evas_object_move(ad->map.mapService, 0, 0);
	evas_object_show(ad->map.mapService);

	elm_map_region_show(ad->map.mapService, 2.665, 39.576);
	ad->map.scale = elm_map_overlay_scale_add(ad->map.mapService, (w*2)/3, h - 20);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

	widget_app_context_set_tag(context, ad);
	return WIDGET_ERROR_NONE;
}

static int
widget_instance_destroy(widget_context_h context, widget_app_destroy_type_e reason, bundle *content, void *user_data)
{
	appdata_s *wid = NULL;
	widget_app_context_get_tag(context,(void**)&wid);

	if (wid->win)
		evas_object_del(wid->win);

	free(wid);

	return WIDGET_ERROR_NONE;
}

static int
widget_instance_pause(widget_context_h context, void *user_data)
{
	/* Take necessary actions when widget instance becomes invisible. */
	return WIDGET_ERROR_NONE;

}

static int
widget_instance_resume(widget_context_h context, void *user_data)
{
	/* Take necessary actions when widget instance becomes visible. */
	return WIDGET_ERROR_NONE;
}

static int
widget_instance_update(widget_context_h context, bundle *content,
                             int force, void *user_data)
{
	/* Take necessary actions when widget instance should be updated. */
	return WIDGET_ERROR_NONE;
}

static int
widget_instance_resize(widget_context_h context, int w, int h, void *user_data)
{
	/* Take necessary actions when the size of widget instance was changed. */
	return WIDGET_ERROR_NONE;
}

static void
widget_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_LANGUAGE_CHANGED */
	char *locale = NULL;
	app_event_get_language(event_info, &locale);
	elm_language_set(locale);
	free(locale);
}

static void
widget_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/* APP_EVENT_REGION_FORMAT_CHANGED */
}

static widget_class_h
widget_app_create(void *user_data)
{
	appdata_s *ad = user_data;

	//create_base_gui(ad);

	struct stat buf;
	if( stat(DIR, &buf) == -1 ){
		mkdir(DIR, 0777);
		mkdir(DIR_TRK, 0777);
		/*mkdir(DIR_MAPS, 0777);
		for(int i=1; i<15;i++){
			char bufd[128];
			sprintf(bufd, DIR_MAPS"/%d", i);
			mkdir(bufd, 0777);
		}*/
	}else{
		if( stat(DIR_TRK, &buf) == -1 )
			mkdir(DIR_TRK, 0777);
		/*if( stat(DIR_MAPS, &buf) == -1 ){
			mkdir(DIR_MAPS, 0777);
			for(int i=1; i<15;i++){
				char bufd[128];
				sprintf(bufd, DIR_MAPS"/%d", i);
				mkdir(bufd, 0777);
			}
		}else{
			for(int i=1; i<15;i++){
				char bufd[128];
				sprintf(bufd, DIR_MAPS"/%d", i);
				if( stat(bufd, &buf) == -1 )
					mkdir(bufd, 0777);
			}
		}*/
	}

	app_event_handler_h handlers[5] = {NULL, };

	widget_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
		APP_EVENT_LANGUAGE_CHANGED, widget_app_lang_changed, user_data);
	widget_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
		APP_EVENT_REGION_FORMAT_CHANGED, widget_app_region_changed, user_data);

	widget_instance_lifecycle_callback_s ops = {
		.create = widget_instance_create,
		.destroy = widget_instance_destroy,
		.pause = widget_instance_pause,
		.resume = widget_instance_resume,
		.update = widget_instance_update,
		.resize = widget_instance_resize,
	};

	return widget_app_class_create(ops, user_data);
}

static void
widget_app_terminate(void *user_data)
{
	/* Release all resources. */
}

int
main(int argc, char *argv[])
{
	widget_app_lifecycle_callback_s ops = {0,};
	int ret;

	ops.create = widget_app_create;
	ops.terminate = widget_app_terminate;

	ret = widget_app_main(argc, argv, &ops, NULL);
	if (ret != WIDGET_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "widget_app_main() is failed. err = %d", ret);
	}

	return ret;
}
