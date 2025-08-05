#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <chrono>

int get_index(int x, int y, int matrix_size) {
    return y * matrix_size + x;
}

void put_particles(std::vector<int>& matrix,int matrix_size,int amount, bool seed=false){
    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(0, matrix_size);
    int random_number_x = 0;
    int random_number_y = 0;
    int i = 0;
    int index;
    int count_break = 0;

    while(i < amount){
        count_break ++;
        random_number_x = distribution(gen);
        random_number_y = distribution(gen);
        index = random_number_y*matrix_size+random_number_x;

        if (matrix[index] == 0 && matrix[index-1] != 1 && matrix[index+1] != 1 && matrix[index+matrix_size] != 1 && matrix[index-matrix_size] != 1
        ){
            if (seed == true){
                matrix[index] = 1;
            }else{
                matrix[index] = 2;
            }   
            i++;
            count_break = 0;
        }
        if (count_break == 25){
            break;
        }
    }
}

void move_walkers(std::vector<int>& matrix, int matrix_size){
    std::vector<int> temp = matrix;

    for (int i=0;i<matrix_size*matrix_size;i++){
        if (matrix[i] == 2){
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> direction(0, 3);
            int dir = direction(gen);
            
            int x = i % matrix_size;
            int y = i / matrix_size;
            int new_x = x;
            int new_y = y;

            switch (dir) {
                case 0: // Up
                    new_y = (y > 0) ? y - 1 : y;
                    break;
                case 1: // Down
                    new_y = (y < matrix_size - 1) ? y + 1 : y;
                    break;
                case 2: // Left
                    new_x = (x > 0) ? x - 1 : x;
                    break;
                case 3: // Right
                    new_x = (x < matrix_size - 1) ? x + 1 : x;
                    break;
            }

            int new_index = get_index(new_x, new_y, matrix_size);

            if (temp[new_index] == 1){
                switch (dir) {
                    case 0: // Up swap
                        new_y = (y < matrix_size - 1) ? y + 1 : y;
                        break;
                    case 1: // Down swap
                        new_y = (y > 0) ? y - 1 : y;
                        break;
                    case 2: // Left swap
                        new_x = (x < matrix_size - 1) ? x + 1 : x;
                        break;
                    case 3: // Right swap
                        new_x = (x > 0) ? x - 1 : x;
                        break;
                }
            }


            if (temp[new_index] == 0) {
                bool contact = false;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int neighbor_x = new_x + dx;
                        int neighbor_y = new_y + dy;
                        if (neighbor_x >= 0 && neighbor_x < matrix_size && neighbor_y >= 0 && neighbor_y < matrix_size) {
                            int neighbor_index = get_index(neighbor_x, neighbor_y, matrix_size);
                            if (matrix[neighbor_index] == 1) {
                                contact = true;
                                break;
                            }
                        }
                    }
                }
                if (contact) {
                    temp[new_index] = 1; // Seed
                } else {
                    temp[new_index] = 2; // Walker
                }
                temp[i] = 0;
            }
        }
    }
    matrix = temp;
}

void createImage(const std::vector<int>& matrix, int width, int height, const std::string& filename) {
    std::ofstream imageFile(filename, std::ios::out | std::ios::binary);
    imageFile << "P6\n" << width << " " << height << "\n255\n";
    
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
    int particles = 5000;
    int seeds = 6;
    int iterations = 2000;

    std::vector<int> matrix(matrix_size*matrix_size,0);

    // programm start
    auto start_time = std::chrono::high_resolution_clock::now();

    // seeds [1]
    put_particles(matrix,matrix_size,seeds,true);
    //matrix[((matrix_size*matrix_size)/2) + matrix_size/2] = 1;
    // walker [2]
    put_particles(matrix,matrix_size,particles);
    
    // simulate the walkers
    for (int iteration = 0; iteration < iterations; iteration++) {
        move_walkers(matrix, matrix_size);

        std::string filename = "frames/frame_" + std::to_string(iteration) + ".ppm";
        createImage(matrix, matrix_size, matrix_size, filename);
    }

    // Stop the timer
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Print the execution time in microseconds
    std::cout << "Execution time: " << duration.count()*0.0000001 << " seconds" << std::endl;

    return 0;
}
