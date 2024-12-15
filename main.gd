@tool
extends Node2D

@export var map_size: int = 1000:
	set(new_value):
		map_size = new_value
		draw_island()
@export var iterations: int = 5:
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
	for child in get_children():
		child.queue_free()
		
	var dtl = DTL.new()
	var island = dtl.CellularAutomatonMixIsland(map_size, map_size, iterations, land_values)
	var height = island.size()
	var width = island[0].size()
	
	# Create an Image to store the pixel data
	var image = Image.create_empty(width, height, false, Image.FORMAT_RGB8)
	
	# Assign colors based on the island data
	for y in range(height):
		for x in range(width):
			var cell = island[y][x]
			var color = Color.DARK_BLUE
			
			if cell == 1:
				color = Color("#e5d9c2")
			elif cell == 2:
				color = Color("#725428")
			elif cell == 3:
				color = Color("#b5ba61")
			elif cell == 4:
				color = Color("#7c8d4c")
			elif cell == 5:
				color = Color.DARK_OLIVE_GREEN

			image.set_pixel(x, y, color)
	
	# Convert the Image to a texture
	var texture = ImageTexture.create_from_image(image)
	
	# Create a Sprite2D to display the texture
	var sprite = Sprite2D.new()
	sprite.texture = texture
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.position = Vector2(width / 2, height / 2)
	
	# Add the sprite as a child of this node so it can be displayed
	add_child(sprite)
