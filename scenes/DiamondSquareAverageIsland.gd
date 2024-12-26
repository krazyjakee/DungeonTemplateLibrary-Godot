@tool
extends Node3D

@export var map_size: int = 200:
	set(new_value):
		map_size = new_value
		draw_island()
@export var min_value: int = 0:
	set(new_value):
		min_value = new_value
		draw_island()
@export var altitude: int = 80:
	set(new_value):
		altitude = new_value
		draw_island()
@export var add_altitude: int = 60:
	set(new_value):
		add_altitude = new_value
		draw_island()

func _ready():
	draw_island()
	
func draw_island():
	var dtl := DTL.new()
	var matrix := dtl.DiamondSquareAverageIsland(map_size, map_size, min_value, altitude, add_altitude)
	for child in get_children():
		if child is DrawMatrix2D or child is DrawMatrix3D:
			child.draw_heightmap(matrix)
