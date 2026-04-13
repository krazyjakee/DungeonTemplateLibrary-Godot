extends Node3D

const CAMERA_OFFSET_Y := 10.0

@onready var demo_container: Node = $DemoContainer
@onready var demo_menu: Control = $DemoMenu
@onready var camera: Camera3D = $Camera3D

func load_demo(scene_path: String) -> void:
	demo_menu.hide()
	var scene := load(scene_path) as PackedScene
	var instance := scene.instantiate()
	demo_container.add_child(instance)
	_position_camera_above_terrain(instance)

func return_to_menu() -> void:
	for child in demo_container.get_children():
		child.queue_free()
	demo_menu.show()

func _position_camera_above_terrain(root: Node) -> void:
	var draw_node := _find_draw_matrix_3d(root)
	if draw_node:
		camera.position = Vector3(0, draw_node.center_height + CAMERA_OFFSET_Y, 0)

func _find_draw_matrix_3d(node: Node) -> DrawMatrix3D:
	if node is DrawMatrix3D:
		return node
	for child in node.get_children():
		var result := _find_draw_matrix_3d(child)
		if result:
			return result
	return null
