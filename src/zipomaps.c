/* Author: Carlos Dominguez */

#include "zipomaps.h"
#include "calcfunctions.h"
#include "button_bar.h"
#include "visor_online.h"
#include "write_file.h"
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

	elm_map_region_show(ad->map.mapService, ad->visor.longitude, ad->visor.latitude);
	if((!ad->map.ovl) && (ad->map.recording))
		show_home_mark(ad->visor.longitude, ad->visor.latitude, ad);
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

static void
print_gps_data(double latitude, double longitude, double altitude, time_t timestamp, appdata_s *ad, int kmh, int print_distance){

	char buf[100];

	double speed = speedAndDistance(latitude, longitude, altitude, timestamp, &(ad->tracker.maxSpeed), ad->tracker.maxAcceleration, ad);

	sprintf(buf, "%sPos:%0.5f/%0.5f Alt:%0.1f%s", LABELFORMATSTART, latitude, longitude, altitude, LABELFORMATEND);
	elm_object_text_set(ad->labelGps, buf);
	if(kmh)
		sprintf(buf, "%sSpeed:%.0f km/h - Max:%.0f km/h%s", LABELFORMATSTART, speed * 3.6, ad->tracker.maxSpeed *3.6, LABELFORMATEND);
	else
		sprintf(buf, "%sSpeed:%.0f m/s - Max:%.0f m/s%s", LABELFORMATSTART, speed, ad->tracker.maxSpeed, LABELFORMATEND);

	/*elm_object_text_set(ad->labelCalc, buf);*/
	if(print_distance){
		sprintf(buf, "%sTotal Distance:%.0f m%s", LABELFORMATSTART, ad->tracker.distance, LABELFORMATEND);
		/*elm_object_text_set(ad->labelDist, buf);*/
	}
}

void
position_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	appdata_s *ad = user_data;
	set_position(latitude, longitude, altitude, timestamp, ad);
	show_map_point(ad);
	print_gps_data(latitude, longitude, altitude, timestamp, ad, true, false);
}

void
position_updated_record_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	appdata_s *ad = user_data;
	set_position(latitude, longitude, altitude, timestamp, ad);
	show_map_point(ad);
	print_gps_data(latitude, longitude, altitude, timestamp, ad, true, true);
	ad->visor.gps_data = 1;

	char *result;

	result = xmlwriterAddNode(longitude, latitude, altitude, timestamp, ad);

	if(ad->xml.writeNextWpt > -1){
		if(ad->xml.writeNextWpt == 0)
				ad->xml.writeNextWpt = -1;
		else {
			free(result);
			result = xmlwriterAddWpt(longitude, latitude, altitude, ad);
			show_wpt_mark(ad->visor.longitude, ad->visor.latitude, ad);
		}
	}
	free(result);
}

static void
slider_Interval_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    appdata_s *ad = data;

    ad->interval = elm_slider_value_get(obj);
    location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
}

static void app_get_resource(const char *res_file_in, char *res_path_out, int res_path_max)
{
    char *res_path = app_get_resource_path();
    if (res_path) {
        snprintf(res_path_out, res_path_max, "%s%s", res_path, res_file_in);
        free(res_path);
    }
}

/*static void my_table_pack(Evas_Object *table, Evas_Object *child, int x, int y, int w, int h)
{
   evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_table_pack(table, child, x, y, w, h);
   evas_object_show(child);
}*/

static int
widget_instance_create(widget_context_h context, bundle *content, int w, int h, void *user_data)
{
	appdata_s *ad = (appdata_s*) malloc(sizeof(appdata_s));
	int ret;
	ad->visor.latitude = 0;
	ad->visor.longitude = 0;
	ad->visor.timestamp = 0;
	ad->interval = 5;
	ad->xml.writeNextWpt = -1;
	ad->xml.docWpt = NULL;
	ad->xml.writerWpt = NULL;
	ad->xml.wptData = false;
	ad->xml.docTrk = NULL;
	ad->xml.writerTrk = NULL;
	ad->xml.trkData = false;
	ad->tracker.maxSpeed = 0;
	ad->tracker.distance = 0;
	ad->tracker.maxAcceleration = 10;
	ad->map.ovl = NULL;
	ad->map.recording = false;
	ad->visor.gps_data = 0;

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

	/*Set slidebar*/
	ad->sliderInterval = elm_slider_add(ad->conform);
	elm_slider_min_max_set(ad->sliderInterval, 1, 100);
	elm_slider_value_set(ad->sliderInterval, ad->interval);
	elm_slider_indicator_show_set(ad->sliderInterval, EINA_TRUE);
	elm_slider_indicator_format_set(ad->sliderInterval, "%1.0f");
	evas_object_smart_callback_add(ad->sliderInterval, "changed", slider_Interval_changed_cb, ad);

	evas_object_size_hint_weight_set(ad->sliderInterval,0.0,1.0);
	evas_object_size_hint_align_set(ad->sliderInterval,-1.0,1.0);
	evas_object_color_set(ad->sliderInterval, 0, 200, 0, 255);
	evas_object_resize(ad->sliderInterval, w, 10);
	evas_object_move(ad->sliderInterval, 0, h - 30);

	/* Label */
	ad->labelGps = elm_label_add(ad->conform);
	elm_object_text_set(ad->labelGps, LABELFORMATSTART "Waiting GPS status" LABELFORMATEND);
	evas_object_size_hint_weight_set(ad->labelGps,0.0,1.0);
	evas_object_size_hint_align_set(ad->labelGps,-1.0,1.0);
	evas_object_resize(ad->labelGps, w, 50);
	evas_object_move(ad->labelGps, 0, h - 50);

	/*Set button bar*/
	ad->btn_on = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_on,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_on,-1.0,1.0);
	elm_object_style_set(ad->btn_on, "circle");
	elm_object_text_set(ad->btn_on,"On");
	evas_object_smart_callback_add(ad->btn_on, "clicked", btn_gps_on_clicked_cb, ad);
	evas_object_color_set(ad->btn_on, 0, 0, 0, 128);
	evas_object_show(ad->btn_on);
	evas_object_resize(ad->btn_on, 100, 100);
	evas_object_move(ad->btn_on, 0, 0);

	Evas_Object *ic;
	char bt_img[PATH_MAX];
	app_get_resource("gps_on.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_on);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_on,"icon",ic);
	evas_object_show(ic);

	ad->btn_clean = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_clean,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_clean,-1.0,1.0);
	elm_object_style_set(ad->btn_clean, "circle");
	elm_object_text_set(ad->btn_on,"clean");
	evas_object_smart_callback_add(ad->btn_clean, "clicked", btn_clean_clicked_cb, ad);
	evas_object_color_set(ad->btn_clean, 0, 0, 0, 128);
	evas_object_show(ad->btn_clean);
	evas_object_resize(ad->btn_clean, 100, 100);
	evas_object_move(ad->btn_clean, w - 100, 0);

	app_get_resource("clean.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_clean);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_clean,"icon",ic);
	evas_object_show(ic);

	ad->btn_off = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_off,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_off,-1.0,1.0);
	elm_object_style_set(ad->btn_off, "circle");
	elm_object_text_set(ad->btn_off,"Off");
	evas_object_smart_callback_add(ad->btn_off, "clicked", btn_gps_off_clicked_cb, ad);
	evas_object_color_set(ad->btn_off, 0, 0, 0, 128);
	evas_object_resize(ad->btn_off, 100, 100);
	evas_object_move(ad->btn_off, 0, 0);

	app_get_resource("gps_off.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_off);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_off,"icon",ic);
	evas_object_show(ic);

	ad->btn_record = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_record,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_record,-1.0,1.0);
	elm_object_style_set(ad->btn_off, "circle");
	elm_object_text_set(ad->btn_off,"Record");
	evas_object_smart_callback_add(ad->btn_record, "clicked", btn_record_clicked_cb, ad);
	evas_object_color_set(ad->btn_record, 0, 0, 0, 128);
	evas_object_resize(ad->btn_record, 100, 100);
	evas_object_move(ad->btn_record, w - 100, 0);

	app_get_resource("record.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_record);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_record,"icon",ic);
	evas_object_show(ic);

	ad->btn_stop = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_stop,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_stop,-1.0,1.0);
	elm_object_style_set(ad->btn_stop, "circle");
	elm_object_text_set(ad->btn_off,"Stop");
	evas_object_smart_callback_add(ad->btn_stop, "clicked", btn_stop_clicked_cb, ad);
	evas_object_color_set(ad->btn_stop, 0, 0, 0, 128);
	evas_object_resize(ad->btn_stop, 100, 100);
	evas_object_move(ad->btn_stop, 0, 0);

	app_get_resource("stop.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_stop);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_stop,"icon",ic);
	evas_object_show(ic);

	ad->btn_point = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_point,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_point,-1.0,1.0);
	elm_object_style_set(ad->btn_off, "circle");
	elm_object_text_set(ad->btn_off,"Point");
	evas_object_smart_callback_add(ad->btn_point, "clicked", btn_point_clicked_cb, ad);
	evas_object_color_set(ad->btn_point, 0, 0, 0, 128);
	evas_object_resize(ad->btn_point, 100, 100);
	evas_object_move(ad->btn_point, w - 100, 0);

	app_get_resource("wpt.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_point);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_point,"icon",ic);
	evas_object_show(ic);

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
	/*appdata_s *ad = user_data;

	create_base_gui(ad);*/

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
