@tool
extends Node2D

@export var map_size: int = 200:
	set(new_value):
		map_size = new_value
		draw_island()
@export var iterations: int = 200:
	set(new_value):
		iterations = new_value
		draw_island()
@export var land_values: int = 5:
	set(new_value):
		land_values = new_value
		draw_island()

func _ready():
	draw_island()
	
func draw_island():
	var dtl := DTL.new()
	var matrix := dtl.CellularAutomatonMixIsland(map_size, map_size, iterations, land_values)
	for child in get_children():
		if child is DrawMatrix2D:
			child.draw_matrix(matrix)
		if child is DrawMatrix3D:
			child.draw_terrain(matrix)
