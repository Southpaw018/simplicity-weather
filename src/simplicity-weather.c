#include <pebble.h>

static Window *window;
static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static Layer *line_layer;

static TextLayer *temperature_layer;
static TextLayer *city_layer;

static AppSync sync;
static uint8_t sync_buffer[64];

enum WeatherKey {
	WEATHER_TEMPERATURE_KEY = 0x0,	// TUPLE_CSTRING
	WEATHER_CITY_KEY = 0x1,			// TUPLE_CSTRING
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	switch (key) {
		case WEATHER_TEMPERATURE_KEY:
			//App Sync keeps new_tuple in sync_buffer, so we may use it directly
			text_layer_set_text(temperature_layer, new_tuple->value->cstring);
			break;
		case WEATHER_CITY_KEY:
			text_layer_set_text(city_layer, new_tuple->value->cstring);
			break;
	}
}

static void send_cmd(void) {
	Tuplet value = TupletInteger(1, 1);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) {
		return;
	}

	dict_write_tuplet(iter, &value);
	dict_write_end(iter);

	app_message_outbox_send();
}

void line_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	//Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	static char date_text[] = "Xxxxxxxxx 00";

	char *time_format;

	//TODO: Only update the date when it's changed.
	strftime(date_text, sizeof(date_text), "%B %e", tick_time);
	text_layer_set_text(text_date_layer, date_text);

	if (clock_is_24h_style()) {
		time_format = "%R";
	} else {
		time_format = "%I:%M";
	}

	strftime(time_text, sizeof(time_text), time_format, tick_time);

	//Kludge to handle lack of non-padded hour format string for twelve hour clock.
	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	text_layer_set_text(text_time_layer, time_text);
}

void deinit(void) {
	tick_timer_service_unsubscribe();
	window_destroy(window);
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);

	//Simplicity date
	text_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
	text_layer_set_text_color(text_date_layer, GColorWhite);
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

	//Simplicity time
	text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

	//Simplicity separator line
	GRect line_frame = GRect(8, 97, 139, 2);
	line_layer = layer_create(line_frame);
	layer_set_update_proc(line_layer, line_layer_update_callback);
	layer_add_child(window_layer, line_layer);

	//Weather temperature
	temperature_layer = text_layer_create(GRect(103, 0, 40, 18));
	text_layer_set_text_color(temperature_layer, GColorWhite);
	text_layer_set_background_color(temperature_layer, GColorClear);
	text_layer_set_text_alignment(temperature_layer, GTextAlignmentRight);
	text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(window_layer, text_layer_get_layer(temperature_layer));

	//Weather location
	city_layer = text_layer_create(GRect(1, 18, 142, 18));
	text_layer_set_text_color(city_layer, GColorWhite);
	text_layer_set_background_color(city_layer, GColorClear);
	text_layer_set_text_alignment(city_layer, GTextAlignmentRight);
	text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	layer_add_child(window_layer, text_layer_get_layer(city_layer));

	//Init weather info
	Tuplet initial_values[] = {
		TupletCString(WEATHER_TEMPERATURE_KEY, "-\u00B0F"),
		TupletCString(WEATHER_CITY_KEY, "St Pebblesburg")
	};

	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);

	send_cmd();
}

static void window_unload(Window *window) {
	app_sync_deinit(&sync);

	text_layer_destroy(city_layer);
	text_layer_destroy(temperature_layer);
}

void init(void) {
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});

	const int inbound_size = 64;
	const int outbound_size = 64;
	app_message_open(inbound_size, outbound_size);

	const bool animated = true;
	window_stack_push(window, animated);

	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
