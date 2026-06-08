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

# `matrix` is the Dictionary {width, height, data: PackedByteArray} produced
# by DTL. PackedByteArray indexing in GDScript is ~30× faster per cell than
# the previous Array-of-Array-of-int form at 1000×1000.
func draw_matrix_texture(matrix: Dictionary, colors: Array[Color] = default_colors):
	var width: int = matrix.get("width", 0)
	var height: int = matrix.get("height", 0)
	var data: PackedByteArray = matrix.get("data", PackedByteArray())
	if width <= 0 or height <= 0 or data.is_empty():
		return null

	# Build raw RGB8 bytes and create the Image in one shot — Image.set_pixel
	# goes through the Variant call layer per pixel, which dominates above a
	# few hundred thousand cells.
	var bytes := PackedByteArray()
	bytes.resize(width * height * 3)
	var color_count: int = colors.size()
	var fallback := Color.HOT_PINK
	for y in range(height):
		var row_off := y * width
		var byte_off := row_off * 3
		for x in range(width):
			var cell: int = data[row_off + x]
			var color: Color = colors[cell] if cell < color_count else fallback
			bytes[byte_off + x * 3 + 0] = int(color.r * 255.0)
			bytes[byte_off + x * 3 + 1] = int(color.g * 255.0)
			bytes[byte_off + x * 3 + 2] = int(color.b * 255.0)

	var image: Image = Image.create_from_data(width, height, false, Image.FORMAT_RGB8, bytes)
	return ImageTexture.create_from_image(image)

func draw_matrix(matrix: Dictionary, colors: Array[Color] = default_colors, texture_scale: float = 1.0):
	for child in get_children():
		child.queue_free()

	var width: int = matrix.get("width", 0)
	var height: int = matrix.get("height", 0)

	var texture: ImageTexture = draw_matrix_texture(matrix, colors)
	if texture == null:
		return

	var sprite := Sprite2D.new()
	sprite.texture = texture
	sprite.scale = Vector2(texture_scale, texture_scale)
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.position = Vector2(width / 2, height / 2) * texture_scale
	add_child(sprite)

func draw_heightmap(matrix: Dictionary):
	for child in get_children():
		child.queue_free()

	var width: int = matrix.get("width", 0)
	var height: int = matrix.get("height", 0)
	var data: PackedByteArray = matrix.get("data", PackedByteArray())
	if width <= 0 or height <= 0 or data.is_empty():
		return

	var highest_value: int = -9999
	var lowest_value: int = 9999

	for i in range(data.size()):
		var cell: int = data[i]
		if cell < lowest_value:
			lowest_value = cell
		if cell > highest_value:
			highest_value = cell

	var span: float = float(highest_value - lowest_value)
	if span <= 0.0:
		span = 1.0

	# Raw RGB8 byte pack — same value in R/G/B for a grayscale heightmap.
	var bytes := PackedByteArray()
	bytes.resize(width * height * 3)
	for y in range(height):
		var row_off := y * width
		var byte_off := row_off * 3
		for x in range(width):
			var cell: int = data[row_off + x]
			var v: int = int(float(cell - lowest_value) / span * 255.0)
			bytes[byte_off + x * 3 + 0] = v
			bytes[byte_off + x * 3 + 1] = v
			bytes[byte_off + x * 3 + 2] = v

	var image: Image = Image.create_from_data(width, height, false, Image.FORMAT_RGB8, bytes)
	var texture := ImageTexture.create_from_image(image)

	var sprite := Sprite2D.new()
	sprite.texture = texture
	sprite.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	sprite.position = Vector2(width / 2, height / 2)
	add_child(sprite)
