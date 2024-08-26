//constants
#define MAX_ENTITY_COUNT 1024

const u32 font_height = 64;
const float32 font_padding = (float32)font_height/10.0f;

const float32 spriteSheetWidth = 240.0;
const s32 tile_width = 20;

const s32 maze_width = 16;
const s32 maze_height = 16;

const s32 layer_stage_bg = 0;
const s32 layer_stage_fg = 5;
const s32 layer_world = 10;
const s32 layer_entity = 20;
const s32 layer_costume = 25;
const s32 layer_view = 15;
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
    ARCH_MAX,
} EntityArchetype;

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
    bool is_sprite;
    SpriteID sprite_id;
    bool is_line;
    Vector2 pos;
    Vector2 size;
    Vector4 color;
    bool has_collision;
} Entity;

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
    Tile tiles[maze_width][maze_height];
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
    en->sprite_id = SPRITE_player;
    Sprite* sprite = get_sprite(en->sprite_id);
    en->size = get_sprite_size(sprite); 
    en->is_sprite = true;
    en->has_collision = true;
    en->color = COLOR_WHITE;
}

void setup_citizen(Entity* en) {
    en->arch = ARCH_monster;
    en->is_sprite = true;
    en->sprite_id = SPRITE_monster;
    en->is_sprite = true;
    en->has_collision = true;
    en->color = COLOR_WHITE;
}

void setup_wall(Entity* en, Vector2 size) {
    en->arch = ARCH_terrain;
    en->is_line = true;
    en->is_sprite = true;
    en->has_collision = true;
    en->color = COLOR_WHITE;
    en->size = size;
}

void render_sprite_entity(Entity* en){
    Sprite* sprite = get_sprite(en->sprite_id);
    Matrix4 xform = m4_scalar(1.0);
    xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
    draw_image_xform(sprite->image, xform, get_sprite_size(sprite), en->color);

    if(world->debug_render){
        draw_text(world->font, sprint(temp_allocator, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
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

void dfs(Vector2 current_tile){
    world->tiles[(int)current_tile.x][(int)current_tile.y].visited = true;

    //shuffle
    s32 order[] = {0,1,2,3};
    for(int i = 3; i > 0; i--){
        int j = get_random_int_in_range(0,i);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    } 
    //log("order %d %d %d %d", order[0], order[1], order[2], order[3]);

    for(int i = 0; i < 4; i++){
        Vector2 next_tile = current_tile;
        if(order[i] == 0){
           next_tile.y = current_tile.y + 1; 
        }
        else if(order[i] == 1){
           next_tile.x = current_tile.x + 1; 
        }
        else if(order[i] == 2){
           next_tile.y = current_tile.y - 1; 
        }
        else if(order[i] == 3){
           next_tile.x = current_tile.x - 1; 
        }
        //log("current %d %d, next %d %d", (int)current_tile.x, (int)current_tile.y, (int)next_tile.x, (int)next_tile.y);
        //check oob
        if(next_tile.x < 0 || next_tile.y < 0){
        }
        else if(next_tile.x > maze_width-1 || next_tile.y > maze_width-1){
        }
        else if(world->tiles[(int)next_tile.x][(int)next_tile.y].visited == false){
            world->tiles[(int)current_tile.x][(int)current_tile.y].walls[order[i]] = false;
            world->tiles[(int)next_tile.x][(int)next_tile.y].walls[(order[i]+2)%4] = false;
            //log("destroyed current wall %d next wall %d", order[i], (order[i]+2)%4);
            dfs(next_tile); 
        }
    }
}

//:entry
int entry(int argc, char **argv) {
	
	window.title = STR("Secret Identity");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = v4(0, 0.3, 0.7, 1);
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
        player_en->pos = v2(5,5);

        //:init tiles
        for(int i = 0; i < maze_width; i++){
            for(int j = 0; j < maze_height; j++){
                for(int k = 0; k < 4; k++){
                    world->tiles[i][j].walls[k] = true;
                }
            }
        }        
        Vector2 current_pos = v2(0,0);
        dfs(current_pos);
        for(int i = 0; i < maze_width; i++){
            for(int j = 0; j < maze_height; j++){
                float x_pos = i * tile_width;
                float y_pos = j * tile_width;
                if(world->tiles[i][j].walls[0]){
                    Entity* en = entity_create();
                    setup_wall(en, v2(tile_width, 1));
                    en->pos = v2(x_pos, y_pos + tile_width);
                } 
                if(world->tiles[i][j].walls[1]){
                    Entity* en = entity_create();
                    setup_wall(en, v2(1, tile_width));
                    en->pos = v2(x_pos + tile_width, y_pos);
                } 
                if(world->tiles[i][j].walls[2]){
                    Entity* en = entity_create();
                    setup_wall(en, v2(tile_width, 1));
                    en->pos = v2(x_pos, y_pos);
                } 
                if(world->tiles[i][j].walls[3]){
                    Entity* en = entity_create();
                    setup_wall(en, v2(1, tile_width));
                    en->pos = v2(x_pos, y_pos);
                } 
                //log("tile %d %d walls %d %d %d %d", i,j, world->tiles[i][j].walls[0], world->tiles[i][j].walls[1], world->tiles[i][j].walls[2], world->tiles[i][j].walls[3]);
            }
        }        

    }

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    s32 last_fps = 0;
    float64 last_time = os_get_elapsed_seconds();
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
			Vector2 target_pos = get_player()->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta_t, 30.0f);

			world_frame.world_view = m4_make_scale(v3(1.0, 1.0, 1.0));
			world_frame.world_view = m4_mul(world_frame.world_view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
			world_frame.world_view = m4_mul(world_frame.world_view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
		}

        //:input
        Vector2 input_axis = v2(0, 0);
        {
            //check exit cond first
            if (is_key_just_pressed(KEY_ESCAPE)){
                window.should_close = true;
            }
             
            if(world->ux_state != UX_win && world->ux_state != UX_lose){
                if(world->ux_state == UX_default){
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
            }

            input_axis = v2_normalize(input_axis);


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
                        case ARCH_terrain:
		                    set_world_space();
                            push_z_layer(layer_entity);
                            render_rect_entity(en);

                            //:collision
                            Vector2 next_pos = v2_add(get_player()->pos, v2_mulf(input_axis, 100.0 * delta_t));
                            if(
                                next_pos.x < en->pos.x + en->size.x &&
                                next_pos.x + get_player()->size.x > en->pos.x &&
                                next_pos.y < en->pos.y + en->size.y &&
                                next_pos.y + get_player()->size.y > en->pos.y
                            ){
                                Vector2 next_pos_x = v2_add(get_player()->pos, v2_mulf(v2(input_axis.x, 0), 100.0 * delta_t));
                                Vector2 next_pos_y = v2_add(get_player()->pos, v2_mulf(v2(0, input_axis.y), 100.0 * delta_t));
                                if(
                                    next_pos_x.x < en->pos.x + en->size.x &&
                                    next_pos_x.x + get_player()->size.x > en->pos.x &&
                                    next_pos_x.y < en->pos.y + en->size.y &&
                                    next_pos_x.y + get_player()->size.y > en->pos.y
                                ){
                                    input_axis.x = 0;
                                }
                                if(
                                    next_pos_y.x < en->pos.x + en->size.x &&
                                    next_pos_y.x + get_player()->size.x > en->pos.x &&
                                    next_pos_y.y < en->pos.y + en->size.y &&
                                    next_pos_y.y + get_player()->size.y > en->pos.y
                                ){
                                    input_axis.y = 0;
                                }
                            }
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
        get_player()->pos = v2_add(get_player()->pos, v2_mulf(input_axis, 100.0 * delta_t));

        //:tile rendering
		{
		    set_world_space();
		    push_z_layer(layer_stage_fg);

			int tile_radius_x = maze_width;
			int tile_radius_y = maze_height;
			for (int x = 0; x < tile_radius_x; x++) {
				for (int y = 0; y < tile_radius_y; y++) {
                    float x_pos = x * tile_width;
                    float y_pos = y * tile_width;
					Vector4 col = v4(0.1, 0.1, 0.1, 1);

					if ((x + (y % 2 == 0) ) % 2 == 0) {
						//draw_rect(v2(x_pos + tile_width * -0.5, y_pos + tile_width * -0.5), v2(tile_width, tile_width), col);
					}
                    
                    if(world->tiles[x][y].walls[0]){
                        draw_line(v2(x_pos, y_pos + tile_width), v2(x_pos + tile_width, y_pos + tile_width), 1, col);
                    } 
                    if(world->tiles[x][y].walls[1]){
                        draw_line(v2(x_pos + tile_width, y_pos + tile_width), v2(x_pos + tile_width, y_pos), 1, col);
                    } 
                    if(world->tiles[x][y].walls[2]){
                        draw_line(v2(x_pos + tile_width, y_pos), v2(x_pos, y_pos), 1, col);
                    } 
                    if(world->tiles[x][y].walls[3]){
                        draw_line(v2(x_pos, y_pos), v2(x_pos, y_pos + tile_width), 1, col);
                    } 
				}
			}

            pop_z_layer();
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

        //:fps
        if(world->debug_render){
            {
                seconds_counter += delta_t;
                frame_count+=1;
                if(seconds_counter > 1.0){
                    last_fps = frame_count;
                    frame_count = 0;
                    seconds_counter = 0.0;
                }
                string text = STR("fps: %i");
                text = sprint(temp_allocator, text, last_fps);
                set_screen_space();
                push_z_layer(layer_text);
                Matrix4 xform = m4_scalar(1.0);
                xform = m4_translate(xform, v3(0,screen_height - (font_height * 0.1), 0));
                draw_text_xform(world->font, text, font_height, xform, v2(0.1, 0.1), COLOR_WHITE);
            }
        }

        gfx_update();
	}

	return 0;
}
