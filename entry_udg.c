//:constants
#define MAX_ENTITY_COUNT 1024
#define MAX_PLAYER_COUNT 4
#define MAX_MONSTER_COUNT MAX_ENTITY_COUNT
const u32 font_height = 64;
const float32 font_padding = (float32)font_height/10.0f;
const float32 spriteSheetWidth = 240.0;
const s32 tile_width = 16;
const s32 layer_world = 10;
const s32 layer_entity = 20;
const s32 layer_ui = 30;
const s32 layer_text = 40;
const s32 layer_cursor = 50;
Vector4 bg_box_col = {0, 0, 1.0, 0.9};
float screen_width = 240.0;
float screen_height = 135.0;

float64 player_hp_max = 50;
float64 player_mp_max = 25;
float64 player_tp_max = 500;
float64 player_tp_rate = 75;
float64 monster_hp_max = 50;
float64 monster_mp_max = 15;
float64 monster_tp_max = 100;
float64 monster_tp_rate = 25;

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
	draw_frame.view = m4_scalar(1.0);
	draw_frame.projection = m4_make_orthographic_projection(0.0, screen_width, 0.0, screen_height, -1, 10);
}
void set_world_space() {
	draw_frame.projection = world_frame.world_proj;
	draw_frame.view = world_frame.world_view;
}

Vector2 world_to_screen(Vector2 world_pos){
    Vector2 screen_pos;
    return screen_pos;
}
Vector2 screen_to_world() {
	float mouse_x = input_frame.mouse_x;
	float mouse_y = input_frame.mouse_y;
	//log("%f, %f", mouse_x, mouse_y);
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

Draw_Quad ndc_quad_to_screen_quad(Draw_Quad ndc_quad) {
	// NOTE: we're assuming these are the screen space matricies.
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.view;

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
    SPRITE_cursor,
    SPRITE_target,
    SPRITE_monster,
    SPRITE_attack,
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
	UX_command,
	UX_attack,
	UX_magic,
	UX_items,
	UX_debug,
} UXState;

typedef enum UXCommandPos {
    CMD_attack = 0,
    CMD_magic,
    CMD_items,
    CMD_MAX,
} UXCommandPos;

//:bar
typedef struct Bar {
    float64 max;
    float64 current;
    float64 rate;
} Bar;

//:element
typedef enum Element {
    ELEM_nil = 0,
    ELEM_default,
    ELEM_fire,
    ELEM_ice,
    ELEM_thunder,
} Element;

//:action
typedef struct ActionData {
    string name;
} ActionData;

//:spell
typedef struct Spell {
    string name;

} Spell;

//:entity
typedef enum EntityArchetype{
    ARCH_nil = 0,
    ARCH_player,
    ARCH_cursor,
    ARCH_target,
    ARCH_attack,
    ARCH_monster,
    ARCH_text,
    ARCH_MAX,
} EntityArchetype;

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
	string name;
    SpriteID sprite_id;
    bool render_sprite;
    Vector2 pos;
    Vector4 color;
    Bar health;
    Bar mana;
    Bar time;
    float64 limit;
    bool is_invincible;
    s32 strength;
    s32 defense;
} Entity;

//:world
typedef struct World{
	Entity entities[MAX_ENTITY_COUNT];
	s32 entity_selected;
	s32 player_selected;
    s32 num_players;
    s32 num_monsters;
	UXState ux_state;
	UXCommandPos ux_cmd_pos;
	Matrix4 world_proj;
	Matrix4 world_view;
    bool debug_render;
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

void select_first_entity_by_arch(EntityArchetype arch_match, bool checkTime, s32* selectIndex){
    *selectIndex = 0;
    Entity* en = &world->entities[*selectIndex];
    u32 tries = 0;
    for (tries = 0; tries <= MAX_ENTITY_COUNT; tries++){
        if(en->arch == arch_match && en->is_valid){
            if(checkTime){
                if(en->time.current >= en->time.max){
                    break;
                }
            }
            else{
                break;
            }
        }
        *selectIndex = (*selectIndex + 1) % MAX_ENTITY_COUNT;
        *selectIndex = (*selectIndex < 0)? MAX_ENTITY_COUNT - 1: *selectIndex;
        en = &world->entities[*selectIndex];
    }
    
    if(tries > MAX_ENTITY_COUNT){
        //no matching entity found
        world->ux_state = UX_default;
    }
}

void select_next_entity_by_arch(EntityArchetype arch_match, bool checkTime, s32* selectIndex) {
    Entity* en = &world->entities[*selectIndex];
    u32 tries = 0;
    for (tries = 0; tries <= MAX_ENTITY_COUNT; tries++){
        *selectIndex = (*selectIndex + 1) % MAX_ENTITY_COUNT;
        *selectIndex = (*selectIndex < 0)? MAX_ENTITY_COUNT - 1: *selectIndex;
        en = &world->entities[*selectIndex];
        if(en->arch == arch_match && en->is_valid){
            if(checkTime){
                if(en->time.current >= en->time.max){
                    break;
                }
            }
            else{
                break;
            }
        }
    }
    if(tries > MAX_ENTITY_COUNT){
        //no matching entity found
        world->ux_state = UX_default;
    }
}

void select_prev_entity_by_arch(EntityArchetype arch_match, bool checkTime, s32* selectIndex) {
    Entity* en = &world->entities[world->entity_selected];
    u32 tries = 0;
    for (tries = 0; tries <= MAX_ENTITY_COUNT; tries++){
        *selectIndex = (*selectIndex - 1) % MAX_ENTITY_COUNT;
        *selectIndex = (*selectIndex < 0)? MAX_ENTITY_COUNT - 1: *selectIndex;
        en = &world->entities[*selectIndex];
        if(en->arch == arch_match && en->is_valid){
            if(checkTime){
                if(en->time.current >= en->time.max){
                    break;
                }
            }
            else{
                break;
            }
        }
    }
    if(tries > MAX_ENTITY_COUNT){
        //no matching entity found
        world->ux_state = UX_default;
    }
}

void select_first_player(bool checkTime, s32* selectIndex){
    select_first_entity_by_arch(ARCH_player, checkTime, selectIndex);
}

void select_next_player(bool checkTime, s32* selectIndex) {
    select_next_entity_by_arch(ARCH_player, checkTime, selectIndex);
}

void select_prev_player(bool checkTime, s32* selectIndex) {
    select_prev_entity_by_arch(ARCH_player, checkTime, selectIndex);
}

void select_random_player(bool checkTime, s32* selectIndex) {
    s64 rand_num = get_random_int_in_range(0, world->num_players);
    for(s64 i = 0; i < rand_num; i++){ 
        select_next_entity_by_arch(ARCH_player, checkTime, selectIndex);
    }
}

void select_random_monster(bool checkTime, s32* selectIndex) {
    s64 rand_num = get_random_int_in_range(0, world->num_monsters);
    for(s64 i = 0; i < rand_num; i++){ 
        select_next_entity_by_arch(ARCH_monster, checkTime, selectIndex);
    }
}

void apply_damage_to_entity(Entity* source_en, Entity* target_en, ActionData action){
}

void render_sprite_entity(Entity* en){
    Sprite* sprite = get_sprite(en->sprite_id);
    Matrix4 xform = m4_scalar(1.0);
    xform         = m4_translate(xform, v3(0, tile_width * -0.5, 0));
    xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
    xform         = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0.0, 0));
    draw_image_xform(sprite->image, xform, get_sprite_size(sprite), en->color);
    // debug pos 
    //draw_text(font, sprint(temp_allocator, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
}

void setup_player(Entity* en) {
    en->arch = ARCH_player;
    en->sprite_id = SPRITE_player;
    en->health.max = player_hp_max;
    en->health.current = en->health.max;
    en->mana.max = player_mp_max;
    en->mana.current = en->mana.max;
    en->time.max = player_tp_max;
    en->time.current = 0;
    en->time.rate = player_tp_rate;
    en->strength = 25;
    en->defense = 10;
    en->color = COLOR_WHITE;
    world->num_players++;
    //en->is_invincible = true;
}

void setup_cursor(Entity* en) {
    en->arch = ARCH_cursor;
    en->sprite_id = SPRITE_cursor;
    en->is_invincible = true;
    en->color = COLOR_WHITE;
}

void setup_target(Entity* en) {
    en->arch = ARCH_target;
    en->sprite_id = SPRITE_target;
    en->health.max = 1;
    en->health.current = 1;
    en->is_invincible = true;
    en->color = COLOR_WHITE;
}

void setup_text(Entity* en, string name){
    en->arch = ARCH_text;
    en->name = name;
    en->is_invincible = true;
    en->color = COLOR_WHITE;
}

void setup_monster(Entity* en) {
    en->arch = ARCH_monster;
    en->sprite_id = SPRITE_monster;
    en->health.max = monster_hp_max;
    en->health.current = en->health.max;
    en->mana.max = monster_mp_max;
    en->mana.current = en->mana.max;
    en->time.max = monster_tp_max;
    en->time.current = 0;
    en->time.rate = monster_tp_rate;
    en->strength = 15;
    en->defense = 5;
    en->color = COLOR_WHITE;
    world->num_monsters++;
}

//:item data
typedef struct ItemData {
	int amount;
} ItemData; 

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
    float y_pos = screen_height/3.0f;
    
    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));
    world->ux_state = UX_default;
    world->ux_cmd_pos = CMD_attack;
    world->debug_render = true;

    sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\undefined.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\dude.png"), get_heap_allocator()) };
    sprites[SPRITE_monster] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\bat.png"), get_heap_allocator()) };
    sprites[SPRITE_cursor] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\cursor.png"), get_heap_allocator()) };
    sprites[SPRITE_target] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\target.png"), get_heap_allocator()) };
    sprites[SPRITE_attack] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\attack.png"), get_heap_allocator()) };
	
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
	
    Entity* cursor_en = entity_create();
    setup_cursor(cursor_en);
        
    Entity* target_en = entity_create();
    setup_target(target_en);

    for (int i = 0; i < 4; i++) {
		Entity* en = entity_create();
		setup_player(en);
        en->name = sprint(temp_allocator, STR("player%f"), i);
		en->pos = v2(0, tile_width*-i + 4 * tile_width);
		en->pos = round_v2_to_tile(en->pos);
        en->time.current = get_random_float32_in_range(en->time.max * 0.1, en->time.max * 0.7);
	}

    for (int i = 0; i < 4; i++) {
        Entity* en = entity_create();
        setup_monster(en);
        en->pos = v2(-4 * tile_width, -i * tile_width + 4* tile_width);
        en->pos = round_v2_to_tile(en->pos);
    }


    Entity* attack_menu_en = entity_create();
    setup_text(attack_menu_en, STR("Attack"));
    attack_menu_en->pos = v2(2.0f * tile_width, y_pos - (font_height + font_padding) * 0.1); 

    Entity* magic_menu_en = entity_create();
    setup_text(magic_menu_en, STR("Magic"));
    magic_menu_en->pos = v2(2.0f * tile_width, y_pos - (font_height + font_padding) * 0.1 * 2); 
    
    Entity* item_menu_en = entity_create();
    setup_text(item_menu_en, STR("Items"));
    item_menu_en->pos = v2(2.0f * tile_width, y_pos - (font_height + font_padding) * 0.1 * 3); 
   
    Entity* fps_count_en = entity_create();
    setup_text(fps_count_en, STR("fps: 0.0"));
    fps_count_en->pos = v2(0, screen_height - (font_height + font_padding) * 0.1);

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    s32 last_fps = 0;
    float64 last_time = os_get_current_time_in_seconds();

    //:loop
    while (!window.should_close) {
		reset_temporary_storage();
		world_frame = (WorldFrame){0};
        float64 now = os_get_current_time_in_seconds();
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
						Vector4 col = v4(0.6, 0.6, 0.6, 0.6);
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
                        case ARCH_cursor:
                            push_z_layer(layer_cursor);
                            if (world->ux_state == UX_command){
		                        set_screen_space();
                                en->pos = v2(tile_width * 1.5, y_pos - (tile_width * 0.25f) - (font_height + font_padding) * world->ux_cmd_pos * 0.1);
                            }
                            else if(world->ux_state == UX_attack){
		                        set_world_space();
                                Entity* selected_en = &world->entities[world->entity_selected];
                                Sprite* sprite = get_sprite(selected_en->sprite_id);
                                en->pos = v2(selected_en->pos.x - get_sprite_size(sprite).x, selected_en->pos.y);
                            }
                            else{
                                set_screen_space();
                                en->pos = v2(-10.0f * tile_width, -10.0f * tile_width);
                            }
                            render_sprite_entity(en);
                            break;
                        case ARCH_target:
                            push_z_layer(layer_cursor);
                            if(world->ux_state == UX_attack || world->ux_state == UX_command){
		                        set_world_space();
                                Entity* selected_player = &world->entities[world->player_selected];
                                Sprite* sprite = get_sprite(selected_player->sprite_id);
                                en->pos = v2(selected_player->pos.x, selected_player->pos.y + get_sprite_size(sprite).y);
                            }
                            else{
                                set_screen_space();
                                en->pos = v2(-10.0f * tile_width, -10.0f * tile_width);
                            }
                            render_sprite_entity(en);
                            break;
                        case ARCH_player:
		                    set_world_space();
                            //:time bar
                            {    
                                push_z_layer(layer_ui);
                                Sprite* sprite = get_sprite(en->sprite_id);
                                Matrix4 xform = m4_scalar(1.0);
                                xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                                xform = m4_translate(xform, v3(-0.5 * get_sprite_size(sprite).x, -.6 * get_sprite_size(sprite).y, 0));
                                draw_rect_xform(xform, v2(en->time.max * 0.1, 2.5), COLOR_BLUE);
                                draw_rect_xform(xform, v2(en->time.current * 0.1, 2.5), COLOR_YELLOW);
                                xform = m4_translate(xform, v3(-0.5 * get_sprite_size(sprite).x, 0, 0));
                                draw_rect_xform(xform, v2(en->health.max * 0.1, 2.5), COLOR_RED);
                                draw_rect_xform(xform, v2(en->health.current * 0.1, 2.5), COLOR_GREEN);
                                pop_z_layer();
                            }
                            push_z_layer(layer_world);
                            if(en->time.current >= en->time.max){
                                if(world->ux_state == UX_default){
                                    world->ux_state = UX_command;
                                    world->player_selected = i;
                                }
                            }
                            render_sprite_entity(en);
                            break;
                        case ARCH_monster:
		                    set_world_space();
                            //:health bar
                            {    
                                push_z_layer(layer_ui);
                                Sprite* sprite = get_sprite(en->sprite_id);
                                Matrix4 xform = m4_scalar(1.0);
                                xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                                xform = m4_translate(xform, v3(-0.5 * get_sprite_size(sprite).x, -.6 * get_sprite_size(sprite).y, 0));
                                draw_rect_xform(xform, v2(en->health.max * 0.1, 2.5), COLOR_RED);
                                draw_rect_xform(xform, v2(en->health.current * 0.1, 2.5), COLOR_GREEN);
                                //draw_text(font, sprint(temp_allocator, STR("%f / %f"), en->health.current, en->health.max), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
                                pop_z_layer();
                            }
                            push_z_layer(layer_world);
                            if(en->time.current >= en->time.max){
                                s32 target_player = 0;
                                select_random_player(false, &target_player);
                                en->time.current = 0;
                                world->entities[target_player].health.current -= (en->strength - world->entities[target_player].defense);
                            }
                            render_sprite_entity(en);
                            break;
                        case ARCH_text: 
                            set_screen_space();
                            push_z_layer(layer_text);
                            Matrix4 xform = m4_scalar(1.0);
                            string text = en->name;
                            xform = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
                            draw_text_xform(font, text, font_height, xform, v2(0.1, 0.1), en->color);
                            break;
                        default:
		                    set_world_space();
                            push_z_layer(layer_world);
                            break;
                    }
                    pop_z_layer();
                   
                    en->time.current = (en->time.current + (en->time.rate * delta) >= en->time.max)? en->time.max: en->time.current + (en->time.rate * delta);
                    if(!en->is_invincible){
                        if(en->health.current <= 0){
                            en->is_valid = false;
                            if(en->arch == ARCH_player){
                                world->num_players--;
                            }
                            else if(en->arch == ARCH_monster){
                                world->num_monsters--;
                            }
                        }
                    }
                }
            }
        }

        //:ui loop 
        {
            set_screen_space();
            push_z_layer(layer_ui);
            // bg box
            {
                Matrix4 xform = m4_scalar(1.0);
                xform = m4_translate(xform, v3(0, y_pos, 0.0));
                draw_rect_xform(xform, v2(screen_width, -y_pos), bg_box_col);
            }

            pop_z_layer();
            attack_menu_en->color = (world->ux_cmd_pos == CMD_attack)?COLOR_YELLOW:COLOR_WHITE;
            magic_menu_en->color = (world->ux_cmd_pos == CMD_magic)?COLOR_YELLOW:COLOR_WHITE;
            item_menu_en->color = (world->ux_cmd_pos == CMD_items)?COLOR_YELLOW:COLOR_WHITE;

            attack_menu_en->color.a = (world->ux_state != UX_default)?1.0:0.1;
            magic_menu_en->color.a = (world->ux_state != UX_default)?1.0:0.1;
            item_menu_en->color.a = (world->ux_state != UX_default)?1.0:0.1;
        }

        //:input
        {
            //check exit cond first
            if (is_key_just_pressed(KEY_ESCAPE)){
                window.should_close = true;
            }

            //error conds
            Entity* selected_player = &world->entities[world->player_selected];
            if(selected_player->time.current < selected_player->time.max){
                world->ux_state = UX_default;
            }
            if(selected_player->health.current <= 0){
                world->ux_state = UX_default;
                selected_player->time.rate = 0;
                selected_player->time.current = 0;
            }

            //:input commands
            if(world->ux_state == UX_command){
                if (is_key_just_pressed('J')) {
                    consume_key_just_pressed('J');
                    world->ux_cmd_pos = (world->ux_cmd_pos + 1) % CMD_MAX;
                    world->ux_cmd_pos = (world->ux_cmd_pos < 0)? CMD_MAX - 1: world->ux_cmd_pos;
                }
                else if (is_key_just_pressed('K')) {
                    consume_key_just_pressed('K');
                    world->ux_cmd_pos = (world->ux_cmd_pos - 1) % CMD_MAX;
                    world->ux_cmd_pos = (world->ux_cmd_pos < 0)? CMD_MAX - 1: world->ux_cmd_pos;
                }
                else if (is_key_just_pressed(KEY_SPACEBAR)) {
                    consume_key_just_pressed(KEY_SPACEBAR);
                    select_next_player(true, &world->player_selected);
                    world->ux_cmd_pos = CMD_attack;
                }
                else if (is_key_just_pressed(KEY_ENTER)) {
                    consume_key_just_pressed(KEY_ENTER);
                    switch (world->ux_cmd_pos){
                        case CMD_attack:
                            world->ux_state = UX_attack;
                            select_first_entity_by_arch(ARCH_monster, false, &world->entity_selected);
                            break;
                        case CMD_magic:
                            world->ux_state = UX_magic;
                            break;
                        case CMD_items:
                            world->ux_state = UX_attack;
                            break;
                        default:
                            break;
                    }
                }
            }
            //:input attack
            else if(world->ux_state == UX_attack){
                if (is_key_just_pressed('J')) {
                    consume_key_just_pressed('J');
                    select_next_entity_by_arch(ARCH_monster, false, &world->entity_selected);
                }
                else if (is_key_just_pressed('K')) {
                    consume_key_just_pressed('K');
                    select_prev_entity_by_arch(ARCH_monster, false, &world->entity_selected);
                }
                else if (is_key_just_pressed(KEY_ENTER)){
                    consume_key_just_pressed(KEY_ENTER);
                    Entity* selected_en = &world->entities[world->entity_selected];
                    Entity* selected_player = &world->entities[world->player_selected];
                    selected_en->health.current -= (selected_player->strength - selected_en->defense);
                    selected_player->time.current = 0;
                    world->ux_state = UX_default;
                    world->ux_cmd_pos = CMD_attack;
                }
                else if (is_key_just_pressed(KEY_TAB)){
                    consume_key_just_pressed(KEY_TAB);
                    world->ux_state = UX_command;
                    world->ux_cmd_pos = CMD_attack;
                }
            }
            //:input magic 
            else if(world->ux_state == UX_magic){
                if (is_key_just_pressed('J')) {
                    consume_key_just_pressed('J');
                    select_next_entity_by_arch(ARCH_monster, false, &world->entity_selected);
                }
                else if (is_key_just_pressed('K')) {
                    consume_key_just_pressed('K');
                    select_prev_entity_by_arch(ARCH_monster, false, &world->entity_selected);
                }
                else if (is_key_just_pressed(KEY_ENTER)){
                    consume_key_just_pressed(KEY_ENTER);
                    Entity* selected_en = &world->entities[world->entity_selected];
                    Entity* selected_player = &world->entities[world->player_selected];
                    selected_en->health.current -= (selected_player->strength - selected_en->defense);
                    selected_player->time.current = 0;
                    world->ux_state = UX_default;
                    world->ux_cmd_pos = CMD_attack;
                }
                else if (is_key_just_pressed(KEY_TAB)){
                    consume_key_just_pressed(KEY_TAB);
                    world->ux_state = UX_command;
                    world->ux_cmd_pos = CMD_attack;
                }
            }
            else{
                
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
