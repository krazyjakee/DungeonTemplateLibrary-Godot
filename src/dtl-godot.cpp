#include <dtl-godot.hpp>
#include <DTL.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <DTL/Shape/CellularAutomatonMixIsland.hpp>

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
    ClassDB::bind_method(D_METHOD("CellularAutomatonMixIsland", "width", "height", "iterations", "land_values"), &DTL::CellularAutomatonMixIsland, DEFVAL(5), DEFVAL(5), DEFVAL(5));
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

    // Convert the resulting matrix into a Godot Array
    Array result;
    result.resize(height);
    for (int y = 0; y < height; ++y)
    {
        Array row;
        row.resize(width);
        for (int x = 0; x < width; ++x)
        {
            row[x] = (int)matrix[y][x];
        }
        result[y] = row;
    }
    return result;
}