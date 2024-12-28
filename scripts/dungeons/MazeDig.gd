@tool
extends Node2D

@export var map_size: int = 200:
	set(new_value):
		map_size = new_value
		draw_dungeon()

func _ready():
	draw_dungeon()
	
func draw_dungeon():
	var colors: Array[Color] = [
		Color.DEEP_SKY_BLUE,
		Color.WHITE
	]
	var dtl := DTL.new()
	var matrix := dtl.MazeDig(map_size, map_size)
	for child in get_children():
		if child is DrawMatrix2D:
			child.draw_matrix(matrix, colors, 3.0)
		if child is DrawMatrix3D:
			child.draw_terrain(matrix)
