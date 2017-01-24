#include "button_bar.h"
#include "zipomaps.h"
#include "xmlfunctions.h"
#include "config.h"

void
btn_exit_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	char *result;
	//location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
	result = xmlwriterWriteTrackDoc(DIR_TRK FILETRACK, ad);
	elm_object_text_set(ad->labelGps, result);
	free(result);
	if(ad->xml.docWpt){
		result = xmlwriterWriteWptDoc(DIR_TRK FILEWPT, ad);
		free(result);
	}
	ui_app_exit();
}

void
btn_point_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	static int counter = 0;
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
	evas_object_smart_callback_add(obj, "clicked", btn_point_clicked_cb, ad);
	char *result;
	result = xmlwriterCreateTrackDoc(ad);
	/*elm_object_text_set(obj, result);*/
	elm_object_text_set(obj, "Way point");
	free(result);

	location_manager_set_position_updated_cb(ad->manager, position_updated_record_cb, ad->interval, ad);
}
