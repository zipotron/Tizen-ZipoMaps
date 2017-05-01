#include "calcfunctions.h"
#include "zipomaps.h"

/*int long2tilex(double lon, int z)
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
}*/

double
distance(double latA, double lonA, double altA, double latB, double lonB, double altB)
{
	latA *=M_PI/180;
	latB *=M_PI/180;
	lonA*=M_PI/180;
	lonB*=M_PI/180;

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
			ad->tracker.distance += d;
		}
		if(abs(timestamp - oldTimestamp) > 0){
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
