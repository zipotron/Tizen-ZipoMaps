/*#include <download.h>
#include "visor.h"
#include "calcfunctions.h"
#include "config.h"
#include "zipomaps.h"

typedef struct {
	double latitude;
	double longitude;
	double altitude;
	int zoom;
	Evas_Object *img;
}_visor;*/

/*struct{
	download_error_e download;
	download_state_e state;
	int download_id;
}downloader;*/

/*_visor *visor;

void setStuctPtr(void *v){
	visor =(_visor*) v;
}*/

/*void downloadFile(){
	//Temporal trick
	char apikey[] = APIKEY;
	    	char bufLink[512];
	    	sprintf(bufLink, "https://tile.thunderforest.com/cycle/%d/%d/%d.png?apikey=%s", visor->zoom, long2tilex(visor->longitude, visor->zoom), lat2tiley(visor->latitude, visor->zoom), apikey);
	    	downloader.state = 0;
	    	downloader.download = download_create(&(downloader.download_id));
	    	//ad->downloader.download = download_set_url(ad->download_id, "https://tile.thunderforest.com/cycle/6/20/20.png?apikey=f23adf67ad974aa38a80c8a94b114e44");
	    	downloader.download = download_set_url(downloader.download_id, bufLink);
	    	char bufd[128];
	    	sprintf(bufd, DIR_MAPS"/%d",visor->zoom);
	    	downloader.download = download_set_destination(downloader.download_id, bufd);
	    	downloader.download = download_set_file_name(downloader.download_id, "map.png");
	    	//downloader.download = download_set_auto_download(downloader.download_id, true);
	    	downloader.download = download_start(downloader.download_id);
	//Temporal trick end
}
void setView(void *user_data){//temporal trick
	appdata_s *ad = user_data;
	    	downloader.download = download_get_state(downloader.download_id, &(downloader.state));
	    	    if (downloader.state == DOWNLOAD_STATE_COMPLETED){
	    	    	downloader.state = 0;

	    	    	char bufd[12];
	    	    	sprintf(bufd, DIR_MAPS"/%d/map.png",visor->zoom);
	    	    	//evas_object_image_file_set(ad->visor.img, bufd, NULL);
	    	    }
}*/
