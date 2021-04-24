#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "imp_sdl.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

typedef uintptr_t umm;

typedef u16 b16;
typedef u32 b32;


#include "colors.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define AUDIO_FREQ 48000
#define FONT_SIZE 24
#define SMALL_FONT_SIZE 12

#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))

struct memory_arena
{
	void *base;
	u32 size;
	u32 used;
};

static void *
push_size(struct memory_arena *arena, u32 size)
{
	u32 remaining = arena->size - arena->used;
	assert(remaining >= size);

	umm p = (umm)arena->base + arena->used;

	arena->used += size;

	void *result = (void *)p;

	return result;
}

#define PUSH_STRUCT(arena, type) (type *)push_size(arena, sizeof(type))

static void
zero_memory(void *base, umm size)
{
	memset(base, 0, size);
}

#define ZERO_STRUCT(source) zero_memory(&source, sizeof(source))

static const f64 F64_ZERO = 0;
static const f32 F32_ZERO = 0;

#define IS_F64_ZERO(x) (memcmp(&x, &F64_ZERO, sizeof(f64)) == 0)
#define IS_F32_ZERO(x) (memcmp(&x, &F32_ZERO, sizeof(f32)) == 0)

struct v2
{
	f32 x;
	f32 y;
};

struct v2i
{
	s32 x;
	s32 y;
};

inline static struct v2
v2(f32 x, f32 y)
{
	struct v2 result;
	result.x = x;
	result.y = y;
	return result;
}

inline static struct v2
add_v2(struct v2 a, struct v2 b)
{
	struct v2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

inline static struct v2
sub_v2(struct v2 a, struct v2 b)
{
	struct v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

inline static f32
dot_v2(struct v2 a, struct v2 b)
{
	f32 result = a.x * b.x + a.y * b.y;
	return result;
}

inline static f32
len_v2(struct v2 v)
{
	f32 sqrd_len = dot_v2(v, v);
	return sqrtf(sqrd_len);
}

inline static struct v2
normalize_v2(struct v2 v)
{
	f32 len = len_v2(v);
	return IS_F32_ZERO(len) ? v2(0, 0) : v2(v.x / len, v.y / len);
}

inline static struct v2
scale_v2(struct v2 a, f32 s)
{
	struct v2 result;
	result.x = a.x * s;
	result.y = a.y * s;
	return result;
}


inline static struct v2i
v2i(s32 x, s32 y)
{
	struct v2i result;
	result.x = x;
	result.y = y;
	return result;
}

inline static struct v2i
add_v2i(struct v2i a, struct v2i b)
{
	struct v2i result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

inline static struct v2i
sub_v2i(struct v2i a, struct v2i b)
{
	struct v2i result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

inline static s32
dot_v2i(struct v2i a, struct v2i b)
{
	s32 result = a.x * b.x + a.y * b.y;
	return result;
}

inline static f32
len_v2i(struct v2i v)
{
	s32 sqrd_len = dot_v2i(v, v);
	return sqrtf(sqrd_len);
}

inline static struct v2
normalize_v2i(struct v2i v)
{
	s32 sqrd_len = dot_v2i(v, v);

	if (sqrd_len == 0)
		return v2(0, 0);

	f32 len = sqrtf(sqrd_len);
	return v2(v.x / len, v.y / len);
}

inline static struct v2i
scale_v2i(struct v2i a, s32 s)
{
	struct v2i result;
	result.x = a.x * s;
	result.y = a.y * s;
	return result;
}

struct entity_part {
	u16 index;
	u16 length;
	u16 size;
	u16 color;
	u16 parent_index;
	u16 render_size;
	f32 mass;
	b32 disposed;
	b32 suspended_for_frame;
	struct v2 p;
	struct v2 v;
	struct v2 a;
	struct v2 force;
};

enum entity_type {
	ENTITY_NONE,
	ENTITY_PLAYER,
	ENTITY_SEED,
	ENTITY_WATER,
	ENTITY_FOOD,
	ENTITY_WORM
};

struct entity {
	u32 id;
	u32 index;
	enum entity_type type;
	u16 part_count;
	b16 internal_collisions;	
	struct entity_part parts[32];
	b32 disposed;
	b32 suspended_for_frame;

	struct v2 target;
	u32 target_entity_index;
	u32 target_entity_id;
	b32 has_target;
	f32 next_target_check_t;
};

enum waveform_type {
	SINE,
	SAW
};

struct waveform {
	u16 t;
	u16 freq;
	f32 amp;
};

enum game_event_type {
	GAME_EVENT_NONE,
	GAME_EVENT_SEED_TOUCH_WATER,
	GAME_EVENT_WORM_EAT_FOOD
};

struct seed_touch_water_event {
	u32 seed_entity_index;
	u32 water_entity_index;
	u32 water_part_index;
};

struct worm_eat_food_event {
	u32 worm_entity_index;
	u32 food_entity_index;
};

struct game_event {
	enum game_event_type type;
	union {
		struct seed_touch_water_event seed_touch_water;
		struct worm_eat_food_event worm_eat_food;
	};
};

struct game_state {
	struct entity entities[32];
	u32 entity_count;
	
	f32 time;

	f32 last_level_end_t;
	f32 tunnel_begin_t;
	
	f32 level_begin_t;
	f32 level_end_t;

	f32 tunnel_size;

	u32 current_level;

	f32 next_note_t;
	u32 note;
	
	struct waveform sine_waves[32];
	u32 sine_wave_count;

	struct waveform saw_waves[32];
	u32 saw_wave_count;

	struct game_event events[64];
	u32 event_count;

	u32 entity_id_seq;
};

struct input_state
{
	u8 left;
	u8 right;
	u8 up;
	u8 down;

	u8 a;
    
	s8 dleft;
	s8 dright;
	s8 dup;
	s8 ddown;
	s8 da;
};

enum text_align
{
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
};



static s32
random_int(s32 min, s32 max)
{
	s32 range = max - min;
	return min + rand() % range;
}

static s32
min(s32 x, s32 y)
{
	return x < y ? x : y;
}

static s32
max(s32 x, s32 y)
{
	return x > y ? x : y;
}


static inline bool
find_intersection_between_lines_(f32 P0_X,
				 f32 P0_Y,
				 f32 P1_X,
				 f32 P1_Y,
				 f32 P2_X,
				 f32 P2_Y,
				 f32 P3_X,
				 f32 P3_Y,
				 f32 *T1,
				 f32 *T2)
{
    f32 D1_X = P1_X - P0_X;
    f32 D1_Y = P1_Y - P0_Y;
    f32 D2_X = P3_X - P2_X;
    f32 D2_Y = P3_Y - P2_Y;

    f32 Denominator = (-D2_X * D1_Y + D1_X * D2_Y);
    
    *T1 = (D2_X * (P0_Y - P2_Y) - D2_Y * (P0_X - P2_X)) / Denominator;
    *T2 = (-D1_Y * (P0_X - P2_X) + D1_X * (P0_Y - P2_Y)) / Denominator;

    if (*T1 >= 0 && *T1 <= 1 && *T2 >= 0 && *T2 <= 1)
        return(true);

    return(false);
}

__attribute__((always_inline))
static inline bool
find_intersection_between_lines(struct v2 P1,
				struct v2 P2,
				struct v2 P3,
				struct v2 P4,
				f32 *T1, f32 *T2)
{
	return find_intersection_between_lines_(P1.x, P1.y,
						P2.x, P2.y,
						P3.x, P3.y,
						P4.x, P4.y,
						T1, T2);
}

__attribute__((always_inline))
static inline bool
test_collision_against_line(struct v2 P,
			    struct v2 W1,
			    struct v2 W2,
			    struct v2 Normal,
			    struct v2 *NewP,
			    struct v2 *NewV,
			    f32 *T)
{
	struct v2 dP = sub_v2(*NewP, P);
	struct v2 Tangent = v2(Normal.y, -Normal.x);

	f32 MagnitudeAlongNormal = dot_v2(dP, Normal);
	struct v2 MovementAlongNormal = scale_v2(Normal, MagnitudeAlongNormal);
		
	if (MagnitudeAlongNormal < 1)
		MovementAlongNormal = normalize_v2(MovementAlongNormal);
	
	f32 T1, T2;
	bool Collision = find_intersection_between_lines(P,
							 *NewP,
							 W1,
							 W2,
							 &T1,
							 &T2);
	if (Collision)
	{
		struct v2 VAlongTangent = scale_v2(Tangent, dot_v2(*NewV, Tangent));
		struct v2 VAlongNormal = scale_v2(Normal, -0.1f * dot_v2(*NewV, Normal));

		*NewP = add_v2(add_v2(P, scale_v2(dP, T1)), Normal); // + NewV;
		*NewV = add_v2(VAlongTangent, VAlongNormal);
		*T = T1;
	}
	else
	{
		Collision = find_intersection_between_lines(*NewP,
							    sub_v2(*NewP, Normal),
							    W1,
							    W2,
							    &T1,
							    &T2);
		if (Collision)
		{
			struct v2 VAlongTangent = scale_v2(Tangent, dot_v2(*NewV, Tangent));
			struct v2 VAlongNormal = scale_v2(Normal, -0.1f * dot_v2(*NewV, Normal));

			*NewP = add_v2(*NewP, scale_v2(Normal, (1.1f - T1)));
			*NewV = add_v2(VAlongTangent, VAlongNormal);
			*T = T1;
		}
	}
	return(Collision);
}

__attribute__((always_inline))
static inline bool
test_collision_against_box(const struct entity_part *Obstacle,
			   struct entity_part *Entity,
			   struct v2 P,
			   struct v2 *NewP,
			   struct v2 *NewV)
{
    struct v2 MinkowskiSize = v2(Obstacle->size + Entity->size, Obstacle->size + Entity->size);

    struct v2 Diff = sub_v2(P, Obstacle->p);
    if (fabsf(Diff.x) > MinkowskiSize.x || fabsf(Diff.y) > MinkowskiSize.y)
        return false;

#if 0
    struct v2 TopLeft = add_v2(Obstacle->p, scale_v2(v2(-MinkowskiSize.x, MinkowskiSize.y), 0.5f));
    struct v2 TopRight = add_v2(Obstacle->p, scale_v2(MinkowskiSize, 0.5f));
    struct v2 BottomRight = add_v2(Obstacle->p, scale_v2(v2(MinkowskiSize.x, -MinkowskiSize.y), 0.5f));
    struct v2 BottomLeft = add_v2(Obstacle->p, scale_v2(MinkowskiSize, 0.5f));
#else
    struct v2 TopLeft = add_v2(Obstacle->p, scale_v2(v2(-MinkowskiSize.x, -MinkowskiSize.y), 0.5f));
    struct v2 TopRight = add_v2(Obstacle->p, scale_v2(v2(MinkowskiSize.x, -MinkowskiSize.y), 0.5f));
    struct v2 BottomRight = add_v2(Obstacle->p, scale_v2(v2(MinkowskiSize.x, MinkowskiSize.y), 0.5f));
    struct v2 BottomLeft = add_v2(Obstacle->p, scale_v2(v2(-MinkowskiSize.x, MinkowskiSize.y), 0.5f));
#endif    
    // NOTE(Omid): Check if inside.
    if (P.x > TopLeft.x &&
        P.x < TopRight.x &&
        P.y > TopLeft.y &&
        P.y < BottomLeft.y)
    {
        f32 D1 = fabsf(P.y - TopLeft.y) + 0.1f;
        f32 D2 = fabsf(P.y - BottomLeft.y) + 0.1f;
        NewP->y -= D1 < D2 ? D1 : -D2;
	return true;
    }
    
    f32 T;
    if (test_collision_against_line(P, TopLeft, TopRight, v2(0, -1), NewP, NewV, &T))
	    return true;

    if (test_collision_against_line(P, BottomLeft, BottomRight, v2(0, 1), NewP, NewV, &T))
	    return true;

    if (test_collision_against_line(P, TopLeft, BottomLeft, v2(-1, 0), NewP, NewV, &T))
	    return true;

    if (test_collision_against_line(P, TopRight, BottomRight, v2(1, 0), NewP, NewV, &T))
	    return true;

    return false;
}



static void
fill_rect(SDL_Renderer *renderer, s32 x, s32 y, s32 width, s32 height, struct color color)
{
	SDL_Rect rect = {0};
	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(renderer, &rect);
}

static void
draw_rect(SDL_Renderer *renderer, s32 x, s32 y, s32 width, s32 height, struct color color)
{
	SDL_Rect rect = {0};
	rect.x = x;
	rect.y = y;
	rect.w = width;
	rect.h = height;
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawRect(renderer, &rect);
}

static void
draw_string(SDL_Renderer *renderer,
            TTF_Font *font,
            const char *text,
            s32 x, s32 y,
            enum text_align alignment,
            struct color color)
{
	SDL_Color sdl_color =  { color.r, color.g, color.b, color.a };
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, sdl_color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_Rect rect;
	rect.y = y;
	rect.w = surface->w;
	rect.h = surface->h;
	switch (alignment) {
	case TEXT_ALIGN_LEFT:
		rect.x = x;
		break;
	case TEXT_ALIGN_CENTER:
		rect.x = x - surface->w / 2;
		break;
	case TEXT_ALIGN_RIGHT:
		rect.x = x - surface->w;
		break;
	}

	SDL_RenderCopy(renderer, texture, 0, &rect);
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

static void
draw_string_f(SDL_Renderer *renderer, TTF_Font *font, s32 x, s32 y, enum text_align alignment, struct color color, const char *format, ...)
{
	char buffer[4096];
	
	va_list args;
	va_start(args, format);
	#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
	vsnprintf(buffer, ARRAY_COUNT(buffer), format, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

	draw_string(renderer, font, buffer, x, y, alignment, color);
	
	va_end(args);
}


static void
render_cell_(SDL_Renderer *renderer,
	     u8 value, u8 alpha, s32 offset_x, s32 offset_y, s32 w, s32 h,
	     b32 outline)
{
	struct color base_color = BASE_COLORS[value];
	struct color light_color = LIGHT_COLORS[value];
	struct color dark_color = DARK_COLORS[value];

	base_color.a = alpha;
	light_color.a = alpha;
	dark_color.a = alpha;
	
	s32 edge = 4; // GRID_SIZE / 8;

	s32 x = (s32)(round(offset_x - (w / 2.0)));
	s32 y = (s32)(round(offset_y - (h / 2.0)));
    
	if (outline) {
		draw_rect(renderer, x, y, w, h, base_color);
		return;
	}

	fill_rect(renderer, x, y, w, h, base_color);
	
	edge = (s32)((f32)w * fabsf((f32)offset_x - WINDOW_WIDTH / 2) / WINDOW_WIDTH);
	
	if (offset_x > (WINDOW_WIDTH / 2)) {
		fill_rect(renderer, x, y, edge, h, light_color);
		fill_rect(renderer, x + w - edge, y, edge, h, dark_color);
	} else {
		fill_rect(renderer, x, y, edge, h, dark_color);
		fill_rect(renderer, x + w - edge, y, edge, h, light_color);
	}

	edge = (s32)((f32)h * fabsf((f32)offset_y - WINDOW_HEIGHT / 2) / WINDOW_HEIGHT);
		
	if (offset_y > (WINDOW_HEIGHT / 2)) {
		fill_rect(renderer, x, y, w, edge, light_color);
		fill_rect(renderer, x, y + h - edge, w, edge, dark_color);
	} else {
		fill_rect(renderer, x, y, w, edge, dark_color);
		fill_rect(renderer, x, y + h - edge, w, edge, light_color);
	}

	
#if 0	
	fill_rect(renderer, x, y, w, h, dark_color);
	fill_rect(renderer, x + edge, y,
		  w - edge, h - edge, light_color);
	fill_rect(renderer, x + edge, y + edge,
		  w - edge * 2, h - edge * 2, base_color);
#endif
}

static void
render_cell(SDL_Renderer *renderer,
	    u8 value, s32 offset_x, s32 offset_y, s32 w, s32 h,
	    b32 outline)
{
	render_cell_(renderer, value, 0xFF, offset_x, offset_y, w, h, outline);
}

static void
render_rect(SDL_Renderer *renderer, struct color color, s32 offset_x, s32 offset_y, s32 w, s32 h, b32 outline)
{
	s32 x = (s32)(round(offset_x - (w / 2.0)));
	s32 y = (s32)(round(offset_y - (h / 2.0)));
    
	if (outline) {
		draw_rect(renderer, x, y, w, h, color);
		return;
	}
    
	fill_rect(renderer, x, y, w, h, color); 
}

static void
fill_cell_(SDL_Renderer *renderer,
	   u8 value, u8 alpha, s32 x, s32 y, s32 w, s32 h)
{
	render_cell_(renderer, value, alpha, x, y, w, h, 0);
}

static void
fill_cell(SDL_Renderer *renderer,
          u8 value, s32 x, s32 y, s32 w, s32 h)
{
	fill_cell_(renderer, value, 0xFF, x, y, w, h);
}

static void
draw_cell(SDL_Renderer *renderer,
          u8 value, s32 x, s32 y, s32 w, s32 h)
{
	render_cell(renderer, value, x, y, w, h, 1);
}

static void
special_fill_cell_(SDL_Renderer *renderer,
		  u8 value, u8 alpha, s32 x, s32 y, s32 w, s32 h)
{
	if (w < 80) {
		fill_cell_(renderer, value, alpha, x, y, w, h);
		return;
	} else if (w < 120) {
		struct color c = BASE_COLORS[value];
		c.a = alpha;
		render_rect(renderer, c, x, y, w, h, false);	
	} else {
		struct color c = DARK_COLORS[value];
		c.a = alpha;
		render_rect(renderer, c, x, y, w, h, false);
	}
}







static void
goto_level(struct game_state *game, u32 level)
{
	game->current_level = level;
	game->last_level_end_t = game->level_end_t;

	game->tunnel_begin_t = game->level_end_t + 5;
	game->level_begin_t = game->tunnel_begin_t + 10;
	game->level_end_t = game->level_begin_t + 60;
		
	game->tunnel_size = 0;
}

static struct game_event *
push_game_event(struct game_state *game)
{
	assert(game->event_count < ARRAY_COUNT(game->events));
	return game->events + (game->event_count++);
}

static bool
check_suspension(struct entity *e1, struct entity_part *p1, struct entity *e2, struct entity_part *p2)
{
	bool result = !e1->suspended_for_frame && !p1->suspended_for_frame && !e2->suspended_for_frame && !p2->suspended_for_frame;
	return result;
}

static struct game_event *
seed_touch_water(struct game_state *game, struct entity *seed, struct entity *water, struct entity_part *droplet)
{
	struct game_event *e = push_game_event(game);
	e->type = GAME_EVENT_SEED_TOUCH_WATER;
	
	seed->suspended_for_frame = true;
	droplet->suspended_for_frame = true;
	e->seed_touch_water.seed_entity_index = seed->index;
	e->seed_touch_water.water_entity_index = water->index;
	e->seed_touch_water.water_part_index = droplet->index;

	return e;
}

static bool
try_seed_touch_water(struct game_state *game, struct entity *e1, struct entity_part *p1, struct entity *e2, struct entity_part *p2)
{
	if (!check_suspension(e1, p1, e2, p2))
		return false;
	
	if (e1->type == ENTITY_SEED && e2->type == ENTITY_WATER)
		return seed_touch_water(game, e1, e2, p2);
	else if (e1->type == ENTITY_WATER && e2->type == ENTITY_SEED)
		return seed_touch_water(game, e2, e1, p1);
	else
		return false;
}


static struct game_event *
worm_eat_food(struct game_state *game, struct entity *worm, struct entity *food)
{
	struct game_event *e = push_game_event(game);
	e->type = GAME_EVENT_WORM_EAT_FOOD;

	food->suspended_for_frame = true;
	e->worm_eat_food.worm_entity_index = worm->index;
	e->worm_eat_food.food_entity_index = food->index;
	return e;
}

static bool
try_worm_eat_food(struct game_state *game, struct entity *e1, struct entity_part *p1, struct entity *e2, struct entity_part *p2)
{
	if (!check_suspension(e1, p1, e2, p2))
		return false;
	
	if (e1->type == ENTITY_WORM && e2->type == ENTITY_FOOD && p1->index == 0)
		return worm_eat_food(game, e1, e2);
	else if (e1->type == ENTITY_FOOD && e2->type == ENTITY_WORM && p2->index == 0)
		return worm_eat_food(game, e2, e1);
	else
		return false;
}



static struct entity *
push_entity(struct game_state *game)
{
	u32 index = game->entity_count++;
	struct entity *result = game->entities + index;
	ZERO_STRUCT(*result);
	result->id = ++game->entity_id_seq;
	result->index = index;
	return result;
}

static struct entity_part *
push_entity_part(struct entity *entity)
{
	assert(entity->part_count < ARRAY_COUNT(entity->parts));
	u16 index = entity->part_count++;
	struct entity_part *result = entity->parts + index;
	result->index = index;
	return result;
}

static void
add_squid_leg(struct entity *entity, u16 count)
{
	struct entity_part *p;
	u16 parent_index = 0;

	for (u16 i = 0; i < count; ++i) {
		p = push_entity_part(entity);
		p->length = 20;
		p->size = 25 - i;
		/* p->mass = 10000; */
		p->color = 6;
		/* p->p = v2(-40, 20); */
		p->parent_index = parent_index;
		parent_index = p->index;
	}
	
	/* p = add_part(entity); */
	/* p->length = 25; */
	/* p->size = 25; */
	/* p->color = 2; */
	/* p->p = v2(-40, 20); */
	/* p->parent_index = parent_index; */
		
	/* parent_index = p->index; */

	/* p = add_part(entity); */
	/* p->length = 25; */
	/* p->size = 20; */
	/* p->color = 4; */
	/* p->parent_index = parent_index; */

	/* parent_index = p->index; */

	/* p = add_part(entity); */
	/* p->length = 25; */
	/* p->size = 20; */
	/* p->color = 5; */
	/* p->parent_index = parent_index; */

	/* parent_index = p->index; */

	/* p = add_part(entity); */
	/* p->length = 25; */
	/* p->size = 20; */
	/* p->color = 6; */
	/* p->parent_index = parent_index; */
}

static void
init_seed(struct entity *entity)
{
	entity->type = ENTITY_SEED;
	entity->part_count = 1;
	entity->parts[0].length = 0;
	entity->parts[0].size = 25;
	entity->parts[0].color = 2;
	entity->parts[0].p = v2(300, 300);
}

static void
init_food(struct entity *entity)
{
	 /* ZERO_STRUCT(*entity); */
	entity->type = ENTITY_FOOD;
	entity->part_count = 1;
	entity->parts[0].length = 0;
	entity->parts[0].size = 25;
	entity->parts[0].color = 3;
}

static void
push_worm_tail(struct entity *entity)
{
	if (entity->part_count >= ARRAY_COUNT(entity->parts))
		return;
	
	struct entity_part *p;
	p = push_entity_part(entity);
	p->length = 25;
	p->size = (u16)(40 - entity->part_count);
	p->render_size = p->size;
	p->mass = (f32)(p->size * p->size);
	p->color = 4;
	p->p = add_v2(entity->parts[p->index - 1].p, v2(25, 0));
	p->parent_index = (u16)(p->index - 1);
}

static void
init_squid(struct entity *entity)
{
	struct entity_part *p;

	entity->type = ENTITY_PLAYER;
	
	p = push_entity_part(entity);
	p->length = 0;
	p->size = 50;
	p->mass = 10000;
	p->color = 1;
	p->p = v2(100, 100);

	/* p = add_part(entity); */
	/* p->length = 120; */
	/* p->size = 50; */
	/* p->color = 2; */
	/* p->p = v2(70, 70); */

	add_squid_leg(entity, 16);

	/* entity->parts[6].length = 60; */
	/* entity->parts[6].size = 25; */
	/* entity->parts[6].color = 2; */
	/* entity->parts[6].p = v2(-40, 20); */

	/* entity->parts[7].length = 60; */
	/* entity->parts[7].size = 20; */
	/* entity->parts[7].color = 4; */
	/* entity->parts[7].parent_index = 6; */

	/* entity->parts[8].length = 60; */
	/* entity->parts[8].size = 20; */
	/* entity->parts[8].color = 5; */
	/* entity->parts[8].parent_index = 7; */

	/* entity->parts[9].length = 60; */
	/* entity->parts[9].size = 20; */
	/* entity->parts[9].color = 6; */
	/* entity->parts[9].parent_index = 8; */
}

static bool
find_entity_by_id(const struct game_state *game, u32 entity_id, u32 *index)
{
	u32 i = *index;
	if (i < game->entity_count && game->entities[i].id == entity_id)
		return true;

	for (i = 0; i < game->entity_count; ++i) {
		if (game->entities[i].id == entity_id) {
			*index = i;
			return true;
		}
	}
	
	return false;
}


static void
update_game(struct game_state *game,
            const struct input_state *input)
{
	game->event_count = 0;
	
	if (game->time < game->level_begin_t)
		game->tunnel_size = WINDOW_WIDTH * (game->time - game->tunnel_begin_t) / (game->level_begin_t - game->tunnel_begin_t);
	else
		game->tunnel_size = WINDOW_WIDTH;
	
	if (game->time > game->level_end_t)
		goto_level(game, (game->current_level + 1) % (ARRAY_COUNT(BASE_COLORS) - 1));

	struct entity *player = &game->entities[0];
	struct entity_part *root = player->parts;

	{
		u32 entity_index = 0;
		while (entity_index < game->entity_count) {
			struct entity *entity = game->entities + entity_index;
			
			if (entity->disposed) {
				game->entities[entity_index] = game->entities[--game->entity_count];
				game->entities[entity_index].index = entity_index;
				continue;
			}
			
			for (;;) {
				bool found_child_to_dispose = false;
				for (u32 i = 0; i < entity->part_count; ++i) {
					struct entity_part *part = entity->parts + i;
					if (part->disposed)
						continue;

					if (part->parent_index != part->index)
						continue;

					if (entity->parts[part->parent_index].disposed)
						found_child_to_dispose = part->disposed = true;
				}

				if (!found_child_to_dispose)
					break;
			}

			u16 part_index = 0;
			while (part_index < entity->part_count) {
				struct entity_part *part = entity->parts + part_index;
				if (part->disposed) {
					for (u32 i = 0; i < entity->part_count; ++i)
						if (entity->parts[i].parent_index == (entity->part_count - 1))
						    entity->parts[i].parent_index = part_index;

					entity->parts[part_index] = entity->parts[--entity->part_count];
					entity->parts[part_index].index = part_index;
				} else {
					++part_index;
				}
			}

			++entity_index;
		}
	}
	
	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		struct entity *entity = game->entities + entity_index;
		struct entity_part *head = entity->parts;
		assert(!entity->disposed);
		entity->suspended_for_frame = false;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			struct entity_part *part = entity->parts + part_index;
			assert(!part->disposed);
			/* assert(part->index = (u16)part_index); */
			part->suspended_for_frame = false;
			part->a = part->force;
			part->force = v2(0, 0);
		}


		if (entity->type == ENTITY_WORM) {
			if (entity->target_entity_id && find_entity_by_id(game, entity->target_entity_id, &entity->target_entity_index)) {
				struct entity *target = game->entities + entity->target_entity_index;
				entity->target = target->parts[0].p;
				entity->has_target = true;
			} else {
				entity->has_target = false;
				entity->target_entity_id = 0;
			}

			
			if (entity->has_target) {
				struct v2 d = normalize_v2(sub_v2(entity->target, head->p));
				head->a = add_v2(head->a, d);					
			}

			if (game->time > entity->next_target_check_t) {
				f32 min_dist = 100000000;
				for (u32 i = 0; i < game->entity_count; ++i) {
					struct entity *other = game->entities + i;
					if (other->type == ENTITY_FOOD) {
						f32 dist = len_v2(sub_v2(head->p, other->parts->p));
						if (dist < min_dist) {
							min_dist = dist;
							entity->target_entity_id = other->id;
							entity->target_entity_index = i;
						}
					}
				}

				entity->next_target_check_t += 2;
			}

			
		}
	}

	if (input->left)
		root->a.x -= 2;

	if (input->right)
		root->a.x += 2;

	if (input->up)
		root->a.y -= 2;

	if (input->down)
		root->a.y += 2;

	s32 mouse_x, mouse_y;
	u32 mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	if (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		struct v2 m = v2(mouse_x, mouse_y);
		struct v2 d = sub_v2(m, root->p);
		root->a = add_v2(root->a, scale_v2(normalize_v2(d), 2));
	}

	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			struct entity_part *part = entity->parts + part_index;
			if (part_index != part->parent_index) {
				struct entity_part *parent = entity->parts + part->parent_index;
        
				struct v2 offset = sub_v2(part->p, parent->p);
				struct v2 ideal = add_v2(parent->p, scale_v2(normalize_v2(offset), part->length));
				struct v2 d = sub_v2(ideal, part->p);

				const f32 K = 10.0f;
				f32 force = len_v2(d) * K;

				part->a = add_v2(part->a, scale_v2(d, force / part->mass));
				parent->a = add_v2(parent->a, scale_v2(d, -force / parent->mass));
			}
			part->a = sub_v2(part->a, scale_v2(part->v, 0.2f));
		}
	}
	
	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			struct entity_part *part = entity->parts + part_index;
			struct v2 orig_new_v = add_v2(part->v, part->a);
			struct v2 new_v = orig_new_v;
			if (len_v2(new_v) > 10)
				new_v = scale_v2(normalize_v2(new_v), 10);
			
			struct v2 new_p = add_v2(part->p, new_v);
			
			for (u32 other_index = 0; other_index < game->entity_count; ++other_index) {
				if (other_index == entity_index && !entity->internal_collisions)
					continue;

				struct entity *other = game->entities + other_index;
				
				for (u32 other_part_index = 0; other_part_index < other->part_count; ++other_part_index) {
					if (other_index == entity_index && part_index == other_part_index)
						continue;
					
					struct entity_part *op = other->parts + other_part_index;
					bool do_collision_response = false;
					if (test_collision_against_box(op, part, part->p, &new_p, &new_v)) {
						do_collision_response = true;

						if (!entity->suspended_for_frame && !part->suspended_for_frame && !other->suspended_for_frame && !op->suspended_for_frame) {
							try_seed_touch_water(game, entity, part, other, op);
							try_worm_eat_food(game, entity, part, other, op);
						}
					}

					if (do_collision_response) {
						new_v = orig_new_v;
						
						/* struct v2 d = normalize_v2(sub_v2(op->p, part->p)); */
						struct v2 d = normalize_v2(new_v);
						f32 v1 = dot_v2(orig_new_v, d);
						f32 v2 = dot_v2(op->v, d);
						f32 total_mass = part->mass + op->mass;
						f32 dv = v1 - v2;

						f32 f1 = -dv * op->mass / total_mass * 2;
						f32 f2 = dv * part->mass / total_mass * 2;

#if 0
						printf("Collision (%u,%u) <=> (%u,%u)\n", entity_index, part_index, other_index, other_part_index);
						printf("\tV1: %f, V2: %f, M1: %f, M2: %f, DV: %f\n", (f64)v1, (f64)v2, (f64)part->mass, (f64)op->mass, (f64)dv);
						printf("\tF1: %f, F2: %f\n", (f64)(f1), (f64)(f2));
#endif						
						part->force = add_v2(part->force, scale_v2(d, f1));
						op->force = add_v2(op->force, scale_v2(d, f2));
					}
				}
			}

			if (len_v2(new_v) > 10)
				new_v = scale_v2(normalize_v2(new_v), 10);
			
			part->p = new_p;
			part->v = new_v;

			if (isnan(part->p.x))
				part->p.x = 0;
			if (isnan(part->p.y))
				part->p.y = 0;
			
			if (part->p.x < 0) {
				part->p.x = 0;
				part->v.x = -part->v.x * 0.4f;
				part->a.x = 0;
			} else if (part->p.x > WINDOW_WIDTH) {
				part->p.x = WINDOW_WIDTH;
				part->v.x = -part->v.x * 0.4f;
				part->a.x = 0;
			}

			if (part->p.y < 0) {
				part->p.y = 0;
				part->v.y = -part->v.y * 0.4f;
				part->a.y = 0;
			} else if (part->p.y > WINDOW_HEIGHT) {
				part->p.y = WINDOW_HEIGHT;
				part->v.y = -part->v.y * 0.4f;
				part->a.y = 0;
			}
		}
	}

	for (u32 i = 0; i < game->event_count; ++i) {
		struct game_event *e = game->events + i;
		switch (e->type) {
		case GAME_EVENT_SEED_TOUCH_WATER: {
			struct entity *seed = game->entities + e->seed_touch_water.seed_entity_index;
			struct entity *water = game->entities + e->seed_touch_water.water_entity_index;
			struct entity_part *droplet = water->parts + e->seed_touch_water.water_part_index;

			droplet->disposed = true;

			init_food(seed);
		} break;

		case GAME_EVENT_WORM_EAT_FOOD: {
			struct entity *worm = game->entities + e->worm_eat_food.worm_entity_index;
			struct entity *food = game->entities + e->worm_eat_food.food_entity_index;
			food->disposed = true;

			push_worm_tail(worm);
			
		} break;
			
		}
	}

	if (game->time > game->next_note_t) {
		game->next_note_t += 4;
		if (game->note == 40)
			game->note = 80;
		else if (game->note == 80)
			game->note = 60;
		else
			game->note = 40;
	}
	
	SDL_LockAudioDevice(1);
	u32 wave_index = 0;
	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			struct entity_part *part = entity->parts + part_index;
		
			struct waveform *sine = game->sine_waves + wave_index;
			struct waveform *saw = game->saw_waves + wave_index;

			f32 v = len_v2(part->v);
			f32 sqrt_v = sqrtf(v);
			
			sine->amp = sqrt_v / 400.0f;
			if (sine->amp > 0.25f)
				sine->amp = 0.25f;
			sine->freq = (u16)((roundf(v * 10 / part->size)) * (f32)game->note);

			saw->amp = sqrt_v / 400.0f; /* part->size / 1000.0f; */
			if (saw->amp > 0.25f)
				saw->amp = 0.25f;
			
			saw->freq = (u16)((roundf(v * 100 / part->mass)) * 4 * (f32)game->note); /* (u16)(roundf(len_v2(part->v)) * 40); */
		
			++wave_index;
		}
	}
	game->sine_wave_count = wave_index;
	game->saw_wave_count = wave_index;
	SDL_UnlockAudioDevice(1);
}

static void
render_game(const struct game_state *game,
            SDL_Renderer *renderer,
            TTF_Font *font,
	    TTF_Font *small_font)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	
	struct color white = color(0xFF, 0xFF, 0xFF, 0xFF);

	f32 elapsed_t = game->time - game->level_begin_t;
	f32 level_progress = elapsed_t / (game->level_end_t - game->level_begin_t);
	if (level_progress < 0)
		level_progress = 0;
	
	struct v2 o = v2(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);
	f32 len_o = len_v2(o) + 100;

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	{
		f32 initial_r = len_o * level_progress * level_progress;
		f32 r = initial_r;
		f32 a = 0.25f * game->time;
		while (r < game->tunnel_size) {
			struct v2 p = add_v2(o, v2(r * cosf(a), r * sinf(a)));

			u8 max_alpha = (u8)(0xE0 * sqrtf(r / len_o));
			u8 alpha = max_alpha;
			if (game->time < game->level_begin_t) {
				f32 tunnel_d = game->level_begin_t - game->tunnel_begin_t;
				f32 fade_progress = (game->time - game->tunnel_begin_t) / tunnel_d;
				alpha = (u8)(max_alpha * fade_progress);
			}
			
			special_fill_cell_(renderer, (u8)(game->current_level + 1), alpha, (s32)p.x, (s32)p.y, (s32)(r / 5.0f), (s32)(r / 5.0f));
			/* r += 0.5f; */
			r += 0.25f + sqrtf(r - initial_r) * 0.01f;
			a += sqrtf(r - initial_r) * 0.1f;
		}
	}

	/* SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); */
	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		const struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			const struct entity_part *part = entity->parts + part_index;
			const struct entity_part *parent = entity->parts + part->parent_index;

			struct v2 ep = parent->p;
			struct v2 pp = part->p;

			struct v2 d = sub_v2(pp, ep);
        
			/* u32 chain_count = (u32)(part->length / 20); */
			/* for (u32 chain_index = 0; chain_index < chain_count; ++chain_index) { */
			/* 	f32 r = (f32)chain_index / (f32)chain_count; */
			/* 	struct v2 p = add_v2(ep, scale_v2(d, r)); */
			/* 	fill_cell(renderer, 3, (s32)p.x, (s32)p.y, 12, 12); */
			/* } */

			struct v2 from_c = sub_v2(ep, o);
			f32 dist_to_center = len_v2(from_c);
			f32 scale = 2 * dist_to_center / len_o;

			struct v2 offset = scale_v2(normalize_v2(from_c), 30);
			struct v2 shadow = add_v2(pp, offset);

			u8 max_alpha = (u8)(0xE0 * sqrtf(dist_to_center / len_o));
			u8 alpha = max_alpha;
			if (game->time < game->level_begin_t) {
				f32 tunnel_d = game->level_begin_t - game->tunnel_begin_t;
				f32 fade_progress = (game->time - game->tunnel_begin_t) / tunnel_d;
				alpha = (u8)(max_alpha * fade_progress);
			}
		
			struct color c = color(0x00, 0x00, 0x00, alpha);
			render_rect(renderer, c, (s32)shadow.x, (s32)shadow.y, (s32)(part->render_size * scale), (s32)(part->render_size * scale), false);
		}
	}
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		const struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			const struct entity_part *part = entity->parts + (entity->part_count - part_index - 1);
			const struct entity_part *parent = entity->parts + part->parent_index;

			struct v2 ep = parent->p;
			struct v2 pp = part->p;

			struct v2 d = sub_v2(pp, ep);
        
			u32 chain_count = (u32)(part->length / 20);

			for (u32 chain_index = 0; chain_index < chain_count; ++chain_index) {
				f32 r = (f32)chain_index / (f32)chain_count;
				struct v2 p = add_v2(ep, scale_v2(d, r));
				fill_cell(renderer, 3, (s32)p.x, (s32)p.y, 12, 12);
			}

			fill_cell(renderer, (u8)part->color, (s32)pp.x, (s32)pp.y, (s32)part->render_size, (s32)part->render_size);
		}
	}

	/* draw_string(renderer, font, "LD48 - InvertedMinds", 5, 5, TEXT_ALIGN_LEFT, white); */
	draw_string_f(renderer, small_font, 5, 5, TEXT_ALIGN_LEFT, white, "T: %f", (f64)game->time);

	if (game->time < game->tunnel_begin_t) {
		f32 fade_in_d = 1;
		f32 fade_in_start_t = game->last_level_end_t;
		f32 fade_in_end_t = fade_in_start_t + fade_in_d;

		f32 fade_out_d = 1;
		f32 fade_out_start_t = game->tunnel_begin_t - fade_out_d;
		f32 fade_out_end_t = game->tunnel_begin_t;

		f32 alpha = 0xFF;
		if (game->time >= fade_in_start_t && game->time <= fade_in_end_t)
			alpha = (0xFF * (game->time - game->last_level_end_t) / fade_in_d);
		if (game->time >= fade_out_start_t && game->time <= fade_out_end_t)
			alpha = (0xFF * (1 - (game->time - fade_out_start_t) / fade_out_d));

		if (alpha > 0xFF)
			alpha = 0xFF;
		else if (alpha < 0)
			alpha = 0;

		if (alpha > 1) {
			struct color c = color(0xFF, 0xFF, 0xFF, (u8)alpha);
			draw_string_f(renderer, font, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 9, TEXT_ALIGN_CENTER, c, "LEVEL %u", game->current_level + 1);
		}
	}


#if 0
	s32 y = 5 + SMALL_FONT_SIZE;
	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		const struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			const struct entity_part *part = entity->parts + (entity->part_count - part_index - 1);
			draw_string_f(renderer, small_font, 5, y, TEXT_ALIGN_LEFT, white, "E (%u, %u): (%f, %f)", entity_index, part_index, (f64)part->p.x, (f64)part->p.y);
			y += SMALL_FONT_SIZE;
		}
	}
#endif
	
	
	SDL_RenderPresent(renderer);
}




static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_AudioDeviceID audio;
static const char *font_name;
static TTF_Font *font;
static TTF_Font *small_font;

static struct game_state *game;
static struct input_state input;
static bool quit;

static s32 window_w, window_h, renderer_w, renderer_h;
static f32 default_scale = 1;


static void
update_and_render()
{
	game->time = (f32)(SDL_GetTicks()) / 1000.0f;
        
	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
		if (e.type == SDL_QUIT)
			quit = true;

	s32 key_count;
	const u8 *key_states = SDL_GetKeyboardState(&key_count);

	if (key_states[SDL_SCANCODE_ESCAPE])
		quit = true;
        
	struct input_state prev_input = input;
        
	input.left = key_states[SDL_SCANCODE_LEFT];
	input.right = key_states[SDL_SCANCODE_RIGHT];
	input.up = key_states[SDL_SCANCODE_UP];
	input.down = key_states[SDL_SCANCODE_DOWN];
	input.a = key_states[SDL_SCANCODE_SPACE];

	input.dleft = (s8)(input.left - prev_input.left);
	input.dright = (s8)(input.right - prev_input.right);
	input.dup = (s8)(input.up - prev_input.up);
	input.ddown = (s8)(input.down - prev_input.down);
	input.da = (s8)(input.a - prev_input.a);
        
	update_game(game, &input);
	render_game(game, renderer, font, small_font);
}

static void
mixaudio(void *unused, Uint8 *stream, int len)
{
	/* struct game_state *game = unused; */

	u32 length = (u32)(len / 4);
	f32 *s = (f32 *)(void *)stream;

	for (u32 i = 0; i < length; ++i) {
		f32 mix = 0;
		for (u32 wave_index = 0; wave_index < game->sine_wave_count; ++wave_index) {
			struct waveform *wave = game->sine_waves + wave_index;
			f32 w = sinf(wave->t * 2.0f * 3.14f / AUDIO_FREQ) * wave->amp;
			mix += w;
			wave->t += wave->freq;
		}

		for (u32 wave_index = 0; wave_index < game->saw_wave_count; ++wave_index) {
			struct waveform *wave = game->saw_waves + wave_index;
			f32 w = 0;
			if (!IS_F32_ZERO(wave->amp))
				w = fmodf(wave->amp * wave->t / AUDIO_FREQ, wave->amp) - wave->amp / 2;
			
			mix += w;
			wave->t += wave->freq;
		}
		
		if (mix < -1.0f)
			mix = -1.0f;
		else if (mix > 1.0f)
			mix = 1.0f;

		s[i] = mix;
	}

#if 0
	for (u32 i = 0; i < length; ++i) {
		s[i] = 0;
		for (u32 j = 0; j < game->entity.part_count; ++j) {
			struct entity_part *p = game->entity.parts + j;
						
			f32 w1 = sinf(p->wave * 2.0f * 3.14f / AUDIO_FREQ) * (p->size / 1000.0f);
			f32 w2 = 0;

			f32 amp = (p->size / 1000.0f);
			if (!IS_F32_ZERO(amp))
				w2 = fmodf(amp * p->wave2 / AUDIO_FREQ, amp) - amp / 2;

			f32 w = w1 + w2;
			if (w < -1.0f)
				w = -1.0f;
			else if (w > 1.0f)
				w = 1.0f;
			
			s[i] += w;

			p->wave += (u16)((roundf(len_v2(p->v) / p->size)) * 40);
			p->wave2 += (u16)(roundf(len_v2(p->v)) * 40);
		}
	}
#endif
}



int
main()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	if (TTF_Init() < 0)
		return 2;
	
	window_w = WINDOW_WIDTH;
	window_h = WINDOW_HEIGHT;
	
	window = SDL_CreateWindow(
		"LD48 -- InvertedMinds",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window_w,
		window_h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	
	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	SDL_GetRendererOutputSize(renderer, &renderer_w, &renderer_h);

	printf("Render Size: %u, %u\n", renderer_w, renderer_h);
	
	default_scale = (f32)renderer_w / (f32)window_w;
	/* SDL_RenderSetScale(renderer, default_scale, default_scale); */
	
	font_name = "novem___.ttf";
	font = TTF_OpenFont(font_name, FONT_SIZE);
	small_font = TTF_OpenFont(font_name, SMALL_FONT_SIZE);

	game = (struct game_state *)malloc(sizeof(struct game_state));
	ZERO_STRUCT(*game);
	/* game->level_end_t = -5; */
	goto_level(game, 0);

	init_squid(push_entity(game));

	/* { */
	/* 	struct entity *entity = game->entities + (game->entity_count++); */
	/* 	entity->part_count = 1; */
	/* 	entity->parts[0].length = 0; */
	/* 	entity->parts[0].size = 50; */
	/* 	entity->parts[0].color = 8; */
	/* 	entity->parts[0].p = v2(500, 500); */
	/* } */

	{
		init_seed(push_entity(game));
		init_seed(push_entity(game));
		init_seed(push_entity(game));
		init_seed(push_entity(game));
	}
	
	{
		struct entity *entity = push_entity(game);
		entity->type = ENTITY_WORM;

		struct entity_part *p;
		p = push_entity_part(entity);
		p->length = 0;
		p->size = 40;
		p->color = 4;
		p->p = v2(500, 500);

		for (u32 i = 1; i < 4; ++i) {
			push_worm_tail(entity);
			/* p = push_entity_part(entity); */
			/* p->length = 25; */
			/* p->size = (u16)(40 - i); */
			/* p->color = 4; */
			/* p->p = add_v2(entity->parts[i - 1].p, v2(25, 0)); */
			/* p->parent_index = (u16)(i - 1); */
		}
	}
	
	{
		struct entity *entity = push_entity(game);
		entity->type = ENTITY_WATER;
		/* entity->internal_collisions = true; */

		struct entity_part *p;
		p = push_entity_part(entity);
		p->length = 0;
		p->size = 10;
		p->render_size = 20;
		p->color = 8;
		p->p = v2(700, 200);

		for (u32 i = 1; i < 8; ++i) {
			p = push_entity_part(entity);
			p->length = 20;
			p->size = 10;
			p->render_size = 20;
			/* p->mass = 5; */
			p->color = 8;
			p->p = add_v2(entity->parts[i - 1].p, v2(cosf((f32)i), sinf((f32)i)));
			p->parent_index = 0;
		}
	}
	
	for (u32 entity_index = 0; entity_index < game->entity_count; ++entity_index) {
		struct entity *entity = game->entities + entity_index;
		for (u32 part_index = 0; part_index < entity->part_count; ++part_index) {
			struct entity_part *part = entity->parts + part_index;
			if (IS_F32_ZERO(part->mass))
				part->mass = (f32)(part->size * part->size);

			if (!part->render_size)
				part->render_size = part->size;
		}
	}
	
	ZERO_STRUCT(input);

	SDL_AudioSpec fmt = { 0 };
	fmt.freq = AUDIO_FREQ;
	fmt.format = AUDIO_F32;
	fmt.channels = 1;
	fmt.samples = 1024;
	fmt.callback = mixaudio;
	fmt.userdata = game;

	SDL_AudioSpec obt;
	
	if (SDL_OpenAudio(&fmt, &obt) < 0)
		return 3;

	audio = 1;
	
	SDL_PauseAudio(0);
	
#if defined(__EMSCRIPTEN__)
	emscripten_set_main_loop(update_and_render, 0, 1);
#else
	while (!quit)
		update_and_render();
#endif
	
	SDL_CloseAudio();

	
	TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();

	return 0;
}
