/* Author: Carlos Dominguez */

#include "zipomaps.h"
#include <locations.h>
#include <download.h>
#include <cairo.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <system_info.h>

#define APIKEY "f23adf67ad974aa38a80c8a94b114e44"
#define FILETRACK "/opt/usr/media/track.gpx"
#define FILEWPT "/opt/usr/media/wpt.gpx"
#define MY_ENCODING "UTF-8"
/*
 * https://{s}.tile.thunderforest.com/cycle/{z}/{x}/{y}.png?apikey=f23adf67ad974aa38a80c8a94b114e44
 */
static const double PI = 3.14159265;

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *labelGps;
	Evas_Object *labelCalc;
	Evas_Object *labelDist;
	Evas_Object *slider;
	download_error_e download;
	download_state_e state;
	int download_id;
	int interval;
	double maxAcceleration;
	int writeNextWpt;
	double distance;
	double maxSpeed;
	xmlTextWriterPtr volatile writerTrk;
	xmlDocPtr volatile docTrk;
	xmlTextWriterPtr volatile writerWpt;
	xmlDocPtr volatile docWpt;
	location_manager_h manager;
	Evas_Object *img;
	cairo_surface_t *surface;
	cairo_t *cairo;
} appdata_s;

int long2tilex(double lon, int z)
{
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int lat2tiley(double lat, int z)
{
	return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

double tilex2long(int x, int z)
{
	return x / pow(2.0, z) * 360.0 - 180;
}

double tiley2lat(int y, int z)
{
	double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
	return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

double
distance(double latA, double lonA, double altA, double latB, double lonB, double altB)
{
	latA *=PI/180;
	latB *=PI/180;
	lonA*=PI/180;
	lonB*=PI/180;

	double dlong = (lonB - lonA);
	double dlat  = (latB - latA);

	/* Haversine formula: */
	double R = 6372797;
	double a = sin(dlat/2)*sin(dlat/2) + cos(latA)*cos(latB)*sin(dlong/2)*sin(dlong/2);
	double c = 2 * atan2( sqrt(a), sqrt(1-a) );
	return sqrt((R * c) * (R * c) + (altB-altA) *(altB-altA));
}
double
speedAndDistance(double latitude, double longitude, double altitude, time_t timestamp, double *maxSpeed, double acceleration, void *data)
{
	static double oldLatitude = 0;
	static double oldLongitude = 0;
	static double oldAltitude = 0;
	static time_t oldTimestamp = 0;
	static double s = 0;
	double stemp;
	int d;

	if(oldTimestamp){
		d = distance(oldLatitude, oldLongitude, oldAltitude, latitude, longitude, altitude);
		if(data){
			appdata_s *ad = data;
			ad->distance += d;
		}
		if((timestamp - oldTimestamp) > 0){
			stemp = d / (timestamp - oldTimestamp);
			if(fabs(s - stemp) < acceleration / (timestamp - oldTimestamp)){
				if(stemp > *maxSpeed) *maxSpeed = stemp;
				s = stemp;
			}
		}
	}
	oldLatitude = latitude;
	oldLongitude = longitude;
	oldAltitude = altitude;
	oldTimestamp = timestamp;
	return s;
}

char
*xmlwriterCreateWptDoc(void *data)
{
    int rc;
    appdata_s *ad = data;
    char *buf =(char *) malloc(100 * sizeof(char));

    sprintf(buf,"OK\n");
    /* Create a new XmlWriter for DOM, with no compression. */
    ad->writerWpt = xmlNewTextWriterDoc((xmlDocPtr*)&(ad->docWpt), 0);
    if (ad->writerWpt == NULL) {
        sprintf(buf,"testXmlwriterDoc: Error creating the xml writer\n");
        return buf;
    }

    rc = xmlTextWriterStartDocument(ad->writerWpt, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartDocument\n");
        return buf;
    }

    rc = xmlTextWriterStartElement(ad->writerWpt, BAD_CAST "gpx");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteAttribute(ad->writerWpt, BAD_CAST "version",
                                         BAD_CAST "1.0");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");
    return buf;
}

char
*xmlwriterCreateTrackDoc(void *data)
{
    int rc;
    appdata_s *ad = data;
    char *buf =(char *) malloc(100 * sizeof(char));

    sprintf(buf,"OK\n");
    /* Create a new XmlWriter for DOM, with no compression. */
    ad->writerTrk = xmlNewTextWriterDoc((xmlDocPtr*)&(ad->docTrk), 0);
    if (ad->writerTrk == NULL) {
        sprintf(buf,"testXmlwriterDoc: Error creating the xml writer\n");
        return buf;
    }

    rc = xmlTextWriterStartDocument(ad->writerTrk, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartDocument\n");
        return buf;
    }

    rc = xmlTextWriterStartElement(ad->writerTrk, BAD_CAST "gpx");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteAttribute(ad->writerTrk, BAD_CAST "version",
                                         BAD_CAST "1.0");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

    rc = xmlTextWriterStartElement(ad->writerTrk, BAD_CAST "trk");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteFormatElement(ad->writerTrk, BAD_CAST "name",
                                             "%s", "Example");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

    rc = xmlTextWriterWriteFormatElement(ad->writerTrk, BAD_CAST "number",
                                                     "%d", 1);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");


    rc = xmlTextWriterStartElement(ad->writerTrk, BAD_CAST "trkseg");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    return buf;
}
char
*xmlwriterWriteTrackDoc(const char *file, void *data)
{
	int rc;
	appdata_s *ad = data;
	char *buf =(char *) malloc(100 * sizeof(char));
	sprintf(buf,"OK\n");

	rc = xmlTextWriterEndElement(ad->writerTrk);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement3\n");

	rc = xmlTextWriterEndElement(ad->writerTrk);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement2\n");

	rc = xmlTextWriterEndElement(ad->writerTrk);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement1\n");

	rc = xmlTextWriterEndDocument(ad->writerTrk);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndDocument\n");

	xmlFreeTextWriter(ad->writerTrk);

	xmlSaveFileEnc(file, ad->docTrk, MY_ENCODING);

	xmlFreeDoc(ad->docTrk);

	return buf;
}

char
*xmlwriterWriteWptDoc(const char *file, void *data)
{
	int rc;
	appdata_s *ad = data;
	char *buf =(char *) malloc(100 * sizeof(char));
	sprintf(buf,"OK\n");

	rc = xmlTextWriterEndDocument(ad->writerWpt);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndDocument\n");

	xmlFreeTextWriter(ad->writerWpt);

	xmlSaveFileEnc(file, ad->docWpt, MY_ENCODING);

	xmlFreeDoc(ad->docWpt);

	return buf;
}

char
*xmlwriterAddWpt(double latitude, double longitude, double altitude, void *data)
{
	int rc;
	appdata_s *ad = data;
	char *buf =(char *) malloc(100 * sizeof(char));
	sprintf(buf,"OK\n");

	rc = xmlTextWriterStartElement(ad->writerWpt, BAD_CAST "wpt");
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

	rc = xmlTextWriterWriteFormatAttribute(ad->writerWpt, BAD_CAST "lat",
	                                             "%f", latitude);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

	rc = xmlTextWriterWriteFormatAttribute(ad->writerWpt, BAD_CAST "lon",
	                                                 "%f", longitude);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

	rc = xmlTextWriterWriteFormatElement(ad->writerWpt, BAD_CAST "ele",
	                                             "%f", altitude);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

	rc = xmlTextWriterWriteFormatElement(ad->writerWpt, BAD_CAST "name",
		                                             "%d", ad->writeNextWpt);
	if (rc < 0)
		sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

	rc = xmlTextWriterEndElement(ad->writerWpt);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement4\n");

	return buf;
}

char
*xmlwriterAddNode(double latitude, double longitude, double altitude, time_t timestamp, void *data)
{
	int rc;
	appdata_s *ad = data;
	char *buf =(char *) malloc(100 * sizeof(char));
	char *tbuf =(char *) calloc(22, sizeof(char));
	sprintf(buf,"OK\n");

    /* Start track */
    rc = xmlTextWriterStartElement(ad->writerTrk, BAD_CAST "trkpt");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteFormatAttribute(ad->writerTrk, BAD_CAST "lat",
                                             "%f", latitude);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

    rc = xmlTextWriterWriteFormatAttribute(ad->writerTrk, BAD_CAST "lon",
                                                 "%f", longitude);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

    rc = xmlTextWriterWriteFormatElement(ad->writerTrk, BAD_CAST "ele",
                                             "%f", altitude);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

    strftime(tbuf, 21, "%Y-%m-%dT%H:%M:%SZ", localtime(&timestamp));
    rc = xmlTextWriterWriteFormatElement(ad->writerTrk, BAD_CAST "time",
                                                 "%s", tbuf);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

    rc = xmlTextWriterEndElement(ad->writerTrk);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement4\n");

    /* End of track*/
    free(tbuf);

    return buf;
}

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	cairo_surface_destroy(ad->surface);
	cairo_destroy(ad->cairo);
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

static void
show_state(appdata_s *ad)
{
	dlog_print(DLOG_INFO, "tag", "%s", __func__);
    location_manager_create(LOCATIONS_METHOD_GPS, &ad->manager);
    location_manager_set_service_state_changed_cb(ad->manager, state_changed_cb, ad);
    location_manager_start(ad->manager);
}

static void
position_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
    appdata_s *ad = user_data;
    char buf[100];
    sprintf(buf, "Pos:%0.5f/%0.5f Alt:%0.1f", latitude, longitude, altitude);
    elm_object_text_set(ad->labelGps, buf);

    sprintf(buf, "Speed:%0.1f m/s - Max:%0.1f m/s", speedAndDistance(latitude, longitude, altitude, timestamp, &(ad->maxSpeed), ad->maxAcceleration,NULL), ad->maxSpeed);
    elm_object_text_set(ad->labelCalc, buf);
}

static void
position_updated_record_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
    appdata_s *ad = user_data;
    char *result;
    char buf[100];

    result = xmlwriterAddNode(latitude, longitude, altitude, timestamp, ad);
    sprintf(buf, "Pos:%0.5f/%0.5f Alt:%0.1f - %s", latitude, longitude, altitude, result);

    if(ad->writeNextWpt){
    	free(result);
    	result = xmlwriterAddWpt(latitude, longitude, altitude, ad);
    	ad->writeNextWpt = 0;
    }

    elm_object_text_set(ad->labelGps, buf);
    sprintf(buf, "Speed:%0.1f m/s - Max:%0.1f m/s", speedAndDistance(latitude, longitude, altitude, timestamp, &(ad->maxSpeed), ad->maxAcceleration, ad), ad->maxSpeed);
    elm_object_text_set(ad->labelCalc, buf);
    sprintf(buf, "Total Distance:%0.1f", ad->distance);
    elm_object_text_set(ad->labelDist, buf);
    free(result);

    ad->download = download_get_state(ad->download_id, &(ad->state));
    if (ad->state == DOWNLOAD_STATE_COMPLETED){
    	ad->state = 0;

    	evas_object_image_file_set(ad->img, "/opt/usr/media/map.png", NULL);
    }
}

static void
slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    appdata_s *ad = data;

    ad->interval = elm_slider_value_get(obj);
    location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
}

static void
btn_exit_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	char *result;
	location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
	result = xmlwriterWriteTrackDoc(FILETRACK, ad);
	elm_object_text_set(ad->labelGps, result);
	free(result);
	if(ad->docWpt){
		result = xmlwriterWriteWptDoc(FILEWPT, ad);
		free(result);
	}
	ui_app_exit();
}

static void
btn_point_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	static int counter = 0;
	if(!ad->writeNextWpt){
		char *result;

		if(counter == 0){
			result = xmlwriterCreateWptDoc(ad);
			free(result);
		}
		counter++;
		ad->writeNextWpt = counter;
	}
}

static void
btn_record_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	evas_object_hide(ad->slider);
	evas_object_smart_callback_add(obj, "clicked", btn_point_clicked_cb, ad);
	char *result;
	result = xmlwriterCreateTrackDoc(ad);
	/*elm_object_text_set(obj, result);*/
	elm_object_text_set(obj, "Way point");
	free(result);
	//Temporal trick
	ad->state = 0;
	ad->download = download_create(&(ad->download_id));
	ad->download = download_set_url(ad->download_id, "https://tile.thunderforest.com/cycle/6/20/20.png?apikey=f23adf67ad974aa38a80c8a94b114e44");
	ad->download = download_set_destination(ad->download_id, "/opt/usr/media/");
	ad->download = download_set_file_name(ad->download_id, "map.png");
	//ad->download = download_set_auto_download(download_id, true);
	ad->download = download_start(ad->download_id);
	//Temporal trick end
	location_manager_set_position_updated_cb(ad->manager, position_updated_record_cb, ad->interval, ad);
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	/* Create and initialize elm_win.
	   elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	ad->interval = 5;
	ad->writeNextWpt = 0;
	ad->docWpt = NULL;
	ad->maxSpeed = 0;
	ad->distance = 0;
	ad->maxAcceleration = 15;

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	/* Create and initialize elm_conformant.
	   elm_conformant is mandatory for base gui to have proper size
	   when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	Evas_Object *table, *btn_exit, *btn_record;//, *btn_config;
	table = elm_table_add(ad->win);
	//elm_table_padding_set(table, 100, 0);

	elm_object_content_set(ad->conform, table);
	evas_object_show(table);

	/* Image */
	Evas *canvas = evas_object_evas_get(ad->win);
	ad->img = evas_object_image_filled_add(canvas);

	int max, screen_size_w, screen_size_h;
	system_info_get_platform_int("tizen.org/feature/screen.height", &screen_size_h);
	system_info_get_platform_int("tizen.org/feature/screen.width", &screen_size_w);
	if(screen_size_w < screen_size_h)
		max = screen_size_w - 20;
	else
		max = screen_size_h - 20;
	evas_object_size_hint_max_set(ad->img, max, max);
	evas_object_size_hint_min_set(ad->img, max, max);
	evas_object_size_hint_weight_set(ad->img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->img, EVAS_HINT_FILL, 0.5);
	elm_table_pack(table, ad->img, 0,0,4,4);
	evas_object_show(ad->img);
	/* Label */
	/* Create an actual view of the base gui.
	   Modify this part to change the view. */
	ad->labelGps = elm_label_add(table);
	elm_object_text_set(ad->labelGps, "<align=center>Waiting GPS status</align>");

	elm_table_pack(table, ad->labelGps,0,4,4,1);
	evas_object_show(ad->labelGps);

	ad->labelCalc = elm_label_add(table);
	elm_object_text_set(ad->labelCalc, "<align=center>GPS Tracker</align>");

	elm_table_pack(table, ad->labelCalc,0,5,4,1);
	evas_object_show(ad->labelCalc);

	ad->labelDist = elm_label_add(table);
	elm_object_text_set(ad->labelDist, "<align=center>By Zipotron</align>");

	elm_table_pack(table, ad->labelDist,0,6,4,1);
	evas_object_show(ad->labelDist);

	ad->slider = elm_slider_add(table);
	elm_slider_min_max_set(ad->slider, 1, 100);
	elm_slider_value_set(ad->slider, ad->interval);
	elm_slider_indicator_show_set(ad->slider, EINA_TRUE);
	elm_slider_indicator_format_set(ad->slider, "%1.0f");
	evas_object_smart_callback_add(ad->slider, "changed", slider_changed_cb, ad);

	evas_object_size_hint_weight_set(ad->slider, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->slider, EVAS_HINT_FILL, 0.5);
	evas_object_color_set(ad->slider, 0, 200, 0, 255);
	elm_table_pack(table, ad->slider,0,7,4,1);
	evas_object_show(ad->slider);

	/* Button exit*/
	btn_exit = elm_button_add(table);
	elm_object_text_set(btn_exit, "Exit");
	evas_object_smart_callback_add(btn_exit, "clicked", btn_exit_clicked_cb, ad);
	evas_object_size_hint_weight_set(btn_exit, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(btn_exit, EVAS_HINT_FILL, 0.5);

	evas_object_color_set(btn_exit, 200, 0, 0, 255);

	elm_table_pack(table, btn_exit,0,8,2,1);
	evas_object_show(btn_exit);

	/* Button start*/
	btn_record = elm_button_add(table);
	elm_object_text_set(btn_record, "Record");
	evas_object_smart_callback_add(btn_record, "clicked", btn_record_clicked_cb, ad);
	evas_object_size_hint_weight_set(btn_record, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(btn_record, EVAS_HINT_FILL, 0.5);

	elm_table_pack(table, btn_record,2,8,2,1);
	evas_object_show(btn_record);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

	show_state(ad);
	location_manager_set_position_updated_cb(ad->manager, position_updated_cb, ad->interval, ad);
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
