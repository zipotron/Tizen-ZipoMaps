#ifndef __zipomaps_H__
#define __zipomaps_H__

#include <app.h>
#include <efl_extension.h>
#include <locations.h>
//#include <download.h>
#include <libxml/xmlwriter.h>

typedef struct {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *table;
	Evas_Object *labelGps;
	Evas_Object *labelCalc;
	Evas_Object *labelDist;
	Evas_Object *sliderInterval;
	Evas_Object *btn_on;
	Evas_Object *btn_off;
	Evas_Object *btn_exit;
	Evas_Object *btn_record;
	Evas_Object *btn_stop;
	Evas_Object *btn_point;
	Evas_Object *btn_info;
	Evas_Object *btn_open;
	Evas_Object *btn_pos;
	Evas_Object *popup_info;
	Evas_Object *popup_gps_disabled;
	Evas_Object *open_win;
	Evas_Object *open_genlist;
	//Evas_Object *sliderZoom;
	/*struct{
		download_error_e download;
		download_state_e state;
		int download_id;
	}downloader;*/
	struct{
		double maxAcceleration;
		double distance;
		double maxSpeed;
	}tracker;
	struct{
		xmlTextWriterPtr volatile writerTrk;
		xmlDocPtr volatile docTrk;
		bool trkData;
		xmlTextWriterPtr volatile writerWpt;
		xmlDocPtr volatile docWpt;
		bool wptData;
		int writeNextWpt;
	}xml;
	struct{
		double latitude;
		double longitude;
		double altitude;
		time_t timestamp;
		int go_position;
		int gps_data;
		//int zoom;
	}visor;
	int interval;
	location_manager_h manager;
	//Evas_Object *img;
	struct{
		Evas_Object *mapService;
		Elm_Map_Overlay *scale;
		Elm_Map_Overlay *ovl;
		bool recording;
	}map;
} appdata_s;

typedef struct {
	appdata_s *ad;
	char *label;
}item_data_s;

void stop_gps(appdata_s *ad);

void start_gps(appdata_s *ad);

void
position_updated_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data);

void
position_updated_record_cb(double latitude, double longitude, double altitude, time_t timestamp, void *user_data);

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "zipomaps"

#if !defined(PACKAGE)
#define PACKAGE "org.zipotron.zipomaps"
#endif

#endif /* __zipomaps_H__ */
