/** 
 * Extension of TH10 -- Torgoen T10 analog style
 * Add functionality to:
 * - Move date window out from under hands
 * - Future: BT disconnect, weather?
 */
 
#include <pebble.h>

static Window *window;
static Layer *rootLayer;
static Layer *bg_layer;
static Layer *hand_layer;
static InverterLayer *inverter_layer;
static time_t t;
static struct tm now;
static GFont  font_time;
static GColor fg_color;
static GColor bg_color;

static int use_24hour;

// TODO: Create a config screen for these
#define USE_0_INSTEAD_OF_24 true
#define WHITE_ON_BLACK true
#define INVERT_DATE false
#define MOVE_SUBTLY true
#undef MOVE_LARGE
#define SHOW_SECONDS true

// Dimensions of the watch face
#define PEBBLE_SCREEN_WIDTH 144
#define PEBBLE_SCREEN_HEIGHT 168
#define W PEBBLE_SCREEN_WIDTH
#define H PEBBLE_SCREEN_HEIGHT
#define R (W/2 - 2)

// Angle of date window at 4:00, 4:30 & 5:00
#define ANGLE130 -1 * TRIG_MAX_ANGLE / 8
#define ANGLE400 TRIG_MAX_ANGLE / 12
#define ANGLE430 TRIG_MAX_ANGLE / 8
#define ANGLE500 TRIG_MAX_ANGLE / 6
#define ANGLE730 3 * TRIG_MAX_ANGLE / 8

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

#define MAKEGPATH(POINTS) { sizeof(POINTS) / sizeof(*POINTS), POINTS }

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
    MAKEGPATH(digit9)
};

// Alternate rotations for moving digits away from hands
static GPathInfo digits4[10];
static GPathInfo digits5[10];

static void hand_layer_update(Layer * me, GContext *ctx) {

    // Draw the minute hand outline in bg_color and filled with fg_color
    // int minute_angle = (now.tm_min * TRIG_MAX_ANGLE) / 60; // for ttss(MINUTE_UNIT)
    int minute_angle = ((now.tm_min * 60 + now.tm_sec) * TRIG_MAX_ANGLE) / 3600; // for ttss(SECOND_UNIT)
    
    gpath_rotate_to(minute_path, minute_angle);
    graphics_context_set_fill_color(ctx, fg_color);
    gpath_draw_filled(ctx, minute_path);
    graphics_context_set_stroke_color(ctx, bg_color);
    gpath_draw_outline(ctx, minute_path);
	
    // Draw the hour hand outline in bg_color and filled with fg_color
    // above the minute hand
    int hour_angle;
    int hour = now.tm_hour%12;
    hour_angle = ((hour * 60 + now.tm_min) * TRIG_MAX_ANGLE) / 720;
    
    gpath_rotate_to(hour_path, hour_angle);
    graphics_context_set_fill_color(ctx, fg_color);
    gpath_draw_filled(ctx, hour_path);
    graphics_context_set_stroke_color(ctx, bg_color);
    gpath_draw_outline(ctx, hour_path);

    graphics_context_set_fill_color(ctx, bg_color);
    graphics_fill_circle(ctx, GPoint(W/2, H/2), 20);

    // Draw the center circle
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_draw_circle(ctx, GPoint(W/2,H/2), 3);
  
    if (SHOW_SECONDS){
      int x, y, radius = R+8;
      const int32_t angle = (now.tm_sec * TRIG_MAX_ANGLE) / 60;
      
      // Keep the sec hand dot on screen by reducing the radius around :15 and :45
      if (now.tm_sec > 10 && now.tm_sec < 20){
        x = W - 2;
      } else if (now.tm_sec > 40 && now.tm_sec < 50){
        x = 2;
      } else {
        x = sin_lookup(angle) * radius / TRIG_MAX_RATIO + W/2;
      }
      
      // Parametric equation of a circle: 
      // x = cx + r * cos(a), y = cy + r * sin(a)
      // also: http://developer.getpebble.com/docs/c/Foundation/Math/
      // x =  sin_lookup(angle) * radius / TRIG_MAX_RATIO + W/2;
      y = -cos_lookup(angle) * radius / TRIG_MAX_RATIO + H/2;
      graphics_context_set_fill_color(ctx, fg_color);
      graphics_fill_circle(ctx, GPoint(x, y), 2);
      
      if (now.tm_sec % 15 == 0 || now.tm_sec % 5 == 0){
        graphics_context_set_stroke_color(ctx, bg_color);
        graphics_draw_circle(ctx, GPoint(x, y), 3);
      }
      
      /* Alternate second hand: invert ticks.
      graphics_context_set_stroke_color(ctx, bg_color);
      graphics_context_set_fill_color(ctx, bg_color);
      
      gpath_rotate_to(minor_tick_path, angle);
      gpath_draw_outline(ctx, minor_tick_path);  
      */
    }
}


/** Draw the initial background image */
static void bg_layer_update(Layer *me, GContext * ctx) {
    graphics_context_set_stroke_color(ctx, fg_color);
    graphics_context_set_fill_color(ctx, fg_color);
    
    // Draw the outside marks
    for (int min = 0 ; min < 60 ; min++)
    {
      const int angle = (min * TRIG_MAX_ANGLE) / 60;
      if ((min % 15) == 0){
        gpath_rotate_to(hour_tick_path, angle);
        gpath_draw_filled(ctx, hour_tick_path);
      } else {
        if ((min % 5) == 0){
          gpath_rotate_to(major_tick_path, angle);
          gpath_draw_filled(ctx, major_tick_path);
        } else {
          gpath_rotate_to(minor_tick_path, angle);
          gpath_draw_outline(ctx, minor_tick_path);
        }
      }
    }
    
    // And the large labels
    graphics_context_set_text_color(ctx, fg_color);
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
    
    int angle = ANGLE430;      // default date to 4:30 tick
    GPathInfo *digit_set = digits;

#ifdef MOVE_SUBTLY
    // Keeping the date window between 4:00 and 5:00 is less noticeable, 
    // but the hands still block portions of the window.
    // Move date box away from min. hand from  :20 to  :25-1/2 minutes
    // Move date box away from hour hand from 3:30 to 5:23
    // Put this in a bookmark to set the time in the Cloudpebble emulator:
    // javascript:(function(){SharedPebble.getPebbleNow().set_time(new Date('2015-02-24T17:17:42').getTime());})()
    // :42 allows emulator to update its clock and update watch face to setup test case.
    // Useful times: 17:17:42, 17:20:42, 
    int mins = now.tm_min        * 60 + now.tm_sec;
    int hrs  =(now.tm_hour % 12) * 60 + now.tm_min;
    
    bool mins1 = mins >= (20*60)      && mins <= (23*60     ); // min hand blocks top date
    bool mins2 = mins >  (23*60     ) && mins <= (25*60 + 30); // min blocks bottom date
    bool hrs1  = hrs  >= ( 3*60 + 45) && hrs  <= ( 4*60 + 24); // hr hand blocks top date
    bool hrs2  = hrs  >  ( 4*60 + 24) && hrs  <= ( 5*60 + 23); // hr blocks bottom date
    bool hrs3  = hrs  >= ( 5*60 + 18) && hrs  <= ( 5*60 + 20); // special case w/ little space
    
    if ( hrs3 ){
      angle = ANGLE430; // 4:30 tick works best from 5:20-5:22:30.
      digit_set = digits;
    } else if ( hrs2 || mins2 ){
    	angle = ANGLE400; // move date to 4:00 tick
    	digit_set = digits4;
    } else if ( hrs1 || mins1 ){
    	angle = ANGLE500; // ~5:00 tick
    	digit_set = digits5;
    }
#endif

#ifdef MOVE_LARGE    
    /*  Date at 4:30, 1:30, or 7:30. */
    int hrs = now.tm_hour % 12;
    bool mins4 = now.tm_min >= 20 && now.tm_min <= 25; // min hand blocks 4:30
    bool mins1 = now.tm_min >= 5 && now.tm_min <= 10;  // min blocks 1:30
    bool mins7 = now.tm_min >= 35 && now.tm_min <= 40; // min blocks 7:30
    bool hrs1 = hrs >= 1 && hrs <= 2; // hour hand blocks 4:30
    bool hrs4 = hrs >= 4 && hrs <= 5; // hour blocks 1:30
    bool hrs7 = hrs >= 7 && hrs <= 8; // hour blocks 7:30
    
    if ( mins4 && !hrs7 ){
    	angle = ANGLE730;
    	digit_set = digits5;
    } else if ( mins4 && !hrs1 ){
    	angle = ANGLE130;
    	digit_set = digits4;
    } else if ( hrs4 && !mins7 ){
    	angle = ANGLE730;
    	digit_set = digits5;
    } else if ( hrs4 && !mins1 ){
    	angle = ANGLE130;
    	digit_set = digits4;
    }
#endif
    
    gpath_rotate_to(mdbox_path, angle);
    
    graphics_context_set_fill_color(ctx, fg_color);
  
    if (INVERT_DATE) {
    	gpath_draw_filled(ctx, mdbox_path);
    	graphics_context_set_stroke_color(ctx, bg_color);
    } else {
    gpath_draw_outline(ctx, mdbox_path);
    	graphics_context_set_stroke_color(ctx, fg_color);
    }

    int ndigits = (mday<10)?0:1;
    int digit[2] = { mday%10, mday/10 };
    
    for (int d=ndigits; d>=0; d--) {
        for (uint i=0; i<digits[digit[d]].num_points-1; i++) {
            for (int j=0; j<4; j++) {
                GPoint p1, p2;
                int xoffset = ndigits * (4 - 9*d); // 4, -5
              int yoffset = xoffset;
              if (angle == ANGLE500){
                yoffset = ndigits * (5 - 10*d); // 5, -5
                xoffset = ndigits * (3 - 6*d) ; // 3, -3
              }else if (angle == ANGLE400){
                xoffset = ndigits * (5 - 10*d); // 6, -5
                yoffset = ndigits * (3 - 6*d) ; // 3, -3
              }

              p1.x = digit_set[digit[d]].points[i].x + j%2 + xoffset; // 0, 1, 0, 1  
              p1.y = digit_set[digit[d]].points[i].y + j/2 + yoffset; // 0, 0, 1, 1 
              p2.x = digit_set[digit[d]].points[i+1].x + j%2 + xoffset;
              p2.y = digit_set[digit[d]].points[i+1].y + j/2 + yoffset;
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
    
  // Redraw every 15 seconds (or 10 seconds? 6 times/minute, 6 degrees/minute)
  if (SHOW_SECONDS){
    layer_mark_dirty(hand_layer);
  }else if ((now.tm_sec % 15) == 0){
		layer_mark_dirty(hand_layer);
	}
  
  now = (struct tm) *tick_time;
}


static void rotate_digits(GPathInfo digits_dest[10], GPathInfo digits_orig[10], int angle){
	
	int32_t cos, sin;
	cos = cos_lookup(angle);
	sin = sin_lookup(angle);

	int x, y;
	uint i, p;
	for (i=0; i<10; i++) {
		// allocate point storage for alternate rotations
		if (digits_dest != digits_orig){
			digits_dest[i].num_points = digits_orig[i].num_points;
			digits_dest[i].points = malloc(digits_orig[i].num_points * sizeof(digits_orig[i].points[0]));
		}
		for (p=0; p<digits_orig[i].num_points; p++) {
			x = cos*digits_orig[i].points[p].x/TRIG_MAX_RATIO - sin*digits_orig[i].points[p].y/TRIG_MAX_RATIO;
			y = sin*digits_orig[i].points[p].x/TRIG_MAX_RATIO + cos*digits_orig[i].points[p].y/TRIG_MAX_RATIO;	
			
			digits_dest[i].points[p].x = W/2 + x;
			digits_dest[i].points[p].y = H/2 + y;
		}
	}
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

    /* Use inverter layer instead
    if (WHITE_ON_BLACK) {
      fg_color = GColorWhite;
      bg_color = GColorBlack;
    } else {
      fg_color = GColorBlack;
      bg_color = GColorWhite;
    }
    */

    fg_color = GColorWhite;
    bg_color = GColorBlack;
   
    window = window_create();
    window_set_background_color(window, bg_color);
    window_stack_push(window, true);
    rootLayer = window_get_root_layer(window);
    
    font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GILLSANS_40));

#ifdef MOVE_SUBTLY
    rotate_digits(digits4, digits, ANGLE400);
    rotate_digits(digits5, digits, ANGLE500);
#endif

#ifdef MOVE_LARGE    	
    rotate_digits(digits4, digits, ANGLE130);
    rotate_digits(digits5, digits, ANGLE730);
#endif

    // Rotate digits[] *last*; it overwrites the original's storage
    rotate_digits(digits,  digits, ANGLE430);
    	
    bg_layer = layer_create(GRect(0, 0, W, H));
    layer_set_update_proc(bg_layer, bg_layer_update);
    layer_add_child(rootLayer, bg_layer);
    
    hand_layer = layer_create(GRect(0, 0, W, H));
    layer_set_update_proc(hand_layer, hand_layer_update);
    layer_add_child(rootLayer, hand_layer);
    
    if (!WHITE_ON_BLACK){
	    inverter_layer = inverter_layer_create(GRect(0, 0, W, H));
	    layer_add_child(rootLayer, (Layer*)inverter_layer);
    };
	
    // tick_timer_service_subscribe(MINUTE_UNIT, handleTick);
    tick_timer_service_subscribe(SECOND_UNIT, handleTick);
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
	if (!WHITE_ON_BLACK){
		inverter_layer_destroy(inverter_layer);
	};
		
	// free points malloc'd for digits4 & digits5
	for(uint i=0;i<10;i++){
			free(digits4[i].points);
			free(digits5[i].points);
	}
	fonts_unload_custom_font(font_time);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
