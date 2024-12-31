@tool
class_name DrawRuins3D extends GridMap

@export var horizontal_orientation: int = 16: set = _set_horizontal_orientation
@export var wall_movement: int = 50: set = _set_wall_movement

var _matrix

func _set_horizontal_orientation(new_value: int):
	horizontal_orientation = new_value
	if _matrix:
		draw_ruins(_matrix)
		
func _set_wall_movement(new_value: int):
	wall_movement = new_value
	if _matrix:
		draw_ruins(_matrix)

func get_item_id(name: String):
	return mesh_library.find_item_by_name(name)

func get_surrounding_cells(matrix: Array, x: int, y: int) -> Array:
	var height: int = matrix.size()
	var width: int = matrix[0].size()
	var cells = []
	
	# Loop through all offsets in a 3x3 grid around (x, y), skipping (0, 0)
	for dy in range(-1, 2):
		for dx in range(-1, 2):
			# Skip the center cell
			if dx == 0 and dy == 0:
				continue
				
			var nx = x + dx
			var ny = y + dy
			
			# Check boundaries
			if nx >= 0 and nx < width and ny >= 0 and ny < height:
				cells.append(matrix[ny][nx])
	
	return cells

func get_orthogonal_surrounding_cells(matrix: Array, x: int, y: int):
	var height: int = matrix.size()
	var width: int = matrix[0].size()
	var cells = []

	# Up
	if y - 1 >= 0:
		cells.append(matrix[y - 1][x])

	# Right
	if x + 1 < width:
		cells.append(matrix[y][x + 1])

	# Down
	if y + 1 < height:
		cells.append(matrix[y + 1][x])

	# Left
	if x - 1 >= 0:
		cells.append(matrix[y][x - 1])

	return cells

func draw_ruins(matrix: Array):
	_matrix = matrix
	var height: int = matrix.size()
	var width: int = matrix[0].size()
	var item_names = ["", "Wall", "Floor_Squares", "Arch_Gothic", "Floor_Diamond", "Column_Round_Short"]
	var item_ids = item_names.map(get_item_id)
	
	clear()
	
	# Assign colors based on the matrix data
	for y in range(height):
		for x in range(width):
			var cell: int = matrix[y][x]
			if cell < item_names.size() and cell > 0:
				var type_id = item_ids[cell]
				var type_name = item_names[cell]
				var orientation = 0
				var new_x = x * 100
				var new_y = y * 100
				if type_name == "Arch_Gothic":
					var surrounding_cells = get_orthogonal_surrounding_cells(matrix, x, y)
					for i in range(surrounding_cells.size()):
						var surrounding_item_name = item_names[surrounding_cells[i]]
						if ["Floor_Squares", "Floor_Diamond"].has(surrounding_item_name):
							if i == 1 or i == 3:
								orientation = horizontal_orientation
								break
				
				if ["Floor_Squares", "Floor_Diamond"].has(type_name):
					# Place walls
					var orthogonal_surrounding_cells = get_orthogonal_surrounding_cells(matrix, x, y)
					for i in range(orthogonal_surrounding_cells.size()):
						if orthogonal_surrounding_cells[i] == 1:
							if i == 0:
								set_cell_item(Vector3i(new_x, 0, new_y - wall_movement), item_ids[1], orientation)
							if i == 1:
								set_cell_item(Vector3i(new_x + wall_movement, 0, new_y), item_ids[1], horizontal_orientation)
							if i == 2:
								set_cell_item(Vector3i(new_x, 0, new_y + wall_movement), item_ids[1], orientation)
							if i == 3:
								set_cell_item(Vector3i(new_x - wall_movement, 0, new_y), item_ids[1], horizontal_orientation)
								
				if type_name == "Floor_Diamond":
					var surrounding_cells = get_surrounding_cells(matrix, x, y)
	
					# -- Place columns in corners if the corner is a Wall (1) 
					#    and the two adjacent orth cells around that corner are Floors (2 or 4).
					# -- Make sure we actually got all 8 neighbors (surrounding_cells.size() == 8).
					if surrounding_cells.size() == 8:
						# Top-left corner: index 0 is top-left, index 1 is top, index 3 is left
						if surrounding_cells[0] == 1 \
						and surrounding_cells[1] in [2, 4] \
						and surrounding_cells[3] in [2, 4]:
							set_cell_item(Vector3i(new_x - wall_movement, 0, new_y - wall_movement),
								item_ids[5], orientation)
						
						# Top-right corner: index 2 is top-right, index 1 is top, index 4 is right
						if surrounding_cells[2] == 1 \
						and surrounding_cells[1] in [2, 4] \
						and surrounding_cells[4] in [2, 4]:
							set_cell_item(Vector3i(new_x + wall_movement, 0, new_y - wall_movement),
								item_ids[5], orientation)
						
						# Bottom-left corner: index 5 is bottom-left, index 3 is left, index 6 is bottom
						if surrounding_cells[5] == 1 \
						and surrounding_cells[3] in [2, 4] \
						and surrounding_cells[6] in [2, 4]:
							set_cell_item(Vector3i(new_x - wall_movement, 0, new_y + wall_movement),
								item_ids[5], orientation)
						
						# Bottom-right corner: index 7 is bottom-right, index 4 is right, index 6 is bottom
						if surrounding_cells[7] == 1 \
						and surrounding_cells[4] in [2, 4] \
						and surrounding_cells[6] in [2, 4]:
							set_cell_item(Vector3i(new_x + wall_movement, 0, new_y + wall_movement),
								item_ids[5], orientation)
								
				if type_name == "Arch_Gothic":
					set_cell_item(Vector3i(new_x, -1, new_y), item_ids[2], 0)
					var doorway_wall_movement = wall_movement
					if orientation == horizontal_orientation:
						set_cell_item(Vector3i(new_x, 0, new_y - 5), type_id, orientation)
						set_cell_item(Vector3i(new_x, 0, new_y - doorway_wall_movement), item_ids[1], 0)
						set_cell_item(Vector3i(new_x, 0, new_y + doorway_wall_movement), item_ids[1], 0)
					else:
						set_cell_item(Vector3i(new_x - 5, 0, new_y), type_id, orientation)
						set_cell_item(Vector3i(new_x - doorway_wall_movement, 0, new_y), item_ids[1], horizontal_orientation)
						set_cell_item(Vector3i(new_x + doorway_wall_movement, 0, new_y), item_ids[1], horizontal_orientation)
						
					var surrounding_cells = get_surrounding_cells(matrix, x, y)
					if surrounding_cells[0] in [2, 4]:
						set_cell_item(Vector3i(new_x - wall_movement - 8, 0, new_y - wall_movement - 8), item_ids[5], orientation)
					if surrounding_cells[2] in [2, 4]:
						set_cell_item(Vector3i(new_x + wall_movement - 8, 0, new_y - wall_movement - 8), item_ids[5], orientation)
					if surrounding_cells[5] in [2, 4]:
						set_cell_item(Vector3i(new_x - wall_movement - 8, 0, new_y + wall_movement - 3), item_ids[5], orientation)
					if surrounding_cells[7] in [2, 4]:
						set_cell_item(Vector3i(new_x + wall_movement, 0, new_y + wall_movement), item_ids[5], orientation)
					continue
					
				if type_name == "Wall":
					continue
					
				set_cell_item(Vector3i(new_x, 0, new_y), type_id, orientation)
				
	
