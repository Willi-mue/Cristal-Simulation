#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <chrono>

int get_index(int x, int y, int matrix_size) {
    return y * matrix_size + x;
}



void createImage(const std::vector<int>& matrix, int width, int height, const std::string& filename) {
    std::ofstream imageFile(filename, std::ios::out | std::ios::binary);
    imageFile << "P6\n" << width << " " << height << "\n255\n";

    //maby use
    
    for (int i = 0; i < width * height; ++i) {
        int value = matrix[i];
        unsigned char color[3];

        if (value == 1) {
            // Green for seeds
            color[0] = 0;
            color[1] = 0;
            color[2] = 255;
        } else if (value == 2) {
            // Red for walkers
            color[0] = 30;
            color[1] = 144;
            color[2] = 255;
        } else {
            // Black for empty
            color[0] = 230;
            color[1] = 230;
            color[2] = 250;
        }

        imageFile.write(reinterpret_cast<char*>(color), 3);
    }

    // Close the image file
    imageFile.close();
}

int main() {
    int matrix_mult = 20;
    int matrix_size = matrix_mult * 16;

    std::vector<int> matrix(matrix_size*matrix_size,0);

    // programm start
    auto start_time = std::chrono::high_resolution_clock::now();

    // PROGRAMM

    // Stop the timer
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Print the execution time in microseconds
    std::cout << "Execution time: " << duration.count()*0.0000001 << " seconds" << std::endl;

    return 0;
}
