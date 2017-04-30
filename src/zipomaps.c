/* Author: Carlos Dominguez */

#include "zipomaps.h"
#include "calcfunctions.h"
#include "xmlfunctions.h"
#include "button_bar.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <system_info.h>
#include <Elementary.h>
#include <system_settings.h>
#include <dlog.h>
#include <libxml/encoding.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

/*
 * https://{s}.tile.thunderforest.com/cycle/{z}/{x}/{y}.png?apikey=f23adf67ad974aa38a80c8a94b114e44
 */

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	/*appdata_s *ad = data;*/

	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

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

void open_file(void *data, Evas_Object *obj, void *event_info)
{
	item_data_s *element_data = data;
	evas_object_hide(element_data->ad->open_win);
	char *label = element_data->label;
	//free(element_data); Core dump
}

static char *_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	char *i = (char *) data;
	if (!strcmp(part, "elm.text"))
		return strdup(i);

	else return NULL;
}

static void _item_del(void *data, Evas_Object *obj)
{
   printf("item(%d) is now deleted", (int) data);
}

static void bg_table_pack(Evas_Object *table, Evas_Object *child, int x, int y, int w, int h)
{
   evas_object_size_hint_align_set(child, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(child, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_table_pack(table, child, x, y, w, h);
   evas_object_show(child);
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

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

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[2] = { 0, 180 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 2);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	Evas_Object *bg;
	char f_bg[PATH_MAX];
	app_get_resource("bg.jpg", f_bg, (int)PATH_MAX);

	bg = elm_bg_add(ad->win);
	elm_bg_file_set(bg, f_bg, NULL);
	elm_bg_option_set(bg, ELM_BG_OPTION_STRETCH);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	ad->table = elm_table_add(ad->win);

	elm_object_content_set(ad->conform, ad->table);
	evas_object_show(ad->table);

	bg_table_pack(ad->table, bg, 0, 0, 4, 10);

	/* Image */
	/*Evas *canvas = evas_object_evas_get(ad->win);
	ad->img = evas_object_image_filled_add(canvas);*/

	int max, screen_size_w, screen_size_h;
	system_info_get_platform_int("tizen.org/feature/screen.height", &screen_size_h);
	system_info_get_platform_int("tizen.org/feature/screen.width", &screen_size_w);
	if(screen_size_w < screen_size_h)
		max = screen_size_w - 20;
	else
		max = screen_size_h - 20;
	/*evas_object_size_hint_max_set(ad->img, max, max);
	evas_object_size_hint_min_set(ad->img, max, max);
	evas_object_size_hint_weight_set(ad->img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->img, EVAS_HINT_FILL, 0.5);
	elm_table_pack(table, ad->img, 0,0,4,4);
	evas_object_show(ad->img);*/

	ad->map.mapService = elm_map_add(ad->table);
	elm_map_zoom_mode_set(ad->map.mapService, ELM_MAP_ZOOM_MODE_MANUAL);
	elm_map_zoom_set(ad->map.mapService, 12);

	evas_object_size_hint_max_set(ad->map.mapService, max, max);
	evas_object_size_hint_min_set(ad->map.mapService, max, max);
	evas_object_size_hint_weight_set(ad->map.mapService, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->map.mapService, EVAS_HINT_FILL, 0.5);
	elm_table_pack(ad->table, ad->map.mapService, 0,0,4,4);
	evas_object_show(ad->map.mapService);

	elm_map_region_show(ad->map.mapService, 2.665, 39.576);
	ad->map.scale = elm_map_overlay_scale_add(ad->map.mapService, (max*2)/3, max - 20);

	ad->btn_info = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_info,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_info,-1.0,1.0);
	elm_object_style_set(ad->btn_info, "circle");
	elm_object_text_set(ad->btn_info,"Info");
	evas_object_smart_callback_add(ad->btn_info, "clicked", btn_info_clicked_cb, ad);
	evas_object_color_set(ad->btn_info, 0, 0, 0, 128);
	evas_object_show(ad->btn_info);
	elm_table_pack(ad->table, ad->btn_info,0,0,2,1);

	Evas_Object *ic;
	char bt_img[PATH_MAX];
	app_get_resource("info.png", bt_img, (int)PATH_MAX);
	ic = elm_icon_add(ad->btn_info);
	elm_image_file_set(ic,bt_img,NULL);
	elm_object_part_content_set(ad->btn_info,"icon",ic);
	evas_object_show(ic);

	ad->btn_open = elm_button_add(ad->conform);
	evas_object_size_hint_weight_set(ad->btn_open,0.0,1.0);
	evas_object_size_hint_align_set(ad->btn_open,-1.0,1.0);
	elm_object_style_set(ad->btn_open, "circle");
	elm_object_text_set(ad->btn_open,"Open");
	evas_object_smart_callback_add(ad->btn_open, "clicked", btn_open_clicked_cb, ad);
	evas_object_color_set(ad->btn_open, 0, 0, 0, 128);
	evas_object_show(ad->btn_open);
	elm_table_pack(ad->table, ad->btn_open,2,0,2,1);

	Evas_Object *ic_open;
	app_get_resource("open.png", bt_img, (int)PATH_MAX);
	ic_open = elm_icon_add(ad->btn_open);
	elm_image_file_set(ic_open,bt_img,NULL);
	elm_object_part_content_set(ad->btn_open,"icon",ic_open);
	evas_object_show(ic_open);

	ad->popup_info = elm_popup_add(ad->conform);
	elm_popup_align_set(ad->popup_info, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->popup_info, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(ad->popup_info, INFO);

	Evas_Object *button_popup;
	button_popup = elm_button_add(ad->popup_info);
	elm_object_text_set(button_popup, "OK");
	evas_object_smart_callback_add(button_popup, "clicked", popup_exit_cb, ad->popup_info);
	elm_object_part_content_set(ad->popup_info, "button1", button_popup);

	ad->popup_gps_disabled = elm_popup_add(ad->conform);
	elm_popup_align_set(ad->popup_gps_disabled, ELM_NOTIFY_ALIGN_FILL, 1.0);
	evas_object_size_hint_weight_set(ad->popup_gps_disabled, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(ad->popup_gps_disabled, GPS_DISABLED_INFO);

	Evas_Object *button_gps_disabled_popup;
	button_gps_disabled_popup = elm_button_add(ad->popup_gps_disabled);
	elm_object_text_set(button_gps_disabled_popup, "OK");
	evas_object_smart_callback_add(button_gps_disabled_popup, "clicked", popup_exit_cb, ad->popup_gps_disabled);
	elm_object_part_content_set(ad->popup_gps_disabled, "button1", button_gps_disabled_popup);

	/* Label */
	ad->labelGps = elm_label_add(ad->table);
	elm_object_text_set(ad->labelGps, LABELFORMATSTART "Waiting GPS status" LABELFORMATEND);

	elm_table_pack(ad->table, ad->labelGps,0,4,4,1);
	evas_object_show(ad->labelGps);

	ad->labelCalc = elm_label_add(ad->table);
	elm_object_text_set(ad->labelCalc, LABELFORMATSTART "GPS Tracker" LABELFORMATEND);

	elm_table_pack(ad->table, ad->labelCalc,0,5,4,1);
	evas_object_show(ad->labelCalc);

	ad->labelDist = elm_label_add(ad->table);
	elm_object_text_set(ad->labelDist, LABELFORMATSTART "By Zipotron" LABELFORMATEND);

	elm_table_pack(ad->table, ad->labelDist,0,6,4,1);
	evas_object_show(ad->labelDist);

	ad->sliderInterval = elm_slider_add(ad->table);
	elm_slider_min_max_set(ad->sliderInterval, 1, 100);
	elm_slider_value_set(ad->sliderInterval, ad->interval);
	elm_slider_indicator_show_set(ad->sliderInterval, EINA_TRUE);
	elm_slider_indicator_format_set(ad->sliderInterval, "%1.0f");
	evas_object_smart_callback_add(ad->sliderInterval, "changed", slider_Interval_changed_cb, ad);

	evas_object_size_hint_weight_set(ad->sliderInterval, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->sliderInterval, EVAS_HINT_FILL, 0.5);
	evas_object_color_set(ad->sliderInterval, 0, 200, 0, 255);
	elm_table_pack(ad->table, ad->sliderInterval,0,7,4,1);

	/*ad->sliderZoom = elm_slider_add(table);
	elm_slider_min_max_set(ad->sliderZoom, 1, 14);
	elm_slider_value_set(ad->sliderZoom, ad->visor.zoom);
	elm_slider_indicator_show_set(ad->sliderZoom, EINA_TRUE);
	elm_slider_indicator_format_set(ad->sliderZoom, "%1.0f");
	evas_object_smart_callback_add(ad->sliderZoom, "changed", slider_Zoom_changed_cb, ad);

	evas_object_size_hint_weight_set(ad->sliderZoom, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->sliderZoom, EVAS_HINT_FILL, 0.5);
	evas_object_color_set(ad->sliderZoom, 0, 200, 0, 255);
	elm_table_pack(table, ad->sliderZoom,0,8,4,1);
	evas_object_show(ad->sliderZoom);*/

	/* Button exit*/
	ad->btn_exit = elm_button_add(ad->table);
	elm_object_text_set(ad->btn_exit, "Exit");
	evas_object_smart_callback_add(ad->btn_exit, "clicked", btn_exit_clicked_cb, ad);
	evas_object_size_hint_weight_set(ad->btn_exit, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->btn_exit, EVAS_HINT_FILL, 0.5);

	evas_object_color_set(ad->btn_exit, 200, 0, 0, 255);

	elm_table_pack(ad->table, ad->btn_exit,0,9,2,1);
	evas_object_show(ad->btn_exit);

	/* Button gps on*/
	ad->btn_on = elm_button_add(ad->table);
	elm_object_text_set(ad->btn_on, "GPS on");
	evas_object_smart_callback_add(ad->btn_on, "clicked", btn_gps_on_clicked_cb, ad);
	evas_object_size_hint_weight_set(ad->btn_on, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->btn_on, EVAS_HINT_FILL, 0.5);

	elm_table_pack(ad->table, ad->btn_on,2,9,2,1);
	evas_object_show(ad->btn_on);

	/* Button gps off*/
	ad->btn_off = elm_button_add(ad->table);
	elm_object_text_set(ad->btn_off, "GPS off");
	evas_object_smart_callback_add(ad->btn_off, "clicked", btn_gps_off_clicked_cb, ad);
	evas_object_size_hint_weight_set(ad->btn_off, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->btn_off, EVAS_HINT_FILL, 0.5);

	evas_object_color_set(ad->btn_off, 200, 0, 0, 255);

	elm_table_pack(ad->table, ad->btn_off,0,9,2,1);

	/* Button record*/
	ad->btn_record = elm_button_add(ad->table);
	elm_object_text_set(ad->btn_record, "Record");
	evas_object_smart_callback_add(ad->btn_record, "clicked", btn_record_clicked_cb, ad);
	evas_object_size_hint_weight_set(ad->btn_record, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->btn_record, EVAS_HINT_FILL, 0.5);

	elm_table_pack(ad->table, ad->btn_record,2,9,2,1);

	/* Button stop*/
	ad->btn_stop = elm_button_add(ad->table);
	elm_object_text_set(ad->btn_stop, "Stop");
	evas_object_smart_callback_add(ad->btn_stop, "clicked", btn_stop_clicked_cb, ad);
	evas_object_size_hint_weight_set(ad->btn_stop, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->btn_stop, EVAS_HINT_FILL, 0.5);

	evas_object_color_set(ad->btn_stop, 200, 0, 0, 255);

	elm_table_pack(ad->table, ad->btn_stop,0,9,2,1);

	/* Button point*/
	ad->btn_point = elm_button_add(ad->table);
	elm_object_text_set(ad->btn_point, "Point");
	evas_object_smart_callback_add(ad->btn_point, "clicked", btn_point_clicked_cb, ad);
	evas_object_size_hint_weight_set(ad->btn_point, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->btn_point, EVAS_HINT_FILL, 0.5);

	elm_table_pack(ad->table, ad->btn_point,2,9,2,1);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

	/*Open file windows drawing*/
	ad->open_win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->open_win, EINA_TRUE);
	Evas_Object *open_conform;

	open_conform = elm_conformant_add(ad->open_win);
	elm_win_indicator_mode_set(ad->open_win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->open_win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(open_conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->open_win, open_conform);
	evas_object_show(open_conform);


	Evas_Object *genlist;
	Elm_Genlist_Item_Class *itc;

	genlist = elm_genlist_add(open_conform);
	evas_object_show(genlist);
	elm_object_content_set(open_conform, genlist);

	itc = elm_genlist_item_class_new();
	itc->item_style = "default";
	itc->func.text_get = _item_label_get;
	itc->func.del = _item_del;

	elm_genlist_item_append(genlist, /* Genlist object */
		                            itc, /* Genlist item class */
		                            (void *)"Return main screen", /* Item data */
		                            NULL, /* Parent item */
		                            ELM_GENLIST_ITEM_NONE, /* Item type */
									popup_exit_cb, /* Select callback */
		                            ad->open_win); /* Callback data */

	struct dirent* dent;
	struct stat st;
	item_data_s *element_data;
	if(( stat(DIR_MAIN, &st) != -1 ) && ( stat(DIR_TRK, &st) != -1 )){

		DIR* srcdir = opendir(DIR_TRK);

		if (srcdir != NULL){

			while((dent = readdir(srcdir)) != NULL){

		        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
		            continue;

		        //if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0){
		        element_data = (item_data_s*)malloc(sizeof(item_data_s));
		        element_data->label = (char*)malloc(255*sizeof(char));
		        element_data->ad = ad;
		        if(stat(dent->d_name, &st)){
		        	sprintf(element_data->label,"%s", dent->d_name);
		        	elm_genlist_item_append(genlist,
		        		                            itc,
		        		                            //(void *)dent->d_name,
													(void *)element_data->label,
		        		                            NULL,
		        		                            ELM_GENLIST_ITEM_NONE,
		        		                            open_file,
													element_data);
		        }
		    }
		closedir(srcdir);
		}
	  /*else
	    perror ("Couldn't open the directory");*/
	}
	elm_genlist_item_class_free(itc);
}

static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);

	struct stat buf;
	if( stat(DIR_MAIN, &buf) == -1 ){
		mkdir(DIR_MAIN, 0777);
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
	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
