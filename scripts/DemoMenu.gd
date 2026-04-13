extends Control

const DEMOS := {
	"Terrain": [
		["Cellular Automaton Island", "res://scenes/terrain/CellularAutomatonIsland.tscn"],
		["Cellular Automaton Mix Island", "res://scenes/terrain/CellularAutomatonMixIsland.tscn"],
		["Perlin Island", "res://scenes/terrain/PerlinIsland.tscn"],
		["Perlin Loop Island", "res://scenes/terrain/PerlinLoopIsland.tscn"],
		["Perlin Solitary Island", "res://scenes/terrain/PerlinSolitaryIsland.tscn"],
		["Fractal Island", "res://scenes/terrain/FractalIsland.tscn"],
		["Fractal Loop Island", "res://scenes/terrain/FractalLoopIsland.tscn"],
		["Diamond Square Average Island", "res://scenes/terrain/DiamondSquareAverageIsland.tscn"],
		["Diamond Square Average Corner Island", "res://scenes/terrain/DiamondSquareAverageCornerIsland.tscn"],
		["Simple Voronoi Island", "res://scenes/terrain/SimpleVoronoiIsland.tscn"],
	],
	"Dungeons": [
		["RogueLike", "res://scenes/dungeons/RogueLike.tscn"],
		["Simple RogueLike", "res://scenes/dungeons/SimpleRogueLike.tscn"],
		["Maze Dig", "res://scenes/dungeons/MazeDig.tscn"],
		["Maze Bar", "res://scenes/dungeons/MazeBar.tscn"],
		["Clustering Maze", "res://scenes/dungeons/ClusteringMaze.tscn"],
	],
	"GridMap": [
		["GridMap Items", "res://scenes/gridmap/GridMapItems.tscn"],
	],
}

func _ready() -> void:
	var scroll := ScrollContainer.new()
	scroll.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(scroll)

	var margin := MarginContainer.new()
	margin.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	margin.add_theme_constant_override("margin_left", 40)
	margin.add_theme_constant_override("margin_right", 40)
	margin.add_theme_constant_override("margin_top", 30)
	margin.add_theme_constant_override("margin_bottom", 30)
	scroll.add_child(margin)

	var vbox := VBoxContainer.new()
	vbox.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	vbox.add_theme_constant_override("separation", 6)
	margin.add_child(vbox)

	var title := Label.new()
	title.text = "Dungeon Template Library - Demos"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	vbox.add_child(title)

	vbox.add_child(_spacer(10))

	for category: String in DEMOS:
		var header := Label.new()
		header.text = category
		header.add_theme_font_size_override("font_size", 20)
		vbox.add_child(header)

		for entry: Array in DEMOS[category]:
			var btn := Button.new()
			btn.text = entry[0]
			btn.pressed.connect(_on_demo_pressed.bind(entry[1]))
			vbox.add_child(btn)

		vbox.add_child(_spacer(8))

func _spacer(height: int) -> Control:
	var s := Control.new()
	s.custom_minimum_size.y = height
	return s

func _on_demo_pressed(scene_path: String) -> void:
	var main := get_parent() as Node
	main.load_demo(scene_path)
