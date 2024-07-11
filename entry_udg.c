
int entry(int argc, char **argv) {
	
	window.title = STR("Untitled Dungeon Game");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = hex_to_rgba(0x6495EDff);

    Gfx_Image* player = load_image_from_disk(fixed_string("asesprite\\dude.png"), get_heap_allocator());
    assert(player, "player image didnt load");

	Vector2 player_pos = v2(0,0);
	const float32 input_speed = 1.0;

    float64 last_time = os_get_current_time_in_seconds();

    while (!window.should_close) {
		reset_temporary_storage();
	
        float64 now = os_get_current_time_in_seconds();
		float64 delta = now - last_time;
		last_time = now;	

        os_update(); 
	
        if (is_key_just_pressed(KEY_ESCAPE)){
            window.should_close = true;
        }

		Vector2 input_axis = v2(0, 0);
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

		player_pos = v2_add(player_pos, v2_mulf(input_axis, input_speed * delta));
		
		Matrix4 xform = m4_scalar(1.0);
		xform = m4_translate(xform, v3(v2_expand(player_pos), 0));
		draw_image_xform(player, xform, v2(.5f, .5f), COLOR_WHITE);
		
		gfx_update();
	}

	return 0;
}
