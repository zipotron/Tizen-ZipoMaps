#include "xmlfunctions.h"
#include "zipomaps.h"

char
*xmlwriterCreateWptDoc(void *data)
{
    int rc;
    appdata_s *ad = data;
    char *buf =(char *) malloc(100 * sizeof(char));

    sprintf(buf,"OK\n");
    /* Create a new XmlWriter for DOM, with no compression. */
    ad->xml.writerWpt = xmlNewTextWriterDoc((xmlDocPtr*)&(ad->xml.docWpt), 0);
    if (ad->xml.writerWpt == NULL) {
        sprintf(buf,"testXmlwriterDoc: Error creating the xml writer\n");
        return buf;
    }

    rc = xmlTextWriterStartDocument(ad->xml.writerWpt, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartDocument\n");
        return buf;
    }

    rc = xmlTextWriterStartElement(ad->xml.writerWpt, BAD_CAST "gpx");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteAttribute(ad->xml.writerWpt, BAD_CAST "version",
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
    ad->xml.writerTrk = xmlNewTextWriterDoc((xmlDocPtr*)&(ad->xml.docTrk), 0);
    if (ad->xml.writerTrk == NULL) {
        sprintf(buf,"testXmlwriterDoc: Error creating the xml writer\n");
        return buf;
    }

    rc = xmlTextWriterStartDocument(ad->xml.writerTrk, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartDocument\n");
        return buf;
    }

    rc = xmlTextWriterStartElement(ad->xml.writerTrk, BAD_CAST "gpx");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteAttribute(ad->xml.writerTrk, BAD_CAST "version",
                                         BAD_CAST "1.0");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

    rc = xmlTextWriterStartElement(ad->xml.writerTrk, BAD_CAST "trk");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteFormatElement(ad->xml.writerTrk, BAD_CAST "name",
                                             "%s", "Example");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

    rc = xmlTextWriterWriteFormatElement(ad->xml.writerTrk, BAD_CAST "number",
                                                     "%d", 1);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");


    rc = xmlTextWriterStartElement(ad->xml.writerTrk, BAD_CAST "trkseg");
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

	if(ad->visor.latitude != 500){
		rc = xmlTextWriterEndElement(ad->xml.writerTrk);
		if (rc < 0)
			sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement3\n");

		rc = xmlTextWriterEndElement(ad->xml.writerTrk);
		if (rc < 0)
			sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement2\n");

		rc = xmlTextWriterEndElement(ad->xml.writerTrk);
		if (rc < 0)
			sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement1\n");

		rc = xmlTextWriterEndDocument(ad->xml.writerTrk);
		if (rc < 0)
			sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndDocument\n");

		xmlSaveFileEnc(file, ad->xml.docTrk, MY_ENCODING);
	}

	xmlFreeTextWriter(ad->xml.writerTrk);

	ad->xml.writerTrk = NULL;

	xmlFreeDoc(ad->xml.docTrk);

	ad->xml.docTrk = NULL;

	return buf;
}

char
*xmlwriterWriteWptDoc(const char *file, void *data)
{
	int rc;
	appdata_s *ad = data;
	char *buf =(char *) malloc(100 * sizeof(char));
	sprintf(buf,"OK\n");

	if(ad->visor.latitude != 500){
		rc = xmlTextWriterEndDocument(ad->xml.writerWpt);
		if (rc < 0)
			sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndDocument\n");

		xmlSaveFileEnc(file, ad->xml.docWpt, MY_ENCODING);
	}

	xmlFreeTextWriter(ad->xml.writerWpt);

	ad->xml.writerWpt = NULL;

	xmlFreeDoc(ad->xml.docWpt);

	ad->xml.docWpt = NULL;

	return buf;
}

char
*xmlwriterAddWpt(double latitude, double longitude, double altitude, void *data)
{
	int rc;
	appdata_s *ad = data;
	char *buf =(char *) malloc(100 * sizeof(char));
	sprintf(buf,"OK\n");

	rc = xmlTextWriterStartElement(ad->xml.writerWpt, BAD_CAST "wpt");
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

	rc = xmlTextWriterWriteFormatAttribute(ad->xml.writerWpt, BAD_CAST "lat",
	                                             "%f", latitude);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

	rc = xmlTextWriterWriteFormatAttribute(ad->xml.writerWpt, BAD_CAST "lon",
	                                                 "%f", longitude);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

	rc = xmlTextWriterWriteFormatElement(ad->xml.writerWpt, BAD_CAST "ele",
	                                             "%f", altitude);
	if (rc < 0)
	    sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

	rc = xmlTextWriterWriteFormatElement(ad->xml.writerWpt, BAD_CAST "name",
		                                             "%d", ad->xml.writeNextWpt);
	if (rc < 0)
		sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

	rc = xmlTextWriterEndElement(ad->xml.writerWpt);
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
	char tbuf[22];
	sprintf(buf,"OK\n");

    /* Start track */
    rc = xmlTextWriterStartElement(ad->xml.writerTrk, BAD_CAST "trkpt");
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterStartElement\n");

    rc = xmlTextWriterWriteFormatAttribute(ad->xml.writerTrk, BAD_CAST "lat",
                                             "%f", latitude);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

    rc = xmlTextWriterWriteFormatAttribute(ad->xml.writerTrk, BAD_CAST "lon",
                                                 "%f", longitude);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteAttribute\n");

    rc = xmlTextWriterWriteFormatElement(ad->xml.writerTrk, BAD_CAST "ele",
                                             "%f", altitude);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

    strftime(tbuf, 21, "%Y-%m-%dT%H:%M:%SZ", localtime(&timestamp));
    rc = xmlTextWriterWriteFormatElement(ad->xml.writerTrk, BAD_CAST "time",
                                                 "%s", tbuf);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterWriteFormatElement\n");

    rc = xmlTextWriterEndElement(ad->xml.writerTrk);
    if (rc < 0)
        sprintf(buf,"testXmlwriterDoc: Error at xmlTextWriterEndElement4\n");

    /* End of track*/

    return buf;
}
