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
@export var probability: float = 0.4:
	set(new_value):
		probability = new_value
		draw_island()

func _ready():
	draw_island()
	
func draw_island():
	var dtl := DTL.new()
	var colors: Array[Color] = [Color.DARK_BLUE, Color.DARK_GREEN]
	var matrix := dtl.CellularAutomatonIsland(map_size, map_size, iterations, probability)
	for child in get_children():
		if child is DrawMatrix2D:
			child.draw_matrix(matrix)
		if child is DrawMatrix3D:
			child.draw_terrain(matrix)
