@tool
class_name DrawMatrix2D extends Node2D

var default_colors: Array[Color] = [
	Color.DARK_BLUE,
	Color("#e5d9c2"),
	Color("#725428"),
	Color("#b5ba61"),
	Color("#7c8d4c"),
	Color.DARK_OLIVE_GREEN
]

func draw_matrix_texture(matrix: Array, colors: Array[Color] = default_colors):
	var height: int = matrix.size()
	var width: int = matrix[0].size()
	
	# Create an Image to store the pixel data
	var image: Image = Image.create_empty(width, height, false, Image.FORMAT_RGB8)
	
	# Assign colors based on the matrix data
	for y in range(height):
		for x in range(width):
			var cell: int = matrix[y][x]
			var color: Color = Color.HOT_PINK
			if cell < colors.size():
				color = colors[cell]

			image.set_pixel(x, y, color)
	
	# Convert the Image to a texture
	var texture := ImageTexture.create_from_image(image)
	return texture

func draw_matrix(matrix: Array, colors: Array[Color] = default_colors, texture_scale: float = 1.0):
	for child in get_children():
		child.queue_free()
	
	var height: int = matrix.size()
	var width: int = matrix[0].size()
	
	var texture: ImageTexture = draw_matrix_texture(matrix, colors)
	
	# Create a Sprite2D to display the texture
	var sprite := Sprite2D.new()
	sprite.texture = texture
	sprite.scale = Vector2(texture_scale, texture_scale)
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.position = Vector2(width / 2, height / 2) * texture_scale
	
	# Add the sprite as a child of this node so it can be displayed
	add_child(sprite)

func draw_heightmap(matrix: Array):
	for child in get_children():
		child.queue_free()
		
	var height: int = matrix.size()
	var width: int = matrix[0].size()
	
	# Create an Image to store the pixel data
	var image: Image = Image.create_empty(width, height, false, Image.FORMAT_RGB8)
	
	var highest_value: int = -9999
	var lowest_value: int = 9999
	
	for y in range(height):
		for x in range(width):
			var cell: int = matrix[y][x]
			if cell < lowest_value:
				lowest_value = cell
			if cell > highest_value:
				highest_value = cell
			
	# Assign colors based on the matrix data
	for y in range(height):
		for x in range(width):
			var cell: int = matrix[y][x]
			var normalized_value: float = float(cell - lowest_value) / float(highest_value - lowest_value)
			var color = Color(normalized_value, normalized_value, normalized_value)
			image.set_pixel(x, y, color)
	
	# Convert the Image to a texture
	var texture := ImageTexture.create_from_image(image)
	
	# Create a Sprite2D to display the texture
	var sprite := Sprite2D.new()
	sprite.texture = texture
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.position = Vector2(width / 2, height / 2)
	
	# Add the sprite as a child of this node so it can be displayed
	add_child(sprite)
