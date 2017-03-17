#include "button_bar.h"
#include "zipomaps.h"
#include "xmlfunctions.h"
#include "config.h"

void
btn_exit_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

void
btn_info_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	evas_object_show(ad->popup_info);
}

void
btn_point_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	static int counter = 0;

	if(ad->xml.writeNextWpt == -1) ad->xml.writeNextWpt = counter = 0;

	if(!ad->xml.writeNextWpt){
		char *result;

		if(counter == 0){
			result = xmlwriterCreateWptDoc(ad);
			free(result);
		}
		counter++;
		ad->xml.writeNextWpt = counter;
	}
}

void
btn_record_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	evas_object_hide(ad->sliderInterval);
	evas_object_hide(ad->btn_record);
	evas_object_show(ad->btn_point);
	evas_object_hide(ad->btn_off);
	evas_object_show(ad->btn_stop);
	char *result;
	result = xmlwriterCreateTrackDoc(ad);
	/*elm_object_text_set(obj, result);*/
	free(result);

	ad->map.recording = true;
	location_manager_set_position_updated_cb(ad->manager, position_updated_record_cb, ad->interval, ad);
}

void
btn_gps_on_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	start_gps(ad);
	evas_object_show(ad->sliderInterval);
	evas_object_hide(ad->btn_exit);
	evas_object_show(ad->btn_off);
	evas_object_hide(ad->btn_on);
	evas_object_show(ad->btn_record);
	evas_object_hide(ad->btn_info);
	location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
}

void
btn_gps_off_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	evas_object_hide(ad->sliderInterval);
	evas_object_hide(ad->btn_off);
	evas_object_show(ad->btn_exit);
	evas_object_hide(ad->btn_record);
	evas_object_show(ad->btn_on);
	evas_object_show(ad->btn_info);
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
	ad->xml.writeNextWpt = -1;
}

void info_popup_exit_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	evas_object_hide(ad->popup_info);
}
