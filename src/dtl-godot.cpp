#include <dtl-godot.hpp>
#include <DTL.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

DTL::DTL()
{
    // Add your initialization here.
}

DTL::~DTL()
{
    // Add your cleanup here.
}

void DTL::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("CellularAutomatonMixIsland", "width", "height", "iterations", "land_values"), &DTL::CellularAutomatonMixIsland, DEFVAL(5), DEFVAL(5));
    ClassDB::bind_method(D_METHOD("CellularAutomatonIsland", "width", "height", "iterations", "probability"), &DTL::CellularAutomatonIsland, DEFVAL(5), DEFVAL(0.4));
    ClassDB::bind_method(D_METHOD("FractalLoopIsland", "width", "height", "min_value", "altitude", "add_altitude"), &DTL::FractalLoopIsland, DEFVAL(10), DEFVAL(150), DEFVAL(70));
    ClassDB::bind_method(D_METHOD("FractalIsland", "width", "height", "min_value", "altitude", "add_altitude"), &DTL::FractalIsland, DEFVAL(10), DEFVAL(150), DEFVAL(75));
    ClassDB::bind_method(D_METHOD("DiamondSquareAverageIsland", "width", "height", "min_value", "altitude", "add_altitude"), &DTL::DiamondSquareAverageIsland, DEFVAL(0), DEFVAL(80), DEFVAL(60));
    ClassDB::bind_method(D_METHOD("DiamondSquareAverageCornerIsland", "width", "height", "min_value", "altitude", "add_altitude"), &DTL::DiamondSquareAverageCornerIsland, DEFVAL(20), DEFVAL(80), DEFVAL(60));
    ClassDB::bind_method(D_METHOD("SimpleVoronoiIsland", "width", "height", "voronoi_num", "probability"), &DTL::SimpleVoronoiIsland, DEFVAL(40.0), DEFVAL(0.5));
    ClassDB::bind_method(D_METHOD("PerlinIsland", "width", "height", "frequency", "octaves", "max_height", "min_height"), &DTL::PerlinIsland, DEFVAL(10.0), DEFVAL(6), DEFVAL(200), DEFVAL(200));
    ClassDB::bind_method(D_METHOD("PerlinLoopIsland", "width", "height", "frequency", "octaves", "max_height", "min_height"), &DTL::PerlinLoopIsland, DEFVAL(10.0), DEFVAL(6), DEFVAL(200), DEFVAL(200));
    ClassDB::bind_method(D_METHOD("PerlinSolitaryIsland", "width", "height", "truncated_proportion_", "mountain_proportion_", "frequency", "octaves", "max_height", "min_height"), &DTL::PerlinSolitaryIsland, DEFVAL(0.5), DEFVAL(0.45), DEFVAL(6.0), DEFVAL(6), DEFVAL(200), DEFVAL(200));
}

Array MatrixToGodotArray(const std::vector<std::vector<uint8_t>> &matrix)
{
    Array result;
    result.resize(matrix.size());
    for (int y = 0; y < matrix.size(); ++y)
    {
        Array row;
        row.resize(matrix[y].size());
        for (int x = 0; x < matrix[y].size(); ++x)
        {
            row[x] = (int)matrix[y][x];
        }
        result[y] = row;
    }
    return result;
}

Array DTL::CellularAutomatonMixIsland(int width, int height, int iterations, int land_values)
{
    // Create a 2D structure compatible with DTL
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));

    // Prepare up to 10 states.
    // If land_values is > 10, we still only use 10 states.
    const int max_states = 10;
    const int sea_value = 0;
    uint8_t states[max_states];

    // Initialize all states to sea_value
    for (int i = 0; i < max_states; ++i)
    {
        states[i] = (uint8_t)sea_value;
    }

    // Fill states with [1, 2, 3, ... land_values] up to 10
    int use_count = (land_values > max_states) ? max_states : land_values;
    for (int i = 0; i < use_count; ++i)
    {
        states[i] = (uint8_t)(i + 1);
    }

    switch (use_count)
    {
    case 1:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0])
            .draw(matrix);
        break;
    case 2:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1])
            .draw(matrix);
        break;
    case 3:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2])
            .draw(matrix);
        break;
    case 4:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3])
            .draw(matrix);
        break;
    case 5:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3], states[4])
            .draw(matrix);
        break;
    case 6:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3], states[4], states[5])
            .draw(matrix);
        break;
    case 7:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3], states[4], states[5], states[6])
            .draw(matrix);
        break;
    case 8:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3], states[4], states[5], states[6], states[7])
            .draw(matrix);
        break;
    case 9:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3], states[4], states[5], states[6], states[7], states[8])
            .draw(matrix);
        break;
    case 10:
        dtl::shape::CellularAutomatonMixIsland<uint8_t>(iterations, sea_value, states[0], states[1], states[2], states[3], states[4], states[5], states[6], states[7], states[8], states[9])
            .draw(matrix);
        break;
    }

    return MatrixToGodotArray(matrix);
}

Array DTL::CellularAutomatonIsland(int width, int height, int iterations, float probability)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::CellularAutomatonIsland(1, 0, iterations, probability).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::FractalLoopIsland(int width, int height, int min_value, int altitude, int add_altitude)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::FractalLoopIsland<uint8_t>(min_value, altitude, add_altitude).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::FractalIsland(int width, int height, int min_value, int altitude, int add_altitude)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::FractalIsland<uint8_t>(min_value, altitude, add_altitude).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::DiamondSquareAverageIsland(int width, int height, int min_value, int altitude, int add_altitude)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::DiamondSquareAverageIsland<uint8_t>(min_value, altitude, add_altitude).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::DiamondSquareAverageCornerIsland(int width, int height, int min_value, int altitude, int add_altitude)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::DiamondSquareAverageCornerIsland<uint8_t>(min_value, altitude, add_altitude).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::SimpleVoronoiIsland(int width, int height, float voronoi_num, float probability)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::SimpleVoronoiIsland<uint8_t>(voronoi_num, probability, 1, 0).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::PerlinIsland(int width, int height, float frequency, int octaves, int max_height, int min_height)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::PerlinIsland<uint8_t>(frequency, octaves, max_height, min_height).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::PerlinLoopIsland(int width, int height, float frequency, int octaves, int max_height, int min_height)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::PerlinLoopIsland<uint8_t>(frequency, octaves, max_height, min_height).draw(matrix);
    return MatrixToGodotArray(matrix);
}

Array DTL::PerlinSolitaryIsland(int width, int height, float truncated_proportion_, float mountain_proportion_, float frequency, int octaves, int max_height, int min_height)
{
    std::vector<std::vector<uint8_t>> matrix(height, std::vector<uint8_t>(width, 0));
    dtl::shape::PerlinSolitaryIsland<uint8_t>(truncated_proportion_, mountain_proportion_, frequency, octaves, max_height, min_height).draw(matrix);
    return MatrixToGodotArray(matrix);
}