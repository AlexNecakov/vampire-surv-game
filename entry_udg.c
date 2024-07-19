typedef enum Global_States {
    STATE_INPUT = 0,
    STATE_MOVE,
    STATE_MOVING,
    STATE_ATTACK,
    STATE_INVENTORY,
    STATE_FLOOR,
} Global_States;

typedef enum Action_Flags {
	ACTION_MOVE_UP      = 1<<0, 
    ACTION_MOVE_DOWN    = 1<<1,
	ACTION_MOVE_LEFT    = 1<<2,
	ACTION_MOVE_RIGHT   = 1<<3,
} Action_Flags;

int entry(int argc, char **argv) {
	
	window.title = STR("Untitled Dungeon Game");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
    window.x = 200;
    window.y = 200;
	window.clear_color = hex_to_rgba(0x6495EDff);

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf, %d", GetLastError());
	
	render_atlas_if_not_yet_rendered(font, 32, 'A');
    
    Gfx_Image* player = load_image_from_disk(fixed_string("asesprite\\dude.png"), get_heap_allocator());
    assert(player, "player image didnt load");

    Gfx_Image* playerAtk = load_image_from_disk(fixed_string("asesprite\\dude_attack.png"), get_heap_allocator());
    assert(playerAtk, "player image didnt load");
	
    Vector2 player_pos = v2(0,0);
    Vector2 input_axis = v2(0, 0);
	const float32 input_speed = 1.0;
    float64 seconds_counter = 0.0;
    s32 frame_count = 0;

    float64 last_time = os_get_current_time_in_seconds();

    Global_States globalState = STATE_INPUT;
    Action_Flags actionQueue = 0;

    while (!window.should_close) {
		reset_temporary_storage();
	
        float64 now = os_get_current_time_in_seconds();
		float64 delta = now - last_time;
		last_time = now;	

        os_update(); 
	
        Global_States nextState = globalState;
        
        float64 gridSize = 0.5;
        float64 xOffset = 0;
        float64 yOffset = 1;
        bool startBlk = false;
        for (int i = -10; i < 10; i++){
            for(int j = 10; j > -10; j--){
                draw_rect(v2(i*gridSize - xOffset, j*gridSize ), v2(gridSize, gridSize), COLOR_PINK);
                if((startBlk+j)%2){
                    draw_rect(v2(i*gridSize - xOffset, j*gridSize ), v2(gridSize, gridSize), COLOR_GREEN);
                }
            }
            startBlk = !startBlk;
        }
    
        if (is_key_just_pressed(KEY_ESCAPE)){
            window.should_close = true;
        }

        Matrix4 xform = m4_scalar(1.0);
        
        switch(globalState){
            case STATE_INPUT: 
                if (is_key_down(KEY_SPACEBAR)) {
                    log("attack!");	
                }	
                if (is_key_down('W')) {
                    actionQueue |= ACTION_MOVE_UP;
                    nextState = STATE_MOVE;
                }
                if (is_key_down('A')) {
                    actionQueue |= ACTION_MOVE_LEFT;
                    nextState = STATE_MOVE;
                }
                if (is_key_down('D')) {
                    actionQueue |= ACTION_MOVE_RIGHT;
                    nextState = STATE_MOVE;
                }
                if (is_key_down('S')) {
                    actionQueue |= ACTION_MOVE_DOWN;
                    nextState = STATE_MOVE;
                }
                break;
                
            case STATE_MOVE: 
                input_axis = (Vector2){0,0};
                if (actionQueue & ACTION_MOVE_UP) {
                    input_axis.y += 1.0;
                    log("up");
                }
                if (actionQueue & ACTION_MOVE_LEFT) {
                    input_axis.x -= 1.0;
                    log("left");
                }
                if (actionQueue & ACTION_MOVE_RIGHT) {
                    input_axis.x += 1.0;
                    log("right");
                }
                if (actionQueue & ACTION_MOVE_DOWN) {
                    input_axis.y -= 1.0;
                    log("down");
                }
                input_axis = v2_normalize(input_axis);
                
                player_pos = v2_add(player_pos, v2_mulf(input_axis, input_speed * delta ));
                nextState = STATE_MOVING;
                actionQueue = 0;
                break;
            case STATE_MOVING: 
                player_pos = v2_add(player_pos, v2_mulf(input_axis, input_speed * delta ));
                nextState = STATE_MOVING;
            default:
                nextState = STATE_INPUT;
                break;
        }

        xform = m4_translate(xform, v3(v2_expand(player_pos), 0));
        draw_image_xform(player, xform, v2(.5f, .5f), COLOR_WHITE);
        
		gfx_update();
        
        seconds_counter += delta;
        frame_count+=1;

        if(seconds_counter > 1.0){
            log("fps: %i", frame_count );
            frame_count = 0;
            seconds_counter = 0.0;
        }

        globalState = nextState;
	}

	return 0;
}
