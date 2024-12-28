# Godot DTL (Dungeon Template Library)

**Godot DTL** is a [GDExtension for Godot 4.x](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html) that wraps the C++ [DTL (Dungeon Template Library)](https://github.com/AsPJT/DungeonTemplateLibrary). It provides a convenient interface to generate various types of 2D map data (islands, mazes, roguelike dungeons, and more) in Godot.

![](https://raw.githubusercontent.com/AsPJT/DungeonPicture/master/Picture/Logo/logo_color800_2.gif)

## Table of Contents
- [Godot DTL (Dungeon Template Library)](#godot-dtl-dungeon-template-library)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
    - [1. Install from the Godot Addons Browser](#1-install-from-the-godot-addons-browser)
    - [1. Manual Installation](#1-manual-installation)
  - [Usage](#usage)
    - [GDScript Example](#gdscript-example)
  - [DTL Class Reference](#dtl-class-reference)
    - [Description](#description)
  - [Methods](#methods)
    - [CellularAutomatonIsland](#cellularautomatonislandwidth-int-height-int-iterations-int--5-probability-float--04)
    - [CellularAutomatonMixIsland](#cellularautomatonmixislandwidth-int-height-int-iterations-int--5-land_values-int--5)
    - [ClusteringMaze](#clusteringmazewidth-int-height-int)
    - [DiamondSquareAverageCornerIsland](#diamondsquareaveragecornerislandwidth-int-height-int-min_value-int--20-altitude-int--80-add_altitude-int--60)
    - [DiamondSquareAverageIsland](#diamondsquareaverageislandwidth-int-height-int-min_value-int--0-altitude-int--80-add_altitude-int--60)
    - [FractalIsland](#fractalislandwidth-int-height-int-min_value-int--10-altitude-int--150-add_altitude-int--75)
    - [FractalLoopIsland](#fractalloopislandwidth-int-height-int-min_value-int--10-altitude-int--150-add_altitude-int--70)
    - [MazeBar](#mazebarwidth-int-height-int)
    - [MazeDig](#mazedigwidth-int-height-int)
    - [PerlinIsland](#perlinislandwidth-int-height-int-frequency-float--100-octaves-int--6-max_height-int--200-min_height-int--200)
    - [PerlinLoopIsland](#perlinloopislandwidth-int-height-int-frequency-float--100-octaves-int--6-max_height-int--200-min_height-int--200)
    - [PerlinSolitaryIsland](#perlinsolitaryislandwidth-int-height-int-truncated_proportion_-float--05-mountain_proportion_-float--045-frequency-float--60-octaves-int--6-max_height-int--200-min_height-int--200)
    - [RogueLike](#roguelikewidth-int-height-int-max_ways-int--20-min_room_width-int--3-max_room_width-int--3-min_room_height-int--4-max_room_height-int--4-min_way_horizontal-int--3-max_way_horizontal-int--4-min_way_vertical-int--3-max_way_vertical-int--4)
    - [SimpleRogueLike](#simpleroguelikewidth-int-height-int-division_min-int--3-division_max-int--4-room_min_x-int--5-room_max_x-int--6-room_min_y-int--7-room_max_y-int--8)
    - [SimpleVoronoiIsland](#simplevoronoiislandwidth-int-height-int-voronoi_num-float--400-probability-float--05)
  - [Contributing](#contributing)
  - [License](#license)

---

## Installation

There are two ways to install **Godot DTL**:

### 1. Install from the Godot Addons Browser

OR

### 1. Manual Installation
1. Clone or download this repository.
2. Open the project in Godot.
3. You’re good to go!

---

## Usage

### GDScript Example

```gdscript
# Suppose you have a script attached to a node where you want to generate a map:

extends Node

func _ready() -> void:
    var dtl = DTL.new()
    
    # Generate a simple roguelike dungeon
    var dungeon_array = dtl.SimpleRogueLike(
        width = 64,
        height = 64,
        division_min = 3,
        division_max = 4,
        room_min_x = 5,
        room_max_x = 6,
        room_min_y = 7,
        room_max_y = 8
    )
    
    # Now do something with 'dungeon_array'.
    # For example, you can iterate over it and create tiles:
    # (The exact usage depends on your project’s tilemap setup.)
    #
    # var map_size = Vector2(64, 64)
    # var tilemap = $TileMap
    # for y in range(map_size.y):
    #     for x in range(map_size.x):
    #         var cell_value = dungeon_array[y * map_size.x + x]
    #         tilemap.set_cell(0, Vector2(x, y), cell_value)
```

---

## DTL Class Reference

### Description
The `DTL` class serves as a wrapper to the [Dungeon Template Library (DTL)](https://github.com/AsPJT/DungeonTemplateLibrary), allowing you to generate various procedural 2D maps. The generated arrays are typically 1D arrays with dimensions `[width * height]` that you can map to tiles, terrain, or other game elements.

---

## Methods

---

### CellularAutomatonIsland(width: int, height: int, iterations: int = 5, probability: float = 0.4)
Generates an “island” by starting with random noise (based on `probability`) and applying a cellular automaton smoothing over several `iterations`. Useful for organic landmass shapes.

---

### CellularAutomatonMixIsland(width: int, height: int, iterations: int = 5, land_values: int = 5)
Similar to `CellularAutomatonIsland`, but it incorporates multiple “land values” (cells) instead of a simple binary map. Creates a blend of different terrain zones.

---

### ClusteringMaze(width: int, height: int)
Generates a maze-like structure by clustering cells. This can be used to create cave-like or labyrinthine environments.

---

### DiamondSquareAverageCornerIsland(width: int, height: int, min_value: int = 20, altitude: int = 80, add_altitude: int = 60)
Uses a variation of the **Diamond-Square** algorithm that averages corner values first. Produces height-map style islands where higher values often form the center regions.

- **min_value**: The lowest possible height value.  
- **altitude**: The mid-range height level.  
- **add_altitude**: The maximum offset added during the diamond-square steps.

---

### DiamondSquareAverageIsland(width: int, height: int, min_value: int = 0, altitude: int = 80, add_altitude: int = 60)
A Diamond-Square island generator without the corner averaging step. It can create more varied or rough edges compared to the corner-averaging variant.

---

### FractalIsland(width: int, height: int, min_value: int = 10, altitude: int = 150, add_altitude: int = 75)
Generates an island using fractal noise-based operations. Useful when you want more chaotic coastline and terrain features.

---

### FractalLoopIsland(width: int, height: int, min_value: int = 10, altitude: int = 150, add_altitude: int = 70)
Similar to `FractalIsland` but attempts a “loop” approach that can result in more continuous or wrapped edges, depending on how you use the data.

---

### MazeBar(width: int, height: int)
Creates a maze by generating “bar-like” corridors. Often results in straight or rectangular corridors for a grid-based, blocky feel.

---

### MazeDig(width: int, height: int)
A maze generator that “digs” through solid space, leaving a path behind. Results in winding tunnels similar to a mining or digging operation.

---

### PerlinIsland(width: int, height: int, frequency: float = 10.0, octaves: int = 6, max_height: int = 200, min_height: int = 200)
Generates an island height-map using **Perlin noise**. Adjusting `frequency` and `octaves` will change the map’s “roughness,” and `max_height` / `min_height` determine altitude ranges.

---

### PerlinLoopIsland(width: int, height: int, frequency: float = 10.0, octaves: int = 6, max_height: int = 200, min_height: int = 200)
Similar to `PerlinIsland`, but wraps the edges in both x and y directions, creating a “looped” or “toroidal” effect. Great if you need seamless transitions at map edges.

---

### PerlinSolitaryIsland(width: int, height: int, truncated_proportion_: float = 0.5, mountain_proportion_: float = 0.45, frequency: float = 6.0, octaves: int = 6, max_height: int = 200, min_height: int = 200)
Generates a solitary island shape by combining Perlin noise and threshold values (`truncated_proportion_` and `mountain_proportion_`) to emphasize island coastlines and mountainous interiors.

---

### RogueLike(width: int, height: int, max_ways: int = 20, min_room_width: int = 3, max_room_width: int = 3, min_room_height: int = 4, max_room_height: int = 4, min_way_horizontal: int = 3, max_way_horizontal: int = 4, min_way_vertical: int = 3, max_way_vertical: int = 4)
Generates a traditional “roguelike” dungeon layout with rooms connected by corridors. Various parameters allow you to tweak room sizes, corridor lengths, and the total number of passages.

---

### SimpleRogueLike(width: int, height: int, division_min: int = 3, division_max: int = 4, room_min_x: int = 5, room_max_x: int = 6, room_min_y: int = 7, room_max_y: int = 8)
A simpler roguelike generator using fewer parameters. It automatically divides the map into regions and places rooms with random dimensions within those divisions.

---

### SimpleVoronoiIsland(width: int, height: int, voronoi_num: float = 40.0, probability: float = 0.5)
Generates an island-like shape using [Voronoi diagrams](https://en.wikipedia.org/wiki/Voronoi_diagram). `voronoi_num` controls how many sites are used, and `probability` influences random fill patterns.

---

## Contributing

Pull requests are welcome! If you have suggestions for new features, performance improvements, or bug fixes, feel free to open an issue or submit a PR. Also, if you’d like to enhance the method descriptions or provide more examples, contributions are gladly accepted.

## License

This project is licensed under the MIT License.  
See [LICENSE](./LICENSE) for details.

---  

**Happy map-making with Godot DTL!**  