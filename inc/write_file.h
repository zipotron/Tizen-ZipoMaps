#include <efl_extension.h>

char
*xmlwriterCreateWptDoc(void *data);

char
*xmlwriterCreateTrackDoc(void *data);

char
*xmlwriterWriteTrackDoc(const char *file, void *data);

char
*xmlwriterWriteWptDoc(const char *file, void *data);

char
*xmlwriterAddWpt(double longitude, double latitude, double altitude, void *data);

char
*xmlwriterAddNode(double longitude, double latitude, double altitude, time_t timestamp, void *data);
