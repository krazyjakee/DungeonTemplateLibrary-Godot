@tool
extends Node3D

@export var map_size: int = 200:
	set(new_value):
		map_size = new_value
		draw_island()
@export var truncated_proportion: float = 0.5:
	set(new_value):
		truncated_proportion = new_value
		draw_island()
@export var mountain_proportion: float = 0.45:
	set(new_value):
		mountain_proportion = new_value
		draw_island()
@export var frequency: float = 10.0:
	set(new_value):
		frequency = new_value
		draw_island()
@export var octaves: int = 6:
	set(new_value):
		octaves = new_value
		draw_island()
@export var max_height: int = 200:
	set(new_value):
		max_height = new_value
		draw_island()
@export var min_height: int = 0:
	set(new_value):
		min_height = new_value
		draw_island()

func _ready():
	draw_island()
	
func draw_island():
	var dtl := DTL.new()
	var matrix := dtl.PerlinSolitaryIsland(map_size, map_size, truncated_proportion, mountain_proportion, frequency, octaves, max_height, min_height)
	for child in get_children():
		if child is DrawMatrix2D or child is DrawMatrix3D:
			child.draw_heightmap(matrix)
