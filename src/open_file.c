#include "open_file.h"
#include "zipomaps.h"

void open_file(void *data, Evas_Object *obj, void *event_info)
{
	item_data_s *element_data = data;
	evas_object_hide(element_data->ad->open_win);
	elm_genlist_clear(element_data->ad->open_genlist);
	char *label = element_data->label;
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
