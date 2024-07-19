typedef enum EntityArchetype{
    arch_nil = 0,
    arch_player = 1,
    arch_monster = 2,
} Archetype;

typedef struct Entity{
    bool is_valid;
    Archetype arch;
    Vector2 pos;
    
    bool render_sprite;
    Vector2 size;
    Gfx_Image* sprite;
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
    en->sprite = load_image_from_disk(fixed_string("asesprite\\dude.png"), get_heap_allocator());
    en->size = (Vector2){en->sprite->width, en->sprite->height};
}

void setup_spider(Entity* en) {
    en->arch = arch_monster;
    en->sprite = load_image_from_disk(fixed_string("asesprite\\spider.png"), get_heap_allocator());
    en->size = (Vector2){en->sprite->width, en->sprite->height};
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

    float32 spriteSheetWidth = 240.0;
    float32 zoom = window.width/spriteSheetWidth;

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
	
        draw_frame.projection = m4_make_orthographic_projection(window.width * -0.5, window.width * 0.5, window.height * -0.5, window.height * 0.5, -1, 10);
        draw_frame.view = m4_make_scale(v3(1.0/zoom, 1.0/zoom, 1.0/zoom));

        float64 now = os_get_current_time_in_seconds();
		float64 delta = now - last_time;
		last_time = now;	

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
                Matrix4 xform = m4_scalar(1.0);
                xform = m4_translate(xform, v3(v2_expand(en->pos), 0));
                xform = m4_translate(xform, v3(en->size.x * -0.5, 0.0, 0));
                draw_image_xform(en->sprite, xform, en->size, COLOR_WHITE);
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
