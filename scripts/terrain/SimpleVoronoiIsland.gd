@tool
extends Node3D

@export var map_size: int = 200:
	set(new_value):
		map_size = new_value
		draw_island()
@export var voronoi_num: float = 40.0:
	set(new_value):
		voronoi_num = new_value
		draw_island()
@export var probability: float = 0.5:
	set(new_value):
		probability = new_value
		draw_island()

func _ready():
	draw_island()
	
func draw_island():
	var colors: Array[Color] = [Color.DARK_BLUE, Color.GREEN]
	var dtl := DTL.new()
	var matrix := dtl.SimpleVoronoiIsland(map_size, map_size, voronoi_num, probability)
	for child in get_children():
		if child is DrawMatrix2D or child is DrawMatrix3D:
			child.draw_matrix(matrix, colors)
