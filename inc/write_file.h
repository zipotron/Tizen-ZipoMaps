#include <efl_extension.h>

#define FILETRACK "/track.gpx"
#define FILEWPT "/wpt.gpx"
#define MY_ENCODING "UTF-8"

char
*xmlwriterCreateWptDoc(void *data);

char
*xmlwriterCreateTrackDoc(void *data);

char
*xmlwriterWriteTrackDoc(const char *file, void *data);

char
*xmlwriterWriteWptDoc(const char *file, void *data);

char
*xmlwriterAddWpt(double latitude, double longitude, double altitude, void *data);

char
*xmlwriterAddNode(double latitude, double longitude, double altitude, time_t timestamp, void *data);