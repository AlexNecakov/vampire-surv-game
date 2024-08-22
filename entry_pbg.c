//:constants
#define MAX_ENTITY_COUNT 1024
#define MAX_PLAYER_COUNT 4
#define MAX_MONSTER_COUNT MAX_ENTITY_COUNT
#define MAX_ACTION_COUNT 1024

const u32 font_height = 64;
const float32 font_padding = (float32)font_height/10.0f;

const float32 spriteSheetWidth = 240.0;
const s32 tile_width = 16;

const s32 layer_stage_bg = 0;
const s32 layer_stage_fg = 5;
const s32 layer_world = 10;
const s32 layer_entity = 20;
const s32 layer_ui_bg = 30;
const s32 layer_ui_fg = 35;
const s32 layer_text = 40;
const s32 layer_cursor = 50;

Vector4 bg_box_col = {0, 0, 1.0, 0.9};

float screen_width = 240.0;
float screen_height = 135.0;

//:math
inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

float sin_breathe(float time, float rate) {
	return (sin(time * rate) + 1.0) / 2.0;
}

bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

//:coordinate conversion
typedef struct WorldFrame {
	Matrix4 world_proj;
	Matrix4 world_view;
} WorldFrame;
WorldFrame world_frame;

void set_screen_space() {
	draw_frame.camera_xform = m4_scalar(1.0);
	draw_frame.projection = m4_make_orthographic_projection(0.0, screen_width, 0.0, screen_height, -1, 10);
}
void set_world_space() {
	draw_frame.projection = world_frame.world_proj;
	draw_frame.camera_xform = world_frame.world_view;
}

Vector2 world_to_screen(Vector2 world_pos){
    Vector2 screen_pos;
    return screen_pos;
}
Vector2 screen_to_world(Vector2 screen_pos) {
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (screen_pos.x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (screen_pos.y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	// log("%f, %f", world_pos.x, world_pos.y);

	// Return as 2D vector
	return (Vector2){ world_pos.x, world_pos.y };
}

int world_pos_to_tile_pos(float world_pos) {
	return roundf(world_pos / (float)tile_width);
}

float tile_pos_to_world_pos(int tile_pos) {
	return ((float)tile_pos * (float)tile_width);
}

Vector2 round_v2_to_tile(Vector2 world_pos) {
	world_pos.x = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.x));
	world_pos.y = tile_pos_to_world_pos(world_pos_to_tile_pos(world_pos.y));
	return world_pos;
}

Vector2 get_mouse_pos_in_ndc() {
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	return (Vector2){ ndc_x, ndc_y };
}

Draw_Quad ndc_quad_to_screen_quad(Draw_Quad ndc_quad) {
	// NOTE: we're assuming these are the screen space matricies.
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;

	Matrix4 ndc_to_screen_space = m4_scalar(1.0);
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, m4_inverse(proj));
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, view);

	ndc_quad.bottom_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_left), 0, 1)).xy;
	ndc_quad.bottom_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_right), 0, 1)).xy;
	ndc_quad.top_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_left), 0, 1)).xy;
	ndc_quad.top_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_right), 0, 1)).xy;

	return ndc_quad;
}

//:animate
bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, -rate * delta_t));
	if (almost_equals(*value, target, 0.001f))
	{
		*value = target;
		return true; // reached
	}
	return false;
}

void animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	animate_f32_to_target(&(value->x), target.x, delta_t, rate);
	animate_f32_to_target(&(value->y), target.y, delta_t, rate);
}

//:sprite
typedef struct Sprite {
    Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_citizen,
    SPRITE_MAX,
} SpriteID;

Sprite sprites[SPRITE_MAX];

Sprite* get_sprite(SpriteID id){
    if (id >= 0 && id < SPRITE_MAX){
    	Sprite* sprite = &sprites[id];
		if (sprite->image) {
			return sprite;
		} else {
			return &sprites[0];
		}
    }
    return &sprites[0];
}

Vector2 get_sprite_size(Sprite* sprite) {
	return (Vector2) { sprite->image->width, sprite->image->height };
}

//:ux state
typedef enum UXState {
	UX_nil,
	UX_default,
	UX_debug,
} UXState;

//:bar
typedef struct Bar {
    float64 max;
    float64 current;
    float64 rate;
} Bar;


//:entity
typedef enum EntityArchetype{
    ARCH_nil = 0,
    ARCH_player,
    ARCH_citizen,
    ARCH_text,
    ARCH_MAX,
} EntityArchetype;

typedef enum EntityState{
    ENT_nil = 0,
    ENT_standing,
    ENT_attacking,
} EntityState;

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
    EntityState state;
	string name;
    SpriteID sprite_id;
    bool render_sprite;
    Vector2 current_pos;
    Vector4 color;
} Entity;

//:world
typedef struct World{
	Entity entities[MAX_ENTITY_COUNT];
	UXState ux_state;
	Matrix4 world_proj;
	Matrix4 world_view;
    bool debug_render;
    Gfx_Font* font;
} World;
World* world = 0;

Entity* entity_create() {
    Entity* entity_found = 0;
    for (int i = 0; i < MAX_ENTITY_COUNT; i++){
        Entity* existing_entity = &world->entities[i];
        if (!existing_entity->is_valid){
            entity_found = existing_entity;
            break;
        }
    }
    assert(entity_found, "No more free entities!");
    entity_found->is_valid = true;
    return entity_found;
}

void entity_destroy(Entity* entity){
    memset(entity, 0, sizeof(Entity));
}

void setup_player(Entity* en) {
    en->arch = ARCH_player;
    en->sprite_id = SPRITE_player;
    en->color = COLOR_WHITE;
    en->name = STR("Player");
    en->state = ENT_standing;
}

void setup_text(Entity* en, string name){
    en->arch = ARCH_text;
    en->name = name;
    en->color = COLOR_WHITE;
}

void setup_citizen(Entity* en) {
    en->arch = ARCH_citizen;
    en->sprite_id = SPRITE_citizen;
    en->color = COLOR_WHITE;
    en->state = ENT_standing;
}


void render_sprite_entity(Entity* en){
    Sprite* sprite = get_sprite(en->sprite_id);
    Matrix4 xform = m4_scalar(1.0);
    xform         = m4_translate(xform, v3(0, tile_width * -0.5, 0));
    xform         = m4_translate(xform, v3(en->current_pos.x, en->current_pos.y, 0));
    xform         = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0.0, 0));
    draw_image_xform(sprite->image, xform, get_sprite_size(sprite), en->color);

    if(world->debug_render){
        draw_text(world->font, sprint(temp_allocator, STR("%f %f"), en->current_pos.x, en->current_pos.y), font_height, en->current_pos, v2(0.1, 0.1), COLOR_WHITE);
    }
}

//:entry
int entry(int argc, char **argv) {
	
	window.title = STR("Secret Identity");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = hex_to_rgba(0x1e1e1eff);
    float32 aspectRatio = (float32)window.width/(float32)window.height; 
    float32 zoom = window.width/spriteSheetWidth;
    float y_pos = (screen_height/3.0f) - 9.0f;
    
    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));
    world->ux_state = UX_default;
    world->debug_render = true;
    world->font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(world->font, "Failed loading arial.ttf, %d", GetLastError());	
	render_atlas_if_not_yet_rendered(world->font, 32, 'A');

    sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\undefined.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\mannequin.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\Fighter.png"), get_heap_allocator()) };
    sprites[SPRITE_citizen] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\metroid.png"), get_heap_allocator()) };
	
    // @ship debug this off
	{
		for (SpriteID i = 0; i < SPRITE_MAX; i++) {
			Sprite* sprite = &sprites[i];
			assert(sprite->image, "Sprite was not setup properly");
		}
	}
	
    Entity* player_en = entity_create();
    setup_player(player_en);

    for (int i = 0; i < 4; i++) {
        Entity* en = entity_create();
        setup_citizen(en);
        en->current_pos = v2(-2 * i * tile_width, -i * tile_width + tile_width);
        en->current_pos = round_v2_to_tile(en->current_pos);
    }

    Entity* fps_count_en = entity_create();
    setup_text(fps_count_en, STR("fps: 0.0"));
    fps_count_en->current_pos = v2(0, screen_height - (font_height + font_padding) * 0.1);

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    s32 last_fps = 0;
    float64 last_time = os_get_elapsed_seconds();

    //:loop
    while (!window.should_close) {
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
        float64 now = os_get_elapsed_seconds();
		float64 delta = now - last_time;
		last_time = now;	
        os_update(); 

        //:frame updating
        draw_frame.enable_z_sorting = true;
		world_frame.world_proj = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
        
        //:camera
		{
			world_frame.world_view = m4_make_scale(v3(1.0, 1.0, 1.0));
			world_frame.world_view = m4_mul(world_frame.world_view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		}

        //:tile rendering
		{
		    set_world_space();
		    push_z_layer(layer_world);

			int player_tile_x = world_pos_to_tile_pos(0);
			int player_tile_y = world_pos_to_tile_pos(0);
			int tile_radius_x = 40;
			int tile_radius_y = 30;
			for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
					if ((x + (y % 2 == 0) ) % 2 == 0) {
						Vector4 col = v4(0.6, 0.6, 0.6, 0.1);
						float x_pos = x * tile_width;
						float tile_y_pos = y * tile_width;
						draw_rect(v2(x_pos + tile_width * -0.5, tile_y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
					}
				}
			}
            pop_z_layer();
		}

        //:entity loop 
        {
            for (int i = 0; i < MAX_ENTITY_COUNT; i++){
                Entity* en = &world->entities[i];
                if (en->is_valid){
                    switch (en->arch){
                        case ARCH_player:
		                    set_world_space();
                            push_z_layer(layer_world);
                            render_sprite_entity(en);
                            break;
                        case ARCH_citizen:
		                    set_world_space();
                            push_z_layer(layer_world);
                            render_sprite_entity(en);
                            break;
                        case ARCH_text: 
                            set_screen_space();
                            push_z_layer(layer_text);
                            Matrix4 xform = m4_scalar(1.0);
                            string text = en->name;
                            xform = m4_translate(xform, v3(en->current_pos.x, en->current_pos.y, 0));
                            draw_text_xform(world->font, text, font_height, xform, v2(0.1, 0.1), en->color);
                            break;
                        default:
		                    set_world_space();
                            push_z_layer(layer_world);
                            break;
                    }
                    pop_z_layer();
                }
                   
            }
        }

        //:input
        {
            //check exit cond first
            if (is_key_just_pressed(KEY_ESCAPE)){
                window.should_close = true;
            }
            	    
        }

        //:fps
        if(world->debug_render){
            seconds_counter += delta;
            frame_count+=1;
            if(seconds_counter > 1.0){
                last_fps = frame_count;
                frame_count = 0;
                seconds_counter = 0.0;
            }
            string text = STR("fps: %i");
            text = sprint(temp_allocator, text, last_fps);
            fps_count_en->name = text;
        }

        gfx_update();
	}

	return 0;
}
