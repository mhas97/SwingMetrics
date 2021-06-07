#include <sensor.h>
#include <swingmetrics.h>

/**
 * This class accesses accelerometer and gyroscope sensory data on a Tizen smartwatch for use in a machine learning model.
 *
 * The following Github repository uploaded by Evgeny Roskach highlights the methodology used:
 * https://github.com/genyrosk
 */
typedef struct appdata {
	/* GUI Objects. */
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *button;

	/* Sensor objects and listeners. */
	sensor_h accelerometer;
	sensor_listener_h accelerometer_listener;
	sensor_h gyroscope;
	sensor_listener_h gyroscope_listener;

	/* Data storage and counts for both accelerometer and gyroscope. */
	float timer;
	float swing_data[64000][7];
	int accel_count;
	int gyro_count;
} appdata_s;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

/**
 * Open a file-stream and use the swing_data array to format a CSV file.
 */
static void
save_data(void* data)
{
	/* File stream. */
	appdata_s *ad = data;
	FILE* fp;
	fp = fopen("/opt/usr/media/Documents/data.csv", "w+");
	char data_string[256];

	/* For each reading, append the corresponding accelerometer and gyroscope values along with a single time-stamp in CSV format,
	 * The sensors start immediately after one another, as such the time difference is negligible and a single timer can be used. */
	for (int i = 0; i < (ad->gyro_count); i++)
	{
		sprintf(data_string, "%f, %f, %f, %f, %f, %f, %f,", (ad->swing_data)[i][0], (ad->swing_data)[i][1], (ad->swing_data)[i][2], (ad->swing_data)[i][3],
				(ad->swing_data)[i][4], (ad->swing_data)[i][5], (ad->swing_data)[i][6]);
		fprintf(fp, "%s\n", data_string);
	}
	fclose(fp);
}

/**
 * Start the sensor listeners if the start button is showing, else stop them. This is used so sessions
 * can easily be recorded.
 */
static void _button_click_cb(void *data, Evas_Object *button, void *ev)
{
	appdata_s *ad = data;
	if (strcmp(elm_object_part_text_get(button, NULL), "START") == 0)
	{
		sensor_listener_start(ad->accelerometer_listener);
		sensor_listener_start(ad->gyroscope_listener);
		elm_object_text_set(button, "STOP");
	}
	else
	{
		sensor_listener_stop(ad->accelerometer_listener);
		sensor_listener_stop(ad->gyroscope_listener);
		elm_object_text_set(button, "START");
		save_data(ad);	// Save the data.
	}
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Button */
	ad->button = elm_button_add(ad->conform);
	elm_object_text_set(ad->button, "START");
	evas_object_size_hint_weight_set(ad->button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_content_set(ad->conform, ad->button);


	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

/**
 * Accelerometer callback function, append the readings to the swing_data array
 * and append the timer.
 */
static void
accelerometer_cb(sensor_h sensor, sensor_event_s *event, void *data)
{
	appdata_s *ad = data;
	ad->swing_data[ad->accel_count][0] = ad->timer;
	ad->swing_data[ad->accel_count][1] = event->values[0];
	ad->swing_data[ad->accel_count][2] = event->values[1];
	ad->swing_data[ad->accel_count][3] = event->values[2];
	ad->accel_count++;
	ad->timer += 0.05f;
}

/**
 * Accelerometer callback function, append the readings to the swing_data array,
 * note there is no timer as this is done via the accelerometer callback.
 */
static void
gyroscope_cb(sensor_h sensor, sensor_event_s *event, void *data)
{
	appdata_s *ad = data;
	ad->swing_data[ad->gyro_count][4] = event->values[0];
	ad->swing_data[ad->gyro_count][5] = event->values[1];
	ad->swing_data[ad->gyro_count][6] = event->values[2];
	ad->gyro_count++;
}

/**
 * Attempt to establish a sensor handle and sensor listener for an accelerometer.
 */
static void
register_accelerometer(void* data)
{
	appdata_s *ad = data;
	bool supported = false;
	sensor_is_supported(SENSOR_ACCELEROMETER, &supported);
	if (!supported) {
	    /* Accelerometer is not supported on the current device */
	}
	sensor_get_default_sensor(SENSOR_ACCELEROMETER, &(ad->accelerometer));

	/* Create listener. */
	sensor_create_listener(ad->accelerometer, &(ad->accelerometer_listener));
	sensor_listener_set_event_cb(ad->accelerometer_listener, 50, accelerometer_cb, ad);

	/* Ensure the sensor continues to listen even during a screen timeout. */
	sensor_listener_set_option(ad->accelerometer_listener, SENSOR_OPTION_ALWAYS_ON);
}

/**
 * Attempt to establish a sensor handle and sensor listener for a gyroscope.
 */
static void
register_gyroscope(void* data)
{
	appdata_s *ad = data;
	bool supported = false;
	if (!supported) {
		/* Gyroscope is not supported on the current device */
	}
	sensor_get_default_sensor(SENSOR_GYROSCOPE, &(ad->gyroscope));

	/* Create listener. */
	sensor_create_listener(ad->gyroscope, &(ad->gyroscope_listener));
	sensor_listener_set_event_cb(ad->gyroscope_listener, 50, gyroscope_cb, ad);

	/* Ensure the sensor continues to listen even during a screen timeout. */
	sensor_listener_set_option(ad->gyroscope_listener, SENSOR_OPTION_ALWAYS_ON);
}

static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);

	register_accelerometer(ad);
	register_gyroscope(ad);
	evas_object_smart_callback_add(ad->button, "clicked", _button_click_cb, ad);

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
