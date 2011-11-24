/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011 GSyC/LibreSoft, Universidad Rey Juan Carlos.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <bluetooth/uuid.h>

#include "plugin.h"
#include "hcid.h"
#include "log.h"
#include "gattrib.h"
#include "gatt-service.h"
#include "att.h"
#include "attrib-server.h"

#define HEART_RATE_SVC_UUID		0x180D
#define HEART_RATE_MEASUREMENT		0x2A37
#define BODY_SENSOR_LOCATION		0x2A38
#define HEART_RATE_CTRL_POINT		0x2A39

#define TRANSMISSION_INTERVAL	1	/* seconds */
#define RR_INTERVAL		10	/* seconds */

typedef enum {
	HR_FORMAT_VAL_UINT8 = 0,
	HR_FORMAT_VAL_UINT16
} HRVALFormat;

typedef enum {
	BODY_SENSOR_OTHER = 0,
	BODY_SENSOR_CHEST,
	BODY_SENSOR_WRIST,
	BODY_SENSOR_FINGER,
	BODY_SENSOR_HAND,
	BODY_SENSOR_EAR_LOBE,
	BODY_SENSOR_FOOT
} BodySensorLocation;

static const HRVALFormat hr_value_format = HR_FORMAT_VAL_UINT8;
static const BodySensorLocation bslocation = BODY_SENSOR_CHEST;
static const gboolean energy_expended = TRUE;


static uint16_t hrhandle;
static guint sourceid;
static guint ttcounter = 0;
static uint16_t rrinterval = 0;

const char *body_sensor_location[] = {
	"Other",
	"Chest",
	"Wrist",
	"Finger",
	"Hand",
	"Ear Lobe",
	"Foot"
};

static const gchar *bsl2str(uint8_t value)
{
	 if (value > 0 && value < G_N_ELEMENTS(body_sensor_location))
		return body_sensor_location[value];

	error("Body sensor location %d reserved for future use", value);
	return NULL;
}

static uint8_t get_size()
{
	uint8_t size = 1;

	switch(hr_value_format) {
	case HR_FORMAT_VAL_UINT8:
		size += 1;
		break;

	case HR_FORMAT_VAL_UINT16:
		size += 2;
		break;

	default:
		error("Heart rate value format %d not supported",
							hr_value_format);
		return 0;
	};

	if (energy_expended)
		size += 2;

	ttcounter = (ttcounter + 1) % RR_INTERVAL;
	if (ttcounter == (RR_INTERVAL - 1))
		size += 2;

	return size;
}

static gboolean create_measure(uint8_t **value, uint8_t *size)
{
	uint8_t *val, len, index;

	len = get_size();
	if (len == 0)
		return FALSE;

	val = g_new0(uint8_t, len);
	index = 1;

	switch(hr_value_format) {
	case HR_FORMAT_VAL_UINT8:
		att_put_u8(125, &val[index]);
		index += 1;
		break;

	case HR_FORMAT_VAL_UINT16:
		val[0] |= 0x01;
		att_put_u16(325, &val[index]);
		index += 2;
		break;

	default:
		g_free(val);
		return FALSE;
	};

	if (energy_expended) {
		val[0] |= 0x08;
		att_put_u16(110, &val[index]);
		index += 2;
	}

	if (ttcounter == (RR_INTERVAL - 1)) {
		val[0] |= 0x10;
		att_put_u16(rrinterval++, &val[index]);
	}

	*value = val;
	*size = len;

	return TRUE;
}

static gboolean hr_notification(gpointer data)
{
	uint8_t *value, len;

	if (!create_measure(&value, &len)) {
		error("Can't generate measurement");
		return FALSE;
	}

	attrib_db_update(hrhandle, NULL, value, len, NULL);
	g_free(value);

	return TRUE;
}

static uint8_t body_sensor_location_cb(struct attribute *a, gpointer user_data)
{
	const char *msg = bsl2str(bslocation);
	uint8_t val;

	if (msg == NULL)
		return ATT_ECODE_IO;

	DBG("Read body sensor location 0x%04x value: %s", a->handle, msg);

	val = bslocation;
	attrib_db_update(a->handle, NULL, &val, 1, NULL);

	return 0;
}

static uint8_t hr_ctrl_point_cb(struct attribute *a, gpointer user_data)
{
	DBG("TODO:");
	return 0;
}

static void register_hr_service(void)
{
	gatt_service_add(GATT_PRIM_SVC_UUID, HEART_RATE_SVC_UUID,
			/* heart rate characteristic */
			GATT_OPT_CHR_UUID, HEART_RATE_MEASUREMENT,
			GATT_OPT_CHR_PROPS, ATT_CHAR_PROPER_NOTIFY,
			GATT_OPT_CHR_VALUE_GET_HANDLE, &hrhandle,

			/* body sensor location characteristic */
			GATT_OPT_CHR_UUID, BODY_SENSOR_LOCATION,
			GATT_OPT_CHR_PROPS, ATT_CHAR_PROPER_READ,
			GATT_OPT_CHR_VALUE_CB, ATTRIB_READ,
							body_sensor_location_cb,

			/* heart rate control point characteristic */
			GATT_OPT_CHR_UUID, HEART_RATE_CTRL_POINT,
			GATT_OPT_CHR_PROPS, ATT_CHAR_PROPER_WRITE,
			GATT_OPT_CHR_VALUE_CB, ATTRIB_WRITE,
							hr_ctrl_point_cb,

			GATT_OPT_INVALID);

	sourceid = g_timeout_add_seconds(TRANSMISSION_INTERVAL, hr_notification,
									NULL);
}

static int heartrate_init(void)
{
	DBG("Heartrate plugin init");
	register_hr_service();
	return 0;
}

static void heartrate_exit(void)
{
	DBG("Heartrate exit");
	g_source_remove(sourceid);
}

BLUETOOTH_PLUGIN_DEFINE(heartrate_svc, VERSION, BLUETOOTH_PLUGIN_PRIORITY_LOW,
						heartrate_init, heartrate_exit)