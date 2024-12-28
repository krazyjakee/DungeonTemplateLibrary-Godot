@tool
extends Node2D

@export var map_size: int = 200:
	set(new_value):
		map_size = new_value
		draw_dungeon()
@export var division_min: int = 50:
	set(new_value):
		division_min = new_value
		draw_dungeon()
@export var division_max: int = 50:
	set(new_value):
		division_max = new_value
		draw_dungeon()
@export var min_room_size: Vector2 = Vector2(3, 3):
	set(new_value):
		min_room_size = new_value
		draw_dungeon()
@export var max_room_size: Vector2 = Vector2(4, 4):
	set(new_value):
		max_room_size = new_value
		draw_dungeon()


func _ready():
	draw_dungeon()
	
func draw_dungeon():
	var colors: Array[Color] = [
		Color.DEEP_SKY_BLUE,
		Color.WHITE,
		Color.WHITE
	]
	var dtl := DTL.new()
	var matrix := dtl.SimpleRogueLike(map_size, map_size, division_min, division_max, min_room_size.x, min_room_size.y, max_room_size.x, max_room_size.y)
	for child in get_children():
		if child is DrawMatrix2D:
			child.draw_matrix(matrix, colors, 3.0)
		if child is DrawMatrix3D:
			child.draw_terrain(matrix)
