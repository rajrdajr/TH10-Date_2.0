/** \file
 * TH10 -- Torgoen T10 analog style
 *
 */
 
#include <pebble.h>

static Window *window;
static Layer *rootLayer;
static Layer *hand_layer;
static Layer *bg_layer;
static time_t t;
static struct tm now;
static GFont font_time;

static int use_24hour;
#define USE_0_INSTEAD_OF_24 true
#define WHITE_ON_BLACK true

// Dimensions of the watch face
#define PEBBLE_SCREEN_WIDTH 144
#define PEBBLE_SCREEN_HEIGHT 168
#define W PEBBLE_SCREEN_WIDTH
#define H PEBBLE_SCREEN_HEIGHT
#define R (W/2 - 2)

// Hour hand
static GPath *hour_path;
static GPoint hour_points[] = {
    {  -8, -10 },
    { -10, -40 },
    {   0, -60 },
    { +10, -40 },
    {  +8, -10 },
};

// Minute hand
static GPath *minute_path;
static GPoint minute_points[] = {
    { -5, -10 },
    { -7, -60 },
    {  0, -76 },
    { +7, -60 },
    { +5, -10 },
};

// Hour hand ticks around the circle (slightly shorter)
static GPath *hour_tick_path;
static GPoint hour_tick_points[] = {
    { -3, 70 },
    { +3, 70 },
    { +3, 84 },
    { -3, 84 },
};

// Non-hour major ticks around the circle
static GPath *major_tick_path;
static GPoint major_tick_points[] = {
    { -3, 60 },
    { +3, 60 },
    { +3, 84 },
    { -3, 84 },
};

// Non-major ticks around the circle; will be drawn as lines
static GPath *minor_tick_path;
static GPoint minor_tick_points[] = {
    { 0, 76 },
    { 0, 84 },
};

// Day of month box
static GPath *mdbox_path;
static GPoint mdbox_points[] = {
    { 24, -8 },
    { 26, -10},
    { 48, -10 },
    { 50, -8 },
    { 50, 8 },
    { 48, 10 },
    { 26, 10 },
    { 24, 8 },
};

#define MAKEGPATH(POINTS) { sizeof(POINTS) / sizeof(*POINTS), POINTS }

// Day of month digits
static GPoint digit0[] =  {
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
    { 33, -5 },
};

static GPoint digit1[] =  {
    { 33, -2 },
    { 37, -7 },
    { 37, 7 },
    { 33, 7 },
    { 41, 7 }
};

static GPoint digit2[] =  {
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, -2 },
    { 33, 7 },
    { 41, 7 },
};

static GPoint digit3[] =  {
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, 0 },
    { 36, 0 },
    { 41, 0 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
};

static GPoint digit4[] =  {
    { 39, 7 },
    { 39, -7 },
    { 33, 3 },
    { 41, 3 },
};

static GPoint digit5[] = {
    { 41, -7 },
    { 33, -7 },
    { 33, 1 },
    { 35, -2 },
    { 39, -2 },
    { 41, 1 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
};
static GPoint digit6[] = {
    { 41, -5 },
    { 39, -7 },
    { 35, -7 },
    { 33, -5 },
    { 33, 5 },
    { 35, 7 },
    { 39, 7 },
    { 41, 5 },
    { 41, 2 },
    { 39, -1 },
    { 35, -1 },
    { 33, 2 },
};

static GPoint digit7[] = {
    { 33, -7 },
    { 41, -7 },
    { 35, 7 },
};

static GPoint digit8[] = {
    { 41, 0 },
    { 33, 0 },
    { 33, -5 },
    { 35, -7 },
    { 39, -7 },
    { 41, -5 },
    { 41, 5 },
    { 39, 7 },
    { 35, 7 },
    { 33, 5 },
    { 33, -5 },
};

static GPoint digit9[] = {
    { 33, 5 },
    { 35, 7 },
    { 39, 7 },
    { 41, 5 },
    { 41, -5 },
    { 39, -7 },
    { 35, -7 },
    { 33, -5 },
    { 33, -2 },
    { 35, 1 },
    { 39, 1 },
    { 41, -2 },
};


static GPathInfo digits[10] = {
    MAKEGPATH(digit0),
    MAKEGPATH(digit1),
    MAKEGPATH(digit2),
    MAKEGPATH(digit3),
    MAKEGPATH(digit4),
    MAKEGPATH(digit5),
    MAKEGPATH(digit6),
    MAKEGPATH(digit7),
    MAKEGPATH(digit8),
    MAKEGPATH(digit9),
};

static void hand_layer_update(Layer * me, GContext *ctx) {
    // Draw the minute hand outline in black and filled with white
    int minute_angle = (now.tm_min * TRIG_MAX_ANGLE) / 60;
    
    gpath_rotate_to(minute_path, minute_angle);
    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, minute_path);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    gpath_draw_outline(ctx, minute_path);
    
    // Draw the hour hand outline in black and filled with white
    // above the minute hand
    int hour_angle;
    int hour = now.tm_hour%12;
    hour_angle = ((hour * 60 + now.tm_min) * TRIG_MAX_ANGLE) / 720;
    
    gpath_rotate_to(hour_path, hour_angle);
    graphics_context_set_fill_color(ctx, GColorWhite);
    gpath_draw_filled(ctx, hour_path);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    gpath_draw_outline(ctx, hour_path);

    graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_circle(ctx, GPoint(W/2, H/2), 20);

    // Draw the center circle
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_circle(ctx, GPoint(W/2,H/2), 3);
}


/** Draw the initial background image */
static void bg_layer_update(Layer *me, GContext * ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    
    // Draw the outside marks
    for (int min = 0 ; min < 60 ; min++)
    {
        const int angle = (min * TRIG_MAX_ANGLE) / 60;
        if ((min % 15) == 0)
        {
            gpath_rotate_to(hour_tick_path, angle);
            gpath_draw_filled(ctx, hour_tick_path);
        } else {
            if ((min % 5) == 0)
            {
                gpath_rotate_to(major_tick_path, angle);
                gpath_draw_filled(ctx, major_tick_path);
            } else {
                gpath_rotate_to(minor_tick_path, angle);
                gpath_draw_outline(ctx, minor_tick_path);
            }
		}
    }
    
    // And the large labels
    graphics_context_set_text_color(ctx, GColorWhite);
	bool shift = use_24hour && (now.tm_hour >= 12);

    graphics_draw_text(ctx,
                       shift ? (USE_0_INSTEAD_OF_24 ? "0" : "24") : "12",
                       font_time,
                       GRect(W/2-30,4,60,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
    
    graphics_draw_text(ctx,
                       shift ? "15" : "3",
                       font_time,
                       GRect(W/2,H/2-26,70,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight,
                       NULL
                       );
    
    graphics_draw_text(ctx,
                       shift ? "18" : "6",
                       font_time,
                       GRect(W/2-30,110,60,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter,
                       NULL
                       );
    
    graphics_draw_text(ctx,
                       shift ? "21" : "9",
                       font_time,
                       GRect(W/2-70,H/2-26,60,50),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft,
                       NULL
                       );
    
    // Draw a small box with the current date
    int mday = now.tm_mday;
   
	gpath_rotate_to(mdbox_path, TRIG_MAX_ANGLE / 8);
    graphics_context_set_fill_color(ctx, GColorWhite);
	if (WHITE_ON_BLACK) {
    	gpath_draw_outline(ctx, mdbox_path);
		graphics_context_set_stroke_color(ctx, GColorWhite);
	} else {
    	gpath_draw_filled(ctx, mdbox_path);
		graphics_context_set_stroke_color(ctx, GColorBlack);
	}

    int ndigits = (mday<10)?0:1;
    int digit[2] = { mday%10, mday/10 };
    
    for (int d=ndigits; d>=0; d--) {
        for (uint i=0; i<digits[digit[d]].num_points-1; i++) {
            for (int j=0; j<4; j++) {
                GPoint p1, p2;
                int offset = ndigits * (4 - 9*d);

                p1.x = digits[digit[d]].points[i].x + j%2 + offset;
                p1.y = digits[digit[d]].points[i].y + j/2 + offset;
                p2.x = digits[digit[d]].points[i+1].x + j%2 + offset;
                p2.y = digits[digit[d]].points[i+1].y + j/2 + offset;
                graphics_draw_line(ctx, p1, p2);
            }
        }
	}
}


static void handleTick(struct tm *tick_time, TimeUnits units_changed) {
    // If the day of month changes, for a redraw of the background
    if (now.tm_mday != tick_time->tm_mday) {
		layer_mark_dirty(bg_layer);
	}
    
    layer_mark_dirty(hand_layer);
    now = *tick_time;
}


#define GPATH_INIT(PATH, POINTS) ({ \
GPathInfo __info = { sizeof(POINTS) / sizeof(*POINTS), POINTS }; \
PATH = gpath_create(&__info); \
})

static void init() {
    GPATH_INIT(hour_path, hour_points);
    gpath_move_to(hour_path, GPoint(W/2,H/2));
    
    GPATH_INIT(minute_path, minute_points);
    gpath_move_to(minute_path, GPoint(W/2,H/2));
    
    GPATH_INIT(major_tick_path, major_tick_points);
    gpath_move_to(major_tick_path, GPoint(W/2,H/2));
    
    GPATH_INIT(hour_tick_path, hour_tick_points);
    gpath_move_to(hour_tick_path, GPoint(W/2,H/2));
    
    GPATH_INIT(minor_tick_path, minor_tick_points);
    gpath_move_to(minor_tick_path, GPoint(W/2,H/2));
    
    GPATH_INIT(mdbox_path, mdbox_points);
    gpath_move_to(mdbox_path, GPoint(W/2,H/2));
    

	t = time(NULL);
	now = *localtime(&t);
    use_24hour = clock_is_24h_style();
    
    window = window_create();
    window_set_background_color(window, GColorBlack);
    window_stack_push(window, true);
    rootLayer = window_get_root_layer(window);
    
    font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GILLSANS_40));
    
	// Rotate digits
	uint i, p;
	int32_t cos, sin;
	int32_t a = TRIG_MAX_ANGLE / 8;
	int x, y;
	cos = cos_lookup(a);
	sin = sin_lookup(a);
	for (i=0; i<10; i++) {
		for (p=0; p<digits[i].num_points; p++) {
			x = cos*digits[i].points[p].x/TRIG_MAX_RATIO - sin*digits[i].points[p].y/TRIG_MAX_RATIO;
			y = sin*digits[i].points[p].x/TRIG_MAX_RATIO + cos*digits[i].points[p].y/TRIG_MAX_RATIO;

			digits[i].points[p].x = W/2 + x;
			digits[i].points[p].y = H/2 + y;
		}
	}

    bg_layer = layer_create(GRect(0, 0, W, H));
	layer_set_update_proc(bg_layer, bg_layer_update);
    layer_add_child(rootLayer, bg_layer);
    
    hand_layer = layer_create(GRect(0, 0, W, H));
    layer_set_update_proc(hand_layer, hand_layer_update);
    layer_add_child(rootLayer, hand_layer);
    
   	tick_timer_service_subscribe(MINUTE_UNIT, handleTick);
}


static void deinit() {
	tick_timer_service_unsubscribe();
	
	gpath_destroy(hour_path);
	gpath_destroy(minute_path);
	gpath_destroy(major_tick_path);
	gpath_destroy(hour_tick_path);
	gpath_destroy(minor_tick_path);
	gpath_destroy(mdbox_path);

	layer_destroy(hand_layer);
	layer_destroy(bg_layer);
	
	fonts_unload_custom_font(font_time);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
