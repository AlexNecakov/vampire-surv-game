inline float v2_dist(Vector2 a, Vector2 b) {
    return v2_length(v2_sub(a, b));
}

#define m4_identity m4_make_scale(v3(1, 1, 1))

Vector2 range2f_get_center(Range2f r) {
	return (Vector2) { (r.max.x - r.min.x) * 0.5 + r.min.x, (r.max.y - r.min.y) * 0.5 + r.min.y };
}

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

float sin_breathe(float time, float rate) {
	return (sin(time * rate) + 1.0) / 2.0;
}

bool almost_equals(float a, float b, float epsilon) {
 return fabs(a - b) <= epsilon;
}

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

Range2f quad_to_range(Draw_Quad quad) {
	return (Range2f){quad.bottom_left, quad.top_right};
}

Vector4 bg_box_col = {0, 0, 0, 0.5};
const int tile_width = 16;
const float entity_selection_radius = 16.0f;
const float player_pickup_radius = 20.0f;

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

typedef struct Sprite {
    Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
    SPRITE_rock,
    SPRITE_spider,
    SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite* get_sprite(SpriteID id){
    if (id >= 0 && id < SPRITE_MAX){
        return &sprites[id];
    }
    return &sprites[0];
}
Vector2 get_sprite_size(Sprite* sprite) {
	return (Vector2) { sprite->image->width, sprite->image->height };
}

typedef enum EntityArchetype{
    arch_nil = 0,
    arch_player,
    arch_rock,
    arch_monster,
    ARCH_MAX,
} EntityArchetype;

SpriteID get_sprite_id_from_archetype(EntityArchetype arch) {
	switch (arch) {
		//case arch_item_pine_wood: return SPRITE_item_pine_wood; break;
		//case arch_item_rock: return SPRITE_item_rock; break;
		default: return 0;
	}
}

string get_archetype_pretty_name(EntityArchetype arch) {
	switch (arch) {
		//case arch_item_pine_wood: return STR("Pine Wood");
		//case arch_item_rock: return STR("Rock");
		default: return STR("nil");
	}
}

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
    Vector2 pos;
    bool render_sprite;
    SpriteID sprite_id;
} Entity;
#define MAX_ENTITY_COUNT 1024

typedef struct ItemData {
	int amount;
} ItemData; 

typedef enum UXState {
	UX_nil,
	UX_inventory,
	UX_debug,
} UXState;

typedef struct World{
	Entity entities[MAX_ENTITY_COUNT];
	ItemData inventory_items[ARCH_MAX];
	UXState ux_state;
	float inventory_alpha;
	float inventory_alpha_target;
} World;
World* world = 0;

typedef struct WorldFrame {
	Entity* selected_entity;
} WorldFrame;
WorldFrame world_frame;

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

void setup_spider(Entity* en) {
    en->arch = arch_monster;
    en->sprite_id = SPRITE_spider;
}

void setup_rock(Entity* en) {
    en->arch = arch_rock;
    en->sprite_id = SPRITE_rock;
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

int entry(int argc, char **argv) {
	
	window.title = STR("The Dark");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = hex_to_rgba(0x1e1e1eff);
    float32 aspectRatio = (float32)window.width/(float32)window.height; 
    
    world = alloc(get_heap_allocator(), sizeof(World));
    memset(world, 0, sizeof(World));

    float32 spriteSheetWidth = 240.0;
    float32 zoom = window.width/spriteSheetWidth;
    Vector2 camera_pos = v2(0,0);
   
    sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\undefined.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\dude.png"), get_heap_allocator()) };
    sprites[SPRITE_rock] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\rock.png"), get_heap_allocator()) };
    sprites[SPRITE_spider] = (Sprite){.image = load_image_from_disk(fixed_string("res\\sprites\\spider.png"), get_heap_allocator()) };

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf, %d", GetLastError());	
    const u32 font_height = 48;
	
	render_atlas_if_not_yet_rendered(font, 32, 'A');
	
	float32 input_speed = 50.0;

    Entity* player_en = entity_create();
    setup_player(player_en);

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
        
        draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

        //:camera
        {
            Vector2 target_pos = player_en->pos;
            animate_v2_to_target(&camera_pos, target_pos, delta, 15.0);

            draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
            draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
            draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
        }

        //:mouse
        Vector2 mouse_pos_world = screen_to_world();
		int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
		int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);

		{
			// log("%f, %f", mouse_pos_world.x, mouse_pos_world.y);
			// draw_text(font, sprint(temp, STR("%f %f"), mouse_pos_world.x, mouse_pos_world.y), font_height, mouse_pos_world, v2(0.1, 0.1), COLOR_RED);

			float smallest_dist = INFINITY;
			for (int i = 0; i < MAX_ENTITY_COUNT; i++) {
				Entity* en = &world->entities[i];
				if (en->is_valid) {
					Sprite* sprite = get_sprite(en->sprite_id);

					int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(en->pos.y);

					float dist = fabsf(v2_dist(en->pos, mouse_pos_world));
					if (dist < entity_selection_radius) {
						if (!world_frame.selected_entity || (dist < smallest_dist)) {
							world_frame.selected_entity = en;
							smallest_dist = dist;
						}
					}
				}
			}
		}

		// :tile rendering
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

			draw_rect(v2(tile_pos_to_world_pos(mouse_tile_x) + tile_width * -0.5, tile_pos_to_world_pos(mouse_tile_y) + tile_width * -0.5), v2(tile_width, tile_width), v4(0.5, 0.5, 0.5, 0.5));
		}

        //:input
        Vector2 input_axis = v2(0, 0);
        if (is_key_just_pressed(KEY_ESCAPE)){
            window.should_close = true;
        }
        if (is_key_down(KEY_SPACEBAR)) {
            log("attack!");	
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
        player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, input_speed * delta ));
        

        //:render entities
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
                if (world_frame.selected_entity == en) {
                    col = COLOR_RED;
                }

                draw_image_xform(sprite->image, xform, get_sprite_size(sprite), col);

                // debug pos 
                draw_text(font, sprint(temp, STR("%f %f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE);
            }
        }

        //:ui rendering
		{
			float width = 240.0;
			float height = 135.0;
			draw_frame.view = m4_scalar(1.0);
			draw_frame.projection = m4_make_orthographic_projection(0.0, width, 0.0, height, -1, 10);

			// :inventory UI
			if (is_key_just_pressed(KEY_TAB)) {
				consume_key_just_pressed(KEY_TAB);
				world->ux_state = (world->ux_state == UX_inventory ? UX_nil : UX_inventory);
			}
			world->inventory_alpha_target = (world->ux_state == UX_inventory ? 1.0 : 0.0);
			animate_f32_to_target(&world->inventory_alpha, world->inventory_alpha_target, delta, 15.0);
			bool is_inventory_enabled = world->inventory_alpha_target == 1.0;
			if (world->inventory_alpha_target != 0.0)
			{
				// TODO - some opacity thing here.
				float y_pos = 70.0;

				int item_count = 0;
				for (int i = 0; i < ARCH_MAX; i++) {
					ItemData* item = &world->inventory_items[i];
					if (item->amount > 0) {
						item_count += 1;
					}
				}

				const float icon_thing = 8.0;
				float icon_width = icon_thing;

				const int icon_row_count = 8;

				float entire_thing_width_idk = icon_row_count * icon_width;
				float x_start_pos = (width/2.0)-(entire_thing_width_idk/2.0);

				// bg box rendering thing
				{
					Matrix4 xform = m4_identity;
					xform = m4_translate(xform, v3(x_start_pos, y_pos, 0.0));
					draw_rect_xform(xform, v2(entire_thing_width_idk, icon_width), bg_box_col);
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
                    text = sprint(temp, text, last_fps);
                    draw_text_xform(font, text, font_height, xform, v2(0.1, 0.1), COLOR_WHITE);
				}

				int slot_index = 0;
				for (int i = 0; i < ARCH_MAX; i++) {
					ItemData* item = &world->inventory_items[i];
					if (item->amount > 0) {

						float slot_index_offset = slot_index * icon_width;

						Matrix4 xform = m4_scalar(1.0);
						xform = m4_translate(xform, v3(x_start_pos + slot_index_offset, y_pos, 0.0));

						Sprite* sprite = get_sprite(get_sprite_id_from_archetype(i));

						float is_selected_alpha = 0.0;

						Draw_Quad* quad = draw_rect_xform(xform, v2(8, 8), v4(1, 1, 1, 0.2));
						Range2f icon_box = quad_to_range(*quad);
						if (is_inventory_enabled && range2f_contains(icon_box, get_mouse_pos_in_ndc())) {
							is_selected_alpha = 1.0;
						}

						Matrix4 box_bottom_right_xform = xform;

						xform = m4_translate(xform, v3(icon_width * 0.5, icon_width * 0.5, 0.0));

						if (is_selected_alpha == 1.0)
						{
							float scale_adjust = 0.3; //0.1 * sin_breathe(os_get_current_time_in_seconds(), 20.0);
							xform = m4_scale(xform, v3(1 + scale_adjust, 1 + scale_adjust, 1));
						}
						{
							// could also rotate ...
							// float adjust = PI32 * 2.0 * sin_breathe(os_get_current_time_in_seconds(), 1.0);
							// xform = m4_rotate_z(xform, adjust);
						}
						xform = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, get_sprite_size(sprite).y * -0.5, 0));
						draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);

						// draw_text_xform(font, STR("5"), font_height, box_bottom_right_xform, v2(0.1, 0.1), COLOR_WHITE);

						// tooltip
						if (is_selected_alpha == 1.0)
						{
							Draw_Quad screen_quad = ndc_quad_to_screen_quad(*quad);
							Range2f screen_range = quad_to_range(screen_quad);
							Vector2 icon_center = range2f_get_center(screen_range);

							// icon_center
							Matrix4 xform = m4_scalar(1.0);

							// TODO - we're guessing at the Y box size here.
							// in order to automate this, we would need to move it down below after we do all the element advance things for the text.
							// ... but then the box would be drawing in front of everyone. So we'd have to do Z sorting.
							// Solution for now is to just guess at the size and OOGA BOOGA.
							Vector2 box_size = v2(40, 14);

							// xform = m4_pivot_box(xform, box_size, PIVOT_top_center);
							xform = m4_translate(xform, v3(box_size.x * -0.5, -box_size.y - icon_width * 0.5, 0));
							xform = m4_translate(xform, v3(icon_center.x, icon_center.y, 0));
							draw_rect_xform(xform, box_size, bg_box_col);

							float current_y_pos = icon_center.y;
							{
								string title = get_archetype_pretty_name(i);
								Gfx_Text_Metrics metrics = measure_text(font, title, font_height, v2(0.1, 0.1));
								Vector2 draw_pos = icon_center;
								draw_pos = v2_sub(draw_pos, metrics.visual_pos_min);
								draw_pos = v2_add(draw_pos, v2_mul(metrics.visual_size, v2(-0.5, -1.0))); // top center

								draw_pos = v2_add(draw_pos, v2(0, icon_width * -0.5));
								draw_pos = v2_add(draw_pos, v2(0, -2.0)); // padding

								draw_text(font, title, font_height, draw_pos, v2(0.1, 0.1), COLOR_WHITE);

								current_y_pos = draw_pos.y;
							}

							{
								string text = STR("x%i");
								text = sprint(temp, text, item->amount);

								Gfx_Text_Metrics metrics = measure_text(font, text, font_height, v2(0.1, 0.1));
								Vector2 draw_pos = v2(icon_center.x, current_y_pos);
								draw_pos = v2_sub(draw_pos, metrics.visual_pos_min);
								draw_pos = v2_add(draw_pos, v2_mul(metrics.visual_size, v2(-0.5, -1.0))); // top center

								draw_pos = v2_add(draw_pos, v2(0, -2.0)); // padding

								draw_text(font, text, font_height, draw_pos, v2(0.1, 0.1), COLOR_WHITE);
							}
						}

						slot_index += 1;
					}
				}

			}
		}
        
		gfx_update();
       
	}

	return 0;
}
