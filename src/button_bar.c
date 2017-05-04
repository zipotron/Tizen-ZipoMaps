#include "button_bar.h"
#include "zipomaps.h"
#include "config.h"
#include "open_file.h"
#include "write_file.h"
#include <runtime_info.h>
#include <notification.h>

static void open_file_exit_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	evas_object_hide(ad->open_win);
	elm_genlist_clear(ad->open_genlist);
}

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
btn_open_clicked_cb(void *data, Evas_Object *obj, void *event_info){
	appdata_s *ad = data;
	Elm_Genlist_Item_Class *itc;
	itc = elm_genlist_item_class_new();
	itc->item_style = "default";
	itc->func.text_get = open_file_item_label_get;
	itc->func.del = open_file_item_del;

	elm_genlist_item_append(ad->open_genlist, /* Genlist object */
		                            itc, /* Genlist item class */
		                            (void *)"Return main screen", /* Item data */
		                            NULL, /* Parent item */
		                            ELM_GENLIST_ITEM_NONE, /* Item type */
									open_file_exit_cb, /* Select callback */
		                            ad); /* Callback data */

	struct dirent* dent;
	struct stat st;
	item_data_s *element_data;
	if(( stat(DIR_MAIN, &st) != -1 ) && ( stat(DIR_TRK, &st) != -1 )){

		DIR* srcdir = opendir(DIR_TRK);

		if (srcdir != NULL){

			while((dent = readdir(srcdir)) != NULL){

		        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
		            continue;

		        element_data = (item_data_s*)malloc(sizeof(item_data_s));
		        element_data->label = (char*)malloc(255*sizeof(char));
		        element_data->ad = ad;
		        if(stat(dent->d_name, &st)){
		        	sprintf(element_data->label,"%s", dent->d_name);
		        	elm_genlist_item_append(ad->open_genlist,
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

	evas_object_show(ad->open_win);
}

void
btn_point_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	bool gps_service_on = false;

	runtime_info_get_value_bool(RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, &gps_service_on);

	if(gps_service_on){
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
	} else {
		evas_object_show(ad->popup_gps_disabled);
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
		evas_object_hide(ad->btn_open);
		evas_object_show(ad->btn_stop);
		char *result;
		result = xmlwriterCreateTrackDoc(ad);
		/*elm_object_text_set(obj, result);*/
		free(result);

		ad->map.recording = true;
		location_manager_set_position_updated_cb(ad->manager, position_updated_record_cb, ad->interval, ad);
	} else {
		evas_object_show(ad->popup_gps_disabled);
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
		evas_object_hide(ad->btn_exit);
		evas_object_show(ad->btn_off);
		evas_object_hide(ad->btn_on);
		evas_object_show(ad->btn_record);
		evas_object_hide(ad->btn_info);
		location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
		runtime_info_set_changed_cb(RUNTIME_INFO_KEY_LOCATION_SERVICE_ENABLED, gps_settings_changed_cb, ad);
	} else {
		evas_object_show(ad->popup_gps_disabled);
	}
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
	evas_object_show(ad->btn_open);
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
	ad->xml.writeNextWpt = -1;
}

void popup_exit_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *popup = data;
	evas_object_hide(popup);
}
