@tool
class_name DrawMatrix3D extends Node3D

@export var amplitude: float = 10.0: set = _set_amplitude
@export var sea_level: float = 10.0: set = _set_sea_level
@export var beach_level: float = 10.0: set = _set_beach_level
@export var grass_level: float = 10.0: set = _set_grass_level
@export var cliff_level: float = 10.0: set = _set_cliff_level
@export var snow_level: float = 10.0: set = _set_snow_level

var terrain_material := ShaderMaterial.new()
var terrain_shader: Shader = load("res://assets/shaders/terrain.gdshader")
var default_colors: Array[Color] = [
	Color.DARK_BLUE,
	Color("#e5d9c2"),
	Color("#725428"),
	Color("#b5ba61"),
	Color("#7c8d4c"),
	Color.DARK_OLIVE_GREEN
]

func _set_amplitude(new_value: float):
	amplitude = new_value
	terrain_material.set_shader_parameter("amplitude", new_value)

func _set_sea_level(new_value: float):
	sea_level = new_value
	terrain_material.set_shader_parameter("sea_level", new_value)

func _set_beach_level(new_value: float):
	beach_level = new_value
	terrain_material.set_shader_parameter("beach_level", new_value)

func _set_grass_level(new_value: float):
	grass_level = new_value
	terrain_material.set_shader_parameter("grass_level", new_value)

func _set_cliff_level(new_value: float):
	cliff_level = new_value
	terrain_material.set_shader_parameter("cliff_level", new_value)

func _set_snow_level(new_value: float):
	snow_level = new_value
	terrain_material.set_shader_parameter("snow_level", new_value)
	
func draw_heightmap(matrix: Array) -> ImageTexture:
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
	
	return texture

func draw_terrain(matrix: Array):
	for child in get_children():
		child.queue_free()
		
	var mesh := MeshInstance3D.new()
	var quadmesh := QuadMesh.new()
	quadmesh.add_uv2 = true 
	quadmesh.orientation = PlaneMesh.FACE_Y
	quadmesh.size = Vector2(500, 500)
	quadmesh.subdivide_width = 500
	quadmesh.subdivide_depth = 500
	
	terrain_material.shader = terrain_shader
	quadmesh.material = terrain_material
	mesh.mesh = quadmesh
	terrain_material.set_shader_parameter("amplitude", amplitude)
	terrain_material.set_shader_parameter("height_texture", draw_heightmap(matrix))
	
	add_child(mesh)
