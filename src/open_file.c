#include "open_file.h"
#include "zipomaps.h"
#include "config.h"
#include <libxml/parser.h>

void
get_geoptr_extra_info(xmlDoc *doc, xmlNode * node){
	xmlChar *ele;
	xmlChar *name;
	xmlNode *cur = NULL;

	for (cur = node; cur; cur = cur->next) {
		if(cur->xmlChildrenNode){
			if(!xmlStrcmp(cur->name, (const xmlChar *)"ele")){
				ele = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
				//printf("Ele: %s ", ele);
				xmlFree(ele);
			}else if(!xmlStrcmp(cur->name, (const xmlChar *)"name")){
				name = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
				//printf("name: %s ", name);
				xmlFree(name);
			}
		}
	}
}

void
find_trkpt_recursive(xmlDoc *doc, xmlNode * node) {
	xmlNode *cur = NULL;
	xmlChar *lat;
	xmlChar *lon;

	for (cur = node; cur; cur = cur->next) {
        if ((cur->type == XML_ELEMENT_NODE)&&(!xmlStrcmp(cur->name, (const xmlChar *)"trkpt"))) {
            lat = xmlGetProp(cur, "lat");
		    lon = xmlGetProp(cur, "lon");
		    //printf("lat: %s lon: %s\n",lat, lon);
		    get_geoptr_extra_info(doc, cur->xmlChildrenNode);
		    xmlFree(lat);
		    xmlFree(lon);
        }
        find_trkpt_recursive(doc, cur->children);
    }
}


void
parse_gpx_doc(char *docname) {

	xmlDoc *doc;
	xmlNode *cur;

	doc = xmlParseFile(docname);

	if (doc == NULL ) {
		//fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		//fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "gpx")) {
		//fprintf(stderr,"document of the wrong type, root node != story");
		xmlFreeDoc(doc);
		return;
	}

	xmlChar *lat;
	xmlChar *lon;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"wpt"))) {
		    lat = xmlGetProp(cur, "lat");
		    lon = xmlGetProp(cur, "lon");
		    if(cur->xmlChildrenNode)
				get_geoptr_extra_info(doc, cur->xmlChildrenNode);

		    //printf("lat: %s lon: %s\n", lat, lon);
		    xmlFree(lat);
		    xmlFree(lon);

	    }
	    else if ((!xmlStrcmp(cur->name, (const xmlChar *)"trk")))
			find_trkpt_recursive(doc, cur->xmlChildrenNode);

	    cur = cur->next;
	}
	xmlFreeDoc(doc);
}

void open_file(void *data, Evas_Object *obj, void *event_info)
{
	item_data_s *element_data = data;
	char buf[255];
	evas_object_hide(element_data->ad->open_win);
	elm_genlist_clear(element_data->ad->open_genlist);
	sprintf(buf, "%s/%s", DIR_TRK, element_data->label);
	parse_gpx_doc(buf);
	//char *label = element_data->label;
	//free(element_data); Core dump
}

char *open_file_item_label_get(void *data, Evas_Object *obj, const char *part)
{
	char *i = (char *) data;
	if (!strcmp(part, "elm.text"))
		return strdup(i);

	else return NULL;
}

void open_file_item_del(void *data, Evas_Object *obj)
{
   //free((char*) data);
}
