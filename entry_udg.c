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

typedef struct Sprite {
    Gfx_Image* image;
} Sprite;

typedef enum SpriteID {
    SPRITE_nil,
    SPRITE_player,
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
    arch_player = 1,
    arch_monster = 2,
    ARCH_MAX,
} EntityArchetype;

typedef struct Entity{
    bool is_valid;
    EntityArchetype arch;
    Vector2 pos;
    bool render_sprite;
    SpriteID sprite_id;
} Entity;
#define MAX_ENTITY_COUNT 1024

typedef struct World{
    Entity entities[MAX_ENTITY_COUNT];
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
    en->arch = arch_player;
    en->sprite_id = SPRITE_player;
}

void setup_spider(Entity* en) {
    en->arch = arch_monster;
    en->sprite_id = SPRITE_spider;
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
   
    sprites[0] = (Sprite){.image = load_image_from_disk(fixed_string("asesprite\\dude.png"), get_heap_allocator()) };
    sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(fixed_string("asesprite\\dude.png"), get_heap_allocator()) };
    sprites[SPRITE_spider] = (Sprite){.image = load_image_from_disk(fixed_string("asesprite\\spider.png"), get_heap_allocator()) };

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf, %d", GetLastError());
	
	render_atlas_if_not_yet_rendered(font, 32, 'A');
	
	float32 input_speed = 50.0;

    Entity* player_en = entity_create();
    setup_player(player_en);

    for (int i = 0; i < 10; i++){
        Entity* en = entity_create();
        setup_spider(en);
        en->pos = v2(i * 10.0, 0.0);
    }

    float64 seconds_counter = 0.0;
    s32 frame_count = 0;
    float64 last_time = os_get_current_time_in_seconds();

    while (!window.should_close) {
		reset_temporary_storage();
	
        float64 now = os_get_current_time_in_seconds();
		float64 delta = now - last_time;
		last_time = now;	
        
        draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);

        //camera
        {
            Vector2 target_pos = player_en->pos;
            animate_v2_to_target(&camera_pos, target_pos, delta, 15.0);

            draw_frame.view = m4_make_scale(v3(1.0, 1.0, 1.0));
            draw_frame.view = m4_mul(draw_frame.view, m4_make_translation(v3(camera_pos.x, camera_pos.y, 0)));
            draw_frame.view = m4_mul(draw_frame.view, m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0)));
        }


        os_update(); 

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
        if (is_key_down('D')) {
            input_axis.x += 1.0;
        }
        if (is_key_down('S')) {
            input_axis.y -= 1.0;
        }
        
        input_axis = v2_normalize(input_axis);

        for (int i = 0; i < MAX_ENTITY_COUNT; i++){
            Entity* en = &world->entities[i];
            if (en->is_valid){
                switch (en->arch){
                    case arch_player:
                        en->pos = v2_add(en->pos, v2_mulf(input_axis, input_speed * delta ));
                        break;
                    case arch_monster:
                        break;
                    default:
                        break;
                }
                Sprite* sprite = get_sprite(en->sprite_id);
                Matrix4 xform = m4_scalar(1.0);
                xform = m4_translate(xform, v3(v2_expand(en->pos), 0));
                xform = m4_translate(xform, v3(get_sprite_size(sprite).x * -0.5, 0.0, 0));
                draw_image_xform(sprite->image, xform, get_sprite_size(sprite), COLOR_WHITE);
            }
        }

		gfx_update();
        
        seconds_counter += delta;
        frame_count+=1;

        if(seconds_counter > 1.0){
            log("fps: %i", frame_count );
            frame_count = 0;
            seconds_counter = 0.0;
        }

	}

	return 0;
}
