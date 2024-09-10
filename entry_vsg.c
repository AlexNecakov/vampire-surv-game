//constants
#define MAX_ENTITY_COUNT 1024

#define PI32 3.14159265359f
#define PI64 3.14159265358979323846
#define TAU32 (2.0f * PI32)
#define TAU64 (2.0 * PI64)
#define RAD_PER_DEG (PI64 / 180.0)
#define DEG_PER_RAD (180.0 / PI64)

#define to_radians(degrees) ((degrees)*RAD_PER_DEG)
#define to_degrees(radians) ((radians)*DEG_PER_RAD)

#define clamp_bottom(a, b) max(a, b)
#define clamp_top(a, b) min(a, b)

const u32 font_height = 64;
const float32 font_padding = (float32)font_height/10.0f;

const float32 spriteSheetWidth = 240.0;
const s32 tile_width = 16;

bool debug_render;
Gfx_Font* font;

//:layer
typedef enum Layer{
    layer_stage_bg = 0,
    layer_stage_fg = 5,
    layer_world = 10,
    layer_view = 15,
    layer_entity = 20,
    layer_ui_bg = 30,
    layer_ui_fg = 35,
    layer_text = 40,
    layer_cursor = 50,
} Layer;

Vector4 bg_box_col = {0, 0, 1.0, 0.9};

float screen_width = 240.0;
float screen_height = 135.0;

float exp_error_flash_alpha = 0;
float exp_error_flash_alpha_target = 0;
float camera_trauma = 0;
Vector2 camera_pos = {0};
float max_cam_shake_translate = 200.0f;
float max_cam_shake_rotate = 4.0f;

void camera_shake(float amount) {
	camera_trauma += amount;
}

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

bool get_random_bool() {
	return get_random_int_in_range(0, 1);
}

int get_random_sign() {
	return (get_random_int_in_range(0, 1) == 0 ? -1 : 1);
}

//:sprite
typedef struct Sprite {
    Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_monster,
    SPRITE_experience,
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

//:bar
typedef struct Bar {
    float64 max;
    float64 current;
    float64 rate;
} Bar;

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
    ARCH_weapon,
    ARCH_pickup,
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
    bool is_attached_to_player;
    SpriteID sprite_id;
    bool is_line;
    Vector2 pos;
    Vector2 size;
    Vector4 color;
    float angle;
    CollisionBox collider;
    bool is_static;
    Vector2 move_vec;
    Vector2 input_axis;
    Bar health;
    float power;
    float move_speed;
    Bar experience;
} Entity;

Vector2 get_entity_midpoint(Entity* en){
    return v2(en->pos.x + en->size.x/2.0, en->pos.y + en->size.y/2.0);
}

//:collision
bool check_entity_collision(Entity* en_1, Entity* en_2){
    bool collision_detected = false;
    if(en_1->is_valid && en_2 -> is_valid){
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
    }

    Vector2 temp_vec_1 = en_1->move_vec;

    en_1->move_vec = v2_normalize(v2(temp_vec_1.x, 0));
    if(check_entity_will_collide(en_1, en_2, delta_t)){
        temp_vec_1.x = 0;
    }

    en_1->move_vec = v2_normalize(v2(0, temp_vec_1.y));
    if(check_entity_will_collide(en_1, en_2, delta_t)){
        temp_vec_1.y = 0;
    }

    en_1->move_vec = v2_normalize(temp_vec_1);
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



//:world
typedef struct World{
	Entity entities[MAX_ENTITY_COUNT];
	UXState ux_state;
    float64 timer;
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
    en->health.max = 100;
    en->health.current = en->health.max;
    en->experience.max = 100;
    en->experience.current = 0;
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
    en->health.max = 50;
    en->health.current = en->health.max;
    en->power = 25;
}

void setup_sword(Entity* en) {
    en->arch = ARCH_weapon;
    en->is_line = true;
    en->collider = COLL_rect;
    en->color = COLOR_WHITE;
    en->size = v2(35,2);
    en->power = 500;
}

void setup_experience(Entity* en) {
    en->arch = ARCH_pickup;
    en->collider = COLL_rect;
    en->color = COLOR_WHITE;
    en->is_sprite = true;
    en->sprite_id = SPRITE_experience;
    Sprite* sprite = get_sprite(en->sprite_id);
    en->size = get_sprite_size(sprite); 
    en->power = 50;
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

void setup_world(){
    
    world->ux_state = UX_default;
    world->timer = 0;

    Entity* player_en = entity_create();
    setup_player(player_en);
    player_en->pos = v2(0,0);
    
    Entity* weapon_en = entity_create();
    setup_sword(weapon_en);

    for(int i = 0; i < 10; i++){
        Entity* monster_en = entity_create();
        setup_monster(monster_en);
        monster_en->pos = v2(get_random_int_in_range(5,15) * tile_width, 0);
        monster_en->pos = v2_rotate_point_around_pivot(monster_en->pos, v2(0,0), get_random_float32_in_range(0,2*PI64)); 
        monster_en->pos = v2_add(monster_en->pos, player_en->pos);
        //log("monster pos %f %f", monster_en->pos.x, monster_en->pos.y);
    }

}

void teardown_world(){
    for(int i = 0; i < MAX_ENTITY_COUNT; i++){
        Entity* en = &world->entities[i];
        entity_destroy(en);
    }
}

World* world_create() {
    return world;
}

void render_sprite_entity(Entity* en){
    if(en->is_valid){
        Sprite* sprite = get_sprite(en->sprite_id);
        Matrix4 xform = m4_scalar(1.0);
        xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
        draw_image_xform(sprite->image, xform, get_sprite_size(sprite), en->color);
    }

    if(debug_render){
        //draw_text(font, sprint(temp_allocator, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
    }
}

void render_rect_entity(Entity* en){
    if(en->is_valid){
        Matrix4 xform = m4_scalar(1.0);
        xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
        draw_rect_xform(xform, en->size, en->color);
    }
}

void render_line_entity(Entity* en){
    if(en->is_valid){
        Vector2 endpoint = v2_add(en->pos, v2(en->size.x, 0));
        endpoint = v2_rotate_point_around_pivot(endpoint, en->pos, to_radians(en->angle));
        log("angle %f rads %f", en->angle, to_radians(en->angle));
        draw_line(en->pos, endpoint, en->size.y, en->color); 
    }
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
	window.point_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.point_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = v4(0, 0.7, .3, 1);
	window.force_topmost = false;

	seed_for_random = rdtsc();
	
    {
        sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\undefined.png"), get_heap_allocator()) };
        sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\player.png"), get_heap_allocator()) };
        sprites[SPRITE_monster] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\monster.png"), get_heap_allocator()) };
        sprites[SPRITE_experience] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\sword.png"), get_heap_allocator()) };
		
        for (SpriteID i = 0; i < SPRITE_MAX; i++) {
			Sprite* sprite = &sprites[i];
			assert(sprite->image, "Sprite was not setup properly");
		}
    }

    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));
    setup_world();

    debug_render = true;
    font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf, %d", GetLastError());	
	render_atlas_if_not_yet_rendered(font, 32, 'A');

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    s32 last_fps = 0;
    float64 last_time = os_get_elapsed_seconds();
    float64 start_time = os_get_elapsed_seconds();
    Vector2 camera_pos = v2(0,0);
    bool reset_world = false;

    //:loop
    while (!window.should_close) {
		reset_temporary_storage();

        if(reset_world){
            teardown_world();
            setup_world();
            reset_world = false;
        }

		world_frame = (WorldFrame){0};
        float64 now = os_get_elapsed_seconds();
		float64 delta_t = now - last_time;
		last_time = now;	
	
        float zoom = 5.3;
		
        // find player 
		for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
			Entity* en = &world->entities[i];
            if (!en->is_valid){
                //clean up flagged entities before these pointers get used and after render
                entity_destroy(en);
            }
            else if (en->is_valid && en->arch == ARCH_player) {
                if(en->health.current < 0){
                    world->ux_state = UX_lose;
                }
				world_frame.player = en;
			}
		}

        //:frame updating
        draw_frame.enable_z_sorting = true;
		world_frame.world_proj = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

        // :camera
		{
			// camera shake - https://www.youtube.com/watch?v=tu-Qe66AvtY
			camera_trauma -= delta_t;
			camera_trauma = clamp_bottom(camera_trauma, 0);
			float cam_shake = clamp_top(pow(camera_trauma, 3), 1);

			Vector2 target_pos = get_player()->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);

			world_frame.world_view = m4_identity();

			// randy: these might be ordered incorrectly for the camera shake. Not sure.

			// translate into position
			world_frame.world_view = m4_translate(world_frame.world_view, v3(camera_pos.x, camera_pos.y, 0));

			// translational shake
			float shake_x = max_cam_shake_translate * cam_shake * get_random_float32_in_range(-1, 1);
			float shake_y = max_cam_shake_translate * cam_shake * get_random_float32_in_range(-1, 1);
			world_frame.world_view = m4_translate(world_frame.world_view, v3(shake_x, shake_y, 0));

			// rotational shake
			// float shake_rotate = max_cam_shake_rotate * cam_shake * get_random_float32_in_range(-1, 1);
			// world_frame.world_view = m4_rotate_z(world_frame.world_view, shake_rotate);

			// scale the zoom
			world_frame.world_view = m4_scale(world_frame.world_view, v3(1.0/zoom, 1.0/zoom, 1.0));
		}

        //:input
        {
            //check exit cond first
            if (is_key_just_pressed(KEY_ESCAPE)){
                window.should_close = true;
            }
            if (is_key_just_pressed('R')) {
                reset_world = true;
            }
             
            get_player()->input_axis = v2(0, 0);
            if(world->ux_state != UX_win && world->ux_state != UX_lose){
                if (is_key_down('A')) {
                    get_player()->input_axis.x -= 1.0;
                }
                if (is_key_down('D')) {
                    get_player()->input_axis.x += 1.0;
                }
                if (is_key_down('S')) {
                    get_player()->input_axis.y -= 1.0;
                }
                if (is_key_down('W')) {
                    get_player()->input_axis.y += 1.0;
                }
            }

            get_player()->input_axis = v2_normalize(get_player()->input_axis);
            get_player()->move_vec = get_player()->input_axis;

            float uDotV = v2_dot(v2(1,0), get_player()->input_axis);
            float uDetV = get_player()->input_axis.y*1 - get_player()->input_axis.x*0;
            float angle = to_degrees(atan2(uDetV, uDotV)); 
            get_player()->angle = angle;
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
                            en->pos = v2_add(en->pos, v2_mulf(en->move_vec, en->move_speed * delta_t));
                            render_sprite_entity(en);
                            if(en->health.current <= 0){
                                en->color = v4(0,0,0,0);
                            }
                            //:hp
                            {    
                                push_z_layer(layer_ui_fg);
                                Matrix4 xform = m4_scalar(1.0);
                                xform = m4_translate(xform, v3(get_player()->pos.x, get_player()->pos.y, 0)); 
                                draw_rect_xform(xform, v2(10, -5), COLOR_RED);
                                draw_rect_xform(xform, v2((get_player()->health.current / get_player()->health.max) * 10.0f, -5), COLOR_GREEN);
                                pop_z_layer();
                            }

                            break;
                        case ARCH_weapon:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            for(int j = 0; j < MAX_ENTITY_COUNT; j++){
                                Entity* other_en = &world->entities[j];
                                if(i != j){
                                    if(other_en->arch == ARCH_monster){
                                        if(check_entity_collision(en, other_en)){
                                            other_en->health.current -= (en->power * delta_t);
                                        }
                                    }
                                }
                            }
                            en->pos = get_player()->pos;
                            en->angle = get_player()->angle;

                            if(get_player()->experience.current >= get_player()->experience.max){
                                get_player()->experience.current = 0;
                                get_player()->experience.max = get_player()->experience.max * 1.1;
                                get_player()->health.max = get_player()->health.max * 1.1;
                                get_player()->health.current = get_player()->health.max;
                                en->size = v2(en->size.x * 1.1, en->size.y);
                            }
                            render_line_entity(en);
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
                                            other_en->health.current -= (en->power * delta_t);
                                        }
                                    }
                                }
                            }
                            en->pos = v2_add(en->pos, v2_mulf(en->move_vec, en->move_speed * delta_t));
                            
                            if(debug_render){
                                draw_line(en->pos, v2_add(en->pos, v2_mulf(en->move_vec, tile_width)), 1, COLOR_RED);
                            }

                            if(en->health.current <= 0){
                                en->color = v4(0,0,0,0);
                                en->is_valid = false;

                                Entity* pickup_en = entity_create();
                                setup_experience(pickup_en);
                                pickup_en->pos = en->pos;
                            }

                            string text = sprint(temp_allocator, STR("%f %f"), en->pos.x, en->pos.y);
                            push_z_layer(layer_text);
                            Matrix4 xform = m4_scalar(1.0);
                            xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                            draw_text_xform(font, text, font_height, xform, v2(0.1, 0.1), COLOR_YELLOW);
                            pop_z_layer();

                            if(
                                en->pos.x - camera_pos.x < -screen_width * 2 || 
                                en->pos.y - camera_pos.y < -screen_height * 2 ||
                                en->pos.x - camera_pos.x > screen_width * 2 ||
                                en->pos.y - camera_pos.y > screen_height * 2
                            )
                            {
                                en->color = v4(0,0,0,0);
                                en->is_valid = false;
                                //log("destroyed offscreen monster at screen pos %f %f", en->pos.x, en->pos.y);
                            } 
                            break;
                        case ARCH_pickup:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            render_sprite_entity(en);
                            if(check_entity_collision(en, get_player())){
                                get_player()->experience.current += en->power;
                                en->color = v4(0,0,0,0);
                                en->is_valid = false;
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
            draw_text_xform(font, text, font_height, xform, v2(0.5, 0.5), COLOR_YELLOW);
        }
        else if(world->ux_state == UX_lose){
            string text = STR("You Lose!");
            set_screen_space();
            push_z_layer(layer_text);
            Matrix4 xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(screen_width / 4.0, screen_height / 2.0, 0));
            draw_text_xform(font, text, font_height, xform, v2(0.5, 0.5), COLOR_RED);
        }

        //:bar rendering
        {    
            set_screen_space();
            push_z_layer(layer_ui_fg);
            Matrix4 xform = m4_scalar(1.0);
            xform = m4_translate(xform, v3(0, screen_height - 10, 0)); 
            draw_rect_xform(xform, v2(screen_width, 10), COLOR_GREY);
            draw_rect_xform(xform, v2((get_player()->experience.current / get_player()->experience.max) * screen_width, 10), COLOR_RED);
            xform = m4_translate(xform, v3(30, 0, 0));
            draw_rect_xform(xform, v2(25, 0.5), COLOR_GREY);
            draw_rect_xform(xform, v2((get_player()->health.current / get_player()->health.max) * 25.0f, 0.5), COLOR_GREEN);
            draw_text_xform(font, sprint(temp_allocator, STR("%.0f/%.0f"), get_player()->health.current, get_player()->health.max), font_height, xform, v2(0.1, 0.1), COLOR_WHITE);
            pop_z_layer();
        }

        //:world->timer
        if(debug_render){
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
                    if(world->ux_state != UX_lose){
                        for(int i = 0; i < 15; i++){
                            Entity* monster_en = entity_create();
                            setup_monster(monster_en);
                            monster_en->pos = v2(get_random_int_in_range(5,15) * tile_width, 0);
                            monster_en->pos = v2_rotate_point_around_pivot(monster_en->pos, v2(0,0), get_random_float32_in_range(0,2*PI64)); 
                            monster_en->pos = v2_add(monster_en->pos, get_player()->pos);
                            //log("monster pos %f %f", monster_en->pos.x, monster_en->pos.y);
                        }
                    }

                }
                string text = STR("fps: %i time: %.2f health: %.2f");
                text = sprint(temp_allocator, text, last_fps, world->timer, get_player()->health);
                set_screen_space();
                push_z_layer(layer_text);
                Matrix4 xform = m4_scalar(1.0);
                xform = m4_translate(xform, v3(0,screen_height - (font_height * 0.1), 0));
                draw_text_xform(font, text, font_height, xform, v2(0.1, 0.1), COLOR_RED);
            }
        }

        os_update();
        gfx_update();
	}

	return 0;
}
