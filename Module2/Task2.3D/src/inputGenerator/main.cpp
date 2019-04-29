#include <iostream>
#include <fstream>
#include <chrono>

#define DEFAULT_LIGHTS 10
#define MAX_CARS 100
#define SAMPLES 288 //24hrs of 5 minute intervals

using namespace std;

int main(int argc, char* argv[]) {
    
    int trafficLights = DEFAULT_LIGHTS;
    if (argc > 1) trafficLights = atoi(argv[0]);
    
    long ts = time(NULL);
    
    srand(ts);

    ofstream fh;
    fh.open("trafficData.csv");

    for (int i = 0; i < SAMPLES; i++) {
        for (int j = 0; j < trafficLights; j++) {
            fh << j << "," << ts << "," << rand() % MAX_CARS << endl;
        }
        ts += 300;
    }

    fh.close();

    return 0;
}