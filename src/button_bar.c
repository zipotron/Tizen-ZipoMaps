#include "button_bar.h"
#include "zipomaps.h"
#include "visor_online.h"
#include "write_file.h"
#include "config.h"
#include <runtime_info.h>
#include <notification.h>

void gps_settings_changed_cb(runtime_info_key_e key, void *data){
	appdata_s *ad = data;
	bool gps_service_on = false;

	runtime_info_get_value_bool(key, &gps_service_on);
	if(!gps_service_on){
		notification_status_message_post("Zipomaps is recording and GPS became disabled, please enable it for continue track.");
		elm_object_text_set(ad->labelGps, "<+backing=on backing_color=#F33737 color=#000000><align=center>GPS is disabled!</align><br/>");
	} else {
		elm_object_text_set(ad->labelGps, "<+backing=on backing_color=#F7D358 color=#000000><align=center>Waiting GPS status</align><br/>");
	}
}

void
btn_clean_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{

}

void
btn_point_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
		bool gps_service_on = false;

		runtime_info_get_value_bool(RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, &gps_service_on);

		if(gps_service_on){
			static int counter = 0;

			if(ad->xml.writeNextWpt == -2){
				counter = 0;
				ad->xml.writeNextWpt = -1;
			}

			if(ad->xml.writeNextWpt == -1){
				char *result;
				if(!counter){
					result = xmlwriterCreateWptDoc(ad);
					free(result);
				}
				counter++;
				ad->xml.writeNextWpt = counter;
				if(ad->visor.timestamp){
					result = xmlwriterAddWpt(ad->visor.latitude, ad->visor.longitude, ad->visor.altitude, ad);
					show_wpt_mark(ad->visor.longitude, ad->visor.latitude, ad);
					ad->visor.gps_data = 1;
					free(result);
				}
			}
		} else {
			notification_status_message_post("GPS is disabled.");
	}
}

void
btn_record_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	bool gps_service_on = false;

	runtime_info_get_value_bool(RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, &gps_service_on);

	if(gps_service_on){
		evas_object_hide(ad->sliderInterval);
		evas_object_hide(ad->btn_record);
		evas_object_show(ad->btn_point);
		evas_object_hide(ad->btn_off);
		evas_object_show(ad->btn_stop);
		//evas_object_show(ad->labelGps);
		char *result;
		result = xmlwriterCreateTrackDoc(ad);
		/*elm_object_text_set(obj, result);*/
		free(result);

		ad->map.recording = true;
		location_manager_set_position_updated_cb(ad->manager, position_updated_record_cb, ad->interval, ad);
	} else {
		notification_status_message_post("GPS is disabled.");
	}
}

void
btn_gps_on_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	bool gps_service_on = false;

	runtime_info_get_value_bool(RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, &gps_service_on);

	if(gps_service_on){
		start_gps(ad);
		evas_object_show(ad->sliderInterval);
		evas_object_show(ad->btn_off);
		evas_object_hide(ad->btn_on);
		evas_object_hide(ad->btn_clean);
		evas_object_show(ad->btn_record);
		evas_object_show(ad->btn_zoom_in);
		evas_object_show(ad->btn_zoom_out);
		location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
		runtime_info_set_changed_cb(RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, gps_settings_changed_cb, ad);
	} else {
		notification_status_message_post("GPS is disabled.");
	}
}

void
btn_gps_off_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	evas_object_hide(ad->sliderInterval);
	evas_object_hide(ad->btn_off);
	evas_object_hide(ad->btn_record);
	evas_object_hide(ad->btn_zoom_in);
	evas_object_hide(ad->btn_zoom_out);
	evas_object_show(ad->btn_on);
	evas_object_show(ad->btn_clean);
	stop_gps(ad);
}

void
btn_stop_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	evas_object_show(ad->sliderInterval);
	evas_object_hide(ad->btn_stop);
	evas_object_show(ad->btn_off);
	evas_object_hide(ad->btn_point);
	evas_object_show(ad->btn_record);
	//evas_object_hide(ad->labelGps);
	char *result;

	location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
	ad->map.recording = false;

	//struct stat buf;

	char timestring[22];
	strftime(timestring, 21, "_%Y-%m-%d-%H:%M:%S", localtime(&(ad->visor.timestamp)));

	char bufd[128];
	sprintf(bufd, "%s%s%s%s", DIR_TRK, FILETRACK, timestring, FILE_EXT);
	if(ad->xml.docTrk){
		result = xmlwriterWriteTrackDoc(bufd, ad);
		elm_object_text_set(ad->labelGps, result);
		free(result);
	}
	/*}else{
		elm_object_text_set(ad->labelGps, "Error in system time");
	}*/

	sprintf(bufd, "%s%s%s%s", DIR_TRK, FILEWPT, timestring, FILE_EXT);
	if(ad->xml.docWpt){
		result = xmlwriterWriteWptDoc(bufd, ad);
		elm_object_text_set(ad->labelGps, result);
		free(result);
	}
	/*}else{
		elm_object_text_set(ad->labelGps, "Error in system time");
	}*/
	ad->xml.trkData = false;
	ad->xml.wptData = false;
	ad->xml.writeNextWpt = -2;
	ad->visor.gps_data = 0;
}

void
btn_zoom_in_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	ad->visor.zoom++;
	elm_map_zoom_set(ad->map.mapService, ad->visor.zoom);
}

void
btn_zoom_out_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	ad->visor.zoom--;
	elm_map_zoom_set(ad->map.mapService, ad->visor.zoom);
}
