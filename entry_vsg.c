//constants
#define MAX_ENTITY_COUNT 1024

const u32 font_height = 64;
const float32 font_padding = (float32)font_height/10.0f;

const float32 spriteSheetWidth = 240.0;
const s32 tile_width = 16;

const s32 layer_stage_bg = 0;
const s32 layer_stage_fg = 5;
const s32 layer_world = 10;
const s32 layer_view = 15;
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
//:sprite
typedef struct Sprite {
    Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_monster,
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
    UX_win,
    UX_lose,
} UXState;

//:entity
typedef enum EntityArchetype{
    ARCH_nil = 0,
    ARCH_player,
    ARCH_monster,
    ARCH_terrain,
    ARCH_powerup,
    ARCH_MAX,
} EntityArchetype;

typedef enum CollisionBox{
    COLL_nil = 0,
    COLL_rect,
    COLL_circ,
    COLL_complex,
} CollisionBox;

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
    bool is_sprite;
    SpriteID sprite_id;
    bool is_line;
    Vector2 pos;
    Vector2 size;
    Vector4 color;
    CollisionBox collider;
    bool is_static;
    Vector2 move_vec;
    float move_speed;
} Entity;

Vector2 get_entity_midpoint(Entity* en){
    return v2(en->pos.x + en->size.x/2.0, en->pos.y + en->size.y/2.0);
}

//:collision
bool check_entity_collision(Entity* en_1, Entity* en_2){
    bool collision_detected = false;
    if(en_1->collider == COLL_rect && en_2->collider == COLL_rect){
        if(
            en_1->pos.x < en_2->pos.x + en_2->size.x &&
            en_1->pos.x + en_1->size.x > en_2->pos.x &&
            en_1->pos.y < en_2->pos.y + en_2->size.y &&
            en_1->pos.y + en_1->size.y > en_2->pos.y
        ){
            collision_detected = true;
        }
    }
    return collision_detected;
}

bool check_entity_will_collide(Entity* en_1, Entity* en_2, float64 delta_t){
    bool collision_detected = false;

    en_1->pos = v2_add(en_1->pos, v2_mulf(en_1->move_vec, en_1->move_speed * delta_t));
    en_2->pos = v2_add(en_2->pos, v2_mulf(en_2->move_vec, en_2->move_speed * delta_t));
    collision_detected = check_entity_collision(en_1, en_2);

    en_1->pos = v2_sub(en_1->pos, v2_mulf(en_1->move_vec, en_1->move_speed * delta_t));
    en_2->pos = v2_sub(en_2->pos, v2_mulf(en_2->move_vec, en_2->move_speed * delta_t));

    return collision_detected;
}

void solid_entity_collision(Entity* en_1, Entity* en_2, float64 delta_t){
    // repel out of other entity if dynamic solid
    if(check_entity_collision(en_1, en_2)){
        Vector2 en_to_en_vec = v2_sub(get_entity_midpoint(en_1), get_entity_midpoint(en_2));
        if(!en_1->is_static){
            en_1->pos = v2_add(en_1->pos, v2_mulf(v2_normalize(en_to_en_vec), delta_t));
        }
        if(!en_2->is_static){
            en_2->pos = v2_add(en_2->pos, v2_mulf(v2_normalize(en_to_en_vec), delta_t));
        }
    }

    Vector2 temp_vec_1 = en_1->move_vec;
    Vector2 temp_vec_2 = en_2->move_vec;

    en_1->move_vec = v2_normalize(v2(temp_vec_1.x, 0));
    en_2->move_vec = v2_normalize(v2(temp_vec_2.x, 0));
    if(check_entity_will_collide(en_1, en_2, delta_t)){
        temp_vec_1.x = 0;
        temp_vec_2.x = 0;
    }

    en_1->move_vec = v2_normalize(v2(0, temp_vec_1.y));
    en_2->move_vec = v2_normalize(v2(0, temp_vec_2.y));
    if(check_entity_will_collide(en_1, en_2, delta_t)){
        temp_vec_1.y = 0;
        temp_vec_2.y = 0;
    }

    en_1->move_vec = v2_normalize(temp_vec_1);
    en_2->move_vec = v2_normalize(temp_vec_2);

}

bool check_ray_collision(Vector2 ray, Entity* en_1, Entity* en_2){
    bool collision_detected = false;
    if(
        en_1->pos.x < en_2->pos.x + en_2->size.x &&
        en_1->pos.x + en_1->size.x + ray.x > en_2->pos.x &&
        en_1->pos.y < en_2->pos.y + en_2->size.y &&
        en_1->pos.y + en_1->size.y + ray.y > en_2->pos.y
    ){
        collision_detected = true;
    }
    else if(
        ray.x < en_2->pos.x + en_2->size.x &&
        en_1->pos.x + en_1->size.x + ray.x > en_2->pos.x &&
        en_1->pos.y < en_2->pos.y + en_2->size.y &&
        en_1->pos.y + en_1->size.y + ray.y > en_2->pos.y
    ){
        collision_detected = true;
    }
    else if(
        ray.x < en_2->pos.x + en_2->size.x &&
        en_1->pos.x + en_1->size.x + ray.x > en_2->pos.x &&
        ray.y < en_2->pos.y + en_2->size.y &&
        en_1->pos.y + en_1->size.y + ray.y > en_2->pos.y
    ){
        collision_detected = true;
    }
    else if(
        en_1->pos.x < en_2->pos.x + en_2->size.x &&
        en_1->pos.x + en_1->size.x + ray.x > en_2->pos.x &&
        ray.y < en_2->pos.y + en_2->size.y &&
        en_1->pos.y + en_1->size.y + ray.y > en_2->pos.y
    ){
        collision_detected = true;
    }
    return collision_detected;
}

//:tile
typedef struct Tile{
    bool visited;
    //n,e,s,w
    bool walls[4];
} Tile;

//:world
typedef struct World{
	Entity entities[MAX_ENTITY_COUNT];
	UXState ux_state;
    bool debug_render;
    float64 timer;
    Gfx_Font* font;
} World;
World* world = 0;

typedef struct WorldFrame {
	Entity* selected_entity;
	Matrix4 world_proj;
	Matrix4 world_view;
	Entity* player;
} WorldFrame;
WorldFrame world_frame;

Entity* get_player() {
	return world_frame.player;
}

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
    en->is_sprite = true;
    en->sprite_id = SPRITE_player;
    Sprite* sprite = get_sprite(en->sprite_id);
    en->size = get_sprite_size(sprite); 
    en->collider = COLL_rect;
    en->color = COLOR_WHITE;
    en->move_speed = 150.0;
}

void setup_monster(Entity* en) {
    en->arch = ARCH_monster;
    en->is_sprite = true;
    en->sprite_id = SPRITE_monster;
    Sprite* sprite = get_sprite(en->sprite_id);
    en->size = get_sprite_size(sprite); 
    en->collider = COLL_rect;
    en->color = COLOR_WHITE;
    en->move_speed = 25;
}

void setup_wall(Entity* en, Vector2 size) {
    en->arch = ARCH_terrain;
    en->is_line = true;
    en->is_sprite = true;
    en->collider = COLL_rect;
    en->is_static = true;
    en->color = COLOR_WHITE;
    en->size = size;
}

void render_sprite_entity(Entity* en){
    Sprite* sprite = get_sprite(en->sprite_id);
    Matrix4 xform = m4_scalar(1.0);
    xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
    draw_image_xform(sprite->image, xform, get_sprite_size(sprite), en->color);

    if(world->debug_render){
        //draw_text(world->font, sprint(temp_allocator, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
    }
}

void render_rect_entity(Entity* en){
    Matrix4 xform = m4_scalar(1.0);
    xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
    draw_rect_xform(xform, en->size, en->color);

}

//:coordinate conversion
void set_screen_space() {
	draw_frame.camera_xform = m4_scalar(1.0);
	draw_frame.projection = m4_make_orthographic_projection(0.0, screen_width, 0.0, screen_height, -1, 10);
}
void set_world_space() {
	draw_frame.projection = world_frame.world_proj;
	draw_frame.camera_xform = world_frame.world_view;
}

Vector2 world_to_screen(Vector2 p) {
    Vector4 in_cam_space  = m4_transform(draw_frame.camera_xform, v4(p.x, p.y, 0.0, 1.0));
    Vector4 in_clip_space = m4_transform(draw_frame.projection, in_cam_space);
    
    Vector4 ndc = {
        .x = in_clip_space.x / in_clip_space.w,
        .y = in_clip_space.y / in_clip_space.w,
        .z = in_clip_space.z / in_clip_space.w,
        .w = in_clip_space.w
    };
    
    return v2(
        (ndc.x + 1.0f) * 0.5f * (f32)window.width,
        (ndc.y + 1.0f) * 0.5f * (f32)window.height
    );
}

Vector2 world_size_to_screen_size(Vector2 s) {
    Vector2 origin = v2(0, 0);
    
    Vector2 screen_origin = world_to_screen(origin);
    Vector2 screen_size_point = world_to_screen(s);
    
    return v2(
        screen_size_point.x - screen_origin.x,
        screen_size_point.y - screen_origin.y
    );
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

//:entry
int entry(int argc, char **argv) {
	
	window.title = STR("Survivors");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = hex_to_rgba(0x7fff94ff);
    float32 aspectRatio = (float32)window.width/(float32)window.height; 
    float32 zoom = window.width/spriteSheetWidth;
    float y_pos = (screen_height/3.0f) - 9.0f;
    
	seed_for_random = rdtsc();
	
    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));
    world->ux_state = UX_default;
    world->debug_render = true;
    world->font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(world->font, "Failed loading arial.ttf, %d", GetLastError());	
	render_atlas_if_not_yet_rendered(world->font, 32, 'A');

    sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\undefined.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\player.png"), get_heap_allocator()) };
    sprites[SPRITE_monster] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\monster.png"), get_heap_allocator()) };
	
    // @ship debug this off
	{
		for (SpriteID i = 0; i < SPRITE_MAX; i++) {
			Sprite* sprite = &sprites[i];
			assert(sprite->image, "Sprite was not setup properly");
		}
	}
    
    //:setup world
    {	
        Entity* player_en = entity_create();
        setup_player(player_en);
        player_en->pos = v2(0,0);

        for(int i = 0; i < 10; i++){
            Entity* monster_en = entity_create();
            setup_monster(monster_en);
            monster_en->pos = v2(get_random_int_in_range(5,15) * tile_width, 0);
            monster_en->pos = v2_rotate_point_around_pivot(monster_en->pos, v2(0,0), get_random_float32_in_range(0,2*PI64)); 
            monster_en->pos = v2_add(monster_en->pos, player_en->pos);
            log("monster pos %f %f", monster_en->pos.x, monster_en->pos.y);
        }

    }

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    s32 last_fps = 0;
    float64 last_time = os_get_elapsed_seconds();
    float64 start_time = os_get_elapsed_seconds();
    world->timer = 0;
    Vector2 camera_pos = v2(0,0);

    //:loop
    while (!window.should_close) {
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
        float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;	
        os_update(); 
	
		
        // find player 
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* en = &world->entities[i];
			if (en->is_valid && en->arch == ARCH_player) {
				world_frame.player = en;
			}
		}
       

        //:frame updating
        draw_frame.enable_z_sorting = true;
		world_frame.world_proj = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

        // :camera
		{
			Vector2 target_pos = get_entity_midpoint(get_player());
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);

			world_frame.world_view = m4_make_scale(v3(1.0, 1.0, 1.0));
			world_frame.world_view = m4_mul(world_frame.world_view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			world_frame.world_view = m4_mul(world_frame.world_view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		}

        //:input
        {
            //check exit cond first
            if (is_key_just_pressed(KEY_ESCAPE)){
                window.should_close = true;
            }
             
            Vector2 input_axis = v2(0, 0);
            if(world->ux_state != UX_win && world->ux_state != UX_lose){
                if (is_key_down('A')) {
                    input_axis.x -= 1.0;
                }
                if (is_key_down('D')) {
                    input_axis.x += 1.0;
                }
                if (is_key_down('S')) {
                    input_axis.y -= 1.0;
                }
                if (is_key_down('W')) {
                    input_axis.y += 1.0;
                }
            }

            input_axis = v2_normalize(input_axis);
            get_player()->move_vec = input_axis;

        }

        //:entity loop 
        {
            for (int i = 0; i < MAX_ENTITY_COUNT; i++){
                Entity* en = &world->entities[i];
                if (en->is_valid){
                    switch (en->arch){
                        case ARCH_player:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            render_sprite_entity(en);
                            break;
                        case ARCH_monster:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            render_sprite_entity(en);
                            en->move_vec = v2_normalize(v2_sub(get_player()->pos, en->pos));
                            for(int j = 0; j < MAX_ENTITY_COUNT; j++){
                                Entity* other_en = &world->entities[j];
                                if(i != j){
                                    if(other_en->arch == ARCH_monster){
                                        solid_entity_collision(en, other_en, delta_t);
                                    }
                                    else if(other_en->arch == ARCH_player){
                                        if(check_entity_collision(en, other_en)){
                                            if(world->ux_state == UX_win){
                                                world->ux_state = UX_win; 
                                            }
                                            else{
                                                world->ux_state = UX_lose;
                                            }
                                        }
                                    }
                                }
                            }
                            en->pos = v2_add(en->pos, v2_mulf(en->move_vec, en->move_speed * delta_t));
                            
                            if(world->debug_render){
                                draw_line(en->pos, v2_add(en->pos, v2_mulf(en->move_vec, tile_width)), 1, COLOR_RED);
                            }
                            break;
                        case ARCH_terrain:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            render_rect_entity(en);
                            break;
                        default:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            render_sprite_entity(en);
                            break;
                    }
                    pop_z_layer();
                }
                   
            }
        }
        get_player()->pos = v2_add(get_player()->pos, v2_mulf(get_player()->move_vec, get_player()->move_speed * delta_t));

		// :tile rendering
		{
		    set_world_space();
		    push_z_layer(layer_stage_fg);
			int player_tile_x = world_pos_to_tile_pos(get_player()->pos.x);
			int player_tile_y = world_pos_to_tile_pos(get_player()->pos.y);
			int tile_radius_x = 40;
			int tile_radius_y = 30;
			for (int x = player_tile_x - tile_radius_x; x < player_tile_x + tile_radius_x; x++) {
				for (int y = player_tile_y - tile_radius_y; y < player_tile_y + tile_radius_y; y++) {
					if ((x + (y % 2 == 0) ) % 2 == 0) {
						Vector4 col = v4(0.1, 0.1, 0.1, 0.1);
						float x_pos = x * tile_width;
						float y_pos = y * tile_width;
						draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
					}
				}
			}

            pop_z_layer();
			// draw_rect(v2(tile_pos_to_world_pos(mouse_tile_x) + tile_width * -0.5, tile_pos_to_world_pos(mouse_tile_y) + tile_width * -0.5), v2(tile_width, tile_width), v4(0.5, 0.5, 0.5, 0.5));
		}

        //:ui
        if(world->ux_state == UX_win){
            string text = STR("You Win!");
            set_screen_space();
            push_z_layer(layer_text);
            Matrix4 xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(screen_width / 4.0, screen_height / 2.0, 0));
            draw_text_xform(world->font, text, font_height, xform, v2(0.5, 0.5), COLOR_YELLOW);
        }
        else if(world->ux_state == UX_lose){
            string text = STR("You Lose!");
            set_screen_space();
            push_z_layer(layer_text);
            Matrix4 xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(screen_width / 4.0, screen_height / 2.0, 0));
            draw_text_xform(world->font, text, font_height, xform, v2(0.5, 0.5), COLOR_RED);
        }

        //:timer
        if(world->debug_render){
            {
                seconds_counter += delta_t;
                if(world->ux_state != UX_win && world->ux_state != UX_lose){
                    world->timer += delta_t;
                }
                frame_count+=1;
                if(seconds_counter > 1.0){
                    last_fps = frame_count;
                    frame_count = 0;
                    seconds_counter = 0.0;
                    for(int i = 0; i < 0; i++){
                        Entity* monster_en = entity_create();
                        setup_monster(monster_en);
                        monster_en->pos = v2(get_random_int_in_range(5,15) * tile_width, 0);
                        monster_en->pos = v2_rotate_point_around_pivot(monster_en->pos, v2(0,0), get_random_float32_in_range(0,2*PI64)); 
                        monster_en->pos = v2_add(monster_en->pos, get_player()->pos);
                        log("monster pos %f %f", monster_en->pos.x, monster_en->pos.y);
                    }

                }
                string text = STR("fps: %i time: %.2f");
                text = sprint(temp_allocator, text, last_fps, world->timer);
                set_screen_space();
                push_z_layer(layer_text);
                Matrix4 xform = m4_scalar(1.0);
                xform = m4_translate(xform, v3(0,screen_height - (font_height * 0.1), 0));
                draw_text_xform(world->font, text, font_height, xform, v2(0.1, 0.1), COLOR_RED);
            }
        }

        gfx_update();
	}

	return 0;
}
