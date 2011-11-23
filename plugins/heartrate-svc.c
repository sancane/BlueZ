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

static uint8_t body_sensor_location_cb(struct attribute *a, gpointer user_data)
{
	DBG("TODO:");
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
}

BLUETOOTH_PLUGIN_DEFINE(heartrate_svc, VERSION, BLUETOOTH_PLUGIN_PRIORITY_LOW,
						heartrate_init, heartrate_exit)