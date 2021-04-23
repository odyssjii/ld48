#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

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

typedef u32 b32;

#include "colors.h"

#define WIDTH 10
#define HEIGHT 22
#define VISIBLE_HEIGHT 20
#define GRID_SIZE 10

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
	{
		return v2(0, 0);
	}
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

struct entity_part
{
	u16 length;
	u16 size;
	u16 color;
	u16 parent_index;
	struct v2 p;
	struct v2 v;
	struct v2 a;
};

struct entity
{
	/* struct v2 p; */
	/* struct v2 v; */
	/* struct v2 a; */
	struct entity_part parts[20];
	u32 part_count;
};

struct game_state
{
	struct entity entity;
	f32 time;
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

static inline s32
random_int(s32 min, s32 max)
{
	s32 range = max - min;
	return min + rand() % range;
}

static inline s32
min(s32 x, s32 y)
{
	return x < y ? x : y;
}

static inline s32
max(s32 x, s32 y)
{
	return x > y ? x : y;
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
	switch (alignment)
	{
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
render_cell(SDL_Renderer *renderer,
            u8 value, s32 offset_x, s32 offset_y, s32 w, s32 h,
            b32 outline)
{
	struct color base_color = BASE_COLORS[value];
	struct color light_color = LIGHT_COLORS[value];
	struct color dark_color = DARK_COLORS[value];
   
	s32 edge = 4; // GRID_SIZE / 8;

	s32 x = (s32)(round(offset_x - (w / 2.0)));
	s32 y = (s32)(round(offset_y - (h / 2.0)));
    
	if (outline)
	{
		draw_rect(renderer, x, y, w, h, base_color);
		return;
	}
    
	fill_rect(renderer, x, y, w, h, dark_color);
	fill_rect(renderer, x + edge, y,
		  w - edge, h - edge, light_color);
	fill_rect(renderer, x + edge, y + edge,
		  w - edge * 2, h - edge * 2, base_color); 
}

static inline void
fill_cell(SDL_Renderer *renderer,
          u8 value, s32 x, s32 y, s32 w, s32 h)
{
	render_cell(renderer, value, x, y, w, h, 0);
}

static inline void
draw_cell(SDL_Renderer *renderer,
          u8 value, s32 x, s32 y, s32 w, s32 h)
{
	render_cell(renderer, value, x, y, w, h, 1);
}

static void
update_game(struct game_state *game,
            const struct input_state *input)
{
	struct entity *entity = &game->entity;
	struct entity_part *root = entity->parts;
    
	root->a = v2(0, 0);
	if (input->left)
	{
		root->a.x -= 1;
	}
	if (input->right)
	{
		root->a.x += 1;
	}
	if (input->up)
	{
		root->a.y -= 1;
	}
	if (input->down)
	{
		root->a.y += 1;
	}
    
	/* entity->v = add_v2(entity->v, entity->a); */
	/* entity->p = add_v2(entity->p, entity->v); */
    
	for (u32 part_index = 0;
	     part_index < entity->part_count;
	     ++part_index)
	{
		struct entity_part *part = entity->parts + part_index;
		if (part_index != part->parent_index)
		{
			struct entity_part *parent = entity->parts + part->parent_index;
        
			struct v2 offset = sub_v2(part->p, parent->p);
			struct v2 ideal = add_v2(parent->p, scale_v2(normalize_v2(offset), part->length));
			struct v2 d = sub_v2(ideal, part->p);
        
			part->a = sub_v2(scale_v2(d, 0.1f), scale_v2(part->v, 0.1f));
		}
		part->v = add_v2(part->v, part->a);
		part->p = add_v2(part->p, part->v);
	}
}

static void
render_game(const struct game_state *game,
            SDL_Renderer *renderer,
            TTF_Font *font)
{
	struct color c = color(0xFF, 0xFF, 0xFF, 0xFF);

	draw_string(renderer, font, "Simple spring physics", 5, 5, TEXT_ALIGN_LEFT, c);

	// struct v2 ep = game->entity.p;

	/* fill_cell(renderer, 1, (s32)ep.x, (s32)ep.y, 50, 50); */
	for (u32 part_index = 0;
	     part_index < game->entity.part_count;
	     ++part_index)
	{
		const struct entity_part *part = game->entity.parts + part_index;
		const struct entity_part *parent = game->entity.parts + part->parent_index;

		struct v2 ep = parent->p;
		struct v2 pp = part->p;

		struct v2 d = sub_v2(pp, ep);
        
		u32 chain_count = (u32)(part->length / 20);

		for (u32 chain_index = 0;
		     chain_index < chain_count;
		     ++chain_index)
		{
			f32 r = (f32)(chain_index ) / chain_count;
			struct v2 p = add_v2(ep, scale_v2(d, r));
			fill_cell(renderer, 3, (s32)p.x, (s32)p.y, 12, 12);
		}

		fill_cell(renderer, (u8)part->color, (s32)pp.x, (s32)pp.y, (s32)part->size, (s32)part->size);
	}
}

static struct entity_part *
add_part(struct entity *entity)
{
	assert(entity->part_count < ARRAY_COUNT(entity->parts));
	return entity->parts + (entity->part_count++);
}




static SDL_Window *window;
static SDL_Renderer *renderer;
static const char *font_name;
static TTF_Font *font;
static struct game_state *game;
static struct input_state input;
static bool quit;

static s32 window_w, window_h, renderer_w, renderer_h;
static s32 default_scale = 1;

static void
update_and_render()
{
	game->time = SDL_GetTicks() / 1000.0f;
        
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
        
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
        
	update_game(game, &input);
	render_game(game, renderer, font);

	SDL_RenderPresent(renderer);
}

int
main()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return 1;

	if (TTF_Init() < 0)
		return 2;

	window_w = 1280;
	window_h = 720;
	
	window = SDL_CreateWindow(
		"Tetris",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window_w,
		window_h,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
	
	renderer = SDL_CreateRenderer(
		window,
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	SDL_GetRendererOutputSize(renderer, &renderer_w, &renderer_h);

	default_scale = renderer_w / window_w;
	SDL_RenderSetScale(renderer, default_scale, default_scale);
	
	font_name = "novem___.ttf";
	font = TTF_OpenFont(font_name, 24);

	game = (struct game_state *)malloc(sizeof(struct game_state));
	ZERO_STRUCT(*game);
	
	game->entity.part_count = 10;
	game->entity.parts[0].length = 0;
	game->entity.parts[0].size = 50;
	game->entity.parts[0].color = 1;
	game->entity.parts[0].p = v2(100, 100);
    
	game->entity.parts[1].length = 100;
	game->entity.parts[1].size = 25;
	game->entity.parts[1].color = 2;
	game->entity.parts[1].p = v2(70, 70);

	game->entity.parts[2].length = 50;
	game->entity.parts[2].size = 25;
	game->entity.parts[2].color = 2;
	game->entity.parts[2].p = v2(-40, 20);

	game->entity.parts[3].length = 50;
	game->entity.parts[3].size = 20;
	game->entity.parts[3].color = 4;
	game->entity.parts[3].parent_index = 2;

	game->entity.parts[4].length = 50;
	game->entity.parts[4].size = 20;
	game->entity.parts[4].color = 5;
	game->entity.parts[4].parent_index = 3;

	game->entity.parts[5].length = 50;
	game->entity.parts[5].size = 20;
	game->entity.parts[5].color = 6;
	game->entity.parts[5].parent_index = 4;
    

	game->entity.parts[6].length = 60;
	game->entity.parts[6].size = 25;
	game->entity.parts[6].color = 2;
	game->entity.parts[6].p = v2(-40, 20);

	game->entity.parts[7].length = 60;
	game->entity.parts[7].size = 20;
	game->entity.parts[7].color = 4;
	game->entity.parts[7].parent_index = 6;

	game->entity.parts[8].length = 60;
	game->entity.parts[8].size = 20;
	game->entity.parts[8].color = 5;
	game->entity.parts[8].parent_index = 7;

	game->entity.parts[9].length = 60;
	game->entity.parts[9].size = 20;
	game->entity.parts[9].color = 6;
	game->entity.parts[9].parent_index = 8;

	ZERO_STRUCT(input);
	

#if defined(__EMSCRIPTEN__)
	emscripten_set_main_loop(update_and_render, 0, 1);
#else
	while (!quit)
		update_and_render();
#endif
        
	TTF_CloseFont(font);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();

	return 0;
}
