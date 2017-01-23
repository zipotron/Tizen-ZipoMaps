#include <Elementary.h>

/*int long2tilex(double lon, int z);

int lat2tiley(double lat, int z);

double tilex2long(int x, int z);

double tiley2lat(int y, int z);*/

double
distance(double latA, double lonA, double altA, double latB, double lonB, double altB);

double
speedAndDistance(double latitude, double longitude, double altitude, time_t timestamp, double *maxSpeed, double acceleration, void *data);
