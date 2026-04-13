extends Node

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		var main := get_tree().current_scene
		if main.has_method("return_to_menu") and main.get_node("DemoContainer").get_child_count() > 0:
			main.return_to_menu()
