#ifndef __zipomaps_H__
#define __zipomaps_H__

#include <app.h>
#include <efl_extension.h>
#include <locations.h>
//#include <download.h>
#include <libxml/xmlwriter.h>

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *labelGps;
	Evas_Object *labelCalc;
	Evas_Object *labelDist;
	Evas_Object *sliderInterval;
	Evas_Object *sliderZoom;
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
		xmlTextWriterPtr volatile writerWpt;
		xmlDocPtr volatile docWpt;
		int writeNextWpt;
	}xml;
	struct{
		double latitude;
		double longitude;
		double altitude;
		int zoom;
	}visor;
	int interval;
	location_manager_h manager;
	//Evas_Object *img;
	Evas_Object *map;
} appdata_s;

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "zipomaps"

#if !defined(PACKAGE)
#define PACKAGE "org.zipotron.zipomaps"
#endif

#endif /* __zipomaps_H__ */
