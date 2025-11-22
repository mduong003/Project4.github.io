#define MAX_INPUT 256
#include <string>

struct Map{
    int width, height;
    char grid[MAX_INPUT][MAX_INPUT];
    int startX, startZ;
    bool keyCollected[5] = {false, false, false, false, false}; 
};

Map parseMapFile(std::string fileName);