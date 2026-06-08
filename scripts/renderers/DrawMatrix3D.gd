@tool
class_name DrawMatrix3D extends Node3D

# Emitted after draw_terrain() finishes configuring both materials with a
# fresh heightmap. Consumers (e.g. WaterPlane) rebuild off this instead of
# every scene's per-terrain script forwarding the matrix manually.
signal terrain_generated

@export var amplitude: float = 10.0: set = _set_amplitude
@export var sea_level: float = 10.0: set = _set_sea_level
@export var beach_level: float = 10.0: set = _set_beach_level
@export var grass_level: float = 10.0: set = _set_grass_level
@export var cliff_level: float = 10.0: set = _set_cliff_level
@export var snow_level: float = 10.0: set = _set_snow_level
@export var terrain_size: float = 500.0: set = _set_terrain_size
@export var max_subdivisions: int = 256

# --- Paths & Roads ---------------------------------------------------------
# Both layers share path_road_seed so a given seed reproduces the whole
# network. Paths are narrow/winding trails; roads are wider and graded
# flatter. Routes only ever run above sea level and pathfind *around* steep
# terrain rather than straight over it. All of the heavy lifting (blur,
# waypoints, full-resolution A*, carve) happens in the C++ TerrainBuilder.
@export_group("Paths & Roads")
@export var paths_enabled: bool = false: set = _set_paths_enabled
@export var roads_enabled: bool = false: set = _set_roads_enabled
@export var path_road_seed: int = 0: set = _set_path_road_seed
# Number of waypoints connected into each network (min-spanning-tree).
@export var path_node_count: int = 7: set = _set_path_node_count
@export var road_node_count: int = 5: set = _set_road_node_count
# Carved track width, in world units.
@export var path_width: float = 4.0: set = _set_path_width
@export var road_width: float = 11.0: set = _set_road_width
# Max longitudinal grade (rise/run) of the carved centerline. Lower = flatter
# road; this is the "don't angle it too extremely" control.
@export var path_max_grade: float = 0.14: set = _set_path_max_grade
@export var road_max_grade: float = 0.08: set = _set_road_max_grade
# 0 = ignore terrain steepness when routing, 1 = aggressively detour around
# hills/mountains instead of climbing them.
@export var hill_avoidance: float = 0.7: set = _set_hill_avoidance
@export var path_color: Color = Color("#6b5a3e"): set = _set_path_color
@export var road_color: Color = Color("#80807a"): set = _set_road_color

@export_group("Surface Textures")
# World size (in metres) of one tile of the detail / macro texture layers.
@export var texture_world_size: float = 6.0: set = _set_texture_world_size
@export var macro_world_size: float = 52.0: set = _set_macro_world_size
@export var detail_normal_strength: float = 1.0: set = _set_detail_normal_strength

@export_group("Water Animation")
@export var water_wave_speed: float = 0.35: set = _set_water_wave_speed
@export var water_wave_scale: float = 80.0: set = _set_water_wave_scale
@export var water_wave_strength: float = 0.2: set = _set_water_wave_strength

@export_group("Water Edge")
@export var water_edge_color: Color = Color(0.85, 0.95, 1.0): set = _set_water_edge_color
@export var water_edge_width: float = 5.0: set = _set_water_edge_width
@export var water_edge_intensity: float = 0.9: set = _set_water_edge_intensity

var center_height: float = 0.0
var terrain_material := ShaderMaterial.new()
var water_material := ShaderMaterial.new()
var terrain_shader: Shader = load("res://assets/shaders/terrain.gdshader")
var water_shader: Shader = load("res://assets/shaders/water.gdshader")

# The carved heightmap + path/road mask, produced by TerrainBuilder.
# TerrainLOD reads these directly so the visible LOD mesh, its colliders, and
# the water plane all share one deformed source of truth.
var height_image: Image
var height_texture: ImageTexture
var path_mask_texture: ImageTexture
# Raw 0..1 height samples, same dims as height_texture. TerrainLOD samples
# these for collider building so it never has to pay Image::get_pixel().
var heights_packed: PackedFloat32Array
var heights_width: int = 0
var heights_height: int = 0

var default_colors: Array[Color] = [
	Color.DARK_BLUE,
	Color("#e5d9c2"),
	Color("#725428"),
	Color("#b5ba61"),
	Color("#7c8d4c"),
	Color.DARK_OLIVE_GREEN
]

# Cached so a Paths & Roads tweak in the editor can regenerate without the
# parent terrain script re-running the (expensive) DTL pass when possible.
# Matrix format is the Dictionary {width, height, data: PackedByteArray}
# produced by DTL.
var _last_matrix: Dictionary = {}
var _initialized: bool = false

# Surface name -> base of the generated albedo/normal pair. Sampled triplanar
# at two scales in the terrain shader to break tiling.
const SURFACE_TEX := {
	"sand": "sand", "grass": "grass", "rock": "rock",
	"snow": "snow", "path": "dirt", "road": "gravel",
}
const TEX_DIR := "res://assets/textures/terrain/"

func _ready():
	terrain_material.shader = terrain_shader
	water_material.shader = water_shader
	_assign_surface_textures()

# Loads the Imagen-generated PBR maps once and binds them to the terrain
# shader. Safe to call repeatedly; ShaderMaterial keeps the params across the
# shader reassignment in draw_terrain().
func _assign_surface_textures() -> void:
	for shader_name in SURFACE_TEX:
		var base: String = SURFACE_TEX[shader_name]
		var alb: Texture2D = load(TEX_DIR + base + "_albedo.png")
		var nrm: Texture2D = load(TEX_DIR + base + "_normal.png")
		if alb != null:
			terrain_material.set_shader_parameter(shader_name + "_albedo", alb)
		if nrm != null:
			terrain_material.set_shader_parameter(shader_name + "_normal", nrm)
	terrain_material.set_shader_parameter("texture_world_size", texture_world_size)
	terrain_material.set_shader_parameter("macro_world_size", macro_world_size)
	terrain_material.set_shader_parameter("detail_normal_strength", detail_normal_strength)

func _set_texture_world_size(v: float):
	texture_world_size = maxf(0.5, v)
	terrain_material.set_shader_parameter("texture_world_size", texture_world_size)

func _set_macro_world_size(v: float):
	macro_world_size = maxf(8.0, v)
	terrain_material.set_shader_parameter("macro_world_size", macro_world_size)

func _set_detail_normal_strength(v: float):
	detail_normal_strength = clampf(v, 0.0, 2.0)
	terrain_material.set_shader_parameter("detail_normal_strength", detail_normal_strength)

func _set_amplitude(new_value: float):
	amplitude = new_value
	terrain_material.set_shader_parameter("amplitude", new_value)
	water_material.set_shader_parameter("amplitude", new_value)

func _set_sea_level(new_value: float):
	sea_level = new_value
	terrain_material.set_shader_parameter("sea_level", new_value)
	water_material.set_shader_parameter("sea_level", new_value)

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

func _set_terrain_size(new_value: float):
	terrain_size = new_value
	water_material.set_shader_parameter("terrain_size", new_value)

func _set_water_wave_speed(new_value: float):
	water_wave_speed = new_value
	water_material.set_shader_parameter("wave_speed", new_value)

func _set_water_wave_scale(new_value: float):
	water_wave_scale = new_value
	water_material.set_shader_parameter("wave_scale", new_value)

func _set_water_wave_strength(new_value: float):
	water_wave_strength = new_value
	water_material.set_shader_parameter("wave_strength", new_value)

func _set_water_edge_color(new_value: Color):
	water_edge_color = new_value
	water_material.set_shader_parameter("edge_color", new_value)

func _set_water_edge_width(new_value: float):
	water_edge_width = new_value
	water_material.set_shader_parameter("edge_width", new_value)

func _set_water_edge_intensity(new_value: float):
	water_edge_intensity = new_value
	water_material.set_shader_parameter("edge_intensity", new_value)

# --- Paths & Roads setters: these change heightmap generation, so they need a
# full regenerate. Guarded by _initialized so scene load doesn't thrash.
func _set_paths_enabled(v: bool):
	paths_enabled = v
	_regenerate()

func _set_roads_enabled(v: bool):
	roads_enabled = v
	_regenerate()

func _set_path_road_seed(v: int):
	path_road_seed = v
	_regenerate()

func _set_path_node_count(v: int):
	path_node_count = maxi(2, v)
	_regenerate()

func _set_road_node_count(v: int):
	road_node_count = maxi(2, v)
	_regenerate()

func _set_path_width(v: float):
	path_width = maxf(0.5, v)
	_regenerate()

func _set_road_width(v: float):
	road_width = maxf(0.5, v)
	_regenerate()

func _set_path_max_grade(v: float):
	path_max_grade = clampf(v, 0.01, 1.0)
	_regenerate()

func _set_road_max_grade(v: float):
	road_max_grade = clampf(v, 0.01, 1.0)
	_regenerate()

func _set_hill_avoidance(v: float):
	hill_avoidance = clampf(v, 0.0, 1.0)
	_regenerate()

func _set_path_color(v: Color):
	path_color = v
	terrain_material.set_shader_parameter("path_color", v)

func _set_road_color(v: Color):
	road_color = v
	terrain_material.set_shader_parameter("road_color", v)

func _regenerate():
	if not _initialized:
		return
	var parent := get_parent()
	if parent != null and parent.has_method("draw_island"):
		# Re-runs every renderer (DrawMatrix3D + TerrainLOD + water) so the
		# visible LOD mesh and colliders pick up the new carve too.
		parent.draw_island()
	elif not _last_matrix.is_empty():
		draw_terrain(_last_matrix)

func _build_config() -> Dictionary:
	return {
		"amplitude": amplitude,
		"sea_level": sea_level,
		"terrain_size": terrain_size,
		"paths_enabled": paths_enabled,
		"roads_enabled": roads_enabled,
		"seed": path_road_seed,
		"path_node_count": path_node_count,
		"road_node_count": road_node_count,
		"path_width": path_width,
		"road_width": road_width,
		"path_max_grade": path_max_grade,
		"road_max_grade": road_max_grade,
		"hill_avoidance": hill_avoidance,
	}

# Runs the C++ pipeline (blur -> path/road carve -> textures) and caches the
# results. `matrix` is the Dictionary {width, height, data: PackedByteArray}
# produced by DTL.
func draw_heightmap(matrix: Dictionary) -> ImageTexture:
	var builder := TerrainBuilder.new()
	var result: Dictionary = builder.build(matrix, _build_config())
	height_texture = result.get("height")
	path_mask_texture = result.get("mask")
	heights_packed = result.get("heights", PackedFloat32Array())
	# Heights buffer is the same dims as the input matrix.
	heights_width = int(matrix.get("width", 0))
	heights_height = int(matrix.get("height", 0))
	# Avoid get_image()/get_pixel — TerrainBuilder hands back the carved
	# center height directly. `center_height` from build() is already in world
	# units, so don't multiply by amplitude here.
	center_height = float(result.get("center_height", 0.0))
	height_image = null
	return height_texture

func draw_terrain(matrix: Dictionary):
	_last_matrix = matrix
	for child in get_children():
		child.queue_free()

	terrain_material.shader = terrain_shader
	water_material.shader = water_shader
	_assign_surface_textures()
	var height_tex := draw_heightmap(matrix)
	if height_tex == null:
		return
	var tex_size := height_tex.get_size()
	terrain_material.set_shader_parameter("amplitude", amplitude)
	terrain_material.set_shader_parameter("height_texture", height_tex)
	terrain_material.set_shader_parameter("texel_size", Vector2(1.0 / tex_size.x, 1.0 / tex_size.y))
	terrain_material.set_shader_parameter("path_mask", path_mask_texture)
	terrain_material.set_shader_parameter("path_color", path_color)
	terrain_material.set_shader_parameter("road_color", road_color)

	# Water material shares the heightmap so it can sample seafloor depth
	# per-fragment for color gradient, alpha, and shoreline foam.
	water_material.set_shader_parameter("amplitude", amplitude)
	water_material.set_shader_parameter("sea_level", sea_level)
	water_material.set_shader_parameter("terrain_size", terrain_size)
	water_material.set_shader_parameter("height_texture", height_tex)

	var subs: int = mini(maxi(int(tex_size.x), int(tex_size.y)), max_subdivisions)
	var quadmesh := PlaneMesh.new()
	quadmesh.orientation = PlaneMesh.FACE_Y
	quadmesh.size = Vector2(terrain_size, terrain_size)
	quadmesh.subdivide_width = subs
	quadmesh.subdivide_depth = subs
	quadmesh.material = terrain_material

	var mesh := MeshInstance3D.new()
	mesh.mesh = quadmesh
	add_child(mesh)

	_initialized = true
	terrain_generated.emit()
