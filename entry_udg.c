//:constants
#define MAX_ENTITY_COUNT 1024
const u32 font_height = 48;
const float32 spriteSheetWidth = 240.0;
const int tile_width = 16;
const s32 layer_ui = 30;
const s32 layer_world = 10;
const s32 layer_entity = 20;
const s32 layer_cursor = 50;
Vector4 bg_box_col = {0, 0, 1.0, 0.5};
float screen_width = 240.0;
float screen_height = 135.0;

//:math
#define m4_identity m4_make_scale(v3(1, 1, 1))

inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

Vector2 range2f_get_center(Range2f r) {
	return (Vector2) { (r.max.x - r.min.x) * 0.5 + r.min.x, (r.max.y - r.min.y) * 0.5 + r.min.y };
}

float sin_breathe(float time, float rate) {
	return (sin(time * rate) + 1.0) / 2.0;
}

bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

//:coordinate conversion
Draw_Quad ndc_quad_to_screen_quad(Draw_Quad ndc_quad) {

	// NOTE: we're assuming these are the screen space matricies.
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;

	Matrix4 ndc_to_screen_space = m4_identity;
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, m4_inverse(proj));
	ndc_to_screen_space = m4_mul(ndc_to_screen_space, view);

	ndc_quad.bottom_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_left), 0, 1)).xy;
	ndc_quad.bottom_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.bottom_right), 0, 1)).xy;
	ndc_quad.top_left = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_left), 0, 1)).xy;
	ndc_quad.top_right = m4_transform(ndc_to_screen_space, v4(v2_expand(ndc_quad.top_right), 0, 1)).xy;

	return ndc_quad;
}

Range2f quad_to_range(Draw_Quad quad) {
	return (Range2f){quad.bottom_left, quad.top_right};
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
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	return (Vector2){ ndc_x, ndc_y };
}

Vector2 screen_to_world() {
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;
	float window_w = window.width;
	float window_h = window.height;

	// Normalize the mouse coordinates
	float ndc_x = (mouse_x / (window_w * 0.5f)) - 1.0f;
	float ndc_y = (mouse_y / (window_h * 0.5f)) - 1.0f;

	// Transform to world coordinates
	Vector4 world_pos = v4(ndc_x, ndc_y, 0, 1);
	world_pos = m4_transform(m4_inverse(proj), world_pos);
	world_pos = m4_transform(view, world_pos);
	// log("%f, %f", world_pos.x, world_pos.y);

	// Return as 2D vector
	return (Vector2){ world_pos.x, world_pos.y };
}

void set_screen_space() {
	draw_frame.view = m4_scalar(1.0);
	draw_frame.projection = m4_make_orthographic_projection(0.0, screen_width, 0.0, screen_height, -1, 10);
}
void set_world_space() {
	draw_frame.projection = world_frame.world_proj;
	draw_frame.view = world_frame.world_view;
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

//:world
typedef struct World{
	Entity entities[MAX_ENTITY_COUNT];
	UXState ux_state;
	Matrix4 world_proj;
	Matrix4 world_view;
} World;
World* world = 0;

//:sprite
typedef struct Sprite {
    Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_cursor,
    SPRITE_attack,
    SPRITE_rock,
    SPRITE_spider,
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

//:entity
typedef enum EntityArchetype{
    arch_nil = 0,
    arch_player,
    arch_cursor,
    arch_attack,
    arch_rock,
    arch_monster,
    ARCH_MAX,
} EntityArchetype;

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
    Vector2 pos;
    bool render_sprite;
    SpriteID sprite_id;
} Entity;

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
    en->arch = arch_player;
    en->sprite_id = SPRITE_player;
}

void setup_cursor(Entity* en) {
    en->arch = arch_cursor;
    en->sprite_id = SPRITE_cursor;
}

void setup_spider(Entity* en) {
    en->arch = arch_monster;
    en->sprite_id = SPRITE_spider;
}

void setup_rock(Entity* en) {
    en->arch = arch_rock;
    en->sprite_id = SPRITE_rock;
}

//:item data
typedef struct ItemData {
	int amount;
} ItemData; 

//:ux state
typedef enum UXState {
	UX_nil,
	UX_default,
	UX_debug,
} UXState;

//:entry
int entry(int argc, char **argv) {
	
	window.title = STR("Endless Fantasy");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = hex_to_rgba(0x1e1e1eff);
    float32 aspectRatio = (float32)window.width/(float32)window.height; 
    float32 zoom = window.width/spriteSheetWidth;
    
    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));
    world->ux_state = UX_default;

    sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\undefined.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\dude.png"), get_heap_allocator()) };
    sprites[SPRITE_cursor] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\arrow.png"), get_heap_allocator()) };
    sprites[SPRITE_attack] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\attack.png"), get_heap_allocator()) };
    sprites[SPRITE_rock] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\rock.png"), get_heap_allocator()) };
    sprites[SPRITE_spider] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\spider.png"), get_heap_allocator()) };
	
    // @ship debug this off
	{
		for (SpriteID i = 0; i < SPRITE_MAX; i++) {
			Sprite* sprite = &sprites[i];
			assert(sprite->image, "Sprite was not setup properly");
		}
	}
	
    Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf, %d", GetLastError());	
	render_atlas_if_not_yet_rendered(font, 32, 'A');
	
	float32 input_speed = 150.0;

    Entity* player_en = entity_create();
    setup_player(player_en);

    Entity* cursor_en = entity_create();
    setup_cursor(cursor_en);

    for (int i = 0; i < 10; i++) {
		Entity* en = entity_create();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
		en->pos = round_v2_to_tile(en->pos);
	}

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    s32 last_fps = 0;
    float64 last_time = os_get_current_time_in_seconds();

    //:loop
    while (!window.should_close) {
		reset_temporary_storage();
        os_update(); 

        float64 now = os_get_current_time_in_seconds();
		float64 delta = now - last_time;
		last_time = now;	
        
        //:input
        Vector2 input_axis = v2(0, 0);
        float32 dash_speed = 1.0;
        if (is_key_just_pressed(KEY_ESCAPE)){
            window.should_close = true;
        }
        if (is_key_down('W')) {
            input_axis.y += 1.0;
        }
        if (is_key_down('A')) {
            input_axis.x -= 1.0;
        }
        if (is_key_down('S')) {
            input_axis.y -= 1.0;
        }
        if (is_key_down('D')) {
            input_axis.x += 1.0;
        }
        input_axis = v2_normalize(input_axis);
        cursor_en->pos = v2_add(cursor_en->pos, v2_mulf(input_axis, input_speed * delta ));
    
        //:camera
        draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
        draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		
        //:tile rendering
		{
			int player_tile_x = world_pos_to_tile_pos(player_en->pos.x);
			int player_tile_y = world_pos_to_tile_pos(player_en->pos.y);
			int tile_radius_x = 40;
			int tile_radius_y = 30;
			for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
					if ((x + (y % 2 == 0) ) % 2 == 0) {
						Vector4 col = v4(0.6, 0.6, 0.6, 0.6);
						float x_pos = x * tile_width;
						float y_pos = y * tile_width;
						draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
					}
				}
			}
		}

        //:ui rendering
		{
			float width = window.width;
			float height = window.height / 3.0f;

            // bg box 
            {
                Matrix4 xform = draw_frame.view;
                draw_rect_xform(xform, v2(width, height), bg_box_col);
                //:fps
                seconds_counter += delta;
                frame_count+=1;
                if(seconds_counter > 1.0){
                    last_fps = frame_count;
                    log("fps: %i", last_fps );
                    frame_count = 0;
                    seconds_counter = 0.0;
                }
                string text = STR("fps: %i");
                text = sprint(temp_allocator, text, last_fps);
                //draw_text_xform(font, text, font_height, xform, v2(0.1, 0.1), COLOR_WHITE);
            }

		}

        //:entity rendering
        for (int i = 0; i < MAX_ENTITY_COUNT; i++){
            Entity* en = &world->entities[i];
            if (en->is_valid){
                switch (en->arch){
                    default:
                        break;
                }
                Sprite* sprite = get_sprite(en->sprite_id);
                Matrix4 xform = m4_scalar(1.0);
                xform         = m4_translate(xform, v3(0, tile_width * -0.5, 0));
                xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                xform         = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0.0, 0));


                Vector4 col = COLOR_WHITE;
                draw_image_xform(sprite->image, xform, get_sprite_size(sprite), col);

                // debug pos 
                draw_text(font, sprint(temp_allocator, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
            }
        }

		gfx_update();
       
	}

	return 0;
}
