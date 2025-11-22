#include "map.h"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>


Map parseMapFile(std::string fileName) {
    std::ifstream myFile;
    myFile.open("map/" + fileName);
    if (!myFile){
      printf("ERROR: Could not read the map '%s'\n",fileName.c_str());
      exit(1);
    }
    Map map = Map();
    myFile >> map.width >> map.height;
    //ignore the newline
    std::string newline;
    std::getline(myFile, newline);
    //loop through each line of the map to save each tile and find where the start position is
    std::string line;
    int row = 0;
    while (row < map.height && std::getline(myFile, line)) {
        for (int col = 0; col < map.width; col++) {
            char curr = line[col];
            map.grid[row][col] = curr;
            if (curr == 'S') {
                map.startX = col;
                map.startZ = row;
            }
        }
        row++;
    }
    myFile.close();
    return map;
}
